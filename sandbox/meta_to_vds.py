#!/usr/bin/env python

import sys;
import os.path;
import logging;
import h5py;
import numpy as np;
import argparse;
from collections import OrderedDict;


def create_event_vdset(vds_file, total_events, longest_meta_dset, meta_dset_names_list, ts_size, raw_files, dset_type, the_dset_name):
    raw_index = {}
    for dset_name in meta_dset_names_list:
        raw_index[dset_name] = 0

    # Create the virtual dataset dataspace
    virt_dspace = h5py.h5s.create_simple((total_events,), (total_events,))

    # Create the virtual dataset property list
    dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

    dset_ptr = 0

    # Loop over the ts_rank dataset elements, creating a mapping for each
    # Check for the longest list
    logging.info("Processing up to {} time slice entries for dset {}".format(longest_meta_dset, the_dset_name))
    for dset_index in range(0, longest_meta_dset):
        for dset_name in meta_dset_names_list:
            # If the element exists and is greater than 0 then create the mapping
            try:
                event_count = ts_size[dset_name][dset_index]
                if event_count > 0:
                    src_dspace = h5py.h5s.create_simple((event_count,), (event_count,))

                    # Select the source dataset hyperslab
                    src_dspace.select_hyperslab(start=(raw_index[dset_name],), count=(1,), block=(event_count,))

                    # Select the virtual dataset first hyperslab (for the first source dataset)
                    virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(event_count,))

                    logging.debug("creating mapping from {} of {} events at offset {}".format(dset_name, event_count, dset_ptr));
                    dset_ptr += event_count
                    raw_index[dset_name] += event_count

                    # Set the virtual dataset hyperslab to point to the real dataset
                    dcpl.set_virtual(virt_dspace, os.path.basename(raw_files[dset_name]), "/" + the_dset_name, src_dspace)

            except IndexError:
                pass
    # Create the virtual dataset
    h5py.h5d.create(vds_file.id, name=the_dset_name, tid=dset_type, space=virt_dspace, dcpl=dcpl)

def create_ctrl_vdset(meta_dset_ordered_list, raw_files, ctrl_sizes, vds_file, dset_type, vds_dset_name):

    total_num_ctrls = sum(ctrl_sizes)
    logging.info("Processing {} cues for dset {}".format(total_num_ctrls, vds_dset_name))

    # Create the virtual dataset dataspace
    virt_dspace = h5py.h5s.create_simple((total_num_ctrls,), (total_num_ctrls,))

    # Create the virtual dataset property list
    dcpl = h5py.h5p.create(h5py.h5p.DATASET_CREATE)

    dset_ptr = 0

    # Loop over the ts_rank dataset elements, creating a mapping for each
    # Check for the longest list

    # Loop over the ordered dset_names
    for dset_name, ctrl_size in zip(meta_dset_ordered_list, ctrl_sizes):
        # If the element exists and is greater than 0 then create the mapping
        try:
            src_dspace = h5py.h5s.create_simple((ctrl_size,), (ctrl_size,))

            # Select the source dataset hyperslab
            src_dspace.select_hyperslab(start=(0,), count=(1,), block=(ctrl_size,))

            # Select the virtual dataset first hyperslab (for the first source dataset)
            virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(ctrl_size,))

            logging.debug("creating mapping of {} cues at offset {}".format(ctrl_size, dset_ptr)); 
            dset_ptr += ctrl_size

            # Set the virtual dataset hyperslab to point to the real dataset
            dcpl.set_virtual(virt_dspace, os.path.basename(raw_files[dset_name]), "/" + vds_dset_name, src_dspace)

        except IndexError:
            pass
    # Create the virtual dataset
    h5py.h5d.create(vds_file.id, name=vds_dset_name, tid=dset_type, space=virt_dspace, dcpl=dcpl)


def create_vds_file(metafile_path, out_directory=None):
    """
    create the vds file with the virtual mappings

    :param metafilepath: the filepath of the metafile blah_meta.h5
    :param out_directory: the place where you want the vds file;
            by default, this is the same directory as the metafile
    """

    if os.path.isfile(metafile_path)==False:
        logging.error("{} does not exist".format(metafile_path));
        return;


    rawfile_directory = os.path.dirname(metafile_path);
    if out_directory==None:
        out_directory = rawfile_directory;

    file_prefix = os.path.basename(metafile_path)[0:-8];

    logging.info("Creating VDS in {}".format(out_directory));
    logging.info("from {}".format(metafile_path));

    meta_file = h5py.File(metafile_path, 'r', libver='latest', swmr=True);

    dset_names = meta_file.keys()
    # orderedDict remembers the order items were added, and uses that order in iterations
    raw_files = OrderedDict()
    raw_index = {}
    ts_size = {}

    longest_meta_dset = 0
    meta_dset_names_list = []
    total_events = 0
    # Now generate the raw data filenames, there should be 1 for each ts_rank_n value
    for index in range(0, len(dset_names)-1):
        raw_filename = os.path.join(rawfile_directory, file_prefix + "_{:06}.h5".format(index+1))
        dset_name = "ts_rank_{}".format(index)
        logging.debug("Analysing dataset {} for {}".format(dset_name, raw_filename))
        meta_dset_names_list.append(dset_name)

        if len(meta_file[dset_name]) > longest_meta_dset:
            longest_meta_dset = len(meta_file[dset_name])

        raw_files[dset_name] = raw_filename
        ts_size[dset_name] = meta_file[dset_name][:]
        total_events += sum(ts_size[dset_name])

    for idx in raw_files:
        if os.path.isfile(raw_files[idx]) == False:
            logging.Error("can not find {}".format(raw_files[idx]));
            return;
        
    logging.info("Total number of events: {}".format(total_events))

    vds_filename = os.path.join(out_directory, file_prefix + "_vds.h5")
    vds_file = h5py.File(vds_filename, 'w', libver='latest')

    create_event_vdset(vds_file, total_events, longest_meta_dset, meta_dset_names_list, ts_size, raw_files, h5py.h5t.NATIVE_UINT32, "event_id")
    create_event_vdset(vds_file, total_events, longest_meta_dset, meta_dset_names_list, ts_size, raw_files, h5py.h5t.NATIVE_UINT32, "event_energy")
    create_event_vdset(vds_file, total_events, longest_meta_dset, meta_dset_names_list, ts_size, raw_files, h5py.h5t.NATIVE_UINT64, "event_time_offset")

    # Now open the dataset for cue_id and timestamp
    ctrl_lens = []
    for raw_index in raw_files:
        raw_file = h5py.File(raw_files[raw_index], 'r', libver='latest', swmr=True)
        cue_dset = np.array(raw_file['cue_id'][:])
        # argmin gives the index of the first 0 in the dset, signalling the end of the cues
        ctrl_lens.append(np.argmin(cue_dset))
        
    create_ctrl_vdset(meta_dset_names_list, raw_files, ctrl_lens, vds_file, h5py.h5t.NATIVE_UINT16, "cue_id")
    create_ctrl_vdset(meta_dset_names_list, raw_files, ctrl_lens, vds_file, h5py.h5t.NATIVE_UINT64, "cue_timestamp_zero")

    vds_file.close()
    logging.info("created {}".format(vds_filename));



def main():
    logging.basicConfig(level=logging.INFO);
    parser = argparse.ArgumentParser(description="create tristan vds file from metafile+raw data files");
    parser.add_argument("-i", nargs=1, required=True, help="full path to metafile, raw files should be next to it");
    parser.add_argument("--outdir", nargs=1, required=False, help="directory for vds file (default = same as metafile)");    

    args = parser.parse_args();

    metafile = None;
    outdir = None;
    if(len(args.i)==1):
        metafile = args.i[0]; 
    if(args.outdir and len(args.outdir)==1):
        outdir = args.outdir[0];
    create_vds_file(metafile, outdir);


if __name__ == '__main__':
    main ();


