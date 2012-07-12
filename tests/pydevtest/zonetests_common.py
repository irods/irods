import re
import os
import socket
from icommands import *

if os.name != "nt":
    import fcntl
    import struct

def get_interface_ip(ifname):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    return socket.inet_ntoa( fcntl.ioctl( s.fileno(),
        0x8915,  # SIOCGIFADDR 
        struct.pack('256s', ifname[:15]) )[20:24])

def get_lan_ip():
    ip = socket.gethostbyname(socket.gethostname())
    if ip.startswith("127.") and os.name != "nt":
        interfaces = ["eth0","eth1","eth2","wlan0","wlan1","wifi0","ath0","ath1","ppp0"]
        for ifname in interfaces:
            try:
                ip = get_interface_ip(ifname)
                break;
            except IOError:
                pass
    return ip

def getiCmdBoolean(mysession,fullcmd,outputtype="",expectedresults=""):
  result = False # should start as failing, then get set to pass
  parameters = fullcmd.split(' ')
  print "running icommand: "+mysession.getUserName()+"["+fullcmd+"]"
  if parameters[0] == "iadmin":
    output = mysession.runAdminCmd(parameters[0],parameters[1:])
  else:
    output = mysession.runCmd(parameters[0],parameters[1:])
  # check result listing for expected results
  if (outputtype == "LIST"):
    print "  LIST output:"
    print "    | "+"\n    | ".join(output[0].splitlines())
    print "  error output... ["+output[1].strip()+"]"
    lines = output[0].splitlines()
    for line in lines:
      print "  searching ["+line.strip()+"] for ["+expectedresults+"] ...",
      if not re.search(expectedresults,line.strip()) == None:
        print "found"
        result = True
        break
      else:
        print "NOTFOUND"
  # check that icommand returned no result
  elif (outputtype == "EMPTY" or outputtype == ""):
    print "  EMPTY output: ["+",".join(output[0].splitlines())+"]"
    print "  error output... ["+output[1].strip()+"]"
    if output[0] == "":
      result = True
  # bad test formatting
  else:
    print "  unknown outputtype requested: ["+outputtype+"]"
  # return error if thrown
  if output[1] != "":
    return False
  # return value
  return result

def assertiCmd(mysession,fullcmd,outputtype="",expectedresults=""):
  print "\nASSERTING PASS"
  assert getiCmdBoolean(mysession,fullcmd,outputtype,expectedresults)

def assertiCmdFail(mysession,fullcmd,outputtype="",expectedresults=""):
  print "\nASSERTING FAIL"
  assert not getiCmdBoolean(mysession,fullcmd,outputtype,expectedresults)

