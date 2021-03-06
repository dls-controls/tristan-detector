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
from enum import Enum
import getpass


class ModeType(Enum):
    """Enumeration of operational modes
    """
    Time_Energy = 0
    Time_Only = 1
    Count = 2


class ProfileType(Enum):
    """Enumeration of operational modes
    """
    Standard = 0
    Fast = 1
    Energy = 2
    User = 3
    Current = 4


class ParameterType(Enum):
    """Enumeration of all available types
    """
    UNKNOWN = 0
    DICT = 1
    LIST = 2
    INT = 3
    DOUBLE = 4
    STRING = 5
    ENUM = 6


class Parameter(object):
    def __init__(self, name, data_type=ParameterType.UNKNOWN, value=None, callback=None, every_time=False):
        self._name = name
        self._datatype = data_type
        self._value = value
        self._callback = callback
        self._every_time = every_time

    @property
    def value(self):
        return self.get()['value']

    def get(self):
        # Create the dictionary of information
        return_value = {'value': self._value,
                        'type': self._datatype.value
                        }
        logging.error("Parameter return_value: %s", return_value)
        return return_value

    def set_value(self, value, callback=True):
        changed = False
        if self._value != value:
            self._value = value
            changed = True
        if self._callback is not None:
            if callback:
                if self._every_time:
                    self._callback(self._name, self._value)
                elif changed:
                    self._callback(self._name, self._value)


class EnumParameter(Parameter):
    def __init__(self, name, value=None, allowed_values=None, callback=None, every_time=False):
        super(EnumParameter, self).__init__(name, data_type=ParameterType.ENUM, value=value,
                                            callback=callback, every_time=every_time)
        self._allowed_values = allowed_values

    def get(self):
        # Create the dictionary of information
        return_value = super(EnumParameter, self).get()
        if self._allowed_values is not None:
            return_value['allowed_values'] = self._allowed_values
        return return_value

    def set_value(self, value, callback=True):
        logging.error("enum set_value called with: %s", value)
        # Call super set with the name of the enum type
        if isinstance(value, EnumParameter):
            logging.error("value considered an enum")
            super(EnumParameter, self).set_value(value.name, callback)
        else:
            logging.error("value is a string")
            super(EnumParameter, self).set_value(value, callback)

    @property
    def index(self):
        return self.get()['allowed_values'].index(self.value)


class IntegerParameter(Parameter):
    def __init__(self, name, value=None, limits=None, callback=None, every_time=False):
        super(IntegerParameter, self).__init__(name, data_type=ParameterType.INT, value=value,
                                               callback=callback, every_time=every_time)
        self._limits = limits

    def get(self):
        # Create the dictionary of information
        return_value = super(IntegerParameter, self).get()
        if self._limits is not None:
            return_value['limits'] = self._limits
        return return_value


class DoubleParameter(Parameter):
    def __init__(self, name, value=None, limits=None, callback=None, every_time=False):
        super(DoubleParameter, self).__init__(name, data_type=ParameterType.DOUBLE, value=value,
                                              callback=callback, every_time=every_time)
        self._limits = limits

    def get(self):
        # Create the dictionary of information
        return_value = super(DoubleParameter, self).get()
        if self._limits is not None:
            return_value['limits'] = self._limits
        return return_value


class StringParameter(Parameter):
    def __init__(self, name, value=None, callback=None, every_time=False):
        super(StringParameter, self).__init__(name, data_type=ParameterType.STRING, value=value,
                                              callback=callback, every_time=every_time)


class TristanControlAdapter(ApiAdapter):
    """
    TristanControlAdapter class

    This class provides the adapter interface between the ODIN server and the Tristan detector system,
    transforming the REST-like API HTTP verbs into the appropriate Tristan ZeroMQ control messages
    """
    ADODIN_MAPPING = {
        'status/sensor/width': 'status/detector/x_pixels_in_detector',
        'status/sensor/height': 'status/detector/y_pixels_in_detector'
    }

    CORE_STATUS_LIST = {
        'manufacturer': 'DLS/STFC',
        'model': 'Odin [Tristan]',
        'error': ''
    }

    CONFIG_ITEM_LIST = {'exposure_time': float,
                        'repeat_interval': float,
                        'frames': int,
                        'frames_per_trigger': int,
                        'n_trigger': int,
                        'threshold': str,
                        'mode': str,
                        'profile': str
                        }

    STATUS_ITEM_LIST = [
        {
            "detector": {
                "state": None,
                "description": None,
                "serial_number": None,
                "software_version": None,
                "sensor_material": None,
                "sensor_thickness": None,
                "x_pixel_size": None,
                "y_pixel_size": None,
                "x_pixels_in_detector": None,
                "y_pixels_in_detector": None,
                "timeslice_number": None
            }
        }
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
        self._param = {
            'exposure_time': DoubleParameter('exposure_time', 1.0),
            'repeat_interval': DoubleParameter('repeat_interal', 1.0),
            'frames': IntegerParameter('frames', 0),
            'frames_per_trigger': IntegerParameter('frames_per_trigger', 0),
            'n_trigger': IntegerParameter('n_trigger', 0),
            'threshold': StringParameter('threshold', ''),
            'mode': EnumParameter('mode',
                                  ModeType.Time_Energy.name,
                                  [e.name for e in ModeType]),
            'profile': EnumParameter('profile',
                                     ProfileType.Energy.name,
                                     [e.name for e in ProfileType])
        }
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
        logging.error("GET path: %s", path)
        logging.error("GET request: %s", request)

        # Check if the adapter type is being requested
        request_command = path.strip('/')
        if request_command in self.ADODIN_MAPPING:
            request_command = self.ADODIN_MAPPING[request_command]
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
                if request_items[0] == 'config':
                    try:
                        item_dict = self._param
                        for item in request_items[1:]:
                            item_dict = item_dict[item]
                        response_item = item_dict.get()
                    except:
                        response_item = None

                    logging.debug(response_item)
                    response.update(response_item)
                else:
                    try:
                        item_dict = self._parameters
                        for item in request_items:
                            item_dict = item_dict[item]
                    except:
                        item_dict = None
                    response_item = item_dict

                    logging.debug(response_item)
                    response['value'] = response_item
            except:
                logging.debug(TristanControlAdapter.ERROR_FAILED_GET)
                status_code = 503
                response['error'] = TristanControlAdapter.ERROR_FAILED_GET

        logging.error("Full response from FP: %s", response)

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
        logging.error("PUT path: %s", path)
        logging.debug("PUT request: %s", request)
        logging.debug("PUT body: %s", request.body)

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
            command, value_dict = self.uri_params_to_dictionary(request_command, request)

            logging.error("Config dict: %s", value_dict)
            msg = PutMessage()
            msg.set_param('config', value_dict)
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

    @staticmethod
    def uri_params_to_dictionary(request_command, request):
        # Parse the parameters
        request_list = None
        parameters = None
        if request.body == '':
            # The value will be the final item of the request_command
            request_list = request_command.split('/')
            parameters = request_list[-1]
            try:
                parameters = TristanControlAdapter.CONFIG_ITEM_LIST[request_list[-2]](float((request_list[-1])))
            except:
                parameters = request_list[-1]
            request_list = request_list[:-1]
        else:
            try:
                parameters = json.loads(str(escape.url_unescape(request.body)))
            except ValueError:
                # If the body could not be parsed into an object it may be a simple string
                parameters = str(escape.url_unescape(request.body))

        # Check to see if the request contains more than one item
        if request_command is not None:
            if request_list is None:
                request_list = request_command.split('/')
            logging.debug("URI request list: %s", request_list)
            param_dict = {}
            command = None
            if len(request_list) > 1:
                # We need to create a dictionary structure that contains the request list
                current_dict = param_dict
                for item in request_list[1:-1]:
                    current_dict[item] = {}
                    current_dict = current_dict[item]

                item = request_list[-1]
                current_dict[item] = parameters
                command = request_list[0]
            else:
                param_dict = parameters
                command = request_command
        else:
            param_dict = parameters
            command = request_command

        logging.error("Command [%s] parameter dictionary: %s", command, param_dict)
        return command, param_dict

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

            #status_dict = {}
            #for item in TristanControlAdapter.STATUS_ITEM_LIST:
            #    status_dict[item] = None
            msg = GetMessage()
            msg.set_param('status', TristanControlAdapter.STATUS_ITEM_LIST)
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
                    self._parameters['status'] = data['status']
                    self._parameters['status']['connected'] = True
                    if self._firmware == self._parameters['status']['detector']['software_version']:
                        self._parameters['status']['detector']['version_check'] = True
                    else:
                        self._parameters['status']['detector']['version_check'] = False

                    # Set the acquisition state item
                    if data['status']['detector']['state'] == 'Idle':
                        self._parameters['status']['acquisition_complete'] = True
            else:
                self._parameters['status']['connected'] = False
                self._parameters['status']['detector']['version_check'] = False
            self._parameters['status'].update(self.CORE_STATUS_LIST)

            # Detector status message
#            msg = GetMessage()
#            msg.set_param('Status', None)
#            logging.debug("Message Request: %s", msg)
#            self._detector.send(msg)
#
#            pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
#            reply = None
#            if pollevts == LATRDChannel.POLLIN:
#                reply = LATRDMessage.parse_json(self._detector.recv())
#
#            while reply and reply.msg_id != msg.msg_id:
#                pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
#                reply = None
#                if pollevts == LATRDChannel.POLLIN:
#                    reply = LATRDMessage.parse_json(self._detector.recv())
#
#            if reply:
#                logging.debug("Raw reply: %s", reply)
#                if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
#                    data = reply.data
#                    self._parameters['status']['State'] = data['Status']

            config_dict = {}
            for item in TristanControlAdapter.CONFIG_ITEM_LIST:
                config_dict[item] = None
            msg = GetMessage()
            msg.set_param('config', config_dict)
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
                logging.debug("TESTING: %s", reply)
                if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                    data = reply.data['config']
                    logging.debug("Reply data: %s", data)
                    for item in data:
                        logging.debug("Setting parameter for data item: %s", item)
                        if item in self._param:
                            logging.debug("Setting value to: %s", str(data[item]))
                            self._param[item].set_value(data[item])
                    # Translate config strings to enum IDs.
#                    if 'mode' in data['config']:
#                        data['config']['mode'] = ModeType[data['config']['mode']].value
                    self._parameters['config'] = data['config']

            logging.debug("Status items: %s", self._parameters)
            logging.debug("Config parameters: %s", self._param)

        except:
            pass

        # Schedule the update loop to run in the IOLoop instance again after appropriate interval
        IOLoop.instance().call_later(self._update_interval, self.update_loop)
