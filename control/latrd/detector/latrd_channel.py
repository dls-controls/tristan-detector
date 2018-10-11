import random
import zmq
from zmq.utils.strtypes import unicode, cast_bytes

from latrd_message import LATRDMessage

class LATRDChannelException(Exception):
    
    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno
        
    def __str__(self):
        return str(self.msg)
    

class LATRDChannel(object):
    
    CHANNEL_TYPE_PAIR = zmq.PAIR
    CHANNEL_TYPE_REQ  = zmq.REQ
    CHANNEL_TYPE_SUB  = zmq.SUB
    CHANNEL_TYPE_PUB  = zmq.PUB
    CHANNEL_TYPE_DEALER = zmq.DEALER
    CHANNEL_TYPE_ROUTER = zmq.ROUTER

    POLLIN = zmq.POLLIN
    
    def __init__(self, channel_type, endpoint=None, context=None, identity=None):
        self.channel_type = channel_type
        self.context = context or zmq.Context().instance()
        self.socket  = self.context.socket(channel_type)
        
        if endpoint:
            self.endpoint = endpoint

        # If the socket type is DEALER, set the identity, chosing a random
        # UUID4 value if not specified
        if self.channel_type == self.CHANNEL_TYPE_DEALER:
            if identity is None:
                identity = "{:04x}-{:04x}".format(
                    random.randrange(0x10000), random.randrange(0x10000)
                )
            self.identity = identity
            self.socket.setsockopt(zmq.IDENTITY, cast_bytes(identity))  # pylint: disable=no-member

    def bind(self, endpoint=None):
        
        if endpoint:
            self.endpoint = endpoint
            
        self.socket.bind(self.endpoint)
        
    def connect(self, endpoint=None):
        
        if endpoint:
            self.endpoint = endpoint
            
        self.socket.connect(self.endpoint)
    
    def close(self):
        
        self.socket.close()

    def send(self, data):
        if isinstance(data, LATRDMessage):
            data = data.encode()
        self.socket.send_string(data)

    def send_multi(self, data):
        send_list = []
        for item in data:
            if isinstance(item, LATRDMessage):
                item = item.encode()
            send_list.append(item)
        self.socket.send_multipart(send_list)

    def recv(self):
        
        data = self.socket.recv()
        return data
    
    def poll(self, timeout=None):
        
        pollevts = self.socket.poll(timeout)
        return pollevts
    
    def subscribe(self, topic=b''):
        
        self.socket.setsockopt(LATRDChannel.CHANNEL_TYPE_SUB, topic)