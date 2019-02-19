import zmq
#import numpy as np
#import matplotlib.pyplot as plt
#import matplotlib.animation as animation
from datetime import datetime

context = zmq.Context()

# Connect our subscriber socket
subscriber = context.socket(zmq.SUB)
subscriber.setsockopt(zmq.IDENTITY, "Hello")
subscriber.setsockopt(zmq.SUBSCRIBE, "TP1")
subscriber.setsockopt(zmq.SNDHWM, 5)
subscriber.connect ("tcp://192.168.1.2:5555")

publisher = context.socket(zmq.PUB)
#publisher.setsockopt(zmq.IDENTITY, "Test")
#publisher.setsockopt(zmq.PUBLISH, "")
#publisher.setsockopt(zmq.LINGER, 500)
publisher.bind("tcp://*:9999")

# Get updates, expect random Ctrl-C death
print "Collecting updates from ViewFinder server..."
while True:
    #plt.cla()
    start = datetime.now()
    message = subscriber.recv_multipart()
    header = message[0]
    data = message[1]
    print(header)

    frame_num = 0
    new_header = {
        'frame_num': frame_num,
        'acquisition_id': '',
        'dtype': 'u8',
        'dsize': 1,
        'dataset': 'data',
        'compression': 0,
        'shape': ["512", "2048"]
    }
    publisher.send_json(new_header, flags=zmq.SNDMORE)
    publisher.send(data, 0)

#    vf_tmp_file = open('vf_tmp_file.bin', 'wb')
#    vf_tmp_file.write(data)
#    vf_tmp_file.close()
#    byte_number = 0
#    viewfinder_image = np.zeros((512, 2048))
#    with open('vf_tmp_file.bin', 'rb') as f:
#        byte = f.read(1)
#        while byte != "":
#            viewfinder_image[(byte_number >> 11) & 0x000001FF][byte_number & 0x000007FF] = int(byte.encode('hex'), 16)
#            byte = f.read(1)
#            byte_number = byte_number + 1
#    #    viewfinder_image_single_chip = viewfinder_image[256:511,1790:2047]
#    viewfinder_image_single_chip = viewfinder_image
#    plt.imshow(viewfinder_image_single_chip)
#    plt.gca().invert_yaxis()
#    plt.draw()
#    plt.pause(0.1)
#    end = datetime.now()
#    elapsed = end - start
#    print('Build of latest frame took ' + str(elapsed.microseconds/1000) + ' ms')
