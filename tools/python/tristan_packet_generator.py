"""
LATRDProducer - load LATRD packets from packet capture file and send via UDP.

Tim Nicholls, STFC Application Engineering Group.
"""

import struct
import logging
import argparse
import os
import socket
import time
import random
import threading


class TristanDefinitions(object):
    IDLE_PACKET_MASK       = 0x000000000003F800
    PRODUCER_ID_MASK       = 0x03FC000000000000
    TIME_SLICE_WRAP_MASK   = 0x0003FFFFFFFC0000
    TIME_SLICE_BUFFER_MASK = 0x000000FF00000000
    WORD_COUNT_MASK        = 0x00000000000007FF
    PACKET_ID_MASK         = 0x00000000FFFFFFFF
    COURSE_TIMESTAMP_MASK  = 0x000FFFFFFFFFFFF8
    FINE_TIMESTAMP_MASK    = 0x00000000007FFFFF
    ENERGY_MASK            = 0x0000000000003FFF
    POSITION_MASK          = 0x0000000003FFFFFF
    TIMESTAMP_CONTROL_WORD = 0x8000000000000000
    HEADER_WORD_1          = 0x0000000000000000
    HEADER_WORD_2          = 0xE000000000000000
    HEADER_WORD_3          = 0xE400000000000000
    NO_OF_BUFFERS          = 4


class TristanData(object):
    WORD_INDEX = 0
    TIMESTAMP = 9033808973057
    TIME_SLICE_NUMBER = 0
    PACKET_NUMBER = 0


class TristanTimestampWord(object):
    def __init__(self, ts):
        self._ts = ts

    def to_64_bit_word(self):
        data_word = (self._ts&TristanDefinitions.COURSE_TIMESTAMP_MASK) | TristanDefinitions.TIMESTAMP_CONTROL_WORD
        return data_word


class TristanWord(object):
    def __init__(self, ts):
        self._index = TristanData.WORD_INDEX
        TristanData.WORD_INDEX+=1
        self._ts = ts

    def to_64_bit_word(self):
        data_word = (self._index&TristanDefinitions.POSITION_MASK)<<37 | \
                    (self._ts&TristanDefinitions.FINE_TIMESTAMP_MASK)<<14
        return data_word


class TristanIdlePacket(object):
    def __init__(self):
        self._words = [
            0x0000000000000003,
            0xE00000000003F803,
            0xE400000000000000,
            0xFC00000000000000
        ]

    def to_packet(self):
        byte_array = struct.pack('<Q', self._words[0])
        byte_array += struct.pack('<Q', self._words[1])
        byte_array += struct.pack('<Q', self._words[2])
        byte_array += struct.pack('<Q', self._words[3])
        return byte_array


class TristanPacket(object):
    def __init__(self, words, time_slice):
        self._words = []

        # Calculate the current course and fine timestamp
        self._course_ts = TristanData.TIMESTAMP&TristanDefinitions.COURSE_TIMESTAMP_MASK
        self._fine_ts = TristanData.TIMESTAMP&TristanDefinitions.FINE_TIMESTAMP_MASK

        self._ts_word = TristanTimestampWord(self._course_ts)

        for index in range(words):
            self._words.append(TristanWord(self._fine_ts))
            self._fine_ts += 1
            TristanData.TIMESTAMP += 1

        # Record the header information
        self._ts_buffer = time_slice%TristanDefinitions.NO_OF_BUFFERS
        self._ts_wrap = int(time_slice/TristanDefinitions.NO_OF_BUFFERS)
        self._packet = TristanData.PACKET_NUMBER
        TristanData.PACKET_NUMBER += 1

        # Create the header words
        # Fixed header 1
        self._hdr_1 = TristanDefinitions.HEADER_WORD_1
        # Header 2 contains time slice wrap and word count
        # Word count is no of words + 1 for timestamp + 2 for header words
        word_count = words + 3
        self._hdr_2 = TristanDefinitions.HEADER_WORD_2 | \
                      (word_count&TristanDefinitions.WORD_COUNT_MASK) | \
                      ((self._ts_wrap<<18)&TristanDefinitions.TIME_SLICE_WRAP_MASK)
        # Header 3 contains time slice buffer number and the packet ID
        self._hdr_3 = TristanDefinitions.HEADER_WORD_3 | \
                      ((self._ts_buffer<<32)&TristanDefinitions.TIME_SLICE_BUFFER_MASK) | \
                      (self._packet&TristanDefinitions.PACKET_ID_MASK)

    def to_packet(self):
        words = [self._hdr_1, self._hdr_2, self._hdr_3, self._ts_word.to_64_bit_word()]
        for word in self._words:
            words.append(word.to_64_bit_word())
        byte_array = None
        for word in words:
            if byte_array is None:
                byte_array = struct.pack('<Q', word)
            else:
                byte_array += struct.pack('<Q', word)
        return byte_array


class Range(argparse.Action):
    """
    Range validating action for argument parser.
    """
    def __init__(self, min=None, max=None, *args, **kwargs):
        self.min = min
        self.max = max
        kwargs["metavar"] = "[%d-%d]" % (self.min, self.max)
        super(Range, self).__init__(*args, **kwargs)

    def __call__(self, parser, namespace, value, option_string=None):
        if not self.min <= value <= self.max:
            msg = 'invalid choice: %r (choose from [%d-%d])' % \
                  (value, self.min, self.max)
            raise argparse.ArgumentError(self, msg)
        setattr(namespace, self.dest, value)


class CsvAction(argparse.Action):
    """
    Comma separated list of values action for argument parser.
    """
    def __init__(self, val_type=None, *args, **kwargs):
        self.val_type =val_type
        super(CsvAction, self).__init__(*args, **kwargs)

    def __call__(self, parser, namespace, value, option_string=None):
        item_list=[]
        try:
            for item_str in value.split(','):
                item_list.append(self.val_type(item_str))
        except ValueError as e:
            raise argparse.ArgumentError(self, e)
        setattr(namespace, self.dest, item_list)


class LATRDProducerDefaults(object):
    """
    Holds default values for frame producer parameters.
    """

    def __init__(self):

        self.ip_addr = 'localhost'
        self.port_list = '61649'
        self.num_events = 500000
        self.num_idle = 5
        self.duration = 6.0
        self.drop_frac = 0
        self.drop_list = None

        self.log_level = 'info'
        self.log_levels = {
            'error': logging.ERROR,
            'warning': logging.WARNING,
            'info': logging.INFO,
            'debug': logging.DEBUG,
        }


class LATRDFrameProducer(object):
    """
    LATRD frame procducer - loads frame packets data from capture file and replays it to
    a receiver via a UDP socket.
    """

    def __init__(self):
        """
        Initialise the packet producer object, setting defaults and parsing command-line options.
        """
        # IDLE packet record
        self._idle_packet = None

        # Create an empty list for packet storage
        self._packets = []

        # Create an empty list for timeslice information
        self._ts = []

        # Load default parameters
        self.defaults = LATRDProducerDefaults()

        self._no_of_ports = 1

        # Set the terminal width for argument help formatting
        try:
            term_columns = int(os.environ['COLUMNS']) - 2
        except (KeyError, ValueError):
            term_columns = 100

        # Build options for the argument parser
        parser = argparse.ArgumentParser(
            prog='latrd_pcap_replay.py', description='LATRD frame producer',
            formatter_class=lambda prog: argparse.ArgumentDefaultsHelpFormatter(
                prog, max_help_position=40, width=term_columns)
        )

        parser.add_argument(
            '--address', '-a', type=str, dest='ip_addr',
            default=self.defaults.ip_addr, metavar='ADDR',
            help='Hostname or IP address to transmit UDP frame data to'
        )
        parser.add_argument(
            '--port', '-p', type=str, val_type=int, dest='ports', action=CsvAction,
            default=self.defaults.port_list, metavar='PORT[,PORT,...]',
            help='Comma separatied list of port numbers to transmit UDP frame data to'
        )
        parser.add_argument(
            '--events', '-e', type=int, dest='num_events',
            default=self.defaults.num_events, metavar='FRAMES',
            help='Number of events to transmit'
        )
        parser.add_argument(
            '--duration', '-d', type=float, dest='duration',
            default=self.defaults.duration, metavar='INTERVAL',
            help='Duration in seconds for sending data packets'
        )
        parser.add_argument(
            '--idle', '-i', type=float, dest='num_idle',
            default=self.defaults.num_idle, metavar='IDLE',
            help='Number of idle packets to send before and after'
        )
        parser.add_argument(
            '--pkt_gap', type=int, dest='pkt_gap', metavar='PACKETS',
            help='Insert brief pause between every N packets'
        )
        parser.add_argument(
            '--drop_frac', type=float, dest='drop_frac',
            min=0.0, max=1.0, action=Range,
            default=self.defaults.drop_frac, metavar='FRACTION',
            help='Fraction of packets to drop')
        parser.add_argument(
            '--drop_list', type=int, nargs='+', dest='drop_list',
            default=self.defaults.drop_list,
            help='Packet number(s) to drop from each frame',
        )
        parser.add_argument(
            '--logging', type=str, dest='log_level',
            default=self.defaults.log_level, choices=self.defaults.log_levels.keys(),
            help='Set logging output level'
        )

        # Parse arguments
        self.args = parser.parse_args()

        # Map logging level option onto real level
        if self.args.log_level in self.defaults.log_levels:
            log_level = self.defaults.log_levels[self.args.log_level]
        else:
            log_level = self.defaults.log_levels[self.defaults.log_level]

        # Set up logging
        logging.basicConfig(
            level=log_level, format='%(levelname)1.1s %(message)s',
            datefmt='%y%m%d %H:%M:%S'
        )

    def run(self):
        """
        Run the frame producer.
        """
        self.create_packets(self.args.num_events, 5000)
        self.send_packets()

    def create_packets(self, total_events, per_slice):
        """
        Generate all of the necessary packets.
        """
        # First create the idle packet
        self._idle_packet = TristanIdlePacket().to_packet()

        # Now create enough packets for the number of words
        generated_events = 0
        time_slice = 0

        while generated_events < total_events:
            # Generate a new packet
            slice_events = 0
            while slice_events < per_slice:
                pkt = TristanPacket(800, time_slice)
                generated_events += 800
                slice_events+=800
                self._packets.append(pkt.to_packet())
                self._ts.append(time_slice)
            time_slice += 1
            TristanData.PACKET_NUMBER=0

    def send_packets(self):

        send_threads = []
        if isinstance(self.args.ports, str):
            self.args.ports = [self.args.ports]

        self._no_of_ports = len(self.args.ports)
        logging.info("Launching threads to send packets to {} destination ports".format(
            len(self.args.ports)
        ))

        index = 0
        for port in self.args.ports:
            send_thread = threading.Thread(target=self._send_packets, args=(int(port),int(index)))
            send_threads.append(send_thread)
            send_thread.start()
            index += 1

    def _send_packets(self, port, index):
        """
        Send loaded packets over UDP socket.
        """

        # Create the UDP socket
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        if udp_socket is None:
            logging.error("Failed to open UDP socket")
            return


        idle_bytes_sent = 0
        idle_packets_sent = 0
        logging.info("Sending %d idle packets at 1 Hz", self.args.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.args.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (self.args.ip_addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        # Packet delay is total duration / number of packets
        logging.info("Sending %d data packets in %f seconds", len(self._packets)/self._no_of_ports, self.args.duration)
        data_bytes_sent = 0
        data_packets_sent = 0
        delay = float(self.args.duration)/float(len(self._packets)/self._no_of_ports)
        for packet, ts_id in zip(self._packets, self._ts):
            # Send the packet over the UDP socket
            try:
                if ts_id % self._no_of_ports == index:
                    data_bytes_sent += udp_socket.sendto(packet, (self.args.ip_addr, port))
                    data_packets_sent += 1
                    # Add 1 second delay
                    if delay > 0.001:
                        time.sleep(delay)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        time.sleep(1.0)
        idle_bytes_sent = 0
        idle_packets_sent = 0
        logging.info("Sending %d idle packets at 1 Hz", self.args.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.args.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (self.args.ip_addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        udp_socket.close()

if __name__ == '__main__':

    LATRDFrameProducer().run()
    #ts = TristanTimestampWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(ts.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
    #te = TristanWord(TristanData.TIMESTAMP)
    #print("0x{0:016X}".format(te.to_64_bit_word()))
