import os
import datetime
import commands
import socket
import pydevtest_common as c
import icommands

####################
#  ADMIN SESSIONS  #
####################
def admin_session_up():
  # set up a admin session for testing
  global myip
  global myhost
  global myport
  global myzone
  global myresc
  global mycwd
  global icommands_bin
  global sessionid
  global myenv
  myip           = c.get_lan_ip()
  myhost         = socket.gethostname()
  myport         = "1247"
  myzone         = "tempZone"
  print "ZONE FOR ["+myhost+":"+myip+"] is ["+myzone+"]"
  myresc         = "demoResc"
  mycwd          = os.getcwd()
  icommands_bin  = "/usr/bin"
  sessionid      = datetime.datetime.now().strftime("%Y%m%dT%H%M%S.%f")
  myenv          = icommands.RodsEnv(  myhost,
                                      myport,
                                      myresc,
                                      '/'+myzone+'/home/rods',
                                      '/'+myzone+'/home/rods',
                                      'rods',
                                      myzone,
                                      'rods'
                                      )
  # create session
  global adminsession
  adminsession = icommands.RodsSession(mycwd, icommands_bin, sessionid)
  adminsession.createEnvFiles(myenv)
  adminsession.runCmd('iinit', [myenv.auth])
  adminsession.runCmd('imkdir',[sessionid])
  adminsession.runCmd('icd',[sessionid])
  # local file setup
  global testfile
  testfile = "textfile.txt"
  f = open(testfile,'wb')
  f.write("I AM A TESTFILE -- ["+testfile+"]")
  f.close()
  # remote setup
  global testdir
  testdir = "testdir"
  adminsession.runCmd('imkdir',[testdir])
  adminsession.runCmd('iput',[testfile])
  global testresc
  testresc = "testResc"
  output = commands.getstatusoutput("hostname")
  hostname = output[1]
  adminsession.runAdminCmd('iadmin',["mkresc",testresc,"unix file system","archive",hostname,"/tmp/pydevtest_"+testresc])

def admin_session_down():
  # tear down admin session
  global adminsession
  adminsession.runCmd('icd')
  adminsession.runCmd('irm',['-r',sessionid])
  adminsession.runCmd('irmtrash')
  adminsession.runAdminCmd('iadmin',['rmresc',testresc])
  adminsession.runCmd('iexit', ['full'])
  adminsession.deleteEnvFiles()
  # local file cleanup
  global testfile
  output = commands.getstatusoutput( 'rm '+testfile )
  