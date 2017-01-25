import os
import argparse
import h5py
import sys
from timepix_data_parser import TimepixDataParser
from timepix_vds_writer import TimepixVdsWriter

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--samples", type=int, default=1000000, help="Number of sample events to process")
    parser.add_argument("-f", "--files", type=int, default=2, help="Number of SWMR files to create")
    parser.add_argument("-b", "--blocksize", type=int, default=50000, help="Number of sample events in each VDS file")
    parser.add_argument("-d", "--datadir", default="/dls/detectors/Timepix3/I16_20160422/raw_data/W2J2_top/1hour", help="Path to raw data ASCII files")
    parser.add_argument("-o", "--outdir", default="./data/", help="Path to processed output files")
    args = parser.parse_args()
    return args


def main():
    args = options()
    single_file_dir = os.path.join(args.outdir, "single/")
    if not os.path.exists(single_file_dir):
        os.makedirs(single_file_dir)
    multi_file_dir = os.path.join(args.outdir, "multi/")
    if not os.path.exists(multi_file_dir):
        os.makedirs(multi_file_dir)

    print "Parsing events to create single raw data file"
    # Firstly create a single raw data file with all events recorded
    sf = TimepixDataParser(1, args.datadir, single_file_dir)
    sf.execute(args.samples)

    print "Parsing events to create multiple SWMR data files split by random sized time slices"
    # Now create the multiple dataset files
    mf = TimepixDataParser(args.files, args.datadir, multi_file_dir)
    mf.execute(args.samples)

    print "Creating the intermediate VDS files and the high level full reconstruction VDS file"
    # Execute the VDS writer to create the intermediate and top level VDS files
    vf = TimepixVdsWriter(args.files, multi_file_dir, args.blocksize)
    vf.execute(args.samples)

    # Now test the contents of the top level VDS data file against the contents of the single raw data file
    # Open both files for reading
    raw_file = h5py.File(os.path.join(single_file_dir, "nexus_swmr_1.h5"), 'r', libver='latest', swmr=True)
    raw_event_id_dset = raw_file["/entry/data/event/event_id"]
    vds_file = h5py.File(os.path.join(multi_file_dir, "timepix_vds.h5"), 'r', libver='latest', swmr=True)
    vds_event_id_dset = vds_file["/entry/data/event/event_id"]

    passed = True
    print "Validating high level VDS dataset against raw data file"
    for index in range(0, raw_event_id_dset.shape[0]):
        if (index+1) % 2000 == 0:
            print ".",
            sys.stdout.flush()
        if (index+1) % 50000 == 0:
            print("Checked", index+1)

        if raw_event_id_dset[index] != vds_event_id_dset[index]:
            passed = False
            print("*** Error data mismatch")

    if passed == True:
        print "All validation checks passed"
    else:
        print "Validation failed"


if __name__ == "__main__":
    main()
