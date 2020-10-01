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
from collections import OrderedDict

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

        self._number_of_processors = 16
        self._expected_index = []
        self._time_slices = []
        self._time_slice_data_index = []
        self._vds_index = 0
        self._vds_blocks = []
        self._vds_total_pts = 0
        self._vds_file_count = 0

        self._logger.debug('TristanMetaWriter directory ' + directory)
        # Record the directory for VDS file creation
        #self._directory = directory
        self._acquisition_id = acquisitionID

        # Add data format version dataset
        self.add_dataset_definition('data_version', (0,), maxshape=(None,), dtype='int32', fillvalue=-1)
        
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
        self._logger.info('Handling frame writer start acquisition for acqID ' + self._acquisition_id)
        self._logger.debug(userHeader)

        self.number_processes_running = self.number_processes_running + 1

        if not self.file_created:
            self._logger.info('Creating meta file for acqID ' + self._acquisition_id)
            self.create_file()
            # Create the top level VDS file
            #self._logger.info('Creating top level VDS file for acqID ' + self._acquisition_id)
            #self.create_top_level_vds_file(self.BLOCKSIZE)

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
            # Force a write of the VDS out
            #self.create_vds_file(self.BLOCKSIZE)
            self.close_file()
          # todo spawn separate app  self.create_vds_file()
        else:
            self._logger.info('Processor ended, but not the last for acqID ' + str(self._acquisition_id))

        return

    def close_file(self):
        """Close the file."""

        if self._hdf5_file is not None:
            self._logger.info('Closing file ' + self.full_file_name)
            self._hdf5_file.close()
#            self._logger.info('Meta frames written: ' + str(self._current_frame_count) + ' of ' + str(self._num_frames_to_write))
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

    def handle_data_version(self, user_header, value):
        self._logger.info('Handling data version information for acqID ' + self._acquisition_id)
        self._logger.info(user_header)
        self._logger.info(len(value))

        array = struct.unpack('i', value)
        self._hdf5_datasets['data_version'].resize(1, axis=0)
        self._hdf5_datasets['data_version'][0:1] = array
        self._hdf5_datasets['data_version'].flush()


    def handle_time_slice(self, user_header, value):
        """Handle a time slice message.  Write the time slice information
        to disk and generate any required VDS files

        :param user_header: The header
        :param value: An array of time slice sizes
        """
        self._logger.info('Handling time slice information for acqID ' + self._acquisition_id)
        self._logger.debug(user_header)
        self._logger.debug(len(value))

        # Extract the rank and array size information from the header
        rank = user_header['rank']
        index = user_header['index']
        array_size = user_header['qty']
        format_str = '{}i'.format(array_size)
        array = struct.unpack(format_str, value)
        self._logger.info("Rank: {}  Index {}  Array_Size {}".format(rank, index, array_size))
        self._logger.info(array)

        # Check to see if the expected index matches the index for this rank of message
        if self._expected_index[rank] == index:
            self._logger.info("Index match, writing to file...")
            self._time_slices[rank][self._expected_index[rank]:self._expected_index[rank]+array_size] = array
            self.write_ts_data(rank, self._expected_index[rank], array, array_size)
            self._expected_index[rank] += array_size
            #self.update_vds_blocks()

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
        elif message['parameter'] == "daq_version":
            value = receiver.recv()
            self.handle_data_version(userheader, value)
        else:
            self._logger.error('unknown parameter: ' + str(message))
            value = receiver.recv()
            self._logger.error('value: ' + str(value))

        return

