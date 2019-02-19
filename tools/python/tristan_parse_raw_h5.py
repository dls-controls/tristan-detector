import numpy as np
import h5py
#import shelve
import argparse

class RawParser:
    def __init__(self):
        self._raw = None
        self._x = None
        self._y = None
        self._toa = None
        self._tot = None
        self._file = None
        self._wrong_timing = None

    def parse(self, filename):
        self._file = filename
        f = h5py.File(filename, 'r')

        dataset = f['raw_data']
        raw_data = f.get('raw_data')
        raw_data = np.array(raw_data)
        f.close()

        self._raw = []
        self._x = []
        self._y = []
        self._toa = []
        self._tot = []
        self._wrong_timing = 0
        init_time = 0
        lToA_single = 0

        for item in raw_data:
            single_entry = int(item)
            ##### hunt for SHUTTER OPER TIMESTSAMP PACKET
            if (single_entry >> 56) & 0x00000000000000FF == 0x84:
                #print ('Shutter open timestamp found')
                in_acquisition = True
                init_time = single_entry & 0x00FFFFFFFFFFFFFF
                #            lToA = [0, 0, 0, 0]
                #            lToA[(single_entry >> 21) & 0x0000000000000003] = single_entry & 0x00FFFFFFFFE00000
                #  one the first extnded timestamp is extracted from the shutter open timestamp, the others can be calculated
                lToA_single = single_entry & 0x00FFFFFFFF800000
            else:
                ##### hunt for ETOA PACKET
                if ( (single_entry >> 63) & 0x0000000000000001 == 0x0 ):
                    #print ('Event found')
                    #            single_toa = ((single_entry >> 14) & 0x0000000007FFFFF) + lToA[(single_entry >> 35) & 0x0000000000000003] - init_time
                    single_toa = ((single_entry >> 14) & 0x0000000007FFFFF) + lToA_single # - init_time
                    if ( single_toa >= 0 ):
                        self._raw.append(single_entry)
                        self._x.append((single_entry >> 50) & 0x0000000000001FFF)
                        self._y.append((single_entry >> 37) & 0x0000000000001FFF)
                        self._toa.append(single_toa)
                        self._tot.append(single_entry & 0x0000000000003FFF)
                    else:
                        self._wrong_timing += 1
                ##### hunt for EXTENDED TIMESTAMP PACKET
                elif ( (single_entry >> 56) & 0x00000000000000FF == 0x80 ):
                    #print ('Extended timestamp found')
                    #            lToA[(single_entry >> 21) & 0x0000000000000003] = single_entry & 0x00FFFFFFFFE00000
                    lToA_single = single_entry & 0x00FFFFFFFF800000
                ##### hunt for SHUTTER CLOSE TIMESTAMP PACKET
                elif ( (single_entry >> 56) & 0x00000000000000FF == 0x88 ):
                    #print ('Shutter close timestamp found')
                    # add here when needed
                    continue
                ##### hunt for TRIGGER RE TIMESTAMP PACKET
                #elif ( (single_entry >> 56) & 0x00000000000000FF == 0x8E ):
                #    #print ('Trigger RE timestamp packet found')
                #    # add here when needed
                #    trig_re_ts.append(single_entry & 0x000FFFFFFFFFFFF0)
                #elif ( (single_entry >> 56) & 0x00000000000000FF == 0x8C ):
                #    #print ('Trigger FE timestamp packet found')
                #    # add here when needed
                #    trig_fe_ts.append(single_entry & 0x000FFFFFFFFFFFF0)
                ##### hunt for ToA ERROR/WARNING PACKETS
                elif ( (single_entry >> 56) & 0x00000000000000FF == 0xBF ):
                    print ('ToA error/warning found')
                    if ( single_entry & 0x0000000000000003 == 0x0 ):
                        print ('ToA warning')
                    elif ( single_entry & 0x0000000000000003 == 0x1 ):
                        print ('ToA warning clear')
                    elif ( single_entry & 0x0000000000000003 == 0x2 ):
                        print ('ToA error')
                    elif ( single_entry & 0x0000000000000003 == 0x3 ):
                        print ('ToA error clear')

    def report(self):
        for index in range(0, 100):
            print("[{}] {}".format(hex(self._raw[index]), self._toa[index]))

# Command line inputs
def options():
    parser = argparse.ArgumentParser()
#    parser.add_argument("-p", "--path", default="/tmp", help="Path to write to (/tmp)")
    parser.add_argument("-f", "--file", default="/tmp/test.h5", help="File to read and parse (/tmp/test.h5)")
    args = parser.parse_args()
    return args

def main():
    args = options()
    parser = RawParser()
    parser.parse(args.file)
    parser.report()


if __name__ == '__main__':
    main()

