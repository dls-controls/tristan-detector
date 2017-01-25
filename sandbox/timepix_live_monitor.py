import os
import argparse
from timepix_vds_writer import TimepixVdsWriter

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--samples", type=int, default=1000000, help="Number of sample events to process")
    parser.add_argument("-f", "--files", type=int, default=2, help="Number of SWMR files to create")
    parser.add_argument("-o", "--outdir", default="./data/", help="Path to processed output files")
    parser.add_argument("-b", "--blocksize", type=int, default=50000, help="Size of each mid-level VDS dataset")
    args = parser.parse_args()
    return args


def main():
    args = options()

    fpath = os.path.abspath(args.outdir)
    dp = TimepixVdsWriter(args.files, fpath, args.blocksize)
    dp.execute(args.samples)


if __name__ == "__main__":
    main()
