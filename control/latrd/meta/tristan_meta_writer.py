""" Implementation of meta writer for Tristan

This module is a subclass of the odin_data MetaWriter and handles Tristan specific
meta data messages, writing them to disk
"""

import re
from json import loads
import numpy as np
import re
import struct

from odin_data.meta_writer.meta_writer import MetaWriter, MESSAGE_TYPE_ID, FRAME
from odin_data.meta_writer.hdf5dataset import Int32HDF5Dataset
from odin_data.util import construct_version_dict
import _version as versioneer

# Dataset names
DATASET_TIME_SLICE = "ts_rank"
DATASET_DAQ_VERSION = "data_version"

# Data message parameters
TIME_SLICE = "time_slice"
DAQ_VERSION = "daq_version"

class TristanMetaWriter(MetaWriter):
    """ Implementation of MetaWriter that also handles Tristan meta messages """

    def __init__(self, name, directory, process_count):
        # Add the version and time_slice datasets
        MetaWriter.DETECTOR_DATASETS = [
            Int32HDF5Dataset(DATASET_DAQ_VERSION)
        ]
        for index in range(process_count):
            MetaWriter.DETECTOR_DATASETS.append(Int32HDF5Dataset("{}_{}".format(DATASET_TIME_SLICE, index)))

        # Add the parameters received on meta messages
        MetaWriter.DETECTOR_WRITE_FRAME_PARAMETERS = [
            TIME_SLICE,
            DAQ_VERSION
        ]

        super(TristanMetaWriter, self).__init__(name, directory, process_count)
        for dset in MetaWriter.DETECTOR_DATASETS:
            self._logger.debug("Tristan dataset added: {} [{}]".format(dset.name, dset.dtype))

    def process_message(self, header, data):
        """ Process a message from a data socket

        Check for Tristan specific messages and pass anything else to base class
        """
        self._logger.debug("Message ID: {}".format(header[MESSAGE_TYPE_ID]))
        message_handlers = {
            TIME_SLICE: self.handle_time_slice,
            DAQ_VERSION: self.handle_daq_version
        }

        handler = message_handlers.get(header[MESSAGE_TYPE_ID], None)
        if handler is not None:
            handler(header["header"], data)
        else:
            self._logger.debug(
                "%s | Passing message %s to base class",
                self._name,
                header[MESSAGE_TYPE_ID],
            )
            super(TristanMetaWriter, self).process_message(header, data)

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

        dataset_name = "{}_{}".format(DATASET_TIME_SLICE, rank)
        for value in array:
            self._add_value(dataset_name, value)

    def handle_daq_version(self, user_header, data):
        """Handle a time slice message.  Write the time slice information
        to disk and generate any required VDS files

        :param user_header: The header
        :param value: An array of time slice sizes
        """
        self._logger.debug('handle_daq_version | User information: {}'.format(user_header))

        # Extract the rank information from the header
        rank = user_header['rank']
        self._add_value(DATASET_DAQ_VERSION, data, offset=rank)

