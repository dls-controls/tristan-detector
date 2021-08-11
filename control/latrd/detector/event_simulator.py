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
    IMAGE_NUMBER_MASK      = 0x00FFFFFF00000000
    IMAGE_PACKET_MASK      = 0x00000000FFFFFFFF
    MODE_MASK              = 0x0000000000000800
    COUNT_MODE_WORD        = 0x9000000000000000
    HEADER_WORD_1          = 0x0000000000000000
    HEADER_WORD_2          = 0xE000000000000000
    HEADER_WORD_3          = 0xE400000000000000
    NO_OF_BUFFERS          = 4
    IMAGE_WORDS_PER_PACKET = 800


class TristanData(object):
    WORD_INDEX = 0
    TIMESTAMP = 0
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


class TristanImageWord(object):
    def __init__(self, element, image_x, image_y):
        self._element = element
        self._id = (image_x&0x1FFF)<<37 | (image_y&0x1FFF)<<24

    def to_64_bit_word(self):
        data_word = TristanDefinitions.COUNT_MODE_WORD | (self._element&0x3FF) | self._id
        #logging.info("Image word: {:016X}".format(data_word))
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
        extra_words = 0

        for index in range(words):
            if index > 0:
                if self._fine_ts&TristanDefinitions.FINE_TIMESTAMP_MASK == 0:
                    self._course_ts = TristanData.TIMESTAMP&TristanDefinitions.COURSE_TIMESTAMP_MASK
                    self._words.append(TristanTimestampWord(self._course_ts))
                    extra_words += 1
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
        # plus any additional timestamp words required for wrapping
        word_count = words + 3 + extra_words
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


class TristanImagePacket(object):
    def __init__(self, image_width, image_height, image_array, image_offset, image_number, offset_x, offset_y):
        self._words = []

        self._ts_wrap = 0
        self._course_ts = TristanData.TIMESTAMP&TristanDefinitions.COURSE_TIMESTAMP_MASK
        self._ts_word = TristanTimestampWord(self._course_ts)

        # Loop over the image_array from the image_offset until we've filled the
        # packet with words
        for index in range(TristanDefinitions.IMAGE_WORDS_PER_PACKET):
            if len(image_array) > (index + image_offset):
                element = image_array[(index + image_offset)]
                image_y = ((index + image_offset) / image_width) + offset_y
                image_x = ((index + image_offset) - ((image_y - offset_y) * image_width)) + offset_x
                if image_x > 4181 or image_x < 0:
                    logging.error("BAD image X value: index[{}] offset_x[{}] offset_y[{}]".format(index, offset_x, offset_y))
                    logging.error("BAD image X value: {} {} imge_offset[{}]".format(image_x, image_y, image_offset))
                self._words.append(TristanImageWord(element, image_x, image_y))

        #logging.error("Image words: {}".format(len(self._words)))

        # Create the header words
        # Fixed header 1
        self._hdr_1 = TristanDefinitions.HEADER_WORD_1
        # Header 2 contains producer_ID, modulo timeslice, count mode flag, word count
        words = len(self._words)
        #TristanDefinitions.IMAGE_WORDS_PER_PACKET
        word_count = words + 3
        self._hdr_2 = TristanDefinitions.HEADER_WORD_2 | \
                      (word_count&TristanDefinitions.WORD_COUNT_MASK) | \
                      ((self._ts_wrap<<18)&TristanDefinitions.TIME_SLICE_WRAP_MASK) | \
                      TristanDefinitions.MODE_MASK
        # Header 3 contains frame number and packet number
        packet_number = image_offset / TristanDefinitions.IMAGE_WORDS_PER_PACKET
        self._hdr_3 = TristanDefinitions.HEADER_WORD_3 | \
                      ((image_number<<32)&TristanDefinitions.IMAGE_NUMBER_MASK) | \
                      (packet_number&TristanDefinitions.IMAGE_PACKET_MASK)

    def dump(self):
        words = [self._hdr_1, self._hdr_2, self._hdr_3, self._ts_word.to_64_bit_word()]
        for word in self._words:
            words.append(word.to_64_bit_word())
        for word in words:
            print('{:016X}'.format(word))

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


class TristanImage(object):
    def __init__(self, image_width, image_height, image_array, offset_x, offset_y, image_number):
        self._image_width = image_width
        self._image_height = image_height
        self._image_array = image_array
        self._offset_x = offset_x
        self._offset_y = offset_y
        self._image_number = image_number

    def to_packets(self):
        total_size = self._image_width * self._image_height
        index = 0

        packets = []
        while index < total_size:
            packets.append(TristanImagePacket(self._image_width, self._image_height, self._image_array, index, self._image_number, self._offset_x, self._offset_y))
            index += TristanDefinitions.IMAGE_WORDS_PER_PACKET
        return packets


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


class TristanProducerDefaults(object):
    """
    Holds default values for frame producer parameters.
    """

    def __init__(self):

        self.endpoints = [
            ('localhost', 61649),
            ('localhost', 61650),
            ('localhost', 61651),
            ('localhost', 61652),
            ('localhost', 61653),
            ('localhost', 61654),
            ('localhost', 61655),
            ('localhost', 61656)
        ]
        self.num_events = 50000000
        self.num_idle = 5
        self.duration = 2.0
        self.drop_frac = 0
        self.drop_list = None


class TristanEventProducer(object):
    """
    Tristan event procducer.
    """

    def __init__(self, endpoints=None, config=1, no_image_fps=1):
        """
        Initialise the packet producer object, setting defaults and parsing command-line options.
        """
        # IDLE packet record
        self._idle_packet = None

        # Setup the modules according to the config
        # 1M = 1x1
        # 2M = 2x1
        # 10M = 2x5
        if config == 2:
            self._config = (2, 1)
            self._blocks = [
                (0, 2068, 0, 514),
                (2114, 4182, 0, 514)
                ]
        elif config == 10:
            self._config = (2, 5)
            self._blocks = [
                (0, 2068, 0, 514),
                (2114, 4182, 0, 514),
                (0, 2068, 632, 1146),
                (2114, 4182, 632, 1146),
                (0, 2068, 1264, 1778),
                (2114, 4182, 1264, 1778),
                (0, 2068, 1896, 2410),
                (2114, 4182, 1896, 2410),
                (0, 2068, 2528, 3042),
                (2114, 4182, 2528, 3042)
            ]
        else:
            self._config = (1, 1)
            self._blocks = [
                (0, 2068, 0, 514)
            ]

        # Create an empty list for packet storage
        self._packets = []
        self._image_packets = []

        self._no_image_fps = no_image_fps

        # Create an empty list for timeslice information
        self._ts = []

        # Time slice container
        self._time_slices = []

        # Start packet numbers from 0
        self._pkt_number = 0

        # Load default parameters
        self.defaults = TristanProducerDefaults()

        if endpoints is not None:
            self._endpoints = endpoints
        else:
            self._endpoints = self.defaults.endpoints

        self._no_of_ports = len(self._endpoints)

        self._sent_packets = 0
        self._packets_to_send = 0
        self._image_packets_to_send = 0
        self._last_log = 0
        self._mutex = threading.Lock()

    def init(self, num_events):
        self._time_slices = []
        self.create_packets(num_events, 800)
        self.create_image_packets()

    def increment_packets_sent(self):
        self._mutex.acquire()
        self._sent_packets += 1
        self._mutex.release()

    def arm(self):
        self._sent_packets = 0
        time.sleep(1.0)

    def run(self, mode='event'):
        """
        Run the frame producer.
        """
        self.send_packets(mode)

    def running(self):
        if self._sent_packets != self._last_log:
            print("PACKETS SENT : {}    TO SEND : {}".format(self._sent_packets, self._packets_to_send))
            self._last_log = self._sent_packets
        return self._sent_packets != self._packets_to_send

    def create_image_packets(self):
        # Generate the test image (4183 x 3043)
        #image_width = 4183
        #image_height = 3043

        # Create individual image blocks from the whole image
        #block_width = int(image_width / self._config[0])
        #block_height = int(image_height / self._config[1])

        number_of_blocks = self._config[0] * self._config[1]
        fps_to_blocks = len(self._endpoints) / self._no_image_fps
        print("Number of blocks: {}  fps to blocks: {}".format(number_of_blocks, fps_to_blocks))

        self._image_packets = []
        for x in range(len(self._endpoints)):
            self._image_packets.append([])

        print("image packets: {}".format(self._image_packets))

        endpoint_index = 0
        for block_index in range(number_of_blocks):
            print("Block index: {}".format(block_index))
            image = []
            offset_x = self._blocks[block_index][0]
            width_x = self._blocks[block_index][1] - offset_x
            offset_y = self._blocks[block_index][2]
            width_y = self._blocks[block_index][3] - offset_y
            print("Ofs X: {}  Ofs Y: {}  Width X: {}  Width Y: {}".format(
                offset_x, offset_y, width_x, width_y
                ))
            for y in range(width_y):
                for x in range(width_x):
                    index = (y * width_x) + x
                    i = random.randrange(20)
                    image.append(i)

            #logging.info("{}".format(image))
            img = TristanImage(width_x, width_y, image, offset_x, offset_y, 0)

            pkts = img.to_packets()
            print("Current endpoint_index: {}".format(endpoint_index))
            self._image_packets[endpoint_index] += pkts
            self._image_packets_to_send += len(pkts)
            print("Adding {} image packets, total: {}".format(len(pkts), self._image_packets_to_send))
            endpoint_index += fps_to_blocks
            if endpoint_index >= len(self._endpoints):
                endpoint_index -= len(self._endpoints)


        # for block_x in range(self._config[0]):
        #     for block_y in range(self._config[1]):
        #         image = []
        #         offset_x = block_x * block_width
        #         offset_y = block_y * block_height
        #         for y in range(block_height):
        #             for x in range(block_width):
        #                 index = (y * block_height) + x
        #                 i = random.randrange(20)
        #                 image.append(i)

        #         #logging.info("{}".format(image))
        #         img = TristanImage(block_width, block_height, image, offset_x, offset_y, 0)

        #         pkts = img.to_packets()
        #         print("Current endpoint_index: {}".format(endpoint_index))
        #         self._image_packets[endpoint_index] = pkts
        #         self._image_packets_to_send += len(pkts)
        #         print("Adding {} image packets, total: {}".format(len(pkts), self._image_packets_to_send))
        #         endpoint_index += fps_to_blocks

    def create_packets(self, total_events, per_slice):
        """
        Generate all of the necessary packets.
        """
        # First create the idle packet
        self._idle_packet = TristanIdlePacket().to_packet()

        self._pkt_number = 0
        # Now create enough packets for the number of words
        generated_events = 0
        time_slice = 1

        while generated_events < total_events:
            time_slice_dict = {'id': time_slice, 'packets': [], 'ts': []}
            # Generate a new packet
            slice_events = 0
#            this_slice = int(((random.random() * 0.2) + 0.8) * per_slice)
            this_slice = per_slice
            while slice_events < this_slice:
                pkt = TristanPacket(800, time_slice)
                #print("Packet length bytes: {}".format(len(pkt.to_packet())))
                generated_events += 800
                slice_events+=800

                time_slice_dict['packets'].append(pkt.to_packet())
                time_slice_dict['ts'].append(self._pkt_number)
                #self._packets.append(pkt.to_packet())
                #self._ts.append(self._pkt_number)
                self._pkt_number += 1
            print("Generated time slice {}".format(time_slice))
            self._time_slices.append(time_slice_dict)
            time_slice += 1
            TristanData.PACKET_NUMBER=0
            self._packets_to_send = self._pkt_number

    def send_packets(self, mode='event'):

        send_threads = []
        logging.info("Launching threads to send packets to endpoints: {}".format(self._endpoints))

        index = 0
        for endpoint in self._endpoints:
            addr = endpoint[0]
            port = endpoint[1]
            send_thread = None
            if mode == 'event':
                send_thread = threading.Thread(target=self._send_packets, args=(str(addr),int(port),int(index),self))
            else:
                send_thread = threading.Thread(target=self._send_image_packets, args=(str(addr),int(port),int(index),self))
            send_threads.append(send_thread)
            send_thread.start()
            index += 1

    def _send_packets(self, addr, port, index, owner):
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
        logging.info("Sending %d idle packets at 1 Hz", self.defaults.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.defaults.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        # Packet delay is total duration / number of packets
        packet_count = 0
        for ts in self._time_slices:
            if ts['id'] % self._no_of_ports == index:
                packet_count += len(ts['packets'])
        logging.info("Sending %d data packets in %f seconds", packet_count, self.defaults.duration)
        data_bytes_sent = 0
        data_packets_sent = 0
        delay = 1.0
        if packet_count > 0:
            delay = float(self.defaults.duration)/float(packet_count)
        logging.info("Packet send delay %f", delay)
        if delay < 0.001:
            delay = 0.001
#        for packet, ts_id in zip(self._packets, self._ts):
        for ts in self._time_slices:
            if (ts['id'] - 1) % self._no_of_ports == index:
                # Send the packet over the UDP socket
                logging.info("Sending TS {} to endpoint {}:{}".format(ts['id'], addr, port))
                for packet in ts['packets']:
                    try:
                    #logging.info("Checking port number for %d at index %d", ts_id, index)
                        #logging.info("Sending UDP packet")
                        data_bytes_sent += udp_socket.sendto(packet, (addr, port))
                        #logging.info("Sent UDP packet")
                        data_packets_sent += 1
#                        owner._sent_packets += 1
                        owner.increment_packets_sent()
                        # Add 1 second delay
                        time.sleep(delay)
                        if data_packets_sent % 1000 == 0:
                            logging.info("Sent %d packets", data_packets_sent)
                    except socket.error as exc:
                        logging.error("Got error sending frame packet: %s", exc)
                        break

        time.sleep(1.0)
        idle_bytes_sent = 0
        idle_packets_sent = 0
        logging.info("Sending %d idle packets at 1 Hz", self.defaults.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.defaults.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        udp_socket.close()

    def _send_image_packets(self, addr, port, index, owner):
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
        logging.info("Sending %d idle packets at 1 Hz", self.defaults.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.defaults.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        # Packet delay is total duration / number of packets
        if len(self._image_packets) > index:
            packet_count = len(self._image_packets[index])

            logging.info("Sending %d image packets in %f seconds", packet_count, self.defaults.duration)
            data_bytes_sent = 0
            data_packets_sent = 0
            delay = 1.0
            if packet_count > 0:
                delay = float(self.defaults.duration)/float(packet_count)
            logging.info("Packet send delay %f", delay)
            if delay < 0.001:
                delay = 0.001

            for packet in self._image_packets[index]:
                try:
                #logging.info("Checking port number for %d at index %d", ts_id, index)
                    #logging.info("Sending UDP packet")
                    data_bytes_sent += udp_socket.sendto(packet.to_packet(), (addr, port))
                    #logging.info("Sent UDP packet")
                    data_packets_sent += 1
#                        owner._sent_packets += 1
                    owner.increment_packets_sent()
                    # Add 1 second delay
                    time.sleep(delay)
                    if data_packets_sent % 1000 == 0:
                        logging.info("Sent %d image packets", data_packets_sent)
                except socket.error as exc:
                    logging.error("Got error sending frame packet: %s", exc)
                    break

        time.sleep(1.0)
        idle_bytes_sent = 0
        idle_packets_sent = 0
        logging.info("Sending %d idle packets at 1 Hz", self.defaults.num_idle)
        # Start by sending Idle packets at a rate of 1Hz
        for packets in range(int(self.defaults.num_idle)):
            # Send the packet over the UDP socket
            try:
                idle_bytes_sent += udp_socket.sendto(self._idle_packet, (addr, port))
                idle_packets_sent += 1
                # Add 1 second delay
                time.sleep(1.0)
            except socket.error as exc:
                logging.error("Got error sending frame packet: %s", exc)
                break

        udp_socket.close()


def main():
    logging.getLogger().setLevel(logging.INFO)
    evt = TristanEventProducer(endpoints=None, config=10, no_image_fps=2)
    evt.init(1000000)
    evt.arm()
    evt.run(mode='image')

#    pkt = TristanImagePacket(20, 40, image, 0, 3)
#    pkt.dump()

if __name__ == '__main__':
    main()

