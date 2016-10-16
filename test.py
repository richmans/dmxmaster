import socket
import time
UDP_IP = "127.0.0.1"
UDP_PORT = 5120


print "UDP target IP:", UDP_IP
print "UDP target port:", UDP_PORT

sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
counter = 0
while (True):
  time.sleep(0.5)
  counter = (counter + 1) % 512
  msg = chr(0) * counter + chr(255) + chr(0) * ( 512 - counter)
  print("Enabling fixture %d" % counter)
  sock.sendto(msg, (UDP_IP, UDP_PORT))