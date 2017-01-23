import os

class AsciiStackReader(object):

    def __init__(self, path):
        self.path = path
        cpath = os.getcwd()
        os.chdir(path)
        self.files = filter(os.path.isfile, os.listdir(path))
        self.files = [os.path.join(path, f) for f in self.files] # add path to each file
        self.files.sort(key=lambda x: os.path.getmtime(x), reverse=True)
        os.chdir(cpath)
        self.currentfile = open(self.files.pop(), 'r')
        self.buffer = self.currentfile.readlines(10000)

    def read_next(self):
        if not self.buffer:
            # buffer is empty, add some more content
            self.buffer = self.currentfile.readlines(10000)
            if not self.buffer:
                # buffer is still empty, try a new file
                if self.files:
                    # there are still files avaialble, load the next one#
                    filename = self.files.pop()
                    print("Moving to new file '%s'" %(filename))
                    self.currentfile = open(filename, 'r')
                    self.buffer = self.currentfile.readlines(10000)
                else :
                    # no more files, so return None to signify end of files
                    return None
        return self.buffer.pop(0)

