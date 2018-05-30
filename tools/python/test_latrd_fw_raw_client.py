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
#    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:5004", help="Control endpoint")
#    parser.add_argument("--ready", default="tcp://127.0.0.1:5001", help="Ready endpoint")
#    parser.add_argument("--release", default="tcp://127.0.0.1:5002", help="Release endpoint")
#    parser.add_argument("-b", "--buffer", default="FrameReceiverBuffer", help="Shared memory buffer")
    parser.add_argument("-i", "--init", action="store_true", help="Initialise the FrameProcessor application")
    parser.add_argument("-p", "--path", default="/tmp", help="Path to write to (/tmp)")
    parser.add_argument("-f", "--filename", default="test.h5", help="File name to write to (test.h5)")
    parser.add_argument("-w", "--write", action="store_true", help="Start writing data")
    parser.add_argument("-s", "--stop", action="store_true", help="Stop writing data")
    parser.add_argument("-r", "--raw", action="store_true", help="Write data in raw mode")
    args = parser.parse_args()
    return args


def main():
    args = options()

    client = IpcClient("127.0.0.1", 5004)

    #msg = IpcMessage("cmd", "request_configuration")
    #success, reply = client._send_message(msg, 5.0)
    #print(reply)

    if args.init:
        client.send_configuration("tcp://*:5558", "meta_endpoint")
        config = {
            "fr_release_cnxn": "tcp://127.0.0.1:5002",
            "fr_ready_cnxn": "tcp://127.0.0.1:5001",
            "fr_shared_mem": "FrameReceiverBuffer"
        }
        client.send_configuration(config, "fr_setup")
        config = {
            "load": {
                "library": "/home/gnx91527/work/tristan/LATRD/build/lib/libLATRDProcessPlugin.so",
                "index": "latrd",
                "name": "LATRDProcessPlugin"
            }
        }
        client.send_configuration(config, "plugin")
        config = {
            "load": {
                "library": "/dls_sw/work/tools/RHEL6-x86_64/odin/lab29/odin-data/prefix/lib/libHdf5Plugin.so",
                "index": "hdf",
                "name": "FileWriterPlugin"
            }
        }
        client.send_configuration(config, "plugin")
        config = {
            "connect": {
                "index": "latrd",
                "connection": "frame_receiver"
            }
        }
        client.send_configuration(config, "plugin")
        config = {
            "connect": {
                "index": "hdf",
                "connection": "latrd"
            }
        }
        client.send_configuration(config, "plugin")
        config = {
            "process": {
                "number": 1,
                "rank": 0
            }
        }
        client.send_configuration(config, "latrd")
        config = {
            "dataset": {
                "raw_data": {
                    "datatype": 3,
                    "chunks": [10240]
                }
            }
        }
        client.send_configuration(config, "hdf")
        config = {
            "dataset": {
                "event_id": {
                    "datatype": 2,
                    "chunks": [10240]
                }
            }
        }
        client.send_configuration(config, "hdf")
        config = {
            "dataset": {
                "event_time_offset": {
                    "datatype": 3,
                    "chunks": [10240]
                }
            }
        }
        client.send_configuration(config, "hdf")
        config = {
            "dataset": {
                "event_energy": {
                    "datatype": 2,
                    "chunks": [10240]
                }
            }
        }
        client.send_configuration(config, "hdf")

        msg = IpcMessage("cmd", "request_configuration")
        success, reply = client._send_message(msg, 1.0)

        print(reply)

    if args.raw:
        config = {
            "raw_mode": 1
        }
    else:
        config = {
            "raw_mode": 0
        }
    client.send_configuration(config, "latrd")

    if args.write:
        config = {
            "reset_frame": 1
        }
        client.send_configuration(config, "latrd")
        config = {
            "file": {
                "path": args.path,
                "name": args.filename
            }
        }
        client.send_configuration(config, "hdf")
        config = {
            "write": True
        }
        client.send_configuration(config, "hdf")

    if args.stop:
        config = {
            "write": False
        }
        client.send_configuration(config, "hdf")

if __name__ == '__main__':
    main()

