import os
import datetime
import commands
import socket
import zonetests_common as z

####################
#  ADMIN SESSIONS  #
####################
def admin_session_up():
  #set up a admin session for testing
  global myip
  myip = z.get_lan_ip()
  global myhost
  myhost = socket.gethostname()
  global myport
  myport = "1247"
  global myzone
  myzone = "tempZone"
  print "ZONE FOR ["+myhost+":"+myip+"] is ["+myzone+"]"
  global myresc
  myresc = "demoResc"
  global mycwd
  mycwd = os.getcwd()
  global icommands_bin
  icommands_bin = "/usr/bin"
  global sessionid
  sessionid = datetime.datetime.now().strftime("%Y%m%dT%H%M%S.%f")
  global myenv
  myenv = z.RodsEnv( myhost,
                   myport,
                   myresc,
                   '/'+myzone+'/home/rods',
                   '/'+myzone+'/home/rods',
                   'rods',
                   myzone,
                   'rods')
  # create session
  global adminsession
  adminsession = z.RodsSession(mycwd, icommands_bin, sessionid)
  adminsession.createEnvFiles(myenv)
  adminsession.runCmd('iinit', [myenv.auth])
  adminsession.runCmd('imkdir',[sessionid])
  adminsession.runCmd('icd',[sessionid])

def admin_session_down():
  #tear down admin session
  global adminsession
  adminsession.runCmd('icd')
  adminsession.runCmd('irm',['-r',sessionid])
  adminsession.runCmd('iexit', ['full'])
  adminsession.deleteEnvFiles()
