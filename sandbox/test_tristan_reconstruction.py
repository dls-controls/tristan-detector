import argparse
import sys
#sys.path.append('/dls_sw/work/R3.14.12.3/support/mapping/tools/h5py/prefix/lib/python2.7/site-packages/h5py-2.5.0-py2.7-linux-x86_64.egg')
import os, time, re
from optparse import OptionParser
from subprocess import Popen, PIPE
try:
    from pkg_resources import require
#    require('dls-autotestframework')
    require('h5py')

#    import unittest
#    from dls_autotestframework import *
    import h5py
    import numpy
    from numpy import *

except:
    print "Unexpected error:", sys.exc_info()[0]
    raise



def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-r", "--rawfile", default="/home/gnx91527/work/tristan/data_files/single_000001.h5", help="Full path to processed raw file")
    parser.add_argument("-v", "--vdsfile", default="/tmp/test_complete.h5", help="Full path to processed VDS file")
    args = parser.parse_args()
    return args


def main():
    args = options()
#    single_file_dir = os.path.join(args.outdir, "single/")
#    if not os.path.exists(single_file_dir):
#        os.makedirs(single_file_dir)
#    multi_file_dir = os.path.join(args.outdir, "multi/")
#    if not os.path.exists(multi_file_dir):
#        os.makedirs(multi_file_dir)

    # Now test the contents of the top level VDS data file against the contents of the single raw data file
    # Open both files for reading
    raw_file = h5py.File(args.rawfile, 'r', libver='latest', swmr=True)
    raw_event_id_dset = raw_file["/event_id"]
    vds_file = h5py.File(args.vdsfile, 'r', libver='latest', swmr=True)
    vds_event_id_dset = vds_file["/entry/data/event/event_id"]

    passed = True
    print "Validating high level VDS dataset against raw data file"
    print("Raw dset size: {}".format(raw_event_id_dset.shape[0]))
    print("VDS dset size: {}".format(vds_event_id_dset.shape[0]))
    for index in range(0, raw_event_id_dset.shape[0]):
        if (index+1) % 2000 == 0:
            print ".",
            sys.stdout.flush()
        if (index+1) % 50000 == 0:
            print("Checked", index+1)

        if raw_event_id_dset[index] != vds_event_id_dset[index]:
            passed = False
            print("*** Error data mismatch")
            print("Index of failure: {}".format(index))
            print("Raw value: {}".format(raw_event_id_dset[index]))
            print("VDS value: {}".format(vds_event_id_dset[index]))
            #break

    if passed == True:
        print "All validation checks passed"
    else:
        print "Validation failed"


if __name__ == "__main__":
    main()
