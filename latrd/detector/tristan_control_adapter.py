"""
Created on 1st November 2017

:author: Alan Greer
"""
import json
import logging
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessage, GetMessage
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
    STATUS_ITEM_LIST = ['Exposure',
                        'Repeat_Interval',
                        'readout_time',
                        'Frames',
                        'FramesPerTrigger',
                        'nTrigger',
                        'Mode',
                        'Profile',
                        'description',
                        'serial_number',
                        'software_version',
                        'sensor_material',
                        'Sensor_thickness',
                        'x_pixel_size',
                        'y_pixel_size',
                        'x_pixels_in_detector',
                        'y_pixels_in_detector']
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
        self._status = None
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
        self._detector = LATRDChannel(LATRDChannel.CHANNEL_TYPE_PAIR)
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
            key_list = self._kwargs.keys()
            for status in self._status:
                for key in status:
                    if key not in key_list:
                        key_list.append(key)
            response[request_command] = key_list
        elif request_command in self._kwargs:
            logging.debug("Adapter request for ini argument: %s", request_command)
            response[request_command] = self._kwargs[request_command]
        else:
            try:
                request_items = request_command.split('/')
                logging.debug(request_items)
                # Now we need to traverse the parameters looking for the request

                try:
                    item_dict = self._status
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
        parameters = json.loads(str(escape.url_unescape(request.body)))

        # Check if the parameters object is a list
        # if isinstance(parameters, list):
        #     logging.debug("List of parameters provided: %s", parameters)
        #     # Check the length of the list matches the number of clients
        #     if len(parameters) != len(self._clients):
        #         status_code = 503
        #         response['error'] = TristanControlAdapter.ERROR_PUT_MISMATCH
        #     else:
        #         # Loop over the clients and parameters, sending each one
        #         for client, param_set in zip(self._clients, parameters):
        #             if param_set:
        #                 try:
        #                     response = client.send_configuration(param_set, request_command)[1]
        #                     if not response:
        #                         logging.debug(TristanControlAdapter.ERROR_NO_RESPONSE)
        #                         status_code = 503
        #                         response['error'] = TristanControlAdapter.ERROR_NO_RESPONSE
        #                 except:
        #                     logging.debug(TristanControlAdapter.ERROR_FAILED_TO_SEND)
        #                     status_code = 503
        #                     response['error'] = TristanControlAdapter.ERROR_FAILED_TO_SEND
        #
        # else:
        #     logging.debug("Single parameter set provided: %s", parameters)
        #     for client in self._clients:
        #         try:
        #             response = client.send_configuration(parameters, request_command)[1]
        #             if not response:
        #                 logging.debug(TristanControlAdapter.ERROR_NO_RESPONSE)
        #                 status_code = 503
        #                 response['error'] = TristanControlAdapter.ERROR_NO_RESPONSE
        #         except:
        #             logging.debug(TristanControlAdapter.ERROR_FAILED_TO_SEND)
        #             status_code = 503
        #             response['error'] = TristanControlAdapter.ERROR_FAILED_TO_SEND


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

        logging.debug("Updating status from detector...")
        # Update server uptime
        self._kwargs['up_time'] = str(datetime.now() - self._start_time)

        status_dict = {}
        for item in TristanControlAdapter.STATUS_ITEM_LIST:
            status_dict[item] = None
        msg = GetMessage()
        msg.set_param('Config', status_dict)
        self._detector.send(msg)
        pollevts = self._detector.poll(TristanControlAdapter.DETECTOR_TIMEOUT)

        reply = None
        if pollevts == LATRDChannel.POLLIN:
            reply = LATRDMessage.parse_json(self._detector.recv())

        self._status = {}
        if reply:
            logging.debug("Raw reply: %s", reply)
            if LATRDMessage.MSG_TYPE_RESPONSE in reply.msg_type:
                data = reply.data
                self._status = data['Config']
                self._status['connected'] = True
                if self._firmware == self._status['software_version']:
                    self._status['version_check'] = True
                else:
                    self._status['version_check'] = False
        else:
            self._status['connected'] = False
            self._status['version_check'] = False

        logging.debug("Status items: %s", self._status)

            # Handle background tasks
#        self._status = []
#        for client in self._clients:
#            try:
#                response = client.send_request('status')
#                if response:
#                    if 'ack' in response['msg_type']:
#                        param_set = response['params']
#                        param_set['connected'] = True
#                        self._status.append(response['params'])
#            except:
#                # Any failure to read from FrameProcessor, results in empty status
#                self._status.append({'connected': False})
#        logging.debug("Status updated to: %s", self._status)

        # Schedule the update loop to run in the IOLoop instance again after appropriate interval
        IOLoop.instance().call_later(self._update_interval, self.update_loop)
