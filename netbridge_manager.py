import commands
import xmlrpclib
import re
import dhcp_probe
from SimpleXMLRPCServer import SimpleXMLRPCServer
import dbus

sysbus = dbus.SystemBus()

def shellquote(s):
  return "'" + s.replace("'", "'\\''") + "'"
def scan(interface):
  results = []
  x = commands.getoutput("iwlist "+interface+" scan")
  lines = x.split("\n")
  ssid = ""
  signal = ""
  channel = ""
  macaddr = ""
  enc = False
  for l in lines:
    l = l.strip("\r\t\n ")
    if "Signal level" in l:
      signal = l.split("Signal level=")[1]
    if "ESSID:" in l:
      ssid = l.split("ESSID:")[1].strip("\"")
    if "Channel:" in l:
      channel = l.split("Channel:")[1]
    if "Encryption key:" in l:
      enc = l.split("Encryption key:")[1] == "on"
    if "Address: " in l:
      macaddr = l.split("Address: ")[1]
    if "Mode:Master" in l:
      results.append([macaddr,ssid,signal,channel,enc])
  return results
def connect_to(macaddr,ssid,router_ip):
  if re.match(r"^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$",router_ip) == None:
    return "Failed"
  commands.getoutput("killall wpa_supplicant dhclient netbridge")
  print commands.getoutput("screen -dmS bridge /root/netbridge/netbridge %s %s wclient0 wap0 wap1"%(shellquote(ssid+","+macaddr),router_ip))
  return "OK"
def disconnect():
  commands.getoutput("killall wpa_supplicant dhclient netbridge")
  return "OK"
def probe_ssid(interface,ssid):
  return dhcp_probe.probe_ssid(shellquote(interface),shellquote(ssid))
def change_dns(newdnslist):
  proxy = sysbus.get_object('uk.org.thekelleys.dnsmasq','/uk/org/thekelleys/dnsmasq')
  try:
    print newdnslist
    proxy.SetDhcpDomainServers(newdnslist)
    return True
  except:
    return False
server = SimpleXMLRPCServer(("localhost", 8888))
server.register_function(scan,"scan")
server.register_function(connect_to,"connect")
server.register_function(disconnect,"disconnect")
server.register_function(probe_ssid,"probe_ssid")
server.register_function(change_dns,"change_dns")
server.serve_forever()
