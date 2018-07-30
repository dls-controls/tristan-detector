"""
Created on 13th September 2017

:author: Alan Greer
"""
import json
import logging
from common.zmqclient import ZMQClient
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from tornado import escape


class MetaListenerAdapter(ApiAdapter):
    """
    MetaListenerAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate meta-listener ZeroMQ control messages
    """
    ERROR_NO_RESPONSE = "No response from client, check it is running"
    ERROR_FAILED_TO_SEND = "Unable to successfully send request to client"

    def __init__(self, **kwargs):
        """
        Initialise the FrameProcessorAdapter object

        :param kwargs:
        """
        super(MetaListenerAdapter, self).__init__(**kwargs)

        logging.debug(self.name)
        logging.debug(kwargs)

        self._kwargs = {}
        for arg in kwargs:
            self._kwargs[arg] = kwargs[arg]
        self._kwargs['module'] = self.name

        if 'ip_address' in kwargs:
            ip_address = kwargs['ip_address']
        else:
            raise RuntimeError("No IP address specified for the meta-listener client")
        if 'port' in kwargs:
            port = kwargs['port']
        else:
            raise RuntimeError("No port number specified for the meta-listener client")

        self._client = ZMQClient(ip_address, port)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def get(self, path, request):

        """
        Implementation of the HTTP GET verb for FrameProcessorAdapter

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
        if request_command in self._kwargs:
            logging.debug("Adapter request for ini argument: %s", request_command)
            response[request_command] = self._kwargs[request_command]
        else:
            try:
                response = self._client.send_request(request_command)
                if not response:
                    logging.debug(MetaListenerAdapter.ERROR_NO_RESPONSE)
                    status_code = 503
                    response = {'error': MetaListenerAdapter.ERROR_NO_RESPONSE}
            except:
                logging.debug(MetaListenerAdapter.ERROR_FAILED_TO_SEND)
                status_code = 503
                response['error'] = MetaListenerAdapter.ERROR_FAILED_TO_SEND

        logging.debug("Full response from FP: %s", response)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def put(self, path, request):  # pylint: disable=W0613

        """
        Implementation of the HTTP PUT verb for FrameProcessorAdapter

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
        try:
            response = self._client.send_configuration(request_command, parameters)[1]
            if not response:
                logging.debug(MetaListenerAdapter.ERROR_NO_RESPONSE)
                status_code = 503
                response = {'error': MetaListenerAdapter.ERROR_NO_RESPONSE}
        except:
            logging.debug(MetaListenerAdapter.ERROR_FAILED_TO_SEND)
            status_code = 503
            response['error'] = MetaListenerAdapter.ERROR_FAILED_TO_SEND

        logging.debug("Full response from FP: %s", response)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def delete(self, path, request):  # pylint: disable=W0613
        """
        Implementation of the HTTP DELETE verb for FrameProcessorAdapter

        :param path: URI path of the DELETE request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        response = {'response': '{}: DELETE on path {}'.format(self.name, path)}
        status_code = 501

        logging.debug(response)

        return ApiAdapterResponse(response, status_code=status_code)
