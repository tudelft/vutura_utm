#!/usr/bin/env python3
from nanomsg import Socket, REQ
import vutura_common_pb2
import time
import sys

if len(sys.argv) < 2:
    print("need 1 argument")
    exit(-1)

s1 = Socket(REQ)
s1.connect('ipc:///tmp/avoidance_command_0'.encode('utf-8'))

avoidance_message = vutura_common_pb2.AvoidanceVelocity()
avoidance_message.avoid = int(sys.argv[1])
avoidance_message.vx = 2000
avoidance_message.vy = -2500
avoidance_message.vz = 0

while True:
    s1.send(avoidance_message.SerializeToString())
    print(s1.recv())
    time.sleep(0.2)
    

s1.close()

print("Start")
