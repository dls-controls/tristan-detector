#!/usr/bin/env python

import sys;
import os;
import os.path;
import logging;
import h5py;
import numpy as np;
import argparse;
import re;
import vds_tester;
from collections import OrderedDict;

fp_per_module = None;

# vds_file file-handle of vds file
# total_events number of events across all 8 files
# longest_meta_dset the num of ints in ts_rank_n, where n chosen to make this big
# meta_dset_names_list list of the 8 dataset-names ts_rank_n n in [0,7]
# ts_size a dict of ts_rank_{} 2 ts_rank_{}_array
# raw_files a dict of ts_rank_{} 2 rawfile_name
# dset_type actually the datatype
# the_dset_name name of the dataset to create in the vds file, which is the same as in the raw files
def create_event_vdset(vds_file, total_events, longest_meta_dset, meta_dset_names_list, ts_size, raw_files, dset_type, the_dset_name):
    raw_index = {}
    for dset_name in meta_dset_names_list:
        raw_index[dset_name] = 0

    # Create the virtual dataset dataspace (qty, max_qty)
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


def create_vds_file(metafile_path, out_directory):
    """
    create the vds file with the virtual mappings

    :param metafilepath: the filepath of the metafile blah_meta.h5
    :param out_directory: the place where you want the vds file;
            by default, this is the same directory as the metafile
    """
    global fp_per_module;
    if os.path.isfile(metafile_path)==False:
        logging.error("{} does not exist".format(metafile_path));
        return;

    rawfile_directory = os.path.dirname(metafile_path);
    if out_directory==None:
        out_directory = rawfile_directory;

    logging.info("Creating VDS in {}".format(out_directory));
    logging.info("from {}".format(metafile_path));

    meta_file = h5py.File(metafile_path, 'r', libver='latest', swmr=True);

    meta_version = 0;
    if("meta_version" in meta_file.keys()):
        meta_version = meta_file["meta_version"][0];
        logging.debug("meta_version is {}".format(meta_version));

    if(meta_version==0):
        create_vds_file0(meta_file, metafile_path, out_directory);
    elif(meta_version==1 or meta_version==2): # version 2 is temporary
        if("fp_per_module" in meta_file.keys()):
            fp_per_module = meta_file["fp_per_module"][0];
            logging.debug("fp_per_module is {}".format(fp_per_module));
        else:
            logging.error("Error: fp_per_module is unspecified");
            return;
        create_vds_file1(metafile_path, out_directory);
    else:
        logging.info("unknown meta version");
    
    meta_file.close();


def create_vds_file0(meta_file, metafile_path, out_directory):
    rawfile_directory = os.path.dirname(metafile_path);
    # 8 is the length of "_meta.h5"
    file_prefix = os.path.basename(metafile_path)[0:-8];

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
            logging.error("can not find {}".format(raw_files[idx]));
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


class MyVds:
    def __init__(self):
        self.next_elt_ = {};
        self.dcpl_ = {};
        self.datatype_ = {};
        self.setup('cue_id', h5py.h5t.NATIVE_UINT16);
        self.setup("cue_timestamp_zero", h5py.h5t.NATIVE_UINT64);
        self.setup("event_id", h5py.h5t.NATIVE_UINT32);
        self.setup("event_energy", h5py.h5t.NATIVE_UINT32);
        self.setup("event_time_offset", h5py.h5t.NATIVE_UINT64);

    def setup(self, dset, dtype):
        self.next_elt_[dset] = 0;
        self.datatype_[dset] = dtype;
        self.dcpl_[dset] = h5py.h5p.create(h5py.h5p.DATASET_CREATE);

    def open(self, vds_filename):
        logging.info("creating vds file {}".format(vds_filename));
        self.vds_file_ = h5py.File(vds_filename, 'w', libver='latest');

    def close(self):
        for vds_dset_name in self.dcpl_.keys():
            dset_type = self.datatype_[vds_dset_name];
            dcpl = self.dcpl_.get(vds_dset_name);
            num_elts = self.next_elt_.get(vds_dset_name);
            virt_dspace = h5py.h5s.create_simple((num_elts,), (num_elts,));
            h5py.h5d.create(self.vds_file_.id, name=vds_dset_name, tid=dset_type, space=virt_dspace, dcpl=dcpl)

        self.vds_file_.close();


    def create_ctrl_maps1(self, raw_files):
        ctrl_lens = [];
        for raw_file in raw_files:
            rf = h5py.File(raw_file, 'r', libver='latest', swmr=True)
            cue_dset = np.array(rf['cue_id'][:])
            # argmin gives the index of the first 0 in the dset, signalling the end of the cues?
            ctrl_lens.append(np.argmin(cue_dset))

        for vds_dset_name in ["cue_id","cue_timestamp_zero"]:
            logging.info("Processing {} cues for dset {}".format(sum(ctrl_lens), vds_dset_name))

            for raw_file, ctrl_len in zip(raw_files, ctrl_lens):
                dset_ptr = self.next_elt_[vds_dset_name];
                virt_dspace = h5py.h5s.create_simple((0,), (0,));
                src_dspace  = h5py.h5s.create_simple((0,), (0,));

                src_dspace.select_hyperslab(start=(0,), count=(1,), block=(ctrl_len,));
                virt_dspace.select_hyperslab(start=(dset_ptr,), count=(1,), block=(ctrl_len,));

                logging.debug("creating mapping of {} cues at v-offset of {}".format(ctrl_len, dset_ptr)); 

                # Save this mapping in the dcpl
                self.dcpl_.get(vds_dset_name).set_virtual(virt_dspace, os.path.basename(raw_file), "/" + vds_dset_name, src_dspace);
                self.next_elt_[vds_dset_name] += ctrl_len

    # raw_file can be the absolute path; the function will strip to the basename which we want for relative references
    def create_event_map1(self, vdsoffset, raw_file, roffset, event_count):
        for the_dset_name in ["event_time_offset","event_id","event_energy"]:
            src_dspace  = h5py.h5s.create_simple((0,), (0,));
            virt_dspace = h5py.h5s.create_simple((0,), (0,));

            src_dspace.select_hyperslab(start=(roffset,), count=(1,), block=(event_count,));
            virt_dspace.select_hyperslab(start=(vdsoffset,), count=(1,), block=(event_count,));

            self.dcpl_[the_dset_name].set_virtual(virt_dspace, os.path.basename(raw_file), "/" + the_dset_name, src_dspace);
            if the_dset_name == "event_id":
                logging.debug("creating mapping {} from vds{} rf{} roff{} qty{}".format(the_dset_name, vdsoffset, os.path.basename(raw_file), roffset, event_count) );
            if(vdsoffset != self.next_elt_[the_dset_name]):
                raise Exception("problem with vds offset");
            self.next_elt_[the_dset_name] += event_count;
    

def create_vds_file1(metafile_path, out_directory):

    metafile = vds_tester.Metafile(metafile_path);

    rawfile_directory = os.path.dirname(metafile_path);
    # 8 is the length of "_meta.h5"
    file_prefix = os.path.basename(metafile_path)[0:-8];

    vds_filename = os.path.join(out_directory, metafile.getVdsFilename())
    vds_file = MyVds();
    vds_file.open(vds_filename);

    vds_file.create_ctrl_maps1(metafile.getRawFilenames());

    timeslices = metafile.getTimesliceLookup();

    for vdsoffset in sorted(timeslices.keys()):
        params = timeslices[vdsoffset];
        rfn = params[0];
        roffset = params[1];
        qty = params[2];
        vds_file.create_event_map1(vdsoffset, rfn, roffset, qty);
    vds_file.close();

def createtestfiles1():
    # v1 test files
    rawfile_directory = ".";
    file_prefix = "blah";

    meta_filename = os.path.join(rawfile_directory, file_prefix + "_meta.h5");
    mf = h5py.File(meta_filename, 'w', libver='latest');
    fp_per_mod = 2;
    mf["fp_per_module"] = [fp_per_mod];
    mf["meta_version"] = [1];
    num_modules = 2;
    mf["ts_qty_module00"] = np.array([0,0,0,3,2,4,1,0,0,0]).astype(np.int32);
    mf["ts_qty_module01"] = np.array([0,0,1,1,2,2,0,3,0,0]).astype(np.int32);
    data_array1 = np.array(range(20)).astype(np.int32);
    for index in range(0,fp_per_mod * num_modules):
        raw_filename = os.path.join(rawfile_directory, file_prefix + "_{:06}.h5".format(index+1));
        rf = h5py.File(raw_filename, 'w', libver='latest');

        rf["event_id"] = data_array1;
        rf["event_time_offset"] = data_array1; # astype(np.int64)
        rf["event_energy"] = data_array1+index;

        # reverse the data-array for cue_ids because we want 0 at the end
        rf["cue_id"] = data_array1[20::-1];
        rf["cue_timestamp_zero"] = data_array1+index;
        rf.close();

def main():
    logfile = "vdscreation.log";
    os.remove(logfile) if os.path.exists(logfile) else 0;
    logging.basicConfig(level=logging.DEBUG, filename=logfile);
    parser = argparse.ArgumentParser(description="create tristan vds file from metafile+raw data files. be careful what you run this on as nx-staff is bugged.");
    parser.add_argument("-i", nargs=1, required=True, help="full path to metafile, raw files should be next to it");
    parser.add_argument("--outdir", nargs=1, required=False, help="directory for vds file (default = same as metafile)");    

    args = parser.parse_args();

    metafile = None;
    outdir = None;

    if(len(args.i)==1):
        metafile = args.i[0];
    if(args.outdir and len(args.outdir)==1):
        outdir = args.outdir[0];

    if(metafile == "createtestfiles"):
        createtestfiles1();
    else:
        create_vds_file(metafile, outdir);

if __name__ == '__main__':
    main ();


