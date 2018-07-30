import json
import datetime

class LATRDMessageException(Exception):
    
    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno
        
    def __str__(self):
        return str(self.msg)
    
class LATRDMessage(object):
    msg_counter = 0

    MSG_TYPE_GET = "GET"
    MSG_TYPE_PUT = "PUT"
    MSG_TYPE_POST = "POST"
    MSG_TYPE_RESPONSE = "RESPONSE"

    def __init__(self, msg_type=None, msg_id=None, from_str=None):
        self._attrs = {}
        
        if not from_str:
            self._attrs['MsgType'] = msg_type
            self._attrs['MsgID']  = msg_id
            self._attrs['Parameters'] = {}
            
        else:
            try:
                self._attrs = json.loads(from_str)
                
            except ValueError, e:
                raise LATRDMessageException("Illegal message JSON format: " + str(e))

    @property
    def msg_type(self):
        return self._attrs['MsgType']

    @property
    def msg_id(self):
        return self._attrs['MsgID']

    @property
    def params(self):
        return self._attrs['Parameters']

    def has_attribute(self, name):
        if name in self._attrs:
            return True
        return False

    def get_attribute(self, name):
        return self._attrs[name]

    def get_param(self, param_name):
        try:
            param_value = self._attrs['Parameters'][param_name]
        except KeyError, e:
            raise LATRDMessageException("Missing parameter " + param_name)

        return param_value
    
    def set_param(self, param_name, param_value):
        if not 'Parameters' in self._attrs:
            self._attrs['Parameters'] = {}
            
        self._attrs['Parameters'][param_name] = param_value
        
    def encode(self):
        return json.dumps(self._attrs)
    
    def __eq__(self, other):
        return self._attrs == other.attrs
    
    def __ne__(self, other):
        return self._attrs != other.attrs
    
    def __str__(self):
        return json.dumps(self._attrs, sort_keys=True, indent=4, separators=(',', ': '))
    
    def _get_attr(self, attr_name):
        try:
            attr_value = self._attrs[attr_name]
        except KeyError, e:
            raise LATRDMessageException("Missing attribute " + attr_name)

        return attr_value

    @staticmethod
    def new_id():
        LATRDMessage.msg_counter += 1
        return LATRDMessage.msg_counter

    @staticmethod
    def parse_json(raw_msg):
        msg = LATRDMessage(from_str=raw_msg)
        if msg.msg_type == LATRDMessage.MSG_TYPE_GET:
            reply_msg = GetMessage(msg.msg_id, msg.params)
        elif msg.msg_type == LATRDMessage.MSG_TYPE_PUT:
            reply_msg = PutMessage(msg.msg_id, msg.params)
        elif msg.msg_type == LATRDMessage.MSG_TYPE_POST:
            reply_msg = PostMessage(msg.msg_id)
        elif msg.msg_type == LATRDMessage.MSG_TYPE_RESPONSE:
            data = None
            if msg.has_attribute('Data'):
                data = msg.get_attribute('Data')
            resp_type = None
            if msg.has_attribute('RespType'):
                resp_type = msg.get_attribute('RespType')
            reply_msg = ResponseMessage(msg.msg_id, data, resp_type)
        else:
            raise LATRDMessageException("Cannot parse unknown message type: {}".format(msg.msg_type))
        return reply_msg


class GetMessage(LATRDMessage):
    def __init__(self, msg_id=None, params=None):
        if not msg_id:
            msg_id = LATRDMessage.new_id()
        super(GetMessage, self).__init__(msg_type=LATRDMessage.MSG_TYPE_GET, msg_id=msg_id)
        if params:
            self._attrs['Parameters'] = params


class PutMessage(LATRDMessage):
    def __init__(self, msg_id=None, params=None):
        if not msg_id:
            msg_id = LATRDMessage.new_id()
        super(PutMessage, self).__init__(msg_type=LATRDMessage.MSG_TYPE_PUT, msg_id=msg_id)
        if params:
            self._attrs['Parameters'] = params


class PostMessage(LATRDMessage):
    def __init__(self, msg_id=None):
        if not msg_id:
            msg_id = LATRDMessage.new_id()
        super(PostMessage, self).__init__(msg_type=LATRDMessage.MSG_TYPE_POST, msg_id=msg_id)


class ResponseMessage(LATRDMessage):
    RESPONSE_OK = 0
    RESPONSE_ERROR = 1

    def __init__(self, msg_id=None, data=None, response_type=RESPONSE_OK):
        if not msg_id:
            msg_id = LATRDMessage.new_id()
        super(ResponseMessage, self).__init__(msg_type=LATRDMessage.MSG_TYPE_RESPONSE, msg_id=msg_id)
        del self._attrs['Parameters']
        self._attrs['RespType'] = response_type
        if data:
            self._attrs['Data'] = data
        else:
            self._attrs['Data'] = {}

    @property
    def data(self):
        return self._attrs['Data']

