import zmq
import json
import logging
import struct
from datetime import datetime


class MetaListener:
    def __init__(self, directory, inputs, ctrl, writer_module):
        self.inputs = inputs
        self.directory = directory
        self.ctrl_port = str(ctrl)
        self._writer_module = writer_module
        self.writers = {}
        self.killRequested = False

        # create logger
        self.logger = logging.getLogger('meta_listener')

    def run(self):
        self.logger.info('Starting Meta listener...')

        try:
            inputsList = self.inputs.split(',')

            context = zmq.Context()

            receiverList = []

            # Control socket
            ctrl_address = "tcp://*:" + self.ctrl_port
            self.logger.info('Binding control address to ' + ctrl_address)
            ctrlSocket = context.socket(zmq.REP)
            ctrlSocket.bind(ctrl_address)

            # Socket to receive messages on
            for x in inputsList:
                newReceiver = context.socket(zmq.SUB)
                newReceiver.connect(x)
                newReceiver.setsockopt(zmq.SUBSCRIBE, '')
                receiverList.append(newReceiver)

            poller = zmq.Poller()
            for eachReceiver in receiverList:
                poller.register(eachReceiver, zmq.POLLIN)

            poller.register(ctrlSocket, zmq.POLLIN)

            self.logger.info('Listening to inputs ' + str(inputsList))

            while self.killRequested == False:
                socks = dict(poller.poll())
                for receiver in receiverList:
                    if socks.get(receiver) == zmq.POLLIN:
                        self.handle_message(receiver)

                if socks.get(ctrlSocket) == zmq.POLLIN:
                    self.handle_control_message(ctrlSocket)

            self.logger.info('Finished listening')

        except Exception as err:
            self.logger.error('Unexpected Exception: ' + str(err))

        # Finished
        for receiver in receiverList:
            receiver.close(linger=0)

        ctrlSocket.close(linger=100)

        context.term()

        self.logger.info("Finished run")
        return

    def handle_control_message(self, receiver):
        try:
            message = receiver.recv_json()

            if message['msg_val'] == 'status':
                reply = self.handle_status_message()
            elif message['msg_val'] == 'configure':
                self.logger.debug('handling control configure message')
                self.logger.debug(message)
                params = message['params']
                reply = self.handle_configure_message_params(params)
            else:
                reply = json.dumps({'msg_type': 'ack', 'msg_val': message['msg_val'],
                                    'params': {'error': 'Unknown message value type'},
                                    'timestamp': datetime.now().isoformat()})

        except Exception as err:
            self.logger.error('Unexpected Exception handling control message: ' + str(err))
            reply = json.dumps({'msg_type': 'ack', 'msg_val': message['msg_val'],
                                'params': {'error': 'Error processing control message'},
                                'timestamp': datetime.now().isoformat()})

        receiver.send(reply)

    def handle_status_message(self):
        statusDict = {}
        for key in self.writers:
            writer = self.writers[key]
            statusDict[key] = {'filename': writer.fullFileName, 'num_processors': writer.numberProcessorsRunning,
                               'written': writer.writeCount, 'finished': writer.finished}

        params = {'output': statusDict}
        reply = json.dumps(
            {'msg_type': 'ack', 'msg_val': 'status', 'params': params, 'timestamp': datetime.now().isoformat()})

        # Now delete any finsihed acquisitions
        for key, value in self.writers.items():
            if value.finished == True:
                del self.writers[key]

        return reply

    def handle_configure_message_params(self, params):
        reply = json.dumps(
            {'msg_type': 'nack', 'msg_val': 'configure', 'params': {'error': 'Unable to process configure command'},
             'timestamp': datetime.now().isoformat()})
        if 'kill' in params:
            self.logger.info('Kill reqeusted')
            reply = json.dumps(
                {'msg_type': 'ack', 'msg_val': 'configure', 'params': {}, 'timestamp': datetime.now().isoformat()})
            self.killRequested = True
        elif 'acquisition_id' in params:
            acquisitionID = params['acquisition_id']

            if acquisitionID is not None:
                if acquisitionID in self.writers:
                    self.logger.info('Writer is in writers')
                    acquisitionExists = True
                else:
                    self.logger.info('Writer not in writers for acquisition [' + str(acquisitionID) + ']')
                    acquisitionExists = False

                if 'output_dir' in params:
                    if acquisitionExists == False:
                        self.logger.info(
                            'Creating new acquisition [' + str(acquisitionID) + '] with output directory ' + str(
                                params['output_dir']))
                        self.create_new_acquisition(params['output_dir'], acquisitionID)
                        reply = json.dumps({'msg_type': 'ack', 'msg_val': 'configure', 'params': {},
                                            'timestamp': datetime.now().isoformat()})
                        acquisitionExists = True
                    else:
                        self.logger.info('File already created for acquisition_id: ' + str(acquisitionID))
                        reply = json.dumps({'msg_type': 'nack', 'msg_val': 'configure', 'params': {
                            'error': 'File already created for acquisition_id: ' + str(acquisitionID)},
                                            'timestamp': datetime.now().isoformat()})

                if 'flush' in params:
                    if acquisitionExists == True:
                        self.logger.info(
                            'Setting acquisition [' + str(acquisitionID) + '] flush to ' + str(params['flush']))
                        self.writers[acquisitionID].flushFrequency = params['flush']
                        reply = json.dumps({'msg_type': 'ack', 'msg_val': 'configure', 'params': {},
                                            'timestamp': datetime.now().isoformat()})
                    else:
                        self.logger.info('No acquisition for acquisition_id: ' + str(acquisitionID))
                        reply = json.dumps({'msg_type': 'nack', 'msg_val': 'configure', 'params': {
                            'error': 'No current acquisition with acquisition_id: ' + str(acquisitionID)},
                                            'timestamp': datetime.now().isoformat()})

                if 'stop' in params:
                    if acquisitionExists == True:
                        self.logger.info('Stopping acquisition [' + str(acquisitionID) + ']')
                        self.writers[acquisitionID].stop()
                        reply = json.dumps({'msg_type': 'ack', 'msg_val': 'configure', 'params': {},
                                            'timestamp': datetime.now().isoformat()})
                    else:
                        self.logger.info('No acquisition for acquisition_id: ' + str(acquisitionID))
                        reply = json.dumps({'msg_type': 'nack', 'msg_val': 'configure', 'params': {
                            'error': 'No current acquisition with acquisition_id: ' + str(acquisitionID)},
                                            'timestamp': datetime.now().isoformat()})
            else:
                reply = json.dumps(
                    {'msg_type': 'nack', 'msg_val': 'configure', 'params': {'error': 'Acquisition ID was None'},
                     'timestamp': datetime.now().isoformat()})

        else:
            reply = json.dumps({'msg_type': 'nack', 'msg_val': 'configure', 'params': {'error': 'No params in config'},
                                'timestamp': datetime.now().isoformat()})
        return reply

    def handle_message(self, receiver):
        self.logger.debug('Handling message')

        try:
            message = receiver.recv_json()
            #self.logger.debug(message)
            userheader = message['header']
            raw_value = receiver.recv()
            # Check the type and convert message accordingly
            msg_type = message['type']
            if 'uint64' in msg_type:
                msg_value = struct.unpack("Q", raw_value)[0]
            elif 'string' in msg_type:
                try:
                    # try to see if the bytes are a JSON object
                    msg_value = json.loads(raw_value)
                except Exception as ex:
                    msg_value = raw_value

            message['value'] = msg_value

            if 'acqID' in userheader:
                acquisition_id = userheader['acqID']
            else:
                self.logger.warn('Didnt have header')
                acquisition_id = ''

            if acquisition_id not in self.writers:
                self.logger.error('No writer for acquisition [' + acquisition_id + ']')
                receiver.recv()
            else:
                self.writers[acquisition_id].process_message(message)

        except Exception as err:
            self.logger.error('Unexpected Exception handling message: ' + str(err))

    def create_new_writer(self, directory, acquisition_id):
        module_name = self._writer_module[:self._writer_module.rfind('.')]
        class_name = self._writer_module[self._writer_module.rfind('.')+1:]
        module = __import__(module_name)
        writer_class = getattr(module, class_name)
        writer_instance = writer_class(self.logger, directory, acquisition_id)
        self.logger.debug(writer_instance)
        return writer_instance

    def create_new_acquisition(self, directory, acquisition_id):
        self.logger.info('Creating new acquisition for: ' + str(acquisition_id))
        self.writers[acquisition_id] = self.create_new_writer(directory, acquisition_id)
        # Check if we have built up too many finished acquisitions and delete them if so
        if len(self.writers) > 3:
            for key, value in self.writers.items():
                if value.finished == True:
                    del self.writers[key]
