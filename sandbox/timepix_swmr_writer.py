import os
import argparse
from timepix_data_parser import TimepixDataParser

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--samples", type=int, default=1000000, help="Number of sample events to process")
    parser.add_argument("-f", "--files", type=int, default=2, help="Number of SWMR files to create")
    parser.add_argument("-d", "--datadir", default="/dls/detectors/Timepix3/I16_20160422/raw_data/W2J2_top/1hour", help="Path to raw data ASCII files")
    parser.add_argument("-o", "--outdir", default="./data/", help="Path to processed output files")
    parser.add_argument("-r", "--raw", action="store_const", const=True, help="Flag to parse HDF5 raw input files")
    args = parser.parse_args()
    return args


def main():
    args = options()
    if not os.path.exists(args.outdir):
        os.makedirs(args.outdir)

    hdf = False
    if args.raw:
        hdf = True
    dp = TimepixDataParser(args.files, args.datadir, args.outdir, hdf)
    dp.execute(args.samples)


if __name__ == "__main__":
    main()
