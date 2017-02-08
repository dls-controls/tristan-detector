import numpy as np

class TimepixPacket(object):

    def __init__(self, id):
        self._id = id
        self._size_of_packet = 1024
        self._producer_id = 1
        self._time_slice_id = 1
        self._word_count = 0
        self._header = 0
        self._data = []

    def add_word(self, word):
        self._data.append(word)
        self._word_count += 1

    def create_header(self):
        header = 0xE000000000000000
        header |= (self._producer_id << 50)
        header |= (self._time_slice_id << 42)
        header |= (self._id << 10)
        header |= (self._word_count+1)
        self._header = header

    @property
    def word_count(self):
        return self._word_count

    def to_bytes(self):
        packet_data = np.empty(self._size_of_packet, dtype=np.uint64)
        self.create_header()
        packet_data[0] = self._header
        packet_data[1:self.word_count+1] = self._data
        return packet_data.tostring()

    def from_bytes(self, input_bytes):
        packet_data = np.fromstring(input_bytes, dtype=np.uint64)
        self._header = packet_data[0]
        self._data = packet_data[1:len(packet_data - 1)]

    def report(self):
        self.create_header()
        print "{0:0{1}x}".format(self._header,16).upper()
        for word in self._data:
            print "{0:0{1}x}".format(word,16).upper()

