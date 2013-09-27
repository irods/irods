import os
import datetime
import commands
import socket
import pydevtest_common as c
import icommands
import time
import inspect

global users
output = commands.getstatusoutput("hostname")
hostname = output[1]
users = []
users.append({'name': 'rods',  'passwd': 'rods',  'zone': 'tempZone', 'resc': 'demoResc', 'host': hostname, 'port': '1247'})
users.append({'name': 'alice', 'passwd': 'apass', 'zone': 'tempZone', 'resc': 'demoResc', 'host': hostname, 'port': '1247'})
users.append({'name': 'bobby', 'passwd': 'bpass', 'zone': 'tempZone', 'resc': 'demoResc', 'host': hostname, 'port': '1247'})

global mycwd
mycwd          = os.getcwd()
global icommands_bin
icommands_bin  = "/usr/bin"

######################################################
# generic standup functions for admin and user
#   called by more specific functions down below
######################################################

def admin_up():
    # restart server because overkill
#    os.system("/var/lib/eirods/iRODS/irodsctl restart")

    # set up admin session
    admin = users[0]
    sessionid      = "session-"+datetime.datetime.now().strftime("%Y%m%dT%H%M%S.%f")
    myenv          = icommands.RodsEnv( admin['host'],
                                        admin['port'],
                                        admin['resc'],
                                        '/'+admin['zone']+'/home/'+admin['name'],
                                        '/'+admin['zone']+'/home/'+admin['name'],
                                        admin['name'],
                                        admin['zone'],
                                        admin['passwd']
                                        )
    global adminsession
    adminsession = icommands.RodsSession(mycwd, icommands_bin, sessionid)
    adminsession.createEnvFiles(myenv)
    adminsession.runCmd('iinit', [myenv.auth])
    adminsession.runCmd('imkdir', [sessionid])
    adminsession.runCmd('icd', [sessionid])
    print "admin session created: user["+adminsession.getUserName()+"] zone["+adminsession.getZoneName()+"]"

    # set sessions[0] as adminsession
    global sessions
    sessions = []
    sessions.append(adminsession)

    # users, passwords, and groups
    global testgroup
    testgroup = "pydevtestUserGroup"
    adminsession.runAdminCmd('iadmin',["mkgroup",testgroup])
    for u in users[1:]:
        adminsession.runAdminCmd('iadmin',["mkuser",u['name'],"rodsuser"])
        adminsession.runAdminCmd('iadmin',["moduser",u['name'],"password",u['passwd']])
        adminsession.runAdminCmd('iadmin',["atg",testgroup,u['name']])

    # get back into the proper directory
    adminsession.runCmd('icd', [sessionid])



def admin_down():
    # globals needed
    global adminsession
    global testgroup

    # trash
    adminsession.runCmd('irmtrash',['-M']) # removes all trash for all users (admin mode)
    # users
    for u in users[1:]:
        adminsession.runAdminCmd('iadmin',["rfg",testgroup,u['name']])
        adminsession.runAdminCmd('iadmin',["rmuser",u['name']])
    # groups
    adminsession.runAdminCmd('iadmin',['rmgroup',testgroup])
    print "admin session exiting: user["+adminsession.getUserName()+"] zone["+adminsession.getZoneName()+"]"
    adminsession.runCmd('iexit', ['full'])
    adminsession.deleteEnvFiles()


##################
def user_up(user):
    # set up single user session
    rightnow       = datetime.datetime.now()
    sessionid      = "session-"+rightnow.strftime("%Y%m%dT%H%M%S.")+str(rightnow.microsecond)
    myenv          = icommands.RodsEnv( user['host'],
                                        user['port'],
                                        user['resc'],
                                        '/'+user['zone']+'/home/'+user['name'],
                                        '/'+user['zone']+'/home/'+user['name'],
                                        user['name'],
                                        user['zone'],
                                        user['passwd']
                                        )
    new = icommands.RodsSession(mycwd, icommands_bin, sessionid)
    new.createEnvFiles(myenv)
    new.runCmd('iinit', [myenv.auth])
    new.runCmd('imkdir', [sessionid])
    new.runCmd('icd', [sessionid])
    print "user session created: user["+new.getUserName()+"] zone["+new.getZoneName()+"]"

    # add new session to existing sessions list
    global sessions
    sessions.append(new)

def user_down(usersession):
    # tear down user session
    usersession.runCmd('icd')
    usersession.runCmd('irm',['-rf',usersession.sessionId])
    print "user session exiting: user["+usersession.getUserName()+"] zone["+usersession.getZoneName()+"]"
    usersession.runCmd('iexit', ['full'])
    usersession.deleteEnvFiles()


#################################################################
# routines to be called from with_setup
#   designed this way since with_setup cannot pass variables
#################################################################

def adminonly_up():
    admin_up()

def adminonly_down():
    admin_down()

def oneuser_up():
    admin_up()
    user_up(users[1])

def oneuser_down():
    global sessions
    user_down(sessions[1])
    admin_down()

def twousers_up():
    admin_up()
    user_up(users[1])
    user_up(users[2])

def twousers_down():
    global sessions
    user_down(sessions[2])
    user_down(sessions[1])
    admin_down()
