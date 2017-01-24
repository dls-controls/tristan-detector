import os
import random
import sys
from ascii_stack_reader import AsciiStackReader
from data_word import DataWord
from nexus_swmr_file import NexusSwmrFileWriter

class TimepixDataParser(object):
    def __init__(self, num_files, data_dir, out_dir):
        self._asr = AsciiStackReader(data_dir)
        for x in range(0, 50000):
            self._asr.read_next()

        self._num_files = num_files
        self._data_dir = data_dir
        self._out_dir = out_dir
        self._count = 0
        self._prev_timestamp_course = 0
        self._timestamp_course = 0

        self._nx_files = []
        for fn in range(1, num_files + 1):
            self._nx_files.append(NexusSwmrFileWriter(os.path.join(out_dir, "nexus_swmr_" + str(fn) + ".h5")))

        for x in range(0, 256):
            for fn in range(0, num_files):
                self._nx_files[fn].detector_dset[x,] = range((x * 256), (x * 256) + 256)

        self._event_ids = []
        self._event_times = []

        self._current_file = 0
        self._time_slice_count = 0

        self._time_slices = []
        self._file_counts = []
        self._slice_counts = []
        self._cue_counts = []
        for fn in range(0, num_files):
            self._time_slices.append(random.randint(1000, 20000))
            self._file_counts.append(0)
            self._slice_counts.append(0)
            self._cue_counts.append(0)

    def execute(self, samples):
        eof = False
        print "Working ",
        sys.stdout.flush()
        while not eof:

            if self._time_slice_count == self._time_slices[self._current_file]:
                self._time_slices[self._current_file] = random.randint(1000, 20000)
                self._current_file += 1
                if self._current_file == self._num_files:
                    self._current_file = 0
                self._time_slice_count = 0

            data = self._asr.read_next()  # readline()
            if data is None:
                eof = True
                continue
            packet = DataWord(data)

            if packet.is_event:
                if self._timestamp_course > 0 and self._prev_timestamp_course > 0:
                    # print("Data Event found at index", count)
                    self._count += 1
                    self._time_slice_count += 1
                    self._file_counts[self._current_file] += 1
                    self._event_times.append(packet.full_timestmap(self._prev_timestamp_course, self._timestamp_course))

                    if len(self._event_times) > 999 or self._time_slice_count == self._time_slices[self._current_file] or self._count == samples:
                        self._nx_files[self._current_file].event_time_offset.resize((self._file_counts[self._current_file],))
                        self._nx_files[self._current_file].event_time_offset[self._file_counts[self._current_file] - len(self._event_times):self._file_counts[self._current_file]] = self._event_times
                        self._event_times = []

                    self._event_ids.append(packet.pos_x + (256 * packet.pos_y))

                    if len(self._event_ids) > 999 or self._time_slice_count == self._time_slices[self._current_file] or self._count == samples:
                        self._nx_files[self._current_file].event_id_dset.resize((self._file_counts[self._current_file],))
                        self._nx_files[self._current_file].event_id_dset[self._file_counts[self._current_file] - len(self._event_ids):self._file_counts[self._current_file]] = self._event_ids
                        self._nx_files[self._current_file].event_id_dset.flush()
                        self._event_ids = []

                    if self._time_slice_count == self._time_slices[self._current_file] or self._count == samples:
                        # End of a time slice so put in a marker
                        self._slice_counts[self._current_file] += 1
                        self._nx_files[self._current_file].event_index_dset.resize((self._slice_counts[self._current_file],))
                        self._nx_files[self._current_file].event_index_dset[self._slice_counts[self._current_file] - 1] = self._file_counts[self._current_file]
                        self._nx_files[self._current_file].event_index_dset.flush()
                        self._nx_files[self._current_file].event_time_zero_dset.resize((self._slice_counts[self._current_file],))
                        self._nx_files[self._current_file].event_time_zero_dset[self._slice_counts[self._current_file] - 1] = packet.full_timestmap(self._prev_timestamp_course, self._timestamp_course)

            else:
                if packet.ctrl_type == 0x20:  # Extended Time
                    print("Extended time found at index", self._count)
                    self._prev_timestamp_course = self._timestamp_course
                    self._timestamp_course = packet.timestamp_course
                else:  # Other event
                    print("Event recorded", packet.ctrl_type)
                    self._cue_counts[self._current_file] += 1
                    self._nx_files[self._current_file].cue_index_dset.resize((self._cue_counts[self._current_file],))
                    self._nx_files[self._current_file].cue_index_dset[self._cue_counts[self._current_file] - 1] = self._count
                    # Determine the event type
                    if packet.ctrl_type == 0x21:  # Shutter Open
                        cue_type = 0
                    if packet.ctrl_type == 0x22:  # Shutter Close
                        cue_type = 1
                    if packet.ctrl_type == 0x23:  # Trigger
                        cue_type = 2

            if self._count % 2000 == 0:
                if self._timestamp_course > 0 and self._prev_timestamp_course > 0:
                    print ".",
                    sys.stdout.flush()

            if self._count == samples:
                eof = True

        print ""
        for fn in range(0, self._num_files):
            self._nx_files[fn].close()
