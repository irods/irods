import os
import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands


### Send one XMessage ###
@with_setup(s.oneuser_up,s.oneuser_down)
def test_ixmsg_s():
	# Additional XMessage settings
	os.environ['xmsgHost'] = 'localhost'
	os.environ['xmsgPort'] = '1235'
	
	# Assertions
	assertiCmd(s.sessions[1],"ixmsg s -M 'Hello World!'")

### Receive one XMessage ###
@with_setup(s.oneuser_up,s.oneuser_down)
def test_ixmsg_r():
	# Additional XMessage settings
	os.environ['xmsgHost'] = 'localhost'
	os.environ['xmsgPort'] = '1235'
	
	# Assertions
	assertiCmd(s.sessions[1],"ixmsg r -n 1", "LIST","ixmsg: Hello World!")

