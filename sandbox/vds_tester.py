#!/usr/bin/env python

# This was written to test the vds files created with meta_to_vds.py which creates vds files.
# It can do two things:
#  1) for each module it can count the number of events in two ways: reading the meta file and
#        looking in the rawfiles. The two numbers should be the same.
#  2) it can make a textdump of all the timestamps (or whatever) in two ways: reading the vds
#        file, and reading from the raw files. The text dumps should be the same, and since they
#        are sorted you should be able to check this quickly with diff.
#        

import sys;
import os;
import os.path;
import logging;
import h5py;
import numpy as np;
import argparse;
import re;
from collections import OrderedDict;

class Metafile:
    def __init__(self, metafile):
        # create all the others: rawfilenames by module, vdsfilename
        if os.path.isfile(metafile)==False:
            raise Exception("{} does not exist".format(metafile));

        if "_meta.h5" not in metafile:
            raise Exception("file is not a metafile");

        logging.info("metafile is {}".format(metafile));

        self.file_prefix_ = file_prefix = metafile[0:-8];
        self.metafname_ = metafile;
        self.vdsfname_ = self.file_prefix_ + "_vds.h5";

        self.metafile_ = h5py.File(metafile, "r");
        self.fp_per_module_ = self.metafile_["fp_per_module"].value;

        self.mod2rawfiles_ = {};
        index = 0;
        for mod_idx in range(len(self.fp_per_module_)):
            mod_name = "module{:02}".format(mod_idx);
            self.mod2rawfiles_[mod_name] = [];
            for rf_idx in range(0, self.fp_per_module_[mod_idx]):
                raw_filename = file_prefix + "_{:06}.h5".format(index+1);
                if os.path.isfile(raw_filename)==False:
                    raise Exception("missing rawfile");
                self.mod2rawfiles_[mod_name].append(raw_filename);
                index += 1;

        self.createTimesliceLookup();
        self.valid = True;

    # this is a huge important function that creates the mappings for the vds file.
    # Each timeslice is stored as: (vds-offset, [rf, rf-offset, qty]).
    def createTimesliceLookup(self):
        self.vdsoffset2params_ = {};
        mod_names = self.getModuleNames();
      #  logging.info("module names is sorted? {}".format(mod_names));
        ts_qties = [self.getTsQtiesForModule(m) for m in mod_names];
        rf_names = [self.getRawFilenamesForModule(m) for m in mod_names];
        max_num_ts = max([len(l) for l in ts_qties]);

      #  logging.info("max num ts in any module is {}".format(max_num_ts));

        rawfile2offset = {};
        for rf in self.getRawFilenames():
            rawfile2offset[rf] = 0;

        vdsoffset = 0;
        for idx in range(max_num_ts):
       #     if idx % 1000 == 0:
        #        logging.info("pprogress {}".format(idx));
            for idx2 in range(len(mod_names)):
                tslist = ts_qties[idx2];
                rflist = rf_names[idx2];
                if idx < len(tslist):
                    qty = tslist[idx];
                    if 0<qty:
                        rf = rflist[idx % len(rflist)];
                      #  rf_short = os.path.basename(rf);
                      #  logging.info("rawf: {} qty {} vdsidx {} rawfoffset {}".format(rf_short,qty,vdsoffset,rawfile2offset[rf]));
                        self.vdsoffset2params_[vdsoffset] = [rf, rawfile2offset[rf], qty];
                        vdsoffset += qty;
                        rawfile2offset[rf] += qty;

    # remember you need to sort these yourself; python maps are not sorted.
    # the map is vdsoffset -> [fullyqualif-rawfilename, roffset, qty]
    def getTimesliceLookup(self):
        return self.vdsoffset2params_;


    def getNumModules(self):
        return self.numModules_;

    def getNumRawFilesPerModule(self):
        return self.fp_per_module_[0];

    def getRawFilenamesForModule(self, mod_name):
        return self.mod2rawfiles_[mod_name];

    def getRawFilenames(self):
        return sorted(sum(self.mod2rawfiles_.values(),[]));

    def getVdsFilename(self):
        return self.vdsfname_;

    def getMetafileName(self):
        return self.metafname_;

    def getSourceDir(self):
        return os.path.normpath(os.path.dirname(self.getMetafileName()));

    def getTsQtiesForRawfile(self, rf):
        mod = 0;
        ridx = -1;
        for (m,r) in self.mod2rawfiles_.items():
            if rf in r:
                mod = m;
                ridx = r.index(rf);

        if(mod==0):
            raise Exception("unknown raw file " + rf);

        ts_qties_mod = self.getTsQtiesForModule(mod);
        ts_qties_rf = ts_qties_mod[ridx::self.getNumRawFilesPerModule()];
        return ts_qties_rf;

    def getOffsetsForRawfile(self, rf):
        qties = self.getTsQtiesForRawfile(rf);
        offsets = [];
        offset = 0;
        for qt in qties:
            offsets.append(offset);
            offset += qt;
        return offsets;
        
        
    def getTsQtiesForModule(self, mod_name):
        dset = "ts_qty_{}".format(mod_name);
        return self.metafile_[dset].value;

    def getTsQtiesForAll(self):
        pass;

    def getModuleNames(self):
        return sorted(self.mod2rawfiles_.keys());

# this is the dataset that the 2 dump functions dump to text file.
# Suggest cue_id to check cues,
# and event_time_offset (altho event_id is 4 bytes).
DATASET_TO_DUMP = "event_time_offset";

# this function looks in the metafile and sees how many events are
# referenced for each module / rawfile. then it looks at the rawfiles,
# and sums how many events are the dataset (excl end zeros).
def check_qties(metafile):
    mod2events2 = {};
    mod2events1 = {};
    rf2events2 = {};
    rf2events1 = {};
    total = 0;
    for mod_name in metafile.getModuleNames():
            ts_this_mod = sum(metafile.getTsQtiesForModule(mod_name));
            mod2events1[mod_name] = ts_this_mod;
            logging.info("{} references {} events in the metafile".format(mod_name, ts_this_mod));
            rawfs = metafile.getRawFilenamesForModule(mod_name);
            for rfn in rawfs:
                ts_qties = metafile.getTsQtiesForRawfile(rfn);
                rf2events1[rfn] = sum(ts_qties);
                logging.info("  {} has {} events according to metafile".format(os.path.basename(rfn), sum(ts_qties)));
            total += ts_this_mod;

    logging.info( "total events metafile captures: {}\n".format(total) );

    # now see how many timestamps are in each raw file:
    for mod_name in metafile.getModuleNames():
        total = 0;
        for rfn in metafile.getRawFilenamesForModule(mod_name):
            rawfile = h5py.File(rfn, "r");
            timestamps = rawfile["event_time_offset"].value;
            real_len = len(timestamps);
            while 0<=real_len and timestamps[real_len-1]==0:
                real_len -= 1;
            rawfile.close();
            rf2events2[rfn] = real_len;
            if rf2events1[rfn] == rf2events2[rfn]:
                logging.info("{} has {} events in it (excl final zeros) OK".format(os.path.basename(rfn), real_len));
            else:
                logging.error("{} has {} events in it (excl final zeros) WRONG".format(os.path.basename(rfn), real_len));
            total += real_len;

        mod2events2[mod_name] = total;
        if(mod2events1[mod_name] == mod2events2[mod_name]):
            logging.info("{} has {} events in its raw files (excl final zeros) OK\n".format(mod_name, total));
        else:
            logging.error("{} has {} events in its raw files (excl final zeros) WRONG\n".format(mod_name, total));

# this opens the vds file and for each of the 5 datasets, lists the # of items in the vds, and then 
#  lists the # of items in the rawfiles, added. They should be equal.
def check_vds_qties(metafile):
    vdsfile = h5py.File(metafile.getVdsFilename(), "r");
    logging.info("vds num event_time_offset: %d", vdsfile["event_time_offset"].shape[0]);
    logging.info("vds num event_id: %d", vdsfile["event_id"].shape[0]);
    logging.info("vds num event_energy: %d", vdsfile["event_energy"].shape[0]);
    logging.info("vds num cue_id: %d", vdsfile["cue_id"].shape[0]);
    logging.info("vds num cue_timestamp_zero: %d", vdsfile["cue_timestamp_zero"].shape[0]);
    vdsfile.close();

    # now see how many timestamps are in each raw file:
    total = 0;
    total_cues = 0;
    for rfn in metafile.getRawFilenames():
        rawfile = h5py.File(rfn, "r");
        dset = rawfile["event_id"].value;
        real_len = len(dset);
        while 0<=real_len and dset[real_len-1]==0:
            real_len -= 1;

        logging.debug("{} has {} event_ids in it (excl final zeros)".format(os.path.basename(rfn), real_len));
        total += real_len;

        dset = rawfile["cue_id"].value;
        real_len = len(dset);
        while 0<=real_len and dset[real_len-1]==0:
            real_len -= 1;

        rawfile.close();

        logging.debug("{} has {} cue_ids in it (excl final zeros)".format(os.path.basename(rfn), real_len));
        total_cues += real_len;

    logging.info("there are {} events and {} cues in the raw files (excl final zeros)\n".format(total, total_cues));



# this function reads all the timestamps from the vds file and writes them into a text file
def dump_vds_data(metafile, outdir):
    vdsf = h5py.File(metafile.getVdsFilename(), "r");
    logging.info("loading vds data");
    # value returns a <type 'numpy.ndarray'>
    data = vdsf[DATASET_TO_DUMP].value;
    logging.info("sorting vds data {} items".format(len(data)));
    # np.sort returns an nparray; sorted() returns a list.
    all = np.sort(data);
    filename = "{}_vdsfile.txt".format(DATASET_TO_DUMP);
    logging.info("writing vds data to {}".format(filename));
    out = open(os.path.join( outdir,filename ), "w");
    for idx in range(len(all)):
        line = "{:#x}\n".format( all[idx] ); 
        out.write( line );

    out.close();
    logging.info("finished vds dump to {}".format(filename));

# this function reads all the timestamps from the rawdatafiles and writes them into a text file.
def dump_rf_data(metafile, outdir):
    # get all the raw files
    rawfiles = metafile.getRawFilenames();

    logging.info("loading data from rawfiles");
    # read the dataset from all these rawfiles, and put all into a big list!
    all = [];
    for rfn in rawfiles:
        rf = h5py.File(rfn, "r");
        data = rf[DATASET_TO_DUMP].value;
        leng = data.shape[0];
        while data[leng-1]==0:
            leng -= 1;

        all.extend(data[0:leng]);
        rf.close();
        logging.info( "read {} items from rawfile {}".format(leng, os.path.basename(rfn)) );

    logging.info("sorting...");
    all = sorted(all);

    filename = "{}_rawfiles.txt".format(DATASET_TO_DUMP);
    logging.info("writing rf data to {}".format(filename));
    out = open(os.path.join( outdir, filename ) , "w");
    for d in all:
        out.write("{:#x}\n".format(d));
    out.close();

def append_timestamp_values(metafile):
    lookup = metafile.getTimesliceLookup();

    vdsf = h5py.File(metafile.getVdsFilename(), "r");
    logging.info("loading dataset");
    ds = vdsf["event_time_offset"].value;
    logging.info("appending timestamps");

    for vdsoffset in lookup.keys():
        lis = lookup[vdsoffset];
        qty = lis[-1];
        # qty can not be zero because of the way we created this
        if qty==0:
            raise Exception("qty 0");
        lis.append(ds[vdsoffset]);
        endval = ds[vdsoffset+qty-1];
        lis.append(endval);

    vdsf.close();


# this function does some simple checks on the timestamps in the vds file:
#  it load the first and last timestamp in each timeslice, and checks they
#  are nonzero (a zero indicates an invalid vds mapping). It also checks
#  that the timeslices are ascending in time.
def check_for_zeros(metafile):
    lookup = metafile.getTimesliceLookup();
    append_timestamp_values(metafile);
    logging.info("checking for disordered ts");
    count = 0;
    curmin = 0;
    curmax = 0;
    for vdsoffset in sorted(lookup.keys()):
        lis = lookup[vdsoffset];
        lo = lis[3];
        hi = lis[4];
        if lo==0 or hi==0:
            logging.error("{} zero timestamp at offset {}".format(count, vdsoffset));
        elif curmax < lo:
            # next time segment appears
            curmin = lo;
            curmax = hi;
        else:        
            # check this segment is within bounds
            check = lo < curmax and curmin < hi;
            if(check == False):
                logging.error("{} bad ts order at offset {}".format(count, vdsoffset));

        count += 1;


def main():
    logfile = "vdstester.log";
    os.remove(logfile) if os.path.exists(logfile) else 0;
    logging.basicConfig(level=logging.INFO, filename=logfile, format="%(asctime)s %(levelname)s %(message)s");
    # get the root logger and add stdout as an output.
    logging.getLogger().addHandler(logging.StreamHandler(sys.stdout));

    parser = argparse.ArgumentParser(description="This is an assortment of algorithms to check the vds file created by meta_to_vds.py.");
    parser.add_argument("-i", nargs=1, required=True, help="full path to metafile, raw files & vds should be next to it");
    parser.add_argument("--dumpvds", action="store_true", help="dump the event_timestamps from the vds file as a sorted text file; to same dir as metafile");
    parser.add_argument("--dumprf", action="store_true", help="dump the event timestamps from the rawfiles as a sorted text file");
    parser.add_argument("--mappings", action="store_true", help="nothing"); 
    parser.add_argument("--zeros", action="store_true", help="check the first timestamp in each timeslice in the vds to see if it is zero (=bad mapping), also check the timestamps are incrementing as we go through the vds file.");
    parser.add_argument("--qties", action="store_true", help="list the quantities of events in the rawfiles and modules in two ways: reading the metafile vs reading the rawfiles and summing. (output also in {})".format(logfile));
    parser.add_argument("--other", action="store_true", help="custom, see source");
 #   parser.add_argument("-vds", help="test vds file", action="store_true");
    parser.add_argument("--outdir", nargs=1, required=False, help="not used. output directory for text dumps (default = .)");    

    args = parser.parse_args();

    filename = None;
    outdir = ".";

    if(args.outdir and len(args.outdir)==1):
        outdir = args.outdir[0];

    if os.path.isdir(outdir)==False:
        raise Exception("{} does not exist".format(outdir));

    if(len(args.i)==1):
        filename = args.i[0];
        metafile = Metafile(filename);
        outdir = metafile.getSourceDir();
        if(args.dumpvds):
            dump_vds_data(metafile, outdir);
        if(args.dumprf):
            dump_rf_data(metafile, outdir);
        elif(args.mappings):
            pass;
        elif(args.zeros):
            check_for_zeros(metafile);
        elif(args.other):
            check_vds_qties(metafile);
        elif(args.qties):
            check_qties(metafile);
        else:
            logging.error("No action specified - see help -h");     


if __name__ == '__main__':
    main ();


