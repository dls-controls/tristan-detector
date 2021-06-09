"""
Created on 1st November 2017

:author: Alan Greer
"""
import json
import logging
import time
import threading
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessage, GetMessage, PutMessage, PostMessage
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from tornado import escape
from tornado.ioloop import IOLoop
from datetime import datetime
from dateutil.tz import tzlocal
from enum import Enum
import getpass


class ModeType(Enum):
    """Enumeration of operational modes
    """
    time_energy = 0
    time_only = 1
    count = 2


class ProfileType(Enum):
    """Enumeration of operational modes
    """
    standard = 0
    fast = 1
    energy = 2
    user = 3
    current = 4


class TriggerInType(Enum):
    """Enumeration of trigger in types
    """
    internal = 0
    external_re = 1
    external_fe = 2
    external_lvds_re = 3
    external_lvds_fe = 4


class TriggerTimestampType(Enum):
    """Enumeration of trigger out types
    """
    none = 0
    re = 1
    fe = 2
    both = 3


class TriggerInTerminationType(Enum):
    """Enumeration of trigger out types
    """
    term_50ohm = 0
    hi_z = 1


class TriggerOutTerminationType(Enum):
    """Enumeration of trigger out types
    """
    term_50ohm = 0
    lo_z = 1


class TriggerClockSourceType(Enum):
    """Enumeration of trigger out types
    """
    internal = 0
    external = 1


class TriggerTZeroType(Enum):
    """Enumeration of trigger out types
    """
    internal = 0
    external = 1
    off = 2


class TriggerOutType(Enum):
    """Enumeration of trigger out types
    """
    follow_shutter = 0
    follow_shutter_read = 1
    one_per_acq_burst = 2
    busy = 3
    counter_select = 4
    tzero = 5
    sync = 6
    lvds = 7
    ttl = 8


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
        logging.debug("Parameter return_value: %s", return_value)
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
        logging.debug("enum set_value called with: %s", value)
        # Call super set with the name of the enum type
        if isinstance(value, EnumParameter):
            logging.debug("value considered an enum")
            super(EnumParameter, self).set_value(value.name, callback)
        else:
            logging.debug("value is a string")
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
    TEMP_ASICS_COUNT = 16
    TEMP_PCB_COUNT = 2
    HUMIDITY_COUNT = 2

    ADODIN_MAPPING = {
        'config/exposure_time': 'config/exposure',
        'config/num_images': 'config/frames',
        'status/sensor/width': 'status/detector/x_pixels_in_detector',
        'status/sensor/height': 'status/detector/y_pixels_in_detector',
        'status/sensor/bytes': 'status/detector/bytes'
    }

    CORE_STATUS_LIST = {
        'manufacturer': 'DLS/STFC',
        'model': 'Odin [Tristan]',
        'error': ''
    }

    CONFIG_ITEM_LIST = {'exposure': float,
                        'Repeat_Interval': float,
                        'Readout_Time': float,
                        'frames': int,
                        'Frames_Per_Trigger': int,
                        'nTrigger': int,
                        'Threshold': str,
                        'Mode': str,
                        'Profile': str
                        }
    STATUS_ITEM_LIST = [
        {
            "state": None,
            "detector": {
                "description": None,
                "detector_type": None,
                "serial_number": None,
                'module_dimensions': {
                    "x_min": [
                        None
                    ],
                    "y_min": [
                        None
                    ],
                    "x_max": [
                        None
                    ],
                    "y_max": [
                        None
                    ]
                },
                "software_version": None,
                "software_build": None,
                "sensor_material": None,
                "sensor_thickness": None,
                "x_pixel_size": None,
                "y_pixel_size": None,
                "x_pixels_in_detector": None,
                "y_pixels_in_detector": None,
                "tick_ps": None,
                "tick_hz": None,
                "timeslice_number": None,
                "timing_warning": None,
                "timing_error": None,
                "udp_packets_sent": None,
                "data_overrun": None,
                "frames_acquired": None,
                "shutteropen": None,
                "acquisition_busy": None,
                "shutterbusy": None,
                "fpga_busy": None,
                "loopaction": None,
                "crw": None
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
            'exposure': DoubleParameter('exposure', 1.0),
            'repeat_interval': DoubleParameter('repeat_interal', 1.0),
            'frames': IntegerParameter('frames', 0),
            'frames_per_trigger': IntegerParameter('frames_per_trigger', 0),
            'n_trigger': IntegerParameter('n_trigger', 0),
            'threshold': StringParameter('threshold', ''),
            'mode': EnumParameter('mode',
                                  ModeType.time_energy.name,
                                  [e.name for e in ModeType]),
            'profile': EnumParameter('profile',
                                     ProfileType.energy.name,
                                     [e.name for e in ProfileType]),
            'trigger': {
                'start': EnumParameter('start',
                                       TriggerInType.internal.name,
                                       [e.name for e in TriggerInType]),
                'stop': EnumParameter('stop',
                                      TriggerInType.internal.name,
                                      [e.name for e in TriggerInType]),
                'timestamp': {
                    'fem': EnumParameter('fem',
                                  TriggerTimestampType.none.name,
                                  [e.name for e in TriggerTimestampType]),
                    'lvds': EnumParameter('lvds',
                                  TriggerTimestampType.none.name,
                                  [e.name for e in TriggerTimestampType]),
                    'sync': EnumParameter('sync',
                                  TriggerTimestampType.none.name,
                                  [e.name for e in TriggerTimestampType]),
                    'ttl': EnumParameter('ttl',
                                  TriggerTimestampType.none.name,
                                  [e.name for e in TriggerTimestampType]),
                    'tzero': EnumParameter('tzero',
                                  TriggerTimestampType.none.name,
                                  [e.name for e in TriggerTimestampType])
                },
                'ttl_in_term': EnumParameter('ttl_in_term',
                                             TriggerInTerminationType.term_50ohm.name,
                                             [e.name for e in TriggerInTerminationType]),
                'ttl_out_term': EnumParameter('ttl_out_term',
                                              TriggerOutTerminationType.term_50ohm.name,
                                              [e.name for e in TriggerOutTerminationType]),
                'primary_clock_source': EnumParameter('primary_clock_source',
                                                      TriggerClockSourceType.internal.name,
                                                      [e.name for e in TriggerClockSourceType]),
                'tzero': EnumParameter('tzero',
                                       TriggerTZeroType.internal.name,
                                       [e.name for e in TriggerTZeroType]),
                'ttl_out': EnumParameter('ttl_out',
                                         TriggerOutType.follow_shutter.name,
                                         [e.name for e in TriggerOutType]),
                'lvds_out': EnumParameter('lvds_out',
                                          TriggerOutType.follow_shutter.name,
                                          [e.name for e in TriggerOutType])
            }
        }
        self._parameters = {'status':{'connected':False,
                                      'acquisition_complete':True,
                                      'detector': {
                                          'version_check':False
                                      }}}
        self._endpoint = None
        self._firmware = None
        self._detector = None
        self._update_interval = None
        self._start_time = datetime.now()
        self._username = getpass.getuser()
        self._comms_lock = threading.RLock()

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
        self._executing_updates = True
        self._update_time = datetime.now()
        self._status_thread = threading.Thread(target=self.update_loop)
        self._status_thread.start()

    def cleanup(self):
        self._executing_updates = False

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
        logging.info("HTTP PUT path: %s", path)
        logging.debug("PUT request: %s", request)
        logging.debug("PUT body: %s", request.body)

        self.clear_error()
        request_command = path.strip('/')
        if request_command in self.ADODIN_MAPPING:
            request_command = self.ADODIN_MAPPING[request_command]
        config_items = request_command.split('/')
        # Check to see if this is a command
        if 'command' in config_items[0]:
            logging.debug("Command message: %s", config_items[1])
            # Intercept the start_acquisition command and send arm followed by run
            if 'start_acquisition' == config_items[1]:
                # Send an arm command
                logging.info("Starting acquisition sequence")
                msg = PostMessage()
                msg.set_param('command', 'arm')
                reply = self.send_recv(msg)
                counter = 0
                # Wait for the status to become armed
                logging.info("Arm command sent, waiting for armed response")
                while self._parameters['status']['state'] != 'armed' and counter < 50:
                    time.sleep(0.2)
                    counter += 1
                # Check for a timeout and if there is raise an error
                if self._parameters['status']['state'] != 'armed':
                    self.set_error('Time out waiting for detector armed state')
                    status_code = 408
                    response['reply'] = str('Time out waiting for detector armed state')
                else:
                    logging.info("Confirmed armed response")
                    # Detector armed OK, now tell it to run
                    # Send a run command
                    msg = PostMessage()
                    msg.set_param('command', 'run')
                    with self._comms_lock:
                        reply = self.send_recv(msg)

                    counter = 0
                    # Wait for the status to become running
                    logging.info("Run command sent, waiting for running response")
                    while self._parameters['status']['state'] != 'running' and counter < 50:
                        time.sleep(0.2)
                        counter += 1
                    # Check for a timeout and if there is raise an error
                    if self._parameters['status']['state'] != 'running':
                        self.set_error('Time out waiting for detector running state')
                        status_code = 408
                        response['reply'] = str('Time out waiting for detector running state')

                    else:
                        logging.info("Confirmed running response")
                        with self._comms_lock:
                            # Once the reply has been received set the acquisition status to active
                            self._parameters['status']['acquisition_complete'] = False
                            logging.info("Acquisition confirmed, setting state to active")
                        response['reply'] = str(reply)

            elif 'stop_acquisition' == config_items[1]:
                # Send a stop command
                msg = PostMessage()
                msg.set_param('command', 'stop')
                reply = self.send_recv(msg)
                response['reply'] = str(reply)

            else:
                msg = PostMessage()
                msg.set_param('command', config_items[1])
                reply = self.send_recv(msg)
                response['reply'] = str(reply)

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
            msg.set_param('config', value_dict)
            reply = self.send_recv(msg)
            response['reply'] = str(reply)

        # Verify that config[0] is a config item
        if 'config' in config_items[0]:
            command, value_dict = self.uri_params_to_dictionary(request_command, request)

            logging.debug("Config dict: %s", value_dict)
            msg = PutMessage()
            msg.set_param('config', value_dict)
            reply = self.send_recv(msg)
            response['reply'] = str(reply)

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

    def send_recv(self, msg):
        """
        Send a message and wait for the response
        :param self:
        :param msg:
        :return:
        """
        logging.debug("Message Request: %s", msg)
        reply = None
        with self._comms_lock:
            status = self._detector.send(msg)

            if status > -1:
                pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
                if pollevts == LATRDChannel.POLLIN:
                    reply = LATRDMessage.parse_json(self._detector.recv())

                # Continue to attempt to get the correct reply until we timeout (or get the correct reply)
                while reply and reply.msg_id != msg.msg_id:
                    reply = None
                    pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)
                    if pollevts == LATRDChannel.POLLIN:
                        reply = LATRDMessage.parse_json(self._detector.recv())

        logging.debug("Reply: %s", reply)
        return reply

    def update_loop(self):
        """Handle background update loop tasks.
        This method handles background update tasks executed periodically in a thread
        This includes requesting the status from the underlying application
        and preparing the JSON encoded reply in a format that can be easily parsed.
        """
        while self._executing_updates:
            time.sleep(0.1)
            if (datetime.now() - self._update_time).seconds > self._update_interval:
                self._update_time = datetime.now()
                try:
                    with self._comms_lock:
                        logging.debug("Updating status from detector...")
                        # Update server uptime
                        self._kwargs['up_time'] = str(datetime.now() - self._start_time)

                        msg = GetMessage()
                        msg.set_param('status', TristanControlAdapter.STATUS_ITEM_LIST)
                        reply = self.send_recv(msg)

                        if reply is not None:
                            logging.debug("Raw reply: %s", reply)
                            if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                                data = reply.data
                                currently_connected = self._parameters['status']['connected']
                                self._parameters['status'] = data['status']

                                # Perform post status manipulation
                                if 'detector' in self._parameters['status']:
                                    if 'x_pixels_in_detector' in self._parameters['status']['detector'] and \
                                            'y_pixels_in_detector' in self._parameters['status']['detector']:
                                        x_pixels = self._parameters['status']['detector']['x_pixels_in_detector']
                                        y_pixels = self._parameters['status']['detector']['y_pixels_in_detector']
                                        self._parameters['status']['detector']['bytes'] = x_pixels * y_pixels * 2
                                    # Sum the packets sent from the detector (by module) into a singe total
                                    if 'udp_packets_sent' in self._parameters['status']['detector']:
                                        pckts = self._parameters['status']['detector']['udp_packets_sent']
                                        if isinstance(pckts, list):
                                            self._parameters['status']['detector']['udp_packets_sent'] = sum(pckts)
                                # Check if we have just reconnected
                                if not currently_connected:
                                    # Reconnection event so send down the time stamp config item
                                    connect_time = datetime.now(tzlocal()).strftime("%Y-%m-%dT%H:%M%z")
                                    msg = PutMessage()
                                    msg.set_param('config', {'time': connect_time})
                                    reply = self.send_recv(msg)
                                # Now set the connection status to True
                                self._parameters['status']['connected'] = True
                                if self._firmware == self._parameters['status']['detector']['software_version']:
                                    self._parameters['status']['detector']['version_check'] = True
                                else:
                                    self._parameters['status']['detector']['version_check'] = False

                                try:
                                    if 'clock' in self._parameters['status']:
                                        if 'dpll_lol' in self._parameters['status']['clock']:
                                            self._parameters['status']['clock']['dpll_lol'] = self._parameters['status']['clock']['dpll_lol'][0]
                                        if 'dpll_hold' in self._parameters['status']['clock']:
                                            self._parameters['status']['clock']['dpll_hold'] = self._parameters['status']['clock']['dpll_hold'][0]
                                except Exception as ex:
                                    logging.error("Error reading clock status items [dpll_lol, dpll_hold]")
                                    logging.error("Exception: %s", ex)

                                if 'sensor' in self._parameters['status']:
                                    if 'temp_asics' in self._parameters['status']['sensor']:
                                        # temp_asics is supplied as a 2D array, we need to split that out for monitoring
                                        for index in range(self.TEMP_ASICS_COUNT):
                                            self._parameters['status']['sensor']['temp_asics_{}'.format(index)] = []
                                            for temp in self._parameters['status']['sensor']['temp_asics']:
                                                self._parameters['status']['sensor']['temp_asics_{}'.format(index)].append(temp[index])

                                    if 'temp_pcb' in self._parameters['status']['sensor']:
                                        # temp_pcb is supplied as a 2D array, we need to split that out for monitoring
                                        for index in range(self.TEMP_PCB_COUNT):
                                            self._parameters['status']['sensor']['temp_pcb_{}'.format(index)] = []
                                            for temp in self._parameters['status']['sensor']['temp_pcb']:
                                                self._parameters['status']['sensor']['temp_pcb_{}'.format(index)].append(temp[index])

                                    if 'humidity' in self._parameters['status']['sensor']:
                                        # humidity is supplied as a 2D array, we need to split that out for monitoring
                                        for index in range(self.HUMIDITY_COUNT):
                                            self._parameters['status']['sensor']['humidity_{}'.format(index)] = []
                                            for temp in self._parameters['status']['sensor']['humidity']:
                                                self._parameters['status']['sensor']['humidity_{}'.format(index)].append(temp[index])

                                # Set the acquisition state item
                                if data['status']['state'] == 'idle':
                                    self._parameters['status']['acquisition_complete'] = True
                        else:
                            self._parameters['status']['connected'] = False
                            self._parameters['status']['detector']['version_check'] = False
                        logging.debug("self._parameters: %s", self._parameters)
                        self._parameters['status'].update(self.CORE_STATUS_LIST)

                        config_dict = self.create_config_request(self._param)
                        msg = GetMessage()
                        msg.set_param('config', config_dict)
                        reply = self.send_recv(msg)

                        if reply is not None:
                            logging.debug("Raw reply: %s", reply)
                            logging.debug("TESTING: %s", reply)
                            if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                                data = reply.data['config']
                                logging.debug("Reply data: %s", data)
                                self.update_config(self._param, data)

                        logging.debug("Status items: %s", self._parameters)
                        logging.debug("Config parameters: %s", self._param)

                except Exception as ex:
                    logging.error("Exception: %s", ex)

    def create_config_request(self, config):
        config_dict = {}
        for item in config:
            if isinstance(config[item], dict):
                config_dict[item] = self.create_config_request(config[item])
            else:
                config_dict[item] = None
        return config_dict

    def update_config(self, params, data):
        for item in data:
            logging.debug("Setting parameter for data item: %s", item)
            if isinstance(data[item], dict):
                if item in params:
                    self.update_config(params[item], data[item])
            else:
                try:
                    if item in params:
                        logging.debug("Setting value to: %s", str(data[item]))
                        params[item].set_value(data[item])
                except Exception as ex:
                    logging.error("Couldn't set param {} to {}".format(item, data[item]))
                    logging.error("Exception thrown: %s", ex)

    def clear_error(self):
        self.CORE_STATUS_LIST['error'] = ''

    def set_error(self, err):
        self.CORE_STATUS_LIST['error'] = err
        logging.error(err)
