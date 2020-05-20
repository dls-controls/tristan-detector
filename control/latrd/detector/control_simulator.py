"""
Created on 20 May 2016

@author: Alan Greer
"""
from __future__ import print_function

import argparse
import logging
import time
import threading
import subprocess
from latrd_channel import LATRDChannel
from latrd_message import LATRDMessageException, LATRDMessage, GetMessage, PutMessage, PostMessage, ResponseMessage
from latrd_reactor import LATRDReactor
from tristan_control_adapter import TriggerInType, TriggerOutType, TriggerTimestampType, TriggerInTerminationType, TriggerOutTerminationType, TriggerClockSourceType, TriggerTZeroType
from event_simulator import TristanEventProducer

class LATRDControlSimulator(object):
    DETECTOR_1M  = 1
    DETECTOR_2M  = 2
    DETECTOR_10M = 10

    def __init__(self, sensor=DETECTOR_1M):
        logging.basicConfig(format='%(asctime)-15s %(message)s')
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        self._log.setLevel(logging.DEBUG)
        self._ctrl_channel = None
        self._script = None
        self._script_thread = None
        self._sensor = sensor
        self._reactor = LATRDReactor()
        self._daq = TristanEventProducer()
        self._store = {
            'status':
                {
                    'state': 'idle',
                    'detector':
                        {
                            'description': 'TRISTAN control interface',
                            'serial_number': '0',
                            'software_version': '0.0.1',
                            'sensor_material': 'Silicon',
                            'sensor_thickness': '300 um',
                            'frames_acquired': 0,
                            'x_pixel_size': '55 um',
                            'y_pixel_size': '55 um',
                            'x_pixels_in_detector': 2048,
                            'y_pixels_in_detector': 512,
                            'timeslice_number': 4,
                            'udp_packets_sent': [0]
                        },
                    'housekeeping':
                        {
                            'standby': 'On',
                            'fem_power_enabled': [True] * self._sensor,
                            'psu_temp': [28.6] * self._sensor,
                            'psu_temp_alert': [False] * self._sensor,
                            'fan_alert': [False] * self._sensor,
                            'output_alert': [False] * self._sensor,
                            'current_sense': [1.5] * self._sensor,
                            'voltage_sense': [2.1] * self._sensor,
                            'remote_temp': [30.1] * self._sensor,
                            'fan_control_temp': [36.3] * self._sensor,
                            'tacho': [0.8] * self._sensor,
                            'pwm': [128] * self._sensor
                        },
                    'clock':
                        {
                            'dpll_lol': [True],
                            'dpll_hold': [True],
                            'clock_freq': 65.7
                        },
                    'sensor':
                        {
                            'temp_asics': [[50.0, 50.1, 50.2, 50.3, 50.4, 50.5, 50.6, 50.7,
                                            50.8, 50.9, 51.0, 51.1, 51.2, 51.3, 51.4, 51.5]] * self._sensor,
                            'temp_pcb': [[60.0, 60.1]] * self._sensor,
                            'humidity': [[47.8, 48.1]] * self._sensor,
                            'voltage': [[2.5, 2.6]] * self._sensor,
                            'current': [[1.1, 1.2]] * self._sensor
                        },
                    'fem':
                        {
                            'temp': [45.3] * self._sensor
                        }
                },
            'config':
                {
                    'exposure': 0.0,
                    'gap': 0.0,
                    'repeat_interval': 0.0,
                    'frames': 0,
                    'frames_per_trigger': 0,
                    'n_trigger': 0,
                    'mode': 'time_energy',
                    'profile': 'standard',
                    'threshold': 5.2,
                    'timeslice':
                        {
                            'duration_rollover_bits': 18
                        },
                    'bias':
                        {
                            'voltage': 0.0,
                            'enable': False
                        },
                    'time': '2018-09-26T09:30Z',
                    'trigger':
                        {
                            'start': TriggerInType.internal.name,
                            'stop': TriggerInType.internal.name,
                            'timestamp': {
                                'fem': TriggerTimestampType.none.name,
                                'lvds': TriggerTimestampType.none.name,
                                'sync': TriggerTimestampType.none.name,
                                'ttl': TriggerTimestampType.none.name,
                                'tzero': TriggerTimestampType.none.name
                            },
                            'ttl_in_term': TriggerInTerminationType.term_50ohm.name,
                            'ttl_out_term': TriggerOutTerminationType.term_50ohm.name,
                            'primary_clock_source': TriggerClockSourceType.internal.name,
                            'tzero': TriggerTZeroType.internal.name,
                            'ttl_out': TriggerOutType.follow_shutter.name,
                            'lvds_out': TriggerOutType.follow_shutter.name
                        }
                }
        }

    def register_script(self, script):
        self._script = script

    def setup_control_channel(self, endpoint):
        self._ctrl_channel = LATRDChannel(LATRDChannel.CHANNEL_TYPE_ROUTER)
        self._ctrl_channel.bind(endpoint)
        self._reactor.register_channel(self._ctrl_channel, self.handle_ctrl_msg)

    def start_reactor(self):
        self._log.debug("Starting reactor...")
        self._reactor.run()

    def handle_ctrl_msg(self):
        id = self._ctrl_channel.recv()
        msg = LATRDMessage.parse_json(self._ctrl_channel.recv())

        self._store['status']['detector']['udp_packets_sent'] = [self._daq._sent_packets]

        self._log.debug("Received message ID[%s]: %s", id, msg)
        if isinstance(msg, GetMessage):
            self._log.debug("Received GetMessage, parsing...")
            self.parse_get_msg(msg, id)
        elif isinstance(msg, PutMessage):
            self._log.debug("Received PutMessage, parsing...")
            self.parse_put_msg(msg, id)
        elif isinstance(msg, PostMessage):
            self._log.debug("Received PostMessage, parsing...")
            self.parse_post_msg(msg, id)
        else:
            raise LATRDMessageException("Unknown message type received")

    def parse_get_msg(self, msg, send_id):
        # Check the parameter keys and retrieve the values from the store
        values = {}
        self.read_parameters(self._store, msg.params, values)
        self._log.debug("Return value object: %s", values)
        reply = ResponseMessage(msg.msg_id, values, ResponseMessage.RESPONSE_OK)
        self._ctrl_channel.send_multi([send_id, reply])

    def parse_put_msg(self, msg, send_id):
        # Retrieve the parameters and merge them with the store
        params = msg.params
        for key in params:
            self.apply_parameters(self._store, key, params[key])
        self._log.debug("Updated parameter Store: %s", self._store)
        reply = ResponseMessage(msg.msg_id)
        self._ctrl_channel.send_multi([send_id, reply])

    def parse_post_msg(self, msg, send_id):
        # Nothing to do here, just wait two seconds before replying
        # Check for the "Run" command.  If it is sent and the simulated script has been supplied then execute it
        if 'command' in msg.params:
            if 'arm' == msg.params['command']:
                self._store['status']['state'] = 'arming'
                self._script_thread = threading.Thread(target=self.execute_arm)
                self._script_thread.start()
            elif 'run' == msg.params['command']:
                # Execute any run scripts
                self._store['status']['state'] = 'running'
                self._script_thread = threading.Thread(target=self.execute_script)
                self._script_thread.start()
            elif 'time_zero' == msg.params['command']:
                self._log.error("Time Zero command has been issued")

        reply = ResponseMessage(msg.msg_id)
        self._ctrl_channel.send_multi([send_id, reply])

    def execute_arm(self):
        self._daq.arm(5000000)
        self._store['status']['state'] = 'armed'

    def execute_script(self):
        self._daq.run()
        while self._daq.running():
            time.sleep(0.5)
        self._store['status']['state'] = 'idle'

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
        self._log.debug("Params: %s", param)
        for key in param:
            if isinstance(param[key], dict):
                values[key] = {}
                self.read_parameters(store[key], param[key], values[key])
            else:
                values[key] = store[key]


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:7001", help="Control endpoint")
    parser.add_argument("-m", "--sensor", default=1, help="Sensor module count (1 - 10)")
    parser.add_argument("-s", "--script", default="/home/gnx91527/work/tristan/bin/run_decode_mode_replay")
    args = parser.parse_args()
    return args


def main():
    args = options()

    sensor = 1
    if isinstance(args.sensor, str):
        sensor = int(''.join(filter(lambda i: i.isdigit(), args.sensor)))
    else:
        sensor = args.sensor
    simulator = LATRDControlSimulator(sensor)
    simulator.setup_control_channel(args.control)
    simulator.register_script(args.script)
    simulator.start_reactor()


if __name__ == '__main__':
    main()

