import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
from os import path
import commands

@with_setup(s.adminonly_up,s.adminonly_down)
def test_ireg_as_rodsadmin():
    # local setup
    filename = "newfile.txt"
    filepath = path.abspath(filename)
    f = open(filepath,'wb')
    f.write("TESTFILE -- ["+filepath+"]")
    f.close()
    
    # keyword arguments for icommands
    kwargs={'filepath':filepath, 
    		'filename':filename, 
    		'zone':s.adminsession.getZoneName(),
    		'username':s.adminsession.getUserName(),
    		'sessionId':s.adminsession.sessionId}
    
    # assertions
    assertiCmdFail(s.adminsession,"ils -L {filename}".format(**kwargs),"LIST",filename) # should not be listed
    assertiCmd(s.adminsession,"ireg {filepath} /{zone}/home/{username}/{sessionId}/{filename}".format(**kwargs)) # ireg
    assertiCmd(s.adminsession,"ils -L {filename}".format(**kwargs),"LIST",filename) # should be listed
    
    # local cleanup
    output = commands.getstatusoutput( 'rm '+filepath )



@with_setup(s.oneuser_up,s.oneuser_down)
def test_ireg_as_rodsuser():
    # local setup
    filename = "newfile.txt"
    filepath = path.abspath(filename)
    f = open(filepath,'wb')
    f.write("TESTFILE -- ["+filepath+"]")
    f.close()
    
    # keyword arguments for icommands
    kwargs={'filepath':filepath, 
    		'filename':filename, 
    		'zone':s.sessions[1].getZoneName(),
    		'username':s.sessions[1].getUserName(),
    		'sessionId':s.sessions[1].sessionId}
    
    # assertions
    assertiCmdFail(s.sessions[1],"ils -L {filename}".format(**kwargs),"LIST",filename) # should not be listed
    assertiCmd(s.sessions[1],"ireg {filepath} /{zone}/home/{username}/{sessionId}/{filename}".format(**kwargs),
    	"ERROR","SYS_NO_PATH_PERMISSION") # ireg
    assertiCmdFail(s.sessions[1],"ils -L {filename}".format(**kwargs),"LIST",filename) # should not be listed
    
    # local cleanup
    output = commands.getstatusoutput( 'rm '+filepath )


