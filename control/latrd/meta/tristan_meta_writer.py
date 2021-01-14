""" Implementation of meta writer for Tristan

This module is a subclass of the odin_data MetaWriter and handles Tristan specific
meta data messages, writing them to disk
"""

import re
from collections import OrderedDict
from json import loads
import numpy as np
import re
import struct

from odin_data.meta_writer.meta_writer import MetaWriter, MESSAGE_TYPE_ID, FRAME, require_open_hdf5_file
from odin_data.meta_writer.hdf5dataset import Int32HDF5Dataset
from odin_data.util import construct_version_dict
import _version as versioneer

# Dataset names
DATASET_TIME_SLICE = "ts_qty_module"
DATASET_DAQ_VERSION = "data_version"
DATASET_META_VERSION = "meta_version"
DATASET_FP_PER_MODULE = "fp_per_module"

# Data message parameters
TIME_SLICE = "time_slice"
DAQ_VERSION = "daq_version"

# Meta file version
# Version 1: First revision
#            Table for each FP time slice count.
#            No version information saved.
#
# Version 2: DAQ version and Meta file version saved.
#            Table for each server with condensed time slice information.
META_VERSION_NUMBER = 1

class TristanMetaWriter(MetaWriter):
    """ Implementation of MetaWriter that also handles Tristan meta messages """
    TRISTAN_DATASETS = []

    def __init__(self, name, directory, process_count, endpoints):

        # Create ordered dict to store FP mapping entries
        self._fp_mapping = OrderedDict()
        for endpoint in endpoints:
            protocol, ip, port = endpoint.split(':')
            ip = ip.strip('//')
            if ip in self._fp_mapping:
                self._fp_mapping[ip] += 1
            else:
                self._fp_mapping[ip] = 1


        # Create the tristan specific datasets
        TristanMetaWriter.TRISTAN_DATASETS.append(Int32HDF5Dataset(DATASET_DAQ_VERSION))
        TristanMetaWriter.TRISTAN_DATASETS.append(Int32HDF5Dataset(DATASET_META_VERSION))
        TristanMetaWriter.TRISTAN_DATASETS.append(Int32HDF5Dataset(DATASET_FP_PER_MODULE))
        
        for index in range(len(self._fp_mapping)):
            TristanMetaWriter.TRISTAN_DATASETS.append(Int32HDF5Dataset("{}{:02d}".format(DATASET_TIME_SLICE, index), fillvalue=0, cache=False))

        # Now that we have defined the datasets we can call the base class constructor
        super(TristanMetaWriter, self).__init__(name, directory, process_count, endpoints)

        # Log the current dataset configuration
        self._logger.info("Set up Tristan writer for endpoints: {}".format(endpoints))
        for dset in TristanMetaWriter.TRISTAN_DATASETS:
            self._logger.info("Tristan dataset added: {} [{}]".format(dset.name, dset.dtype))

    @property
    def detector_message_handlers(self):
        message_handlers = {
            TIME_SLICE: self.handle_time_slice,
            DAQ_VERSION: self.handle_daq_version
        }
        return message_handlers

    @staticmethod
    def _define_detector_datasets():
        print("METHOD CALLED: {}".format(TristanMetaWriter.TRISTAN_DATASETS))
        return TristanMetaWriter.TRISTAN_DATASETS

    @require_open_hdf5_file
    def _create_datasets(self, dataset_size):
        super(TristanMetaWriter, self)._create_datasets(dataset_size)
        self._logger.info("Hooked into create datasets...")
        # Save the Met File version
        self._add_value(DATASET_META_VERSION, META_VERSION_NUMBER, offset=0)
        # Save the FP mapping values
        index = 0
        for ip in self._fp_mapping:
            self._add_value(DATASET_FP_PER_MODULE, self._fp_mapping[ip], offset=index)
            index += 1

    def rank_to_module(self, rank):
        module = 0
        for ip in self._fp_mapping:
            if rank >= self._fp_mapping[ip]:
                module += 1
                rank -= self._fp_mapping[ip]
        self._logger.debug("Rank {} calculated to module {}".format(rank, module))
        return module

    def handle_time_slice(self, user_header, data):
        """Handle a time slice message.  Write the time slice information
        to disk and generate any required VDS files

        :param user_header: The header
        :param value: An array of time slice sizes
        """
        self._logger.debug('handle_time_slice | User information: {}'.format(user_header))

        # Extract the rank and array size information from the header
        rank = user_header['rank']
        index = user_header['index']
        array_size = user_header['qty']
        format_str = '{}i'.format(array_size)
        array = struct.unpack(format_str, data)
        self._logger.debug("Rank: {}  Index {}  Array_Size {}".format(rank, index, array_size))
        self._logger.debug(array)
        module = self.rank_to_module(rank)

        dataset_name = "{}{:02d}".format(DATASET_TIME_SLICE, module)
        # Now loop over provided data, and write any non-zero values
        for array_index in range(len(array)):
            if array[array_index] > 0:
                offset = index + array_index
                self._logger.debug("Adding value {} to offset {}".format(array[array_index], offset))
                self._add_value(dataset_name, array[array_index], offset=offset)

    def handle_daq_version(self, user_header, data):
        """Handle a time slice message.  Write the time slice information
        to disk and generate any required VDS files

        :param user_header: The header
        :param value: An array of time slice sizes
        """
        self._logger.debug('handle_daq_version | User information: {}'.format(user_header))

        # Extract the rank information from the header
        rank = user_header['rank']
        value = struct.unpack('i', data)
        self._logger.debug("Rank: {}  Version: {}".format(rank, value))
        self._add_value(DATASET_DAQ_VERSION, value, offset=rank)

