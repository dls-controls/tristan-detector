import os
import time
import h5py
import sys
from nexus_swmr_file import NexusSwmrFileReader


def create_top_level_VDS_file(filename, src_dir="./", block_size=50000):
    # Create the virtual dataset file
    f = h5py.File(os.path.join(src_dir, filename), 'w', libver='latest')

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


    # Create the virtual dataset dataspace
    virt_dspace = h5py.h5s.create_simple((block_size,), (h5py.h5s.UNLIMITED,))

    # Create the virtual dataset property list
    dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

    # Create the source dataset dataspace
    src_dspace = h5py.h5s.create_simple((block_size,))

    # Select the virtual dataset first hyperslab (for the first source dataset)
    virt_dspace.select_hyperslab(start=(0,), count=(h5py.h5s.UNLIMITED,), stride=(block_size,), block=(block_size,))

    dcpl.set_virtual(virt_dspace, os.path.join(src_dir, 'test_file_%b.h5'), "/entry/data/event/event_id", src_dspace)

    # Create the virtual dataset
    dset = h5py.h5d.create(event_group.id, name="event_id", tid=h5py.h5t.NATIVE_INT32, space=virt_dspace, dcpl=dcpl)

    for x in range(0, 256):
        detector_dset[x,] = range((x * 256), (x * 256) + 256)

    cue_desc_dset[0] = "Shutter Open"
    cue_desc_dset[1] = "Shutter Close"
    cue_desc_dset[2] = "Trigger"

    # Close the file
    f.close()


def create_VDS_file(filename, blocks, src_dir="./", block_size=50000):
    # Create the virtual dataset file
    f = h5py.File(os.path.join(src_dir, filename), 'w', libver='latest')

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


    # Create the virtual dataset dataspace
    virt_dspace = h5py.h5s.create_simple((block_size,), (block_size,))

    # Create the virtual dataset property list
    dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

    dset_ptr = 0

    for block in blocks:
        print("Block : ", block)
        file_index = block[0]

        # Size of block
        block_size = block[2] - block[1] + 1
        # Create the source dataset dataspace
        src_dspace = h5py.h5s.create_simple((block_size,), (block_size,))

        # Select the source dataset hyperslab
        src_dspace.select_hyperslab(start=(block[1],), count=(1,), block=(block_size,))

        # Select the virtual dataset first hyperslab (for the first source dataset)
        virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(block_size,))

        dset_ptr += block_size

        # Set the virtual dataset hyperslab to point to the real first dataset
        src_filename = os.path.join(src_dir, "nexus_swmr_" + str(file_index+1) + ".h5")
        dcpl.set_virtual(virt_dspace, src_filename, "/entry/data/event/event_id", src_dspace)

    # Create the virtual dataset
    dset = h5py.h5d.create(event_group.id, name="event_id", tid=h5py.h5t.NATIVE_INT32, space=virt_dspace, dcpl=dcpl)

    for x in range(0, 256):
        detector_dset[x,] = range((x * 256), (x * 256) + 256)

    cue_desc_dset[0] = "Shutter Open"
    cue_desc_dset[1] = "Shutter Close"
    cue_desc_dset[2] = "Trigger"

    # Close the file
    f.close()


class TimepixVdsWriter(object):
    def __init__(self, num_files, data_dir, block_size):

        self._block_size = block_size
        self._src_dir = data_dir
        self._num_files = num_files
        self._nx_files = []
        self._event_ptrs = []
        self._slice_ptrs = []
        for fn in range(1, num_files + 1):
            self._nx_files.append(NexusSwmrFileReader(os.path.join(self._src_dir, "nexus_swmr_" + str(fn) + ".h5")))
            self._event_ptrs.append(0)
            self._slice_ptrs.append(0)

        create_top_level_VDS_file("timepix_vds.h5", self._src_dir, block_size=block_size)

    def execute(self, samples):
        print "Working ",
        sys.stdout.flush()
        eof = False
        count = 0
        current_file = 0
        file_counter = 0

        while not eof:
            time.sleep(1.0)

            event_qty = 0
            for fn in range(0, self._num_files):
                event_qty += self._nx_files[fn].event_id_size()[0]
            #print("Event qty", event_qty)
            while event_qty >= (count + self._block_size):
                blocks = []
                start_index = current_file
                # print("Create new VDS file for block")
                # Work out blocks required for next file
                # Search current file for the next time slice
                events_to_read = self._block_size
                while events_to_read > 0:
                    if self._nx_files[current_file].event_index_size()[0] > self._slice_ptrs[current_file]:
                        next_time_slice = self._nx_files[current_file].event_index_dset[self._slice_ptrs[current_file]]
                        events_ready = next_time_slice - self._event_ptrs[current_file]
                        #print("Next time slice index", next_time_slice)
                        #print("Events ready", events_ready)
                        if events_ready >= events_to_read:
                            blocks.append(
                                (current_file, self._event_ptrs[current_file], self._event_ptrs[current_file] + events_to_read - 1))
                            self._event_ptrs[current_file] += events_to_read
                            events_to_read = 0
                        else:
                            blocks.append((current_file, self._event_ptrs[current_file], next_time_slice - 1))
                            self._event_ptrs[current_file] += events_ready
                            self._slice_ptrs[current_file] += 1
                            events_to_read -= events_ready
                            current_file += 1
                            if current_file == self._num_files:
                                current_file = 0
                    else:
                        time.sleep(0.1)

                print("Blocks: ", str(blocks))

                create_VDS_file("test_file_" + str(file_counter) + ".h5", blocks, self._src_dir, self._block_size)
                file_counter += 1

                count += self._block_size
                #print("Count: ", str(count))

            if count >= samples:
                eof = True

        print ""
        for fn in range(0, self._num_files):
            self._nx_files[fn].close()

