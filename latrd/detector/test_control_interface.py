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
    parser.add_argument("-c", "--control", default="tcp://172.23.244.54:5000", help="Control endpoint")
    args = parser.parse_args()
    return args


def main():
    logging.basicConfig(format='%(asctime)-15s %(message)s')
    log = logging.getLogger("test_log")
    log.setLevel(logging.DEBUG)
    args = options()

    ctrl_channel = LATRDChannel(LATRDChannel.CHANNEL_TYPE_DEALER)
    ctrl_channel.connect(args.control)

    log.debug("*** First test, send a PUT with exposure time")
    msg = PutMessage()
    msg.set_param('Config', {'Exposure': 0.0005, 'Frames': 20})
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Second test, send a GET and request the frames value")
    msg = GetMessage()
    msg.set_param('Config', {'Exposure': None, 'Software_Version': None})
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Third test, send a GET and request the frames value")
    msg = GetMessage()
    msg.set_param('Config', {'Exposure': None, 'Frames': None})
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

    log.debug("*** Fifth test, send a GET for status and wait for reply")
    msg = GetMessage()
    msg.set_param('Status', None)
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Sixth test, send a POST to Arm and wait for reply")
    msg = PostMessage()
    msg.set_param('Command', 'Arm')
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

    log.debug("*** Seventh test, send a GET for status and wait for reply")
    msg = GetMessage()
    msg.set_param('Status', None)
    log.debug("Sending message: %s", msg)
    ctrl_channel.send(msg)
    reply = LATRDMessage.parse_json(ctrl_channel.recv())
    log.debug("Reply: %s", reply)

if __name__ == '__main__':
    main()

