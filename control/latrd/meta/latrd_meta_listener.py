try:
    from pkg_resources import require
    require('pygelf==0.2.11')
    require("h5py==2.7.0")
    require('pyzmq')
except:
    pass


import argparse
import logging
from meta_writer import MetaWriter
from meta_listener import MetaListener


class LATRDMetaWriter(MetaWriter):
    def __init__(self, logger, directory, acquisition_id):
        super(LATRDMetaWriter, self).__init__(logger, directory, acquisition_id)
        self.add_dataset_definition("event_index", (0,), maxshape=(None,), dtype='int64', fillvalue=-1)
        self.add_dataset_definition("event_time_zero", (0,), maxshape = (None,), dtype = 'int64', fillvalue = -1)
        self.add_dataset_definition("cue_index", (0,), maxshape=(None,), dtype='int32', fillvalue=-1)
        self.add_dataset_definition("cue_id", (0,), maxshape=(None,), dtype='int32', fillvalue=-1)
        self.add_dataset_definition("cue_timestamp_zero", (0,), maxshape=(None,), dtype='int64', fillvalue=-1)

    def process_message(self, message):
        self._logger.debug("Message handled within LATRD handler: %s", message)
        if not self._file_created:
            self.create_file()

        # Check for the time_slice_index
        if message['parameter'] == 'time_slice_index':
            self.add_dataset_value('event_index', message['value'])
        elif message['parameter'] == 'time_slice_ts':
            self.add_dataset_value('event_time_zero', message['value'])
        elif message['parameter'] == 'control_word_index':
            self.add_dataset_value('cue_index', message['value'])
        elif message['parameter'] == 'control_word':
            self.add_dataset_value('cue_id', self.ctrl_type(message['value']))
            self.add_dataset_value('cue_timestamp_zero', self.timestamp_course(message['value']))

    def is_ctrl(self, word):
        ctrl_data = word & 0x8000000000000000
        if ctrl_data == 0:
            return False
        return True

    def ctrl_type(self, word):
        if not self.is_ctrl(word):
            raise RuntimeError()
        return (word >> 58) & 0x0F

    def timestamp_course(self, word):
        if not self.is_ctrl(word):
            raise RuntimeError()
        return word & 0x000FFFFFFFFFFFF8


def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputs", default="tcp://127.0.0.1:5558", help="Input endpoints - comma separated list")
    parser.add_argument("-d", "--directory", default="/tmp/", help="Default directory to write meta data files to")
    parser.add_argument("-c", "--ctrl", default="5659", help="Control channel port to listen on")
    args = parser.parse_args()
    return args


def main():
    args = options()
    logging.basicConfig(level=logging.DEBUG)

    mh = MetaListener(args.directory, args.inputs, args.ctrl, "meta_listener_app.LATRDMetaWriter")
    mh.run()

if __name__ == "__main__":
    main()