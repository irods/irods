import pydevtest_sessions as s
from nose.tools import with_setup
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import commands


### Submission of various usernames for new user creation ###
@with_setup(s.adminonly_up,s.adminonly_down)
def test_iadmin_mkuser():
    
    # A few examples of valid and invalid usernames
    valid = ['bob',
             'e-irods',
             '123.456', 
             '___haysoos___']
    
    invalid = ['bo',
               '.bob', 
               'bob.',
               'e--irods', 
               'jamesbond..007',  
               '________________________________longer_than_NAME_LEN________________________________________________']
    
    
    # Test valid names
    for name in valid:
        assertiCmd(s.adminsession,"iadmin mkuser "+name+" rodsuser") # should be accepted
        assertiCmd(s.adminsession,"iadmin lu","LIST",name+"#"+s.adminsession.getZoneName()) # should be listed
        assertiCmd(s.adminsession,"iadmin rmuser "+name) # remove user
        assertiCmdFail(s.adminsession,"iadmin lu","LIST",name+"#"+s.adminsession.getZoneName()) # should be gone


    # Test invalid names
    for name in invalid:
        assertiCmd(s.adminsession,"iadmin mkuser "+name+" rodsuser","ERROR","SYS_INVALID_INPUT_PARAM") # should be rejected


    # Invalid names with special characters
    assertiCmd(s.adminsession,r"iadmin mkuser hawai\'i rodsuser","ERROR","SYS_INVALID_INPUT_PARAM") # should be rejected
    assertiCmd(s.adminsession,r"iadmin mkuser \\\/\!\*\?\|\$ rodsuser","ERROR","SYS_INVALID_INPUT_PARAM") # should be rejected




