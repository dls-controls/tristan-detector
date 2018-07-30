import h5py
import os

class MetaWriter(object):
    def __init__(self, logger, directory, acquisition_id):
        self._logger = logger
        self._directory = directory
        self._acquisition_id = acquisition_id
        self._hdf5_file = None
        self._full_file_name = None
        self._file_created = False
        self._data_set_definition = {}
        self._hdf5_datasets = {}
        self._data_set_arrays = {}
        self.start_new_acquisition()

    def start_new_acquisition(self):
        meta_file_name = self._acquisition_id + '_meta.hdf5'
        self._full_file_name = os.path.join(self._directory, meta_file_name)

    def process_message(self, message):
        self._logger.debug(message)

    def create_file(self):
        self._logger.info("Creating meta data file: %s" % self._full_file_name)
        self._hdf5_file = h5py.File(self._full_file_name, "w", libver='latest')
        self._create_datasets()
        self._file_created = True

    def _create_datasets(self):
        # Create dataset for each definition
        for dset_name in self._data_set_definition:
            dset = self._data_set_definition[dset_name]
            self._data_set_arrays[dset_name] = []
            self._hdf5_datasets[dset_name] = self._hdf5_file.create_dataset(dset_name,
                                                                            dset['shape'],
                                                                            maxshape=dset['maxshape'],
                                                                            dtype=dset['dtype'],
                                                                            fillvalue=dset['fillvalue'])

    def write_datasets(self):
        self._logger.warn("Parent _create_datasets has been called.  Should be overridden")
        for dset_name in self._data_set_definition:
            self._logger.warn("Length of [%s] dataset: %d", dset_name, len(self._data_set_arrays[dset_name]))
            self._hdf5_datasets[dset_name].resize((len(self._data_set_arrays[dset_name]),))
            self._hdf5_datasets[dset_name][0:len(self._data_set_arrays[dset_name])] = self._data_set_arrays[dset_name]

    def add_dataset_definition(self, dset_name, shape, maxshape, dtype, fillvalue):
        self._data_set_definition[dset_name] = {
            'shape': shape,
            'maxshape': maxshape,
            'dtype': dtype,
            'fillvalue': fillvalue
        }

    def add_dataset_value(self, dset_name, value):
        self._data_set_arrays[dset_name].append(value)

    def close_file(self):
        self.write_datasets()

        if self._hdf5_file is not None:
            self._logger.info('Closing file ' + self._full_file_name)
            self._hdf5_file.close()
            self._hdf5_file = None

    def stop(self):
        self.close_file()
