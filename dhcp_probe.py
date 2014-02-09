import socket
import struct
import random
import fcntl, socket, struct
import commands
from scapy.all import *
import time
import sys
def getHwAddr(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    info = fcntl.ioctl(s.fileno(), 0x8927,  struct.pack('256s', ifname[:15]))
    l =['%02x' % ord(char) for char in info[18:24]]
    return ''.join(l)
def getHwAddr2(ifname):
    s = getHwAddr(ifname)
    s2 = ""
    for i in range(0,6):
      s2 += s[i*2:(i+1)*2]
      if i != 5:
        s2 += ":"
    return s2
def probe(interface):

  
  rawSock = socket.socket(socket.AF_PACKET,socket.SOCK_RAW,socket.htons(0x800))
  rawSock.bind((interface,3))
  rawSock.settimeout(1)
  
  
  
  req = random.randint(0,3275635676)
  
  pkt = "01010600%08x"%(req)+"0000"+"0000"+"00000000"+"00000000"+"00000000"+"00000000"+getHwAddr(interface)+"00"*10
  pkt += "00"*67
  pkt += "00"*125
  pkt += "63825363350101"+"37020306ff"
  #print pkt,len(pkt)
  pkt = pkt.decode("hex")
  
  completepacket = Ether(dst="ff:ff:ff:ff:ff:ff",src=getHwAddr2(interface))/IP(src="0.0.0.0",dst="255.255.255.255")/UDP(sport=68,dport=67)/Raw(load=pkt)
  #print str(completepacket).encode("hex")
  rawSock.send(str(completepacket))
  rawSock.send(str(completepacket))
  elapsed = 0
  while True:
    try:
      pkt = rawSock.recv(2048) #La mtu non sara' mai maggiore
    except socket.timeout:
      if elapsed >= 30:
        return ("#DHCP Timeout","#DHCP Timeout")
      elapsed += 3
      rawSock.send(str(completepacket))
      continue
    p =Ether(pkt)
    #print p.summary() , DHCP in p
    if DHCP in p and BOOTP in p:
      if p[BOOTP].xid == req:
        router = "#No router provided"
        dns = "#No dns provided"
        #print p[DHCP].options
        for opt in p[DHCP].options:
          
          if opt[0] == "router":
            #print opt[1]
            router = opt[1]
          if opt[0] == "name_server":
            dns = opt[1:]
        return (router,dns)
  
def probe_ssid(interface,ssid):
  commands.getoutput("ifconfig %s up 0.0.0.0"%interface)
  commands.getoutput("iw dev %s disconnect"%(interface))
  time.sleep(0.5)
  commands.getoutput("iw dev %s connect %s"%(interface,ssid))
  tstart = time.time()
  while "Not connected" in commands.getoutput("iw dev %s link"%(interface)):
    print "Attendo la connessione..."
    time.sleep(0.3)
    if time.time()-tstart > 15.0:
      print "Connessione fallita"
      return ("#Connection failed","#Connection failed")
  routerdns = probe(interface.strip("'"))
  commands.getoutput("iw dev %s disconnect"%(interface))
  return routerdns
#print probe_ssid("wlp0s20u8","test")
if __name__ == "__main__":
  print probe_ssid(sys.argv[1],sys.argv[2])