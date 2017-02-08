import argparse
import socket
import time
from timepix_packet import TimepixPacket

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--samples", type=int, default=1000000, help="Number of sample events to process")
    args = parser.parse_args()
    return args


def main():
    args = options()

    # Set up a UDP server
    server = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

    # Listen on port 21567
    # (to all IP addresses on this system)
    listen_addr = ("",21567)
    server.bind(listen_addr)

    # Report on all data packets received and
    # where they came from in each case (as this is
    # UDP, each may be from a different source and it's
    # up to the server to sort this out!)
    while True:
        data,addr = server.recvfrom(8192)
        print len(data),addr
        packet = TimepixPacket(0)
        packet.from_bytes(data)
        packet.report()
        byte_val = bytearray(data)
        #print byte_val[0],byte_val[1],byte_val[2],byte_val[3],byte_val[4],byte_val[5],byte_val[6]
        print "ID: ", ((int(byte_val[1])&0xFC)>>2)

        print "Received packet %.6f" % (time.time())

if __name__ == "__main__":
    main()
