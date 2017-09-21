"""
Created on 19 September 2017

@author: Alan Greer
"""
from __future__ import print_function

import logging
import argparse
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessage, GetMessage, PutMessage, PostMessage, ResponseMessage


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:7001", help="Control endpoint")
    args = parser.parse_args()
    return args


def main():
    logging.basicConfig(format='%(asctime)-15s %(message)s')
    log = logging.getLogger("test_log")
    log.setLevel(logging.DEBUG)
    args = options()

    ctrl_channel = LATRDChannel(LATRDChannel.CHANNEL_TYPE_PAIR)
    ctrl_channel.connect(args.control)

    log.debug("*** First test, send a PUT with some data items")
    msg = PutMessage()
    msg.set_param('config', {'exposure': 0.0005, 'frames': 1000})
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Second test, send a PUT with additional data items")
    msg = PutMessage()
    msg.set_param('config', {'delay': 0.02})
    msg.set_param('debug', True)
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Third test, send a GET and request the frames value")
    msg = GetMessage()
    msg.set_param('config', {'frames': None})
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Fourth test, send a POST and wait for reply")
    msg = PostMessage()
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

if __name__ == '__main__':
    main()

