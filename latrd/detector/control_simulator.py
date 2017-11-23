"""
Created on 20 May 2016

@author: Alan Greer
"""
from __future__ import print_function

import argparse
import logging
import time
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessageException, GetMessage, PutMessage, PostMessage, ResponseMessage
from latrd_reactor import LATRDReactor


class LATRDControlSimulator(object):
    def __init__(self):
        logging.basicConfig(format='%(asctime)-15s %(message)s')
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        self._log.setLevel(logging.DEBUG)
        self._ctrl_channel = None
        self._reactor = LATRDReactor()
        self._store = {}

    def setup_control_channel(self, endpoint):
        self._ctrl_channel = LATRDChannel(LATRDChannel.CHANNEL_TYPE_PAIR)
        self._ctrl_channel.bind(endpoint)
        self._reactor.register_channel(self._ctrl_channel, self.handle_ctrl_msg)

    def start_reactor(self):
        self._log.debug("Starting reactor...")
        self._reactor.run()

    def handle_ctrl_msg(self, msg):
        self._log.debug("Received message on configuration channel: %s", msg)
        if isinstance(msg, GetMessage):
            self._log.debug("Received GetMessage, parsing...")
            self.parse_get_msg(msg)
        elif isinstance(msg, PutMessage):
            self._log.debug("Received PutMessage, parsing...")
            self.parse_put_msg(msg)
        elif isinstance(msg, PostMessage):
            self._log.debug("Received PostMessage, parsing...")
            self.parse_post_msg(msg)
        else:
            raise LATRDMessageException("Unknown message type received")

    def parse_get_msg(self, msg):
        # Check the parameter keys and retrieve the values from the store
        values = {}
        self.read_parameters(self._store, msg.params, values)
        self._log.debug("Return value object: %s", values)
        reply = ResponseMessage(msg.msg_id, values, ResponseMessage.RESPONSE_OK)
        self._ctrl_channel.send(reply)

    def parse_put_msg(self, msg):
        # Retrieve the parameters and merge them with the store
        params = msg.params
        for key in params:
            self.apply_parameters(self._store, key, params[key])
        self._log.debug("Updated parameter Store: %s", self._store)
        reply = ResponseMessage(msg.msg_id)
        self._ctrl_channel.send(reply)

    def parse_post_msg(self, msg):
        # Nothing to do here, just wait two seconds before replying
        time.sleep(2.0)
        reply = ResponseMessage(msg.msg_id)
        self._ctrl_channel.send(reply)

    def apply_parameters(self, store, key, param):
        if key not in store:
            store[key] = param
        else:
            if isinstance(param, dict):
                for new_key in param:
                    self.apply_parameters(store[key], new_key, param[new_key])
            else:
                store[key] = param

    def read_parameters(self, store, param, values):
        for key in param:
            if isinstance(param[key], dict):
                values[key] = {}
                self.read_parameters(store[key], param[key], values[key])
            else:
                values[key] = store[key]


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:7001", help="Control endpoint")
    args = parser.parse_args()
    return args


def main():
    args = options()

    simulator = LATRDControlSimulator()
    simulator.setup_control_channel(args.control)
    simulator.start_reactor()


if __name__ == '__main__':
    main()

