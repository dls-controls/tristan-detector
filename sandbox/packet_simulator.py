import os
import argparse
import h5py
import sys
from timepix_packet_sender import TimepixPacketSender
from timepix_vds_writer import TimepixVdsWriter

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--samples", type=int, default=1000000, help="Number of sample events to process")
    parser.add_argument("-d", "--datadir", default="/dls/detectors/Timepix3/I16_20160422/raw_data/W2J2_top/1hour", help="Path to raw data ASCII files")
    args = parser.parse_args()
    return args


def main():
    args = options()

    ps = TimepixPacketSender(args.datadir)
    ps.open_connection(["127.0.0.1:21567"])
    ps.execute(args.samples)


if __name__ == "__main__":
    main()
