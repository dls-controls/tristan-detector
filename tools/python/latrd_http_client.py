from __future__ import print_function

import requests
import time
import getpass
from datetime import datetime
import logging
import os, time
import argparse
import zmq
import json
import curses


class LATRDClient(object):
    def __init__(self, address="127.0.0.1:8888", api=0.1):
        self._address = address
        self._api = api
        self._url = "http://" + str(self._address) + "/api/" + str(self._api)
        self._user = getpass.getuser()

    def set(self, adapter, parameter, value):
        try:
            url = self._url + "/" + adapter + "/config/" + parameter
            logging.error("Sending msg to: %s", url)
            result = requests.put(url,
                                  data=json.dumps(value),
                                  headers={
                                      'Content-Type': 'application/json',
                                      'Accept': 'application/json'
                                  }).json()
        except requests.exceptions.RequestException:
            result = {
                "error": "Exception during HTTP request, check address and Odin server instance"
            }
            logging.exception(result['error'])
        return result

    def get(self, adapter, parameter):
        try:
            url = self._url + "/" + adapter + "/status/" + parameter
            logging.error("Sending msg to: %s", url)
            result = requests.get(url,
                                  headers={
                                      'Content-Type': 'application/json',
                                      'Accept': 'application/json'
                                  }).json()
        except requests.exceptions.RequestException:
            result = {
                "error": "Exception during HTTP request, check address and Odin server instance"
            }
            logging.exception(result['error'])
        return result

    def set_fp(self, parameter, value):
        self.set('fp', parameter, value)

    def get_fr_status(self):
        return self.get('fr', '')

    def get_fp_status(self):
        return self.get('fp', '')



def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--address", default="127.0.0.1:8888", help="Odin control server endpoint")
    parser.add_argument("-i", "--init", action="store_true", help="Initialise the FrameProcessor application")
    args = parser.parse_args()
    return args


def main():
    args = options()

    client = LATRDClient(args.address)

    logging.error("FR Status: {}".format(client.get_fr_status()))
    logging.error("FP Status: {}".format(client.get_fp_status()))
    if args.init:
        client.set_fp('meta_endpoint','tcp://*:5558')
        client.set_fp('fr_setup', {
            "fr_release_cnxn": "tcp://127.0.0.1:5002",
            "fr_ready_cnxn": "tcp://127.0.0.1:5001"
        })
        client.set_fp('plugin', {
            "load": {
                "library": "/dls_sw/work/tools/RHEL6-x86_64/LATRD/prefix/lib/libLATRDProcessPlugin.so",
                "index": "latrd",
                "name": "LATRDProcessPlugin"
            }
        })
        config = {
            "load": {
                "library": "/home/gnx91527/work/tristan/LATRD/prefix/lib/libLATRDProcessPlugin.so",
                "index": "latrd",
                "name": "LATRDProcessPlugin"
            }
        }
        client.set_fp("plugin", config)
        config = {
            "load": {
                "library": "/dls_sw/prod/tools/RHEL6-x86_64/odin-data/0-4-0dls2/prefix/lib/libHdf5Plugin.so",
                "index": "hdf",
                "name": "FileWriterPlugin"
            }
        }
        client.set_fp("plugin", config)
        config = {
            "connect": {
                "index": "latrd",
                "connection": "frame_receiver"
            }
        }
        client.set_fp("plugin", config)
        config = {
            "connect": {
                "index": "hdf",
                "connection": "latrd"
            }
        }
        client.set_fp("plugin", config)
        config = {
            "process": {
                "number": 1,
                "rank": 0
            }
        }
        client.set_fp("latrd", config)
        config = {
            "dataset": {
                "raw_data": {
                    "datatype": 3,
                    "chunks": [524288]
                }
            }
        }
        client.set_fp("hdf", config)
        config = {
            "dataset": {
                "event_id": {
                    "datatype": 2,
                    "chunks": [524288]
                }
            }
        }
        client.set_fp("hdf", config)
        config = {
            "dataset": {
                "event_time_offset": {
                    "datatype": 3,
                    "chunks": [524288]
                }
            }
        }
        client.set_fp("hdf", config)
        config = {
            "dataset": {
                "event_energy": {
                    "datatype": 2,
                    "chunks": [524288]
                }
            }
        }
        client.set_fp("hdf", config)

if __name__ == '__main__':
    main()

