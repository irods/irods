import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput, get_hostname, create_directory_of_small_files
import pydevtest_sessions as s
import commands
import os
import shutil
import time
import subprocess
import distutils.spawn
import stat

# =-=-=-=-=-=-=-
# build path magic to import server_config.py
pydevtestdir = os.path.realpath(__file__)
topdir = os.path.dirname(os.path.dirname(os.path.dirname(pydevtestdir)))
packagingdir = os.path.join(topdir,"packaging")
sys.path.append(packagingdir)
from server_config import ServerConfig

class Test_iAdminSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_api_plugin(self):
        assertiCmd(s.adminsession,"iapitest", "LIST", "this")
        p = subprocess.Popen(['grep "HELLO WORLD"  ../../iRODS/server/log/rodsLog.*'], shell=True, stdout=subprocess.PIPE)
        result = p.communicate()[0]
        assert( -1 != result.find( "HELLO WORLD" ) )

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
        
    def test_resource_name_restrictions(self):
        h = get_hostname()
        oversize_name = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" # longer than NAME_LEN
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("?/=*", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM") # invalid char
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("replication.B", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM") # invalid char
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("replication{", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM") # invalid char
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % (oversize_name, h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM") # too long

    def test_modify_resource_name(self):
        h = get_hostname()
        # tree standup
        assertiCmd(s.adminsession,"iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" % ("pt1", h, "pt1"), "LIST", "Creating") # passthru
        assertiCmd(s.adminsession,"iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" % ("repl", h, "repl"), "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unix1", h, "unix1"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" % ("pt2", h, "pt2"), "LIST", "Creating") # passthru
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unix2", h, "unix2"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("pt1",  "repl"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("repl", "unix1"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("repl", "pt2"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("pt2",  "unix2"))

        # rename repl node
        newnodename = "replwithmoreletters"
        assertiCmd(s.adminsession,"iadmin modresc %s name %s" % ("repl", newnodename), "LIST", "OK, performing the resource rename") # rename
        
        # confirm children of pt1 is newnodename
        assertiCmd(s.adminsession,"iadmin lr %s" % "pt1","LIST","resc_children: %s" % newnodename+"{}")
        # confirm parent of newnodename is still pt1
        assertiCmd(s.adminsession,"iadmin lr %s" % newnodename,"LIST","resc_parent: %s" % "pt1")
        # confirm children of newnodename is unix1 and pt2
        assertiCmd(s.adminsession,"iadmin lr %s" % newnodename,"LIST","resc_children: %s" % "unix1{};pt2{}")
        # confirm parent of pt2 is newnodename
        assertiCmd(s.adminsession,"iadmin lr %s" % "pt2","LIST","resc_parent: %s" % newnodename)
        # confirm parent of unix2 is pt2
        assertiCmd(s.adminsession,"iadmin lr %s" % "unix2","LIST","resc_parent: %s" % "pt2")
        # confirm parent of unix1 is newnodename
        assertiCmd(s.adminsession,"iadmin lr %s" % "unix1","LIST","resc_parent: %s" % newnodename)

        # tree teardown
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("pt2", "unix2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % (newnodename, "unix1"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % (newnodename, "pt2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("pt1", newnodename))
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unix2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unix1")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "pt2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % newnodename)
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "pt1")

    def test_resource_hierarchy_manipulation(self):
        h = get_hostname()
        # first tree standup
        assertiCmd(s.adminsession,"iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" % ("pt", h, "pt"), "LIST", "Creating") # passthru
        assertiCmd(s.adminsession,"iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" % ("replA", h, "replA"), "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixA1", h, "unixA1"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixA2", h, "unixA2"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        assertiCmd(s.adminsession,"iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" % ("replB", h, "replB"), "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixB1", h, "unixB1"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixB2", h, "unixB2"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # create some files
        dir1 = "for_pt"
        dir2 = "for_replB"
        tree1 = 5
        tree2 = 8
        doubletree1 = 2 * tree1 # 10
        doubletree2 = 2 * tree2 # 16
        totaltree   = doubletree1 + doubletree2 # 26
        create_directory_of_small_files(dir1,tree1)
        create_directory_of_small_files(dir2,tree2)
        #os.system("ls -al %s" % dir1)
        #os.system("ls -al %s" % dir2)

        # add files
        assertiCmd(s.adminsession,"iput -R %s -r %s" % ("pt", dir1))
        assertiCmd(s.adminsession,"iput -R %s -r %s" % ("replB", dir2))

        # debugging
        assertiCmd(s.adminsession,"ils -L %s" % dir1,"LIST",dir1)
        assertiCmd(s.adminsession,"ils -L %s" % dir2,"LIST",dir2)

        # add tree2 to tree1
        # add replB to replA
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA","replB"))

        # debugging
        assertiCmd(s.adminsession,"ils -L %s" % dir1,"LIST",dir1)
        assertiCmd(s.adminsession,"ils -L %s" % dir2,"LIST",dir2)

        # check object_count on pt
        assertiCmd(s.adminsession,"iadmin lr %s" % "pt","LIST","resc_objcount: %d" % totaltree)
        # check object_count and children on replA
        assertiCmd(s.adminsession,"iadmin lr %s" % "replA","LIST","resc_objcount: %d" % totaltree)
        assertiCmd(s.adminsession,"iadmin lr %s" % "replA","LIST","resc_children: %s" % "unixA1{};unixA2{};replB{}")
        # check object_count on unixA1
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixA1","LIST","resc_objcount: %d" % tree1)
        # check object_count on unixA2
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixA2","LIST","resc_objcount: %d" % tree1)
        # check object_count and parent on replB
        assertiCmd(s.adminsession,"iadmin lr %s" % "replB","LIST","resc_objcount: %d" % doubletree2)
        assertiCmd(s.adminsession,"iadmin lr %s" % "replB","LIST","resc_parent: %s" % "replA")
        # check object_count on unixB1
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixB1","LIST","resc_objcount: %d" % tree2)
        # check object_count on unixB2
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixB2","LIST","resc_objcount: %d" % tree2)
        # check resc_hier on replB files, should have full hierarchy, and should NOT start with replB
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" % "pt;replA;replB", "LIST", "pt")
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "CAT_NO_ROWS_FOUND")
        # check resc_name on replB files
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" % "pt;replA;replB", "LIST", "pt")
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "CAT_NO_ROWS_FOUND")
        # check resc_group_name on replB files
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_GROUP_NAME where DATA_RESC_HIER like '%s;%%'\"" % "pt;replA;replB", "LIST", "pt")
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_GROUP_NAME where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "CAT_NO_ROWS_FOUND")
        
        # remove child
        # rm replB from replA
        assertiCmd(s.adminsession,"iadmin lr %s" % "replA","LIST","replB") # debugging
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA","replB"))

        # check object_count on pt
        assertiCmd(s.adminsession,"iadmin lr %s" % "pt","LIST","resc_objcount: %d" % doubletree1)
        # check object_count on replA
        assertiCmd(s.adminsession,"iadmin lr %s" % "replA","LIST","resc_objcount: %d" % doubletree1)
        # check object_count on unixA1
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixA1","LIST","resc_objcount: %d" % tree1)
        # check object_count on unixA2
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixA2","LIST","resc_objcount: %d" % tree1)
        # check object_count on replB
        assertiCmd(s.adminsession,"iadmin lr %s" % "replB","LIST","resc_objcount: %d" % doubletree2)
        # check object_count on unixB1
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixB1","LIST","resc_objcount: %d" % tree2)
        # check object_count on unixB2
        assertiCmd(s.adminsession,"iadmin lr %s" % "unixB2","LIST","resc_objcount: %d" % tree2)
        # check resc_hier on replB files, should start with replB and not have pt anymore
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "replB")
        # check resc_name on replB files
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "replB")
        # check resc_group_name on replB files
        assertiCmd(s.adminsession,"iquest \"select DATA_RESC_GROUP_NAME where DATA_RESC_HIER like '%s;%%'\"" % "replB", "LIST", "replB")

        # delete files
        assertiCmd(s.adminsession,"irm -rf %s" % dir1)
        assertiCmd(s.adminsession,"irm -rf %s" % dir2)

        # local cleanup
        shutil.rmtree(dir1)
        shutil.rmtree(dir2)

        # second tree teardown
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixB2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixB1")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "replB")
        # first tree teardown
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixA2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixA1")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "replA")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "pt")



    def test_create_and_remove_unixfilesystem_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" unixfilesystem "+hostname+":/tmp/irods/pydevtest_"+testresc1, "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmdFail(s.adminsession,"iadmin rmresc notaresource") # bad remove
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_unixfilesystem_resource_without_spaces(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" unixfilesystem "+hostname+":/tmp/irods/pydevtest_"+testresc1, "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin rmresc "+testresc1) # good remove
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should be gone

    def test_create_and_remove_coordinating_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession,"iadmin lr","LIST",testresc1) # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication", "LIST", "Creating") # replication
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
        assertiCmd(s.adminsession,"iadmin mkresc "+testresc1+" replication '' Context:String", "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin lr","LIST",testresc1) # should be listed
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_net","EMPTY_RESC_HOST"]) # should have empty host
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_def_path","EMPTY_RESC_PATH"]) # should have empty path
        assertiCmd(s.adminsession,"iadmin lr "+testresc1,"LIST",["resc_context","Context:String"]) # should have contextstring
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
                'jim-bob',
                'boB',
                '123.456',
                '___haysoos___']

        invalid = ['bo',
                '.bob',
                'bob.',
                'jim--bob',
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
            assertiCmd(s.adminsession,"iadmin mkuser "+name+" rodsuser","LIST","Invalid user name format") # should be rejected

        # Invalid names with special characters
        assertiCmd(s.adminsession,r"iadmin mkuser hawai\'i rodsuser","LIST","Invalid user name format") # should be rejected
        assertiCmd(s.adminsession,r"iadmin mkuser \\\/\!\*\?\|\$ rodsuser","LIST","Invalid user name format") # should be rejected



    # =-=-=-=-=-=-=-
    # REBALANCE

    def test_rebalance_for_object_count(self):
        # =-=-=-=-=-=-=-
        # read server_config.json and .odbc.ini
        cfg = ServerConfig()

        root_dir = "/tmp/irods/big_dir"
        if os.path.exists( root_dir ):
            shutil.rmtree( root_dir )
        os.makedirs( root_dir )

        for i in range(30):
            path = root_dir + "/rebalance_testfile_"+str(i)
            output = commands.getstatusoutput( 'dd if=/dev/zero of='+path+' bs=1M count=1' )
            print output[1]
            assert output[0] == 0, "dd did not successfully exit"


        # get initial object count
        initial_output = getiCmdOutput(s.adminsession, "iadmin lr demoResc")
        objcount_line = initial_output[0].splitlines()[-1]
        initial_objcount = int(objcount_line.split(":")[-1].strip())
        print "initial: "+str(initial_objcount)

        # put the new files
        assertiCmd( s.adminsession,"iput -r "+root_dir )

        # =-=-=-=-=-=-=-
        # drop several rows from the R_DATA_MAIN table to jkjjq:q
        cfg.exec_sql_cmd( "delete from R_DATA_MAIN where data_name like 'rebalance_testfile_1%'")

        # rebalance
        assertiCmd(s.adminsession,"iadmin modresc demoResc rebalance")

        # expected object count
        expected_objcount = initial_objcount + 19;  # 19 = 30 initial - 11 (1 and 10 through 19) deleted files
        print "expected: "+str(expected_objcount)
        assertiCmd(s.adminsession,"iadmin lr demoResc", "LIST", "resc_objcount: "+str(expected_objcount))

    def test_rebalance_for_repl_node(self):
        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        assertiCmd(s.adminsession,"iadmin mkresc pt passthru", "LIST", "Creating") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_b passthru", "LIST", "Creating") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_c1 passthru", "LIST", "Creating") 
        assertiCmd(s.adminsession,"iadmin mkresc pt_c2 passthru", "LIST", "Creating") 
        assertiCmd(s.adminsession,"iadmin mkresc repl replication", "LIST", "Creating") 

        assertiCmd(s.adminsession,"iadmin mkresc leaf_a unixfilesystem "+hostname+":/tmp/irods/pydevtest_leaf_a", "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc leaf_b unixfilesystem "+hostname+":/tmp/irods/pydevtest_leaf_b", "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc leaf_c unixfilesystem "+hostname+":/tmp/irods/pydevtest_leaf_c", "LIST", "Creating") # unix

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

    def test_rebalance_for_repl_in_repl_node(self):
        output = commands.getstatusoutput("hostname")
        # =-=-=-=-=-=-=-
        # STANDUP
        h = get_hostname()
        # first tree standup
        assertiCmd(s.adminsession,"iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" % ("pt", h, "pt"), "LIST", "Creating") # passthru
        assertiCmd(s.adminsession,"iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" % ("replA", h, "replA"), "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixA1", h, "unixA1"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixA2", h, "unixA2"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        assertiCmd(s.adminsession,"iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" % ("replB", h, "replB"), "LIST", "Creating") # replication
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixB1", h, "unixB1"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" % ("unixB2", h, "unixB2"), "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # wire the repls together
        assertiCmd(s.adminsession,"iadmin addchildtoresc %s %s" % ("replA", "replB"))

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 11
        for i in range( num_children ):
            assertiCmd(s.adminsession,"iput -R pt README foo%d" % i )

        # =-=-=-=-=-=-=-
        # visualize our replication
        assertiCmd(s.adminsession,"ils -AL", "LIST", "foo" )
       
        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession,"itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8" )
        assertiCmd(s.adminsession,"itrim -N1 -n 1 foo1 foo3 foo4 foo9" )
        assertiCmd(s.adminsession,"itrim -N1 -n 2 foo2 foo4 foo5" )
        
        # =-=-=-=-=-=-=-
        # dirty up a foo10 repl to ensure that code path is tested also
        assertiCmd(s.adminsession,"iadmin modresc unixA2 status down" );
        assertiCmd(s.adminsession,"iput -fR pt test_allrules.py foo10" )
        assertiCmd(s.adminsession,"iadmin modresc unixA2 status up" );
        
        # =-=-=-=-=-=-=-
        # visualize our pruning
        assertiCmd(s.adminsession,"ils -AL", "LIST", "foo" )

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession,"iadmin modresc pt rebalance" )

        # =-=-=-=-=-=-=-
        # visualize our rebalance
        assertiCmd(s.adminsession,"ils -AL", "LIST", "foo" )

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 1 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 2 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 3 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 4 ", " foo0" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 0 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 2 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 3 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 4 ", " foo1" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 0 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 1 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 3 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 4 ", " foo2" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 2 ", " foo3" ] )
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 3 ", " foo3" ] )
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 4 ", " foo3" ] )
        assertiCmd(s.adminsession,"ils -AL foo3", "LIST", [" 5 ", " foo3" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 0 ", " foo4" ] )
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 3 ", " foo4" ] )
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 4 ", " foo4" ] )
        assertiCmd(s.adminsession,"ils -AL foo4", "LIST", [" 5 ", " foo4" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 1 ", " foo5" ] )
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 3 ", " foo5" ] )
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 4 ", " foo5" ] )
        assertiCmd(s.adminsession,"ils -AL foo5", "LIST", [" 5 ", " foo5" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 1 ", " foo6" ] )
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 2 ", " foo6" ] )
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 3 ", " foo6" ] )
        assertiCmd(s.adminsession,"ils -AL foo6", "LIST", [" 4 ", " foo6" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 1 ", " foo7" ] )
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 2 ", " foo7" ] )
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 3 ", " foo7" ] )
        assertiCmd(s.adminsession,"ils -AL foo7", "LIST", [" 4 ", " foo7" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 1 ", " foo8" ] )
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 2 ", " foo8" ] )
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 3 ", " foo8" ] )
        assertiCmd(s.adminsession,"ils -AL foo8", "LIST", [" 4 ", " foo8" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 0 ", " foo9" ] )
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 2 ", " foo9" ] )
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 3 ", " foo9" ] )
        assertiCmd(s.adminsession,"ils -AL foo9", "LIST", [" 4 ", " foo9" ] )
        
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 0 ", " & ", " foo10" ] )
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 1 ", " & ", " foo10" ] )
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 2 ", " & ", " foo10" ] )
        assertiCmd(s.adminsession,"ils -AL foo10", "LIST", [" 3 ", " & ", " foo10" ] )

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range( num_children ):
            assertiCmd(s.adminsession,"irm -f foo%d" % i )

        # unwire repl nods
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA", "replB"))

        # second tree teardown
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixB2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixB1")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "replB")
        # first tree teardown
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession,"iadmin rmchildfromresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixA2")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "unixA1")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "replA")
        assertiCmd(s.adminsession,"iadmin rmresc %s" % "pt")

 
    def test_irodsFs_issue_2252(self):
        # =-=-=-=-=-=-=-
        # set up a fuse mount
        mount_point = "fuse_mount_point" 

        if not os.path.isdir( mount_point ):
            os.mkdir( mount_point )
        os.system( "irodsFs "+mount_point )

        largefilename = "big_file.txt"
        output = commands.getstatusoutput( 'dd if=/dev/zero of='+largefilename+' bs=1M count=100' )

        # =-=-=-=-=-=-=-
        # use system copy to put some data into the mount mount
        # and verify that it shows up in the ils
        cmd = "cp ./"+largefilename+" ./"+mount_point+"; ls ./"+mount_point+"/"+largefilename
        output = commands.getstatusoutput( cmd )
        out_str = str( output )
        print( "results["+out_str+"]" )

        os.system( "rm ./"+largefilename )
        os.system( "rm ./"+mount_point+"/"+largefilename )

        # tear down the fuse mount
        os.system( "fusermount -uz "+mount_point )
        if os.path.isdir( mount_point ):
            os.rmdir( mount_point )

        assert( -1 !=  out_str.find( largefilename ) )


    def test_fusermount_permissions(self):
        # Check that fusermount is configured correctly for irodsFs
        fusermount_path = distutils.spawn.find_executable('fusermount')
        assert fusermount_path is not None, 'fusermount binary not found'
        assert os.stat(fusermount_path).st_mode & stat.S_ISUID, 'fusermount setuid bit not set'
        p = subprocess.Popen(['fusermount -V'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        stdoutdata, stderrdata = p.communicate()
        assert p.returncode == 0, '\n'.join(['fusermount not executable',
                                             'return code: '+str(p.returncode),
                                             'stdout: '+stdoutdata,
                                             'stderr: '+stderrdata])
        
    def test_irodsFs(self):
        # =-=-=-=-=-=-=-
        # set up a fuse mount
        mount_point = "fuse_mount_point" 

        if not os.path.isdir( mount_point ):
            os.mkdir( mount_point )
        os.system( "irodsFs "+mount_point )

        # =-=-=-=-=-=-=-
        # put some test data
        cmd = "iput README foo0"
        output = commands.getstatusoutput( cmd )

        # =-=-=-=-=-=-=-
        # see if the data object is actually in the mount point
        # using the system ls
        cmd = "ls -l "+mount_point
        output = commands.getstatusoutput( cmd )
        out_str = str( output )
        print( "mount ls results ["+out_str+"]" )
        assert( -1 != out_str.find( "foo0" ) )

        # =-=-=-=-=-=-=-
        # use system copy to put some data into the mount mount
        # and verify that it shows up in the ils
        cmd = "cp README "+mount_point+"/baz ; ils -l baz"
        output = commands.getstatusoutput( cmd )
        out_str = str( output )
        print( "results["+out_str+"]" )
        assert( -1 !=  out_str.find( "baz" ) )

        # =-=-=-=-=-=-=-
        # now irm the file and verify that it is not visible
        # via the fuse mount
        cmd = "irm -f baz ; ils -l baz"
        output = commands.getstatusoutput( cmd )
        out_str = str( output )
        print( "results["+out_str+"]" )
        assert( -1 != out_str.find( "baz does not exist" ) )
        
        output = commands.getstatusoutput("ls -l "+mount_point )
        out_str = str( output )
        print( "mount ls results ["+out_str+"]" )
        assert( -1 !=  out_str.find( "foo0" ) )

        # =-=-=-=-=-=-=-
        # now rm the foo0 file and then verify it doesnt show
        # up in the ils
        cmd = "rm "+mount_point+"/foo0; ils -l foo0"
        print( "cmd: ["+cmd+"]" )
        output = commands.getstatusoutput( cmd )
        out_str = str( output )
        print( "results["+out_str+"]" )
        assert( -1 != out_str.find( "foo0 does not exist" ) )

        # tear down the fuse mount
        os.system( "fusermount -uz "+mount_point )
        if os.path.isdir( mount_point ):
            os.rmdir( mount_point )

    def test_iexecmd(self):
        assertiCmd( s.adminsession, "iput README foo" )
        assertiCmd( s.adminsession, "iexecmd -p /tempZone/home/rods/"+s.adminsession.sessionId+"/foo hello", "LIST", "Hello world  from irods" )
        assertiCmd( s.adminsession, "irm -f foo" )

    def test_ibun(self):
        cmd = "tar cf somefile.tar ./README"
        output = commands.getstatusoutput( cmd )

        tar_path = "/tempZone/home/rods/"+s.adminsession.sessionId+"/somefile.tar"
        dir_path = "/tempZone/home/rods/"+s.adminsession.sessionId+"/somedir"
       
        assertiCmd( s.adminsession, "iput somefile.tar" )
        assertiCmd( s.adminsession, "imkdir "+dir_path )
        assertiCmd( s.adminsession, "iput README "+dir_path+"/foo0" )
        assertiCmd( s.adminsession, "iput README "+dir_path+"/foo1" )

        assertiCmd( s.adminsession, "ibun -cD tar "+tar_path+" "+dir_path, "ERROR", "OVERWRITE_WITHOUT_FORCE_FLAG" )

        assertiCmd( s.adminsession, "irm -rf "+dir_path )
        assertiCmd( s.adminsession, "irm -rf "+tar_path )

    def test_rebalance_for_repl_node_with_different_users(self):
        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        assertiCmd(s.adminsession,"iadmin mkresc repl replication", "LIST", "Creating")
        assertiCmd(s.adminsession,"iadmin mkresc leaf_a unixfilesystem "+hostname+":/tmp/irods/pydevtest_leaf_a", "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin mkresc leaf_b unixfilesystem "+hostname+":/tmp/irods/pydevtest_leaf_b", "LIST", "Creating") # unix
        assertiCmd(s.adminsession,"iadmin addchildtoresc repl leaf_a" )
        assertiCmd(s.adminsession,"iadmin addchildtoresc repl leaf_b" )

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 3
        for i in range( num_children ):
            assertiCmd(s.adminsession,"iput -R repl README foo%d" % i )
            assertiCmd(s.sessions[1],"iput -R repl README bar%d" % i )

        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession,"itrim -N1 -n 0 foo1" )
        assertiCmd(s.sessions[1], "itrim -N1 -n 0 bar0" )

        # =-=-=-=-=-=-=-
        # dirty up a foo10 repl to ensure that code path is tested also
        assertiCmd(s.adminsession,"iadmin modresc leaf_a status down" );
        assertiCmd(s.sessions[1],"iput -fR repl test_allrules.py bar2" )
        assertiCmd(s.adminsession,"iadmin modresc leaf_a status up" );

        # =-=-=-=-=-=-=-
        # visualize our pruning and dirtying
        assertiCmd(s.adminsession,"ils -ALr /", "LIST", "rods" )

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession,"iadmin modresc repl rebalance" )

        # =-=-=-=-=-=-=-
        # visualize our rebalance
        assertiCmd(s.adminsession,"ils -ALr /", "LIST", "rods" )

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 0 ", " foo0" ] )
        assertiCmd(s.adminsession,"ils -AL foo0", "LIST", [" 1 ", " foo0" ] )

        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 1 ", " foo1" ] )
        assertiCmd(s.adminsession,"ils -AL foo1", "LIST", [" 2 ", " foo1" ] )

        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 0 ", " foo2" ] )
        assertiCmd(s.adminsession,"ils -AL foo2", "LIST", [" 1 ", " foo2" ] )

        assertiCmd(s.sessions[1],"ils -AL bar0", "LIST", [" 1 ", " bar0" ] )
        assertiCmd(s.sessions[1],"ils -AL bar0", "LIST", [" 2 ", " bar0" ] )

        assertiCmd(s.sessions[1],"ils -AL bar1", "LIST", [" 0 ", " bar1" ] )
        assertiCmd(s.sessions[1],"ils -AL bar1", "LIST", [" 1 ", " bar1" ] )

        assertiCmd(s.sessions[1],"ils -AL bar2", "LIST", [" 0 ", " bar2" ] )
        assertiCmd(s.sessions[1],"ils -AL bar2", "LIST", [" 1 ", " bar2" ] )

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range( num_children ):
            assertiCmd(s.adminsession,"irm -f foo%d" % i )
            assertiCmd(s.sessions[1],"irm -f bar%d" % i )

        assertiCmd(s.adminsession,"iadmin rmchildfromresc repl leaf_b" )
        assertiCmd(s.adminsession,"iadmin rmchildfromresc repl leaf_a" )
        assertiCmd(s.adminsession,"iadmin rmresc leaf_b" )
        assertiCmd(s.adminsession,"iadmin rmresc leaf_a" )
        assertiCmd(s.adminsession,"iadmin rmresc repl" )


    def test_rule_engine_2242(self):
        assertiCmdFail(s.adminsession,"irule -F rule1_2242.r", "LIST", "failmsg" )
        assertiCmd(s.adminsession,"irule -F rule2_2242.r", "EMPTY" )









