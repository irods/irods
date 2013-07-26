import unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_iAdminSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    ###################
    # iadmin
    ###################

    # LISTS

    def test_list_zones(self):
        assertiCmd(s.adminsession,"iadmin lz","LIST",s.adminsession.getZoneName())
        assertiCmdFail(s.adminsession,"iadmin lz","LIST","notazone")

    def test_list_resources(self):
        assertiCmd(s.adminsession,"iadmin lr","LIST",self.testresc)
        assertiCmdFail(s.adminsession,"iadmin lr","LIST","notaresource")

    def test_list_users(self):
        assertiCmd(s.adminsession,"iadmin lu","LIST",s.adminsession.getUserName()+"#"+s.adminsession.getZoneName())
        assertiCmdFail(s.adminsession,"iadmin lu","LIST","notauser")

    def test_list_groups(self):
        assertiCmd(s.adminsession,"iadmin lg","LIST",self.testgroup)
        assertiCmdFail(s.adminsession,"iadmin lg","LIST","notagroup")
        assertiCmd(s.adminsession,"iadmin lg "+self.testgroup,"LIST",[s.sessions[1].getUserName()])
        assertiCmd(s.adminsession,"iadmin lg "+self.testgroup,"LIST",[s.sessions[2].getUserName()])
        assertiCmdFail(s.adminsession,"iadmin lg "+self.testgroup,"LIST","notauser")

    # RESOURCES

    def test_create_and_remove_unixfilesystem_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" \"unix file system\" "+hostname+":/tmp/pydevtest_"+testresc1) # unix
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmdFail(s.adminsession,"iadmin rmresc notaresource") # bad remove
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_unixfilesystem_resource_without_spaces(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" \"unixfilesystem\" "+hostname+":/tmp/pydevtest_"+testresc1) # unix
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_coordinating_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication") # replication
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_net","EMPTY_RESC_HOST"]) # should have empty host
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_def_path","EMPTY_RESC_PATH"]) # should have empty path
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_coordinating_resource_with_explicit_contextstring(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication '' Context:String","LIST","resource host:path string is empty") # replication
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_net","EMPTY_RESC_HOST"]) # should have empty host
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_def_path","EMPTY_RESC_PATH"]) # should have empty path
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_context","Context:String"]) # should have contextstring
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_coordinating_resource_with_detected_contextstring(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication ContextString:Because:Colons") # replication
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_net","EMPTY_RESC_HOST"]) # should have empty host
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_def_path","EMPTY_RESC_PATH"]) # should have empty path
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_context","ContextString:Because:Colons"]) # should have contextstring
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_modify_resource_comment(self):
        mycomment = "thisisacomment with some spaces"
        assertiCmdFail(s.adminsession,"iadmin lr "+self.testresc,"LIST",mycomment)
        assertiCmd(s.adminsession,"iadmin modresc "+self.testresc+" comment '"+mycomment+"'")
        assertiCmd(s.adminsession,"iadmin lr "+self.testresc,"LIST",mycomment)

    # USERS

    def test_create_and_remove_new_user(self):
        testuser1 = "testaddandremoveuser"
        assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should not be listed
        assertiCmd(s.adminsession,"iadmin mkuser "+testuser1+" rodsuser") # add rodsuser
        assertiCmd(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should be listed
        assertiCmdFail(s.adminsession,"iadmin rmuser notauser") # bad remove
        assertiCmd(s.adminsession,"iadmin rmuser "+testuser1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lu","LIST",testuser1+"#"+s.adminsession.getZoneName()) # should be gone

    def test_iadmin_mkuser(self):

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
