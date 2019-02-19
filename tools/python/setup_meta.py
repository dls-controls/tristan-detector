from __future__ import print_function

import os, time
import argparse
import zmq
import json
import curses

import os
from odin_data.ipc_client import IpcClient
from odin_data.ipc_message import IpcMessage


def send_configuration(self, config, target):
    success, reply = self._client.send_configuration(config, target)


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", default=5659, help="Control port of Meta Listener")
    args = parser.parse_args()
    return args


def main():
    args = options()

    client = IpcClient("127.0.0.1", args.port)

    msg = IpcMessage("cmd", "configure")
    msg.set_param('acquisition_id', 'test')
    success, reply = client._send_message(msg, 1.0)
    print(reply)
        #if reply is not None:
        #    empty = reply['params']['buffers']['empty']
        #    mapped = reply['params']['buffers']['mapped']
        #    total = empty + mapped
        #    print("Buffers Free {} out of a total {}".format(empty, total))
        #time.sleep(1.0)



if __name__ == '__main__':
    main()

