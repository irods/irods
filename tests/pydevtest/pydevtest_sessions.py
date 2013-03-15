import os
import datetime
import commands
import socket
import pydevtest_common as c
import icommands
import time

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

    # local file setup
    global testfile
    testfile = "textfile.txt"
    f = open(testfile,'wb')
    f.write("I AM A TESTFILE -- ["+testfile+"]")
    f.close()

    # remote setup
    # dir and file
    global testdir
    testdir = "testdir"
    adminsession.runCmd('imkdir',[testdir])
    adminsession.runCmd('iput',[testfile])
    adminsession.runCmd('icp',[testfile,"../../public/"]) # copy of testfile into public
    # resc
    global testresc
    global anotherresc
    testresc = "testResc"
    anotherresc = "anotherResc"
    output = commands.getstatusoutput("hostname")
    hostname = output[1]
    adminsession.runAdminCmd('iadmin',["mkresc",testresc,"unix file system",hostname+":/tmp/pydevtest_"+testresc])
    adminsession.runAdminCmd('iadmin',["mkresc",anotherresc,"unix file system",hostname+":/tmp/pydevtest_"+anotherresc])
    # users, passwords, and groups
    global testgroup
    testgroup = "pydevtestUserGroup"
    adminsession.runAdminCmd('iadmin',["mkgroup",testgroup])
    for u in users[1:]:
        adminsession.runAdminCmd('iadmin',["mkuser",u['name'],"rodsuser"])
        adminsession.runAdminCmd('iadmin',["moduser",u['name'],"password",u['passwd']])
        adminsession.runAdminCmd('iadmin',["atg",testgroup,u['name']])
    # permissions
    adminsession.runCmd('ichmod',["read",users[1]['name'],"../../public/"+testfile]) # read for user1 
    adminsession.runCmd('ichmod',["write",users[2]['name'],"../../public/"+testfile]) # write for user2

    # testallrules setup
    progname = "README"
    rules30dir = "../../iRODS/clients/icommands/test/rules3.0/"
    dir_w = rules30dir+".."
    adminsession.runCmd('icd') # to get into the home directory (for testallrules assumption)
    adminsession.runAdminCmd('iadmin',["mkuser","devtestuser","rodsuser"] )
    adminsession.runAdminCmd('iadmin',["mkresc","testallrulesResc","unix file system",hostname+":/tmp/pydevtest_testallrulesResc"] )
    adminsession.runCmd('imkdir', ["sub1"] )
    adminsession.runCmd('imkdir', ["forphymv"] )
    adminsession.runCmd('imkdir', ["ruletest"] )
    adminsession.runCmd('imkdir', ["test"] )
    adminsession.runCmd('imkdir', ["test/phypathreg"] )
    adminsession.runCmd('imkdir', ["ruletest/subforrmcoll"] )
    adminsession.runCmd('iput', [progname,"test/foo1"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/foo1"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/foo2"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/foo3"] )
    adminsession.runCmd('icp', ["test/foo1","forphymv/phymvfile"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/objunlink1"] )
    adminsession.runCmd('irm', ["sub1/objunlink1"] ) # put it in the trash
    adminsession.runCmd('icp', ["test/foo1","sub1/objunlink2"] )
    adminsession.runCmd('irepl', ["-R","testallrulesResc","sub1/objunlink2"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/freebuffer"] )
    adminsession.runCmd('icp', ["test/foo1","sub1/automove"] )
    adminsession.runCmd('icp', ["test/foo1","test/versiontest.txt"] )
    adminsession.runCmd('icp', ["test/foo1","test/metadata-target.txt"] )
    adminsession.runCmd('icp', ["test/foo1","test/ERAtestfile.txt"] )
    adminsession.runCmd('ichmod', ["read devtestuser","test/ERAtestfile.txt"] )
    adminsession.runCmd('imeta', ["add","-d","test/ERAtestfile.txt","Fun","99","Balloons"] )
    adminsession.runCmd('imkdir', ["sub1/SaveVersions"] )
    adminsession.runCmd('iput', [dir_w+"/misc/devtestuser-account-ACL.txt","test"] )
    adminsession.runCmd('iput', [dir_w+"/misc/load-metadata.txt","test"] )
    adminsession.runCmd('iput', [dir_w+"/misc/load-usermods.txt","test"] )
    adminsession.runCmd('iput', [dir_w+"/misc/sample.email","test"] )
    adminsession.runCmd('iput', [dir_w+"/misc/email.tag","test"] )
    adminsession.runCmd('iput', [dir_w+"/misc/sample.email","test/sample2.email"] )
    adminsession.runCmd('iput', [dir_w+"/misc/email.tag","test/email2.tag"] )

    # get back into the proper directory
    adminsession.runCmd('icd', [sessionid])



def admin_down():
    # globals needed
    global adminsession
    global testresc
    global anotherresc
    global resgroup
    global testgroup

    # testallrules teardown
    adminsession.runCmd('icd') # for home directory assumption
    adminsession.runCmd('ichmod',["-r","own","rods","."] )
    adminsession.runCmd('imcoll',["-U","/tempZone/home/rods/test/phypathreg"] )
    adminsession.runCmd('irm',["-rf","test","ruletest","forphymv","sub1","sub2","bagit","rules","bagit.tar","/tempZone/bundle/home/rods"] )
    adminsession.runAdminCmd('iadmin',["rmresc","testallrulesResc"] )
    adminsession.runAdminCmd('iadmin',["rmuser","devtestuser"] )
    adminsession.runCmd('iqdel',["-a"] ) # remove all/any queued rules

    # tear down admin session
    adminsession.runCmd('icd')
    adminsession.runCmd('irm',['-r',adminsession.sessionId])
    # trash
    adminsession.runCmd('irmtrash',['-M']) # removes all trash for all users (admin mode)
    # users
    for u in users[1:]:
        adminsession.runAdminCmd('iadmin',["rfg",testgroup,u['name']])
        adminsession.runAdminCmd('iadmin',["rmuser",u['name']])
    # groups
    adminsession.runAdminCmd('iadmin',['rmgroup',testgroup])    
    # resc
    adminsession.runAdminCmd('iadmin',['rmresc',testresc])
    adminsession.runAdminCmd('iadmin',['rmresc',anotherresc])
    print "admin session exiting: user["+adminsession.getUserName()+"] zone["+adminsession.getZoneName()+"]"
    adminsession.runCmd('iexit', ['full'])
    adminsession.deleteEnvFiles()
    # local file cleanup
    global testfile
    os.unlink(testfile)


##################
def user_up(user):
    # set up single user session
    sessionid      = "session-"+datetime.datetime.now().strftime("%Y%m%dT%H%M%S.%f")
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
    usersession.runCmd('irm',['-r',usersession.sessionId])
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
