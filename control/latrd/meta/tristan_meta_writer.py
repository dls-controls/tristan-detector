"""Implementation of Tristan Meta Writer

This module is a subclass of the odin_data MetaWriter and handles Tristan specific meta messages, writing them to disk.

Alan Greer, Diamond Light Source
"""
import numpy as np
import time
import os
import re
import struct
import h5py

from odin_data.meta_writer.meta_writer import MetaWriter
import _version as versioneer

MAJOR_VER_REGEX = r"^([0-9]+)[\\.-].*|$"
MINOR_VER_REGEX = r"^[0-9]+[\\.-]([0-9]+).*|$"
PATCH_VER_REGEX = r"^[0-9]+[\\.-][0-9]+[\\.-]([0-9]+).|$"

class TristanMetaWriter(MetaWriter):
    """Tristan Meta Writer class.

    Tristan Detector Meta Writer writes Tristan meta data to disk
    """
    BLOCKSIZE=10000000

    def __init__(self, logger, directory, acquisitionID):
        """Initalise the TristanMetaWriter object.

        :param logger: Logger to use
        :param directory: Directory to create the meta file in
        :param acquisitionID: Acquisition ID of this acquisition
        """
        super(TristanMetaWriter, self).__init__(logger, directory, acquisitionID)

        self._number_of_processors = 4
        self._expected_index = []
        self._time_slices = []
        self._time_slice_data_index = []
        self._vds_index = 0
        self._vds_blocks = []
        self._vds_total_pts = 0
        self._vds_file_count = 0

        # Record the directory for VDS file creation
        self._directory = directory
        self._acquisition_id = acquisitionID

        for index in range(self._number_of_processors):
            self.add_dataset_definition('ts_rank_{}'.format(index), (0,), maxshape=(None,), dtype='int32', fillvalue=-1)
            self._expected_index.append(0)
            self._time_slices.append([])
            self._time_slice_data_index.append(0)

        self.start_new_acquisition()

        #self._hdf5_file.swmr_mode = False
        #self._arrays_created = False

        self._logger.debug('TristanMetaWriter created...')

    @staticmethod
    def get_version():

        version = versioneer.get_versions()["version"]
        major_version = re.findall(MAJOR_VER_REGEX, version)[0]
        minor_version = re.findall(MINOR_VER_REGEX, version)[0]
        patch_version = re.findall(PATCH_VER_REGEX, version)[0]
        short_version = major_version + "." + minor_version + "." + patch_version

        version_dict = {}
        version_dict["full"] = version
        version_dict["major"] = major_version
        version_dict["minor"] = minor_version
        version_dict["patch"] = patch_version
        version_dict["short"] = short_version
        return version_dict

    def create_arrays(self):
        """Currently we do nothing here."""
        #self._hdf5_file.swmr_mode = True
        #self._arrays_created = True

    def start_new_acquisition(self):
        """Performs actions needed when the acquisition is started."""

        return

    def handle_frame_writer_create_file(self, userHeader, fileName):
        """Handle frame writer plugin create file message.

        :param userHeader: The header
        :param fileName: The file name
        """
        self._logger.debug('Handling frame writer create file for acqID ' + self._acquisition_id)
        self._logger.debug(userHeader)
        self._logger.debug(fileName)

        return

    def handle_frame_writer_start_acquisition(self, userHeader):
        """Handle frame writer plugin start acquisition message.

        :param userHeader: The header
        """
        self._logger.debug('Handling frame writer start acquisition for acqID ' + self._acquisition_id)
        self._logger.debug(userHeader)

        self.number_processes_running = self.number_processes_running + 1

        if not self.file_created:
            self.create_file()
            # Create the top level VDS file
            self.create_top_level_vds_file(self.BLOCKSIZE)

        if self._num_frames_to_write == -1:
            self._num_frames_to_write = userHeader['totalFrames']
            self.create_arrays()

        return

    def handle_frame_writer_stop_acquisition(self, userheader):
        """Handle frame writer plugin stop acquisition message.

        :param userheader: The user header
        """
        self._logger.debug('Handling frame writer stop acquisition for acqID ' + self._acquisition_id)
        self._logger.debug(userheader)

        if self.number_processes_running > 0:
            self.number_processes_running = self.number_processes_running - 1

        if self.number_processes_running == 0:
            self._logger.info('Last processor ended for acqID ' + str(self._acquisition_id))
            self.close_file()
        else:
            self._logger.info('Processor ended, but not the last for acqID ' + str(self._acquisition_id))

        return

    def close_file(self):
        """Close the file."""

        if self._hdf5_file is not None:
            self._logger.info('Closing file ' + self.full_file_name)
            self._hdf5_file.close()
            self._logger.info('Meta frames written: ' + str(self._current_frame_count) + ' of ' + str(self._num_frames_to_write))
            self._hdf5_file = None

        self.finished = True

    def handle_frame_writer_close_file(self):
        """Handle frame writer plugin close file message."""
        self._logger.debug('Handling frame writer close file for acqID ' + self._acquisition_id)
        # Do nothing
        return

    def handle_frame_writer_write_frame(self, message):
        """Handle frame writer plugin write frame message.

        :param message: The message
        """
        # For Tristan it is not clear that we will need to do anything here as frames are simply
        # arbitrary blocks of data to be written out
        return

    def handle_time_slice(self, user_header, value):
        """Handle a time slice message.  Write the time slice information
        to disk and generate any required VDS files

        :param user_header: The header
        :param value: An array of time slice sizes
        """
        self._logger.debug('Handling time slice information for acqID ' + self._acquisition_id)
        self._logger.debug(user_header)
        self._logger.debug(len(value))

        # Extract the rank and array size information from the header
        rank = user_header['rank']
        index = user_header['index']
        array_size = user_header['qty']
        format_str = '{}i'.format(array_size)
        array = struct.unpack(format_str, value)
        self._logger.debug(array)

        # Check to see if the expected index matches the index for this rank of message
        if self._expected_index[rank] == index:
            self._logger.debug("Index match, writing to file...")
            self._time_slices[rank][self._expected_index[rank]:self._expected_index[rank]+array_size] = array
            self.write_ts_data(rank, self._expected_index[rank], array, array_size)
            self._expected_index[rank] += array_size
            self.update_vds_blocks()

    def write_ts_data(self, rank, offset, array, array_size):
        """Write the frame data to the arrays and flush if necessary.

        :param rank: The FP index, where to write the data to
        :param offset: The offset to write to in the arrays
        :param array: The time slice array
        :param array_size: The number of elements to write
        """
        #if not self._arrays_created:
        #    self._logger.error('Arrays not created, cannot write frame data')
        #    return

        dset_name = 'ts_rank_{}'.format(rank)
        self._hdf5_datasets[dset_name].resize(offset+array_size, axis=0)
        self._hdf5_datasets[dset_name][offset:offset+array_size] = array
        self._hdf5_datasets[dset_name].flush()

        return

    def update_vds_blocks(self):
        processing = True
        while processing is True:
            for index in range(self._number_of_processors):
                if len(self._time_slices[index]) <= self._vds_index:
                    processing = False
            if processing is True:
                # Print the time slice sizes that we are working on
                debug_str = "VDS Index [{}] Sizes => ".format(self._vds_index)
                for index in range(self._number_of_processors):
                    debug_str="{} [{}]".format(debug_str, self._time_slices[index][self._vds_index])
                self._logger.debug("{}".format(debug_str))
                # Add the time slices to the VDS blocks for the current file
                for index in range(self._number_of_processors):
                    #self._logger.debug(self._vds_index)
                    #self._logger.debug(self._time_slices[index])
                    #self._logger.debug(self._time_slice_data_index[index])

                    if self._time_slices[index][self._vds_index] > 0:
                        new_block = None
                        self._vds_total_pts += self._time_slices[index][self._vds_index]
                        start_index = self._time_slice_data_index[index]
                        end_index = start_index + self._time_slices[index][self._vds_index] - 1
                        self._logger.debug("Current VDS Total Points: {}".format(self._vds_total_pts))
                        if self._vds_total_pts > self.BLOCKSIZE:
#                            self._logger.debug("End index before adjustment: {}".format(end_index))
                            end_index = end_index - (self._vds_total_pts - self.BLOCKSIZE)
#                            self._logger.debug("End index after adjustment: {}".format(end_index))
                            new_block = [index, end_index, end_index + (self._vds_total_pts - self.BLOCKSIZE)]
                        block = [index, start_index, end_index]
                        self._vds_blocks.append(block)
                        self._time_slice_data_index[index] = end_index + 1
                        if self._vds_total_pts >= self.BLOCKSIZE:
                            self.create_vds_file(self.BLOCKSIZE)
                            self._vds_blocks = []
                            self._vds_total_pts = 0
                            self._vds_file_count += 1
                            if new_block is not None:
                                self._vds_blocks.append(new_block)
                                self._vds_total_pts += new_block[2] - new_block[1] + 1
                self._vds_index += 1

    def create_vds_file(self, block_size=50000):
        # Create the virtual dataset file
        filename = '{}_vds_{}.h5'.format(self._acquisition_id, self._vds_file_count)
        f = h5py.File(os.path.join(self._directory, filename), 'w', libver='latest')

        entry_group = f.create_group("entry")
        entry_group.attrs["NX_class"] = "NXentry"

        instrument_group = entry_group.create_group("instrument")
        instrument_group.attrs["NX_class"] = "NXinstrument"

        detector_group = instrument_group.create_group("detector")
        detector_group.attrs["NX_class"] = "NXdetector"

        #detector_dset = detector_group.create_dataset("detector_number", shape=(256, 256),
        #                                              maxshape=(256, 256), chunks=(256, 256), dtype='i4')

        #cue_desc_dset = detector_group.create_dataset("cue_description", shape=(3,), maxshape=(3,), dtype='S32')

        data_group = entry_group.create_group("data")
        data_group.attrs["NX_class"] = "NXdata"

        event_group = data_group.create_group("event")
        event_group.attrs["NX_class"] = "NXevent_data"


        # Create the virtual dataset dataspace
        virt_dspace = h5py.h5s.create_simple((block_size,), (block_size,))

        # Create the virtual dataset property list
        dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

        dset_ptr = 0

        for block in self._vds_blocks:
            print("Block : ", block)
            file_index = block[0] + 1

            # Size of block
            block_size = block[2] - block[1] + 1
            # Create the source dataset dataspace
            src_dspace = h5py.h5s.create_simple((block_size,), (block_size,))

            # Select the source dataset hyperslab
            src_dspace.select_hyperslab(start=(block[1],), count=(1,), block=(block_size,))

            # Select the virtual dataset first hyperslab (for the first source dataset)
            virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(block_size,))

            dset_ptr += block_size
            print("Dset ptr size: {}".format(dset_ptr))

            # Set the virtual dataset hyperslab to point to the real first dataset
            src_filename = os.path.join(self._directory, '{}_{:06d}.h5'.format(self._acquisition_id, file_index))
            dcpl.set_virtual(virt_dspace, src_filename, "/event_id", src_dspace)

        # Create the virtual dataset
        h5py.h5d.create(event_group.id, name="event_id", tid=h5py.h5t.NATIVE_INT32, space=virt_dspace, dcpl=dcpl)

        # Move onto the next VDS

        # # Create the virtual dataset dataspace
        # virt_dspace = h5py.h5s.create_simple((block_size,), (block_size,))
        #
        # # Create the virtual dataset property list
        # dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)
        #
        # dset_ptr = 0
        #
        # for block in self._vds_blocks:
        #     print("Block : ", block)
        #     file_index = block[0]
        #
        #     # Size of block
        #     block_size = block[2] - block[1] + 1
        #     # Create the source dataset dataspace
        #     src_dspace = h5py.h5s.create_simple((block_size,), (block_size,))
        #
        #     # Select the source dataset hyperslab
        #     src_dspace.select_hyperslab(start=(block[1],), count=(1,), block=(block_size,))
        #
        #     # Select the virtual dataset first hyperslab (for the first source dataset)
        #     virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(block_size,))
        #
        #     dset_ptr += block_size
        #     print("Dset ptr size: {}".format(dset_ptr))
        #
        #     # Set the virtual dataset hyperslab to point to the real first dataset
        #     src_filename = os.path.join(self._directory, '{}_{:06d}.h5'.format(self._acquisition_id, file_index))
        #     dcpl.set_virtual(virt_dspace, src_filename, "/event_time_offset", src_dspace)
        #
        # # Create the virtual dataset
        # h5py.h5d.create(event_group.id, name="event_time_offset", tid=h5py.h5t.NATIVE_INT64, space=virt_dspace, dcpl=dcpl)

        #for x in range(0, 256):
        #    detector_dset[x,] = range((x * 256), (x * 256) + 256)

        #cue_desc_dset[0] = "Shutter Open"
        #cue_desc_dset[1] = "Shutter Close"
        #cue_desc_dset[2] = "Trigger"

        # Close the file
        f.close()

    def create_top_level_vds_file(self, block_size=50000):
        # Create the virtual dataset file
        filename = '{}_complete.h5'.format(self._acquisition_id)
        f = h5py.File(os.path.join(self._directory, filename), 'w', libver='latest')

        src_filename = '{}_vds_%b.h5'.format(self._acquisition_id)

        entry_group = f.create_group("entry")
        entry_group.attrs["NX_class"] = "NXentry"

        instrument_group = entry_group.create_group("instrument")
        instrument_group.attrs["NX_class"] = "NXinstrument"

        detector_group = instrument_group.create_group("detector")
        detector_group.attrs["NX_class"] = "NXdetector"

        detector_dset = detector_group.create_dataset("detector_number", shape=(256, 256),
                                                      maxshape=(256, 256), chunks=(256, 256), dtype='i4')

        cue_desc_dset = detector_group.create_dataset("cue_description", shape=(3,), maxshape=(3,), dtype='S32')

        data_group = entry_group.create_group("data")
        data_group.attrs["NX_class"] = "NXdata"

        event_group = data_group.create_group("event")
        event_group.attrs["NX_class"] = "NXevent_data"

        # Event ID virtual dataset

        # Create the virtual dataset dataspace
        virt_dspace = h5py.h5s.create_simple((block_size,), (h5py.h5s.UNLIMITED,))

        # Create the virtual dataset property list
        dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

        # Create the source dataset dataspace
        src_dspace = h5py.h5s.create_simple((block_size,))

        # Select the virtual dataset first hyperslab (for the first source dataset)
        virt_dspace.select_hyperslab(start=(0,), count=(h5py.h5s.UNLIMITED,), stride=(block_size,), block=(block_size,))

        dcpl.set_virtual(virt_dspace, os.path.join(self._directory, src_filename), "/entry/data/event/event_id", src_dspace)

        # Create the virtual dataset
        dset = h5py.h5d.create(event_group.id, name="event_id", tid=h5py.h5t.NATIVE_INT32, space=virt_dspace, dcpl=dcpl)

        # Event time offset virtual dataset

        # Create the virtual dataset dataspace
        virt_dspace = h5py.h5s.create_simple((block_size,), (h5py.h5s.UNLIMITED,))

        # Create the virtual dataset property list
        dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

        # Create the source dataset dataspace
        src_dspace = h5py.h5s.create_simple((block_size,))

        # Select the virtual dataset first hyperslab (for the first source dataset)
        virt_dspace.select_hyperslab(start=(0,), count=(h5py.h5s.UNLIMITED,), stride=(block_size,), block=(block_size,))

        dcpl.set_virtual(virt_dspace, os.path.join(self._directory, src_filename), "/entry/data/event/event_time_offset", src_dspace)

        # Create the virtual dataset
        dset = h5py.h5d.create(event_group.id, name="event_time_offset", tid=h5py.h5t.NATIVE_INT64, space=virt_dspace, dcpl=dcpl)

        for x in range(0, 256):
            detector_dset[x,] = range((x * 256), (x * 256) + 256)

        cue_desc_dset[0] = "Shutter Open"
        cue_desc_dset[1] = "Shutter Close"
        cue_desc_dset[2] = "Trigger"

        # Close the file
        f.close()

    def process_message(self, message, userheader, receiver):
        """Process a meta message.

        :param message: The message
        :param userheader: The user header
        :param receiver: The ZeroMQ socket the data was received on
        """
        self._logger.debug('Tristan Meta Writer Handling message')

        if message['parameter'] == "createfile":
            fileName = receiver.recv()
            self.handle_frame_writer_create_file(userheader, fileName)
        elif message['parameter'] == "closefile":
            receiver.recv()
            self.handle_frame_writer_close_file()
        elif message['parameter'] == "startacquisition":
            receiver.recv()
            self.handle_frame_writer_start_acquisition(userheader)
        elif message['parameter'] == "stopacquisition":
            receiver.recv()
            self.handle_frame_writer_stop_acquisition(userheader)
        elif message['parameter'] == "writeframe":
            value = receiver.recv_json()
            self.handle_frame_writer_write_frame(value)
        elif message['parameter'] == "time_slice":
            value = receiver.recv()
            self.handle_time_slice(userheader, value)
        else:
            self._logger.error('unknown parameter: ' + str(message))
            value = receiver.recv()
            self._logger.error('value: ' + str(value))

        return