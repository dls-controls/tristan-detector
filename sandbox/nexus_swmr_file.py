import h5py

class NexusSwmrFileReader(object):
    def __init__(self, filename):
        self.nx_file = h5py.File(filename, 'r', libver='latest', swmr=True)

        self._event_id_dset = self.nx_file["/entry/data/event/event_id"]
        self._event_index_dset = self.nx_file["/entry/data/event/event_index"]

    def event_id_size(self):
        self._event_id_dset.id.refresh()
        return self._event_id_dset.shape

    def event_index_size(self):
        self._event_index_dset.id.refresh()
        return self._event_index_dset.shape

    @property
    def event_index_dset(self):
        self._event_index_dset.id.refresh()
        return self._event_index_dset

    def close(self):
        self.nx_file.close()


class NexusSwmrFileWriter(object):
    def __init__(self, filename):
        self.nx_file = h5py.File(filename, 'w', libver='latest', swmr=True)

        self.entry_group = self.nx_file.create_group("entry")
        self.entry_group.attrs["NX_class"] = "NXentry"

        self.instrument_group = self.entry_group.create_group("instrument")
        self.instrument_group.attrs["NX_class"] = "NXinstrument"

        self.detector_group = self.instrument_group.create_group("detector")
        self.detector_group.attrs["NX_class"] = "NXdetector"

        self._detector_dset = self.detector_group.create_dataset("detector_number", shape=(256, 256),
                                                                 maxshape=(256, 256), chunks=(256, 256), dtype='i4')

        self._cue_desc_dset = self.detector_group.create_dataset("cue_description", shape=(3,), maxshape=(3,), dtype='S32')

        self.data_group = self.entry_group.create_group("data")
        self.data_group.attrs["NX_class"] = "NXdata"

        self.event_group = self.data_group.create_group("event")
        self.event_group.attrs["NX_class"] = "NXevent_data"

        self._event_id_dset = self.event_group.create_dataset("event_id", shape=(1,), maxshape=(None,),
                                                             chunks=(1024 * 1024,), dtype='i4')
        self._event_time_offset = self.event_group.create_dataset("event_time_offset", shape=(1,), maxshape=(None,),
                                                                 chunks=(1024 * 1024,), dtype='i8')

        self._event_index_dset = self.event_group.create_dataset("event_index", shape=(1,), maxshape=(None,),
                                                              chunks=(1024 * 1024,), dtype='i4')

        self._event_time_zero_dset = self.event_group.create_dataset("event_time_zero", shape=(1,), maxshape=(None,),
                                                              chunks=(1024 * 1024,), dtype='i8')

        self._cue_index_dset = self.event_group.create_dataset("cue_index", shape=(1,), maxshape=(None,),
                                                               chunks=(1024 * 1024,), dtype='i4')

        self._cue_id_dset = self.event_group.create_dataset("cue_id", shape=(1,), maxshape=(None,),
                                                            chunks=(1024 * 1024,), dtype='i4')

        self._cue_timestamp_zero_dset = self.event_group.create_dataset("cue_timestamp_zero", shape=(1,),
                                                                        maxshape=(None,), chunks=(1024 * 1024,),
                                                                        dtype='i8')

        self.cue_desc_dset[0] = "Shutter Open"
        self.cue_desc_dset[1] = "Shutter Close"
        self.cue_desc_dset[2] = "Trigger"

        self.nx_file.swmr_mode = True

    @property
    def detector_dset(self):
        return self._detector_dset

    @property
    def cue_desc_dset(self):
        return self._cue_desc_dset

    @property
    def event_id_dset(self):
        return self._event_id_dset

    @property
    def event_time_offset(self):
        return self._event_time_offset

    @property
    def event_index_dset(self):
        return self._event_index_dset

    @property
    def event_time_zero_dset(self):
        return self._event_time_zero_dset

    @property
    def cue_index_dset(self):
        return self._cue_index_dset

    @property
    def cue_id_dset(self):
        return self._cue_id_dset

    @property
    def cue_timestamp_zero_dset(self):
        return self._cue_timestamp_zero_dset

    def close(self):
        self.nx_file.close()

