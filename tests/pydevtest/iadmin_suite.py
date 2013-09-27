import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands

class Test_iAdminSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
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
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication ContextString:Because:Multiple:Colons") # replication
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_net","EMPTY_RESC_HOST"]) # should have empty host
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_def_path","EMPTY_RESC_PATH"]) # should have empty path
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_context","ContextString:Because:Multiple:Colons"]) # should have contextstring
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

    # REBALANCE
    def test_rebalance_for_repl_node(self):
        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        assertiCmd(s.adminsession,"iadmin mkresc pt passthru") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_b passthru") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_c1 passthru") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_c2 passthru") 
        assertiCmd(s.adminsession,"iadmin mkresc repl replication") 

        assertiCmd(s.adminsession,"iadmin mkresc leaf_a unixfilesystem "+hostname+":/tmp/pydevtest_leaf_a") # unix
        assertiCmd(s.adminsession,"iadmin mkresc leaf_b unixfilesystem "+hostname+":/tmp/pydevtest_leaf_b") # unix
        assertiCmd(s.adminsession,"iadmin mkresc leaf_c unixfilesystem "+hostname+":/tmp/pydevtest_leaf_c") # unix

        assertiCmd(s.adminsession,"iadmin addchildtoresc pt repl" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc repl leaf_a" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc repl pt_b" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc repl pt_c1" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc pt_b leaf_b" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc pt_c1 pt_c2" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc pt_c2 leaf_c" )

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 11
        for i in range( num_children ):
            assertiCmd(s.adminsession,"iput -R pt README foo%d" % i )
       
        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession,"itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8" )
        assertiCmd(s.adminsession,"itrim -N1 -n 1 foo1 foo3 foo4 foo9" )
        assertiCmd(s.adminsession,"itrim -N1 -n 2 foo2 foo4 foo5" )
        
        # =-=-=-=-=-=-=-
        # visualize our pruning
        assertiCmd(s.adminsession,"ils -AL", "LIST", "foo" )

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession,"iadmin modresc pt rebalance" )

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 1 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 2 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 3 ", " foo0" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 0 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 2 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 3 ", " foo1" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 0 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 1 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 2 ", " foo2" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 2 ", " foo3" ] )
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 3 ", " foo3" ] )
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 4 ", " foo3" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 0 ", " foo4" ] )
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 1 ", " foo4" ] )
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 2 ", " foo4" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 1 ", " foo5" ] )
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 2 ", " foo5" ] )
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 3 ", " foo5" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 1 ", " foo6" ] )
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 2 ", " foo6" ] )
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 3 ", " foo6" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 1 ", " foo7" ] )
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 2 ", " foo7" ] )
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 3 ", " foo7" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 1 ", " foo8" ] )
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 2 ", " foo8" ] )
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 3 ", " foo8" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 0 ", " foo9" ] )
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 2 ", " foo9" ] )
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 3 ", " foo9" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 0 ", " foo10" ] )
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 1 ", " foo10" ] )
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 2 ", " foo10" ] )

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range( num_children ):
            assertiCmd(s.adminsession,"irm -f foo%d" % i )

        assertiCmd(s.adminsession,"iadmin rmchildfromresc pt_c2 leaf_c" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc repl leaf_a" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc pt_b leaf_b" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc pt_c1 pt_c2" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc repl pt_c1" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc repl pt_b" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc pt repl" )

        assertiCmd(s.adminsession,"iadmin rmresc leaf_c" )
        assertiCmd(s.adminsession,"iadmin rmresc leaf_b" )
        assertiCmd(s.adminsession,"iadmin rmresc leaf_a" )
        assertiCmd(s.adminsession,"iadmin rmresc pt_c2" )
        assertiCmd(s.adminsession,"iadmin rmresc pt_c1" )
        assertiCmd(s.adminsession,"iadmin rmresc pt_b" )
        assertiCmd(s.adminsession,"iadmin rmresc repl" )
        assertiCmd(s.adminsession,"iadmin rmresc pt" )












