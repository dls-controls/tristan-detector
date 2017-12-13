"""
Created on 1st November 2017

:author: Alan Greer
"""
import json
import logging
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessage, GetMessage, PutMessage, PostMessage
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from tornado import escape
from tornado.ioloop import IOLoop
from datetime import datetime
import getpass


class TristanControlAdapter(ApiAdapter):
    """
    TristanControlAdapter class

    This class provides the adapter interface between the ODIN server and the Tristan detector system,
    transforming the REST-like API HTTP verbs into the appropriate Tristan ZeroMQ control messages
    """
    CONFIG_ITEM_LIST = {'Exposure': float,
                        'Repeat_Interval': float,
                        'Readout_Time': float,
                        'Frames': int,
                        'Frames_Per_Trigger': int,
                        'nTrigger': int,
                        'Threshold': str,
                        'Mode': str,
                        'Profile': str
                        }
    STATUS_ITEM_LIST = ['Status',
                        'Description',
                        'Serial_Number',
                        'Software_Version',
                        'Sensor_Material',
                        'Sensor_Thickness',
                        'x_Pixel_Size',
                        'y_Pixel_Size',
                        'x_Pixels_In_Detector',
                        'y_Pixels_In_Detector'
                        ]
    DETECTOR_TIMEOUT = 1000
    ERROR_NO_RESPONSE = "No response from client, check it is running"
    ERROR_FAILED_TO_SEND = "Unable to successfully send request to client"
    ERROR_FAILED_GET = "Unable to successfully complete the GET request"
    ERROR_PUT_MISMATCH = "The size of parameter array does not match the number of clients"

    def __init__(self, **kwargs):
        """
        Initialise the TristanControlAdapter object

        :param kwargs:
        """
        super(TristanControlAdapter, self).__init__(**kwargs)

        # Status dictionary read from client
        self._parameters = {}
        self._endpoint = None
        self._firmware = None
        self._detector = None
        self._update_interval = None
        self._start_time = datetime.now()
        self._username = getpass.getuser()

        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]
        self._kwargs['module'] = self.name
        self._kwargs['username'] = self._username
        self._kwargs['up_time'] = str(datetime.now() - self._start_time)
        self._kwargs['start_time'] = self._start_time.strftime("%B %d, %Y %H:%M:%S")

        try:
            self._endpoint = self.options.get('endpoint')
            self._kwargs['endpoint'] = self._endpoint
        except:
            raise RuntimeError("No endpoint specified for the Tristan detector")

        # Read the expected version of the firmware
        try:
            self._firmware = self.options.get('firmware')
            self._kwargs['firmware'] = self._firmware
        except:
            raise RuntimeError("No firmware version specified for the Tristan detector")

        # Create the connection to the hardware
        self._detector = LATRDChannel(LATRDChannel.CHANNEL_TYPE_DEALER)
        self._detector.connect(self._endpoint)

        # Setup the time between client update requests
        self._update_interval = float(self.options.get('update_interval', 0.5))
        self._kwargs['update_interval'] = self._update_interval
        # Start up the status loop
        self.update_loop()

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def get(self, path, request):

        """
        Implementation of the HTTP GET verb for TristanControlAdapter

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}
        logging.debug("GET path: %s", path)
        logging.debug("GET request: %s", request)

        # Check if the adapter type is being requested
        request_command = path.strip('/')
        if not request_command:
            # All status items have been requested, so return full tree
            key_list = self._kwargs.keys()
            for status in self._parameters['status']:
                for key in status:
                    if key not in key_list:
                        key_list.append(key)
            response['status'] = key_list
            key_list = []
            for config in self._parameters['config']:
                for key in config:
                    if key not in key_list:
                        key_list.append(key)
            response['config'] = key_list

        elif request_command in self._kwargs:
            logging.debug("Adapter request for ini argument: %s", request_command)
            response[request_command] = self._kwargs[request_command]
        else:
            try:
                request_items = request_command.split('/')
                logging.debug(request_items)
                # Now we need to traverse the parameters looking for the request

                try:
                    item_dict = self._parameters
                    for item in request_items:
                        item_dict = item_dict[item]
                except:
                    item_dict = None
                response_item = item_dict

                logging.debug(response_item)
                response[request_command] = response_item
            except:
                logging.debug(TristanControlAdapter.ERROR_FAILED_GET)
                status_code = 503
                response['error'] = TristanControlAdapter.ERROR_FAILED_GET

        logging.debug("Full response from FP: %s", response)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def put(self, path, request):  # pylint: disable=W0613

        """
        Implementation of the HTTP PUT verb for TristanControlAdapter

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}
        logging.debug("PUT path: %s", path)
        logging.debug("PUT request: %s", request)

        request_command = path.strip('/')
        config_items = request_command.split('/')
        # Check to see if this is a command
        if 'command' in config_items[0]:
            logging.debug("Command message: %s", config_items[1])
            msg = PostMessage()
            msg.set_param('Command', config_items[1])
            logging.debug("Sending message: %s", msg)
            self._detector.send(msg)
            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())
            logging.debug("Reply: %s", reply)

        if 'engineering' in config_items[0]:
            # This is a special command that allows an arbitrary JSON object to be sent to the hardware
            # The JSON object must be encoded into the body of the PUT request
            logging.debug("PUT request.body: %s", str(escape.url_unescape(request.body)))
            value_dict = json.loads(str(escape.url_unescape(request.body)))
            logging.debug("Config dict: %s", value_dict)
            msg = GetMessage()
            if 'engineering_put' in config_items[0]:
                msg = PutMessage()
            if 'engineering_get' in config_items[0]:
                msg = GetMessage()
            msg.set_param('Config', value_dict)
            logging.debug("Sending message: %s", msg)
            self._detector.send(msg)
            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())
            logging.debug("Reply: %s", reply)
            response['reply'] = str(reply)

        # Verify that config[0] is a config item
        if 'config' in config_items[0]:
            value_dict = {}
            temp_dict = value_dict
            for item in config_items[1:-2]:
                temp_dict[item] = {}
                temp_dict = temp_dict[item]
            try:
                temp_dict[config_items[-2]] = TristanControlAdapter.CONFIG_ITEM_LIST[config_items[-2]](float((config_items[-1])))
            except:
                temp_dict[config_items[-2]] = config_items[-1]

            logging.debug("Config dict: %s", value_dict)
            msg = PutMessage()
            msg.set_param('Config', value_dict)
            logging.debug("Sending message: %s", msg)
            self._detector.send(msg)
            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())
            logging.debug("Reply: %s", reply)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def delete(self, path, request):  # pylint: disable=W0613
        """
        Implementation of the HTTP DELETE verb for TristanControlAdapter

        :param path: URI path of the DELETE request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        response = {'response': '{}: DELETE on path {}'.format(self.name, path)}
        status_code = 501

        logging.debug(response)

        return ApiAdapterResponse(response, status_code=status_code)

    def update_loop(self):
        """Handle background update loop tasks.
        This method handles background update tasks executed periodically in the tornado
        IOLoop instance. This includes requesting the status from the underlying application
        and preparing the JSON encoded reply in a format that can be easily parsed.
        """

        try:
            logging.debug("Updating status from detector...")
            # Update server uptime
            self._kwargs['up_time'] = str(datetime.now() - self._start_time)

            status_dict = {}
            for item in TristanControlAdapter.STATUS_ITEM_LIST:
                status_dict[item] = None
            msg = GetMessage()
            msg.set_param('Config', status_dict)
            logging.debug("Message Request: %s", msg)
            self._detector.send(msg)

            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())

            while reply and reply.msg_id != msg.msg_id:
                pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
                reply = None
                if pollevts == LATRDChannel.POLLIN:
                    reply = LATRDMessage.parse_json(self._detector.recv())

            if reply:
                logging.debug("Raw reply: %s", reply)
                if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                    data = reply.data
                    self._parameters['status'] = data['Config']
                    self._parameters['status']['Connected'] = True
                    if self._firmware == self._parameters['status']['Software_Version']:
                        self._parameters['status']['Version_Check'] = True
                    else:
                        self._parameters['status']['Version_Check'] = False
            else:
                self._parameters['status']['Connected'] = False
                self._parameters['status']['Version_Check'] = False

            # Detector status message
            msg = GetMessage()
            msg.set_param('Status', None)
            logging.debug("Message Request: %s", msg)
            self._detector.send(msg)

            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())

            while reply and reply.msg_id != msg.msg_id:
                pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
                reply = None
                if pollevts == LATRDChannel.POLLIN:
                    reply = LATRDMessage.parse_json(self._detector.recv())

            if reply:
                logging.debug("Raw reply: %s", reply)
                if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                    data = reply.data
                    self._parameters['status']['State'] = data['Status']

            config_dict = {}
            for item in TristanControlAdapter.CONFIG_ITEM_LIST:
                config_dict[item] = None
            msg = GetMessage()
            msg.set_param('Config', config_dict)
            logging.debug("Message Request: %s", msg)
            self._detector.send(msg)

            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
            reply = None
            if pollevts == LATRDChannel.POLLIN:
                reply = LATRDMessage.parse_json(self._detector.recv())

            while reply and reply.msg_id != msg.msg_id:
                pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
                reply = None
                if pollevts == LATRDChannel.POLLIN:
                    reply = LATRDMessage.parse_json(self._detector.recv())

            if reply:
                logging.debug("Raw reply: %s", reply)
                if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                    data = reply.data
                    self._parameters['config'] = data['Config']

            logging.debug("Status items: %s", self._parameters)

        except:
            pass

        # Schedule the update loop to run in the IOLoop instance again after appropriate interval
        IOLoop.instance().call_later(self._update_interval, self.update_loop)
