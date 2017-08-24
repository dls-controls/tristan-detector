import socket
import time
from ascii_stack_reader import AsciiStackReader
from data_word import DataWord
from timepix_packet import TimepixPacket


class TimepixPacketSender(object):
    def __init__(self, data_dir):
        self._asr = AsciiStackReader(data_dir)
        for x in range(0, 50000):
            self._asr.read_next()

        self._data_dir = data_dir
        self._count = 0
        self._prev_timestamp_course = 0
        self._timestamp_course = 0
        self._timestamp_word = None
        self._timeslice_id = 1
        self._timeslice_counter = 1
        self._packet_id = 0
        self._host = None
        self._port = None
        self._sock = None
        self._bytes_sent = 0
        self._packets = None

    def open_connection(self, destination_addresses):
        (self._host, self._port) = ([], [])
        for index in destination_addresses:
            (host, port) = index.split(':')
            self._host.append(host)
            self._port.append(int(port))

        print "Starting LATRD data transmission to:", self._host
        # Open UDP socket
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send_packet(self, packet):
        host = self._host[0]
        port = self._port[0]
        self._bytes_sent += self._sock.sendto(packet.to_bytes(), (host, port))

    def execute(self, samples):
        self._packets = []
        eof = False
        udp_packet = TimepixPacket(self._packet_id)
        while not eof:

            data = self._asr.read_next()  # readline()
            if data is None:
                eof = True
                continue
            word = DataWord(data)

            if self._timestamp_course == 0:
                # We want a course timestamp before anything else
                if not word.is_event:
                    if word.ctrl_type == 0x20:  # Extended Time
                        print("Extended time found at index", self._count)
                        self._timestamp_word = word
                        udp_packet.add_word(self._timestamp_word.raw)
                        self._prev_timestamp_course = self._timestamp_course
                        self._timestamp_course = word.timestamp_course
            else:
                # Once we have our first timestamp then we can create UDP packets
                udp_packet.add_word(word.raw)
                if udp_packet.word_count == 1022:
                    print("=== Packet ID ===", udp_packet._packet_number)
                    udp_packet.report()
                    self._packets.append(udp_packet)
                    #self.send_packet(udp_packet)
                    self._packet_id += 1
                    self._timeslice_counter += 1
                    if self._timeslice_counter == 20:
                        self._timeslice_counter = 1
                        self._timeslice_id += 1
                    udp_packet = TimepixPacket(self._packet_id, self._timeslice_id)
                    udp_packet.add_word(self._timestamp_word.raw)
                if word.is_event:
                    self._count += 1
                else:
                    if word.ctrl_type == 0x20:  # Extended Time
                        print("Extended time found at index", self._count)
                        self._timestamp_word = word


            if self._count == samples:
                eof = True

        for packet in self._packets:
            self.send_packet(packet)
            time.sleep(0.02)
