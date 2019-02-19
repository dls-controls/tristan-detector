import os
import h5py

class RawH5StackReader(object):

    def __init__(self, path):
        self.path = path
        cpath = os.getcwd()
        os.chdir(path)
        self.files = filter(os.path.isfile, os.listdir(path))
        self.files = [os.path.join(path, f) for f in self.files] # add path to each file
        self.files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
        os.chdir(cpath)
        self._current_file = h5py.File(self.files.pop(), 'r', libver='latest', swmr=True)
        self._raw_dset = self._current_file["/raw_data"]
        self._current_index = 0
        self.buffer = self.read_lines(10000)

    def read_lines(self, no_of_lines):
        lines = []
        for data in self._raw_dset[self._current_index:self._current_index+no_of_lines]:
            lines.append("{:X}".format(data))
        self._current_index += no_of_lines
        return lines

    def read_next(self):
        if not self.buffer:
            # buffer is empty, add some more content
            self.buffer = self.read_lines(10000)
            if not self.buffer:
                # buffer is still empty, try a new file
                if self.files:
                    # there are still files avaialble, load the next one#
                    filename = self.files.pop()
                    print("Moving to new file '%s'" %(filename))
                    self._current_file = h5py.File(self.files.pop(), 'r', libver='latest', swmr=True)
                    self._raw_dset = self._current_file["/raw_data"]
                    self._current_index = 0
                    self.buffer = self.read_lines(10000)
                else :
                    # no more files, so return None to signify end of files
                    return None
        #print("READ NEXT {}".format(self.buffer[0]))
        return self.buffer.pop(0)

def main():

    rs = RawH5StackReader('/dls/i19-2/data/2018/cm19670-3/timepix/19_PdNO2_-210_286K_timepix_TR_7_ON3_OFF4_repeat10')
    while True:
        print("{}".format(rs.read_lines(20)))


if __name__ == "__main__":
    main()

