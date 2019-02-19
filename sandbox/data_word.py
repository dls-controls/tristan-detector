import re

class DataWordException(Exception):
    pass

class DataWord(object):

    COURSE_ROLLOVER = 0x00000000001FFFFF

    def __init__(self, line_in):
        self._raw_packet = re.sub("[\s]","",line_in)
        self._raw_packet = re.sub("\x00","",self._raw_packet)   # Found a badly formatted file
        self._raw_packet = re.sub(",$","",self._raw_packet)
        self._raw_packet = self._raw_packet.upper()
        line_tmp = re.sub("^","0x0",self._raw_packet)
        self._packet = int(line_tmp, 16)

    @property
    def raw(self):
        return self._packet

    @property
    def is_ctrl(self):
        ctrl_data = self._packet & 0x8000000000000000
        if ctrl_data == 0:
            return False
        return True

    @property
    def is_event(self):
        ctrl_data = self._packet & 0x8000000000000000
        if ctrl_data == 0:
            return True
        return False

    @property
    def ctrl_type(self):
        if not self.is_ctrl:
            raise DataWordException()
        return (self._packet >> 58) & 0x03F

    @property
    def timestamp_course(self):
        if not self.is_ctrl:
            raise DataWordException()
        return self._packet & 0x000FFFFFFFFFFFF8

    @property
    def timestamp_fine(self):
        if not self.is_event:
            raise DataWordException()
        return (self._packet >> 14) & 0x00000000007FFFFF

    @property
    def energy(self):
        if not self.is_event:
            raise DataWordException()
        return self._packet & 0x0000000000003FFF

    @property
    def pos_x(self):
        if not self.is_event:
            raise DataWordException()
        return (self._packet >> 51) & 0x0000000000000FFF

    @property
    def pos_y(self):
        if not self.is_event:
            raise DataWordException()
        return (self._packet >> 39) & 0x0000000000000FFF

    def full_timestmap(self, prev_course, course):
        full_ts = 0
        if prev_course == 0:
            prev_course = course - self.COURSE_ROLLOVER
        if not self.is_event:
            raise DataWordException()
        if self.find_match_ts(course) == self.find_match_ts(self.timestamp_fine) or self.find_match_ts(course)+1 == self.find_match_ts(self.timestamp_fine):
            full_ts = (course & 0x0FFFFFFF800000) + self.timestamp_fine
        elif self.find_match_ts(prev_course) == self.find_match_ts(self.timestamp_fine) or self.find_match_ts(prev_course)+1 == self.find_match_ts(self.timestamp_fine):
            full_ts = (prev_course & 0x0FFFFFFF800000) + self.timestamp_fine
        else:
            print("********* Warning ***********")
        return full_ts

    def find_match_ts(self, time_stamp):
        # This method will return the 2 bits required for
        # matching a course timestamp to an extended timestamp
        match = (time_stamp >> 21) & 0x03
        return match
