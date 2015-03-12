import sys
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
import pydevtest_common
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput, get_hostname, create_directory_of_small_files, get_irods_config_dir, get_irods_top_level_dir, mod_json_file
import pydevtest_sessions as s
import commands
import os
import shutil
import time
import subprocess
import stat
import socket
import json
import getpass

# =-=-=-=-=-=-=-
# build path magic to import server_config.py
pydevtestdir = os.path.realpath(__file__)
topdir = os.path.dirname(os.path.dirname(os.path.dirname(pydevtestdir)))
packagingdir = os.path.join(topdir, "packaging")
sys.path.append(packagingdir)
from server_config import ServerConfig


def write_host_access_control(filename, username, group, address, mask):
    add_ent = {}
    add_ent['user'] = username
    add_ent['group'] = group
    add_ent['address'] = address
    add_ent['mask'] = mask

    address_entries = [add_ent]
    hac = {}
    hac['access_entries'] = address_entries

    with open(filename, 'w') as f:
        json.dump(
            hac,
            f,
            sort_keys=True,
            indent=4,
            ensure_ascii=False)


class Test_iAdminSuite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup": [], "teardown": []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_api_plugin(self):
        assertiCmd(s.adminsession, "iapitest", "LIST", "this")
        p = subprocess.Popen(
            ['grep "HELLO WORLD"  ../../iRODS/server/log/rodsLog.*'], shell=True, stdout=subprocess.PIPE)
        result = p.communicate()[0]
        assert(-1 != result.find("HELLO WORLD"))

    ###################
    # iadmin
    ###################

    # LISTS

    def test_list_zones(self):
        assertiCmd(s.adminsession, "iadmin lz", "LIST", s.adminsession.getZoneName())
        assertiCmdFail(s.adminsession, "iadmin lz", "LIST", "notazone")

    def test_list_resources(self):
        assertiCmd(s.adminsession, "iadmin lr", "LIST", self.testresc)
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", "notaresource")

    def test_list_users(self):
        assertiCmd(s.adminsession, "iadmin lu", "LIST", [
                   s.adminsession.getUserName() + "#" + s.adminsession.getZoneName()])
        assertiCmdFail(s.adminsession, "iadmin lu", "LIST", "notauser")

    def test_list_groups(self):
        assertiCmd(s.adminsession, "iadmin lg", "LIST", self.testgroup)
        assertiCmdFail(s.adminsession, "iadmin lg", "LIST", "notagroup")
        assertiCmd(s.adminsession, "iadmin lg " + self.testgroup, "LIST", [s.sessions[1].getUserName()])
        assertiCmd(s.adminsession, "iadmin lg " + self.testgroup, "LIST", [s.sessions[2].getUserName()])
        assertiCmdFail(s.adminsession, "iadmin lg " + self.testgroup, "LIST", "notauser")

    # RESOURCES

    def test_resource_name_restrictions(self):
        h = get_hostname()
        oversize_name = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"  # longer than NAME_LEN
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("?/=*", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM")  # invalid char
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("replication.B", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM")  # invalid char
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("replication{", h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM")  # invalid char
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   (oversize_name, h, "junk"), "ERROR", "SYS_INVALID_INPUT_PARAM")  # too long

    def test_modify_resource_name(self):
        h = get_hostname()
        # tree standup
        assertiCmd(s.adminsession, "iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" %
                   ("pt1", h, "pt1"), "LIST", "Creating")  # passthru
        assertiCmd(s.adminsession, "iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" %
                   ("repl", h, "repl"), "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unix1", h, "unix1"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" %
                   ("pt2", h, "pt2"), "LIST", "Creating")  # passthru
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unix2", h, "unix2"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("pt1",  "repl"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("repl", "unix1"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("repl", "pt2"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("pt2",  "unix2"))

        # rename repl node
        newnodename = "replwithmoreletters"
        assertiCmd(s.adminsession, "iadmin modresc %s name %s" %
                   ("repl", newnodename), "LIST", "OK, performing the resource rename")  # rename

        # confirm children of pt1 is newnodename
        assertiCmd(s.adminsession, "iadmin lr %s" % "pt1", "LIST", "resc_children: %s" % newnodename + "{}")
        # confirm parent of newnodename is still pt1
        assertiCmd(s.adminsession, "iadmin lr %s" % newnodename, "LIST", "resc_parent: %s" % "pt1")
        # confirm children of newnodename is unix1 and pt2
        assertiCmd(s.adminsession, "iadmin lr %s" % newnodename, "LIST", "resc_children: %s" % "unix1{};pt2{}")
        # confirm parent of pt2 is newnodename
        assertiCmd(s.adminsession, "iadmin lr %s" % "pt2", "LIST", "resc_parent: %s" % newnodename)
        # confirm parent of unix2 is pt2
        assertiCmd(s.adminsession, "iadmin lr %s" % "unix2", "LIST", "resc_parent: %s" % "pt2")
        # confirm parent of unix1 is newnodename
        assertiCmd(s.adminsession, "iadmin lr %s" % "unix1", "LIST", "resc_parent: %s" % newnodename)

        # tree teardown
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("pt2", "unix2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % (newnodename, "unix1"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % (newnodename, "pt2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("pt1", newnodename))
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unix2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unix1")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "pt2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % newnodename)
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "pt1")

    def test_resource_hierarchy_manipulation(self):
        h = get_hostname()
        # first tree standup
        assertiCmd(s.adminsession, "iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" %
                   ("pt", h, "pt"), "LIST", "Creating")  # passthru
        assertiCmd(s.adminsession, "iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" %
                   ("replA", h, "replA"), "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixA1", h, "unixA1"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixA2", h, "unixA2"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        assertiCmd(s.adminsession, "iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" %
                   ("replB", h, "replB"), "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixB1", h, "unixB1"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixB2", h, "unixB2"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # create some files
        dir1 = "for_pt"
        dir2 = "for_replB"
        tree1 = 5
        tree2 = 8
        doubletree1 = 2 * tree1  # 10
        doubletree2 = 2 * tree2  # 16
        totaltree = doubletree1 + doubletree2  # 26
        create_directory_of_small_files(dir1, tree1)
        create_directory_of_small_files(dir2, tree2)
        #os.system("ls -al %s" % dir1)
        #os.system("ls -al %s" % dir2)

        # add files
        assertiCmd(s.adminsession, "iput -R %s -r %s" % ("pt", dir1))
        assertiCmd(s.adminsession, "iput -R %s -r %s" % ("replB", dir2))

        # debugging
        assertiCmd(s.adminsession, "ils -L %s" % dir1, "LIST", dir1)
        assertiCmd(s.adminsession, "ils -L %s" % dir2, "LIST", dir2)

        # add tree2 to tree1
        # add replB to replA
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "replB"))

        # debugging
        assertiCmd(s.adminsession, "ils -L %s" % dir1, "LIST", dir1)
        assertiCmd(s.adminsession, "ils -L %s" % dir2, "LIST", dir2)

        # check object_count on pt
        assertiCmd(s.adminsession, "iadmin lr %s" % "pt", "LIST", "resc_objcount: %d" % totaltree)
        # check object_count and children on replA
        assertiCmd(s.adminsession, "iadmin lr %s" % "replA", "LIST", "resc_objcount: %d" % totaltree)
        assertiCmd(s.adminsession, "iadmin lr %s" % "replA", "LIST", "resc_children: %s" % "unixA1{};unixA2{};replB{}")
        # check object_count on unixA1
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixA1", "LIST", "resc_objcount: %d" % tree1)
        # check object_count on unixA2
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixA2", "LIST", "resc_objcount: %d" % tree1)
        # check object_count and parent on replB
        assertiCmd(s.adminsession, "iadmin lr %s" % "replB", "LIST", "resc_objcount: %d" % doubletree2)
        assertiCmd(s.adminsession, "iadmin lr %s" % "replB", "LIST", "resc_parent: %s" % "replA")
        # check object_count on unixB1
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixB1", "LIST", "resc_objcount: %d" % tree2)
        # check object_count on unixB2
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixB2", "LIST", "resc_objcount: %d" % tree2)
        # check resc_hier on replB files, should have full hierarchy, and should NOT start with replB
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" %
                   "pt;replA;replB", "LIST", "pt")
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" %
                   "replB", "LIST", "CAT_NO_ROWS_FOUND")
        # check resc_name on replB files
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" %
                   "pt;replA;replB", "LIST", "pt")
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" %
                   "replB", "LIST", "CAT_NO_ROWS_FOUND")

        # remove child
        # rm replB from replA
        assertiCmd(s.adminsession, "iadmin lr %s" % "replA", "LIST", "replB")  # debugging
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "replB"))

        # check object_count on pt
        assertiCmd(s.adminsession, "iadmin lr %s" % "pt", "LIST", "resc_objcount: %d" % doubletree1)
        # check object_count on replA
        assertiCmd(s.adminsession, "iadmin lr %s" % "replA", "LIST", "resc_objcount: %d" % doubletree1)
        # check object_count on unixA1
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixA1", "LIST", "resc_objcount: %d" % tree1)
        # check object_count on unixA2
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixA2", "LIST", "resc_objcount: %d" % tree1)
        # check object_count on replB
        assertiCmd(s.adminsession, "iadmin lr %s" % "replB", "LIST", "resc_objcount: %d" % doubletree2)
        # check object_count on unixB1
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixB1", "LIST", "resc_objcount: %d" % tree2)
        # check object_count on unixB2
        assertiCmd(s.adminsession, "iadmin lr %s" % "unixB2", "LIST", "resc_objcount: %d" % tree2)
        # check resc_hier on replB files, should start with replB and not have pt anymore
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_HIER where DATA_RESC_HIER like '%s;%%'\"" %
                   "replB", "LIST", "replB")
        # check resc_name on replB files
        assertiCmd(s.adminsession, "iquest \"select DATA_RESC_NAME where DATA_RESC_HIER like '%s;%%'\"" %
                   "replB", "LIST", "replB")

        # delete files
        assertiCmd(s.adminsession, "irm -rf %s" % dir1)
        assertiCmd(s.adminsession, "irm -rf %s" % dir2)

        # local cleanup
        shutil.rmtree(dir1)
        shutil.rmtree(dir2)

        # second tree teardown
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixB2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixB1")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "replB")
        # first tree teardown
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixA2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixA1")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "replA")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "pt")

    def test_create_and_remove_unixfilesystem_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession, "iadmin mkresc " + testresc1 + " unixfilesystem " +
                   hostname + ":/tmp/irods/pydevtest_" + testresc1, "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be listed
        assertiCmdFail(s.adminsession, "iadmin rmresc notaresource")  # bad remove
        assertiCmd(s.adminsession, "iadmin rmresc " + testresc1)  # good remove
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be gone

    def test_create_and_remove_unixfilesystem_resource_without_spaces(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession, "iadmin mkresc " + testresc1 + " unixfilesystem " +
                   hostname + ":/tmp/irods/pydevtest_" + testresc1, "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be listed
        assertiCmd(s.adminsession, "iadmin rmresc " + testresc1)  # good remove
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be gone

    def test_create_and_remove_coordinating_resource(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession, "iadmin mkresc " + testresc1 + " replication", "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be listed
        # should have empty host
        assertiCmd(s.adminsession, "iadmin lr " + testresc1, "LIST", ["resc_net", "EMPTY_RESC_HOST"])
        # should have empty path
        assertiCmd(s.adminsession, "iadmin lr " + testresc1, "LIST", ["resc_def_path", "EMPTY_RESC_PATH"])
        assertiCmd(s.adminsession, "iadmin rmresc " + testresc1)  # good remove
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be gone

    def test_create_and_remove_coordinating_resource_with_explicit_contextstring(self):
        testresc1 = "testResc1"
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should not be listed
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        assertiCmd(s.adminsession, "iadmin mkresc " + testresc1 +
                   " replication '' Context:String", "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be listed
        # should have empty host
        assertiCmd(s.adminsession, "iadmin lr " + testresc1, "LIST", ["resc_net", "EMPTY_RESC_HOST"])
        # should have empty path
        assertiCmd(s.adminsession, "iadmin lr " + testresc1, "LIST", ["resc_def_path", "EMPTY_RESC_PATH"])
        # should have contextstring
        assertiCmd(s.adminsession, "iadmin lr " + testresc1, "LIST", ["resc_context", "Context:String"])
        assertiCmd(s.adminsession, "iadmin rmresc " + testresc1)  # good remove
        assertiCmdFail(s.adminsession, "iadmin lr", "LIST", testresc1)  # should be gone

    def test_modify_resource_comment(self):
        mycomment = "thisisacomment with some spaces"
        assertiCmdFail(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)
        assertiCmd(s.adminsession, "iadmin modresc " + self.testresc + " comment '" + mycomment + "'")
        assertiCmd(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)
        assertiCmd(s.adminsession, "iadmin modresc " + self.testresc + " comment 'none'")
        assertiCmdFail(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)

    # USERS

    def test_create_and_remove_new_user(self):
        testuser1 = "testaddandremoveuser"
        # should not be listed
        assertiCmdFail(s.adminsession, "iadmin lu", "LIST", [testuser1 + "#" + s.adminsession.getZoneName()])
        assertiCmd(s.adminsession, "iadmin mkuser " + testuser1 + " rodsuser")  # add rodsuser
        # should be listed
        assertiCmd(s.adminsession, "iadmin lu", "LIST", [testuser1 + "#" + s.adminsession.getZoneName()])
        assertiCmdFail(s.adminsession, "iadmin rmuser notauser")  # bad remove
        assertiCmd(s.adminsession, "iadmin rmuser " + testuser1)  # good remove
        # should be gone
        assertiCmdFail(s.adminsession, "iadmin lu", "LIST", [testuser1 + "#" + s.adminsession.getZoneName()])

    def test_iadmin_mkuser(self):

        # A few examples of valid and invalid usernames
        valid = ['bo',
                 'bob',
                 'jim-bob',
                 'boB',
                 '123.456',
                 'user@email',
                 'u@e',
                 'user.@.email',
                 '___haysoos___']

        invalid = ['#',
                   '.',
                   '..',
                   '<bob>',
                   '________________________________longer_than_NAME_LEN________________________________________________']


#         invalid = ['.bob',
#                    'bob.',
#                    'jim--bob',
#                    'user@email.',
#                    'user.',
#                    'jamesbond..007',
#                    '________________________________longer_than_NAME_LEN________________________________________________']

        # Test valid names
        for name in valid:
            assertiCmd(s.adminsession, "iadmin mkuser " + name + " rodsuser")  # should be accepted
            # should be listed
            assertiCmd(s.adminsession, "iadmin lu", "LIST", [name + "#" + s.adminsession.getZoneName()])
            assertiCmd(s.adminsession, "iadmin rmuser " + name)  # remove user
            # should be gone
            assertiCmdFail(s.adminsession, "iadmin lu", "LIST", [name + "#" + s.adminsession.getZoneName()])

        # Test invalid names
        for name in invalid:
            assertiCmd(s.adminsession, "iadmin mkuser " + name + " rodsuser",
                       "LIST", "Invalid user name format")  # should be rejected

        # Invalid names with special characters
        assertiCmd(s.adminsession, r"iadmin mkuser hawai\'i rodsuser",
                   "LIST", "Invalid user name format")  # should be rejected
        assertiCmd(s.adminsession, r"iadmin mkuser \\\/\!\*\?\|\$ rodsuser",
                   "LIST", "Invalid user name format")  # should be rejected

    # =-=-=-=-=-=-=-
    # REBALANCE
    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_AS_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_rebalance_for_object_count(self):
        # =-=-=-=-=-=-=-
        # read server_config.json and .odbc.ini
        cfg = ServerConfig()

        root_dir = "/tmp/irods/big_dir"
        if os.path.exists(root_dir):
            shutil.rmtree(root_dir)
        os.makedirs(root_dir)

        for i in range(30):
            path = root_dir + "/rebalance_testfile_" + str(i)
            output = commands.getstatusoutput('dd if=/dev/zero of=' + path + ' bs=1M count=1')
            print output[1]
            assert output[0] == 0, "dd did not successfully exit"

        # get initial object count
        initial_output = getiCmdOutput(s.adminsession, "iadmin lr demoResc")
        objcount_line = initial_output[0].splitlines()[-1]
        initial_objcount = int(objcount_line.split(":")[-1].strip())
        print "initial: " + str(initial_objcount)

        # put the new files
        assertiCmd(s.adminsession, "iput -r " + root_dir)

        # =-=-=-=-=-=-=-
        # drop several rows from the R_DATA_MAIN table to jkjjq:q
        cfg.exec_sql_cmd("delete from R_DATA_MAIN where data_name like 'rebalance_testfile_1%'")

        # rebalance
        assertiCmd(s.adminsession, "iadmin modresc demoResc rebalance")

        # expected object count
        expected_objcount = initial_objcount + 19
        # 19 = 30 initial - 11 (1 and 10 through 19) deleted files
        print "expected: " + str(expected_objcount)
        assertiCmd(s.adminsession, "iadmin lr demoResc", "LIST", "resc_objcount: " + str(expected_objcount))

    def test_rebalance_for_repl_node(self):
        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        assertiCmd(s.adminsession, "iadmin mkresc pt passthru", "LIST", "Creating")
        assertiCmd(s.adminsession, "iadmin mkresc pt_b passthru", "LIST", "Creating")
        assertiCmd(s.adminsession, "iadmin mkresc pt_c1 passthru", "LIST", "Creating")
        assertiCmd(s.adminsession, "iadmin mkresc pt_c2 passthru", "LIST", "Creating")
        assertiCmd(s.adminsession, "iadmin mkresc repl replication", "LIST", "Creating")

        assertiCmd(s.adminsession, "iadmin mkresc leaf_a unixfilesystem " + hostname +
                   ":/tmp/irods/pydevtest_leaf_a", "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc leaf_b unixfilesystem " + hostname +
                   ":/tmp/irods/pydevtest_leaf_b", "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc leaf_c unixfilesystem " + hostname +
                   ":/tmp/irods/pydevtest_leaf_c", "LIST", "Creating")  # unix

        assertiCmd(s.adminsession, "iadmin addchildtoresc pt repl")
        assertiCmd(s.adminsession, "iadmin addchildtoresc repl leaf_a")
        assertiCmd(s.adminsession, "iadmin addchildtoresc repl pt_b")
        assertiCmd(s.adminsession, "iadmin addchildtoresc repl pt_c1")
        assertiCmd(s.adminsession, "iadmin addchildtoresc pt_b leaf_b")
        assertiCmd(s.adminsession, "iadmin addchildtoresc pt_c1 pt_c2")
        assertiCmd(s.adminsession, "iadmin addchildtoresc pt_c2 leaf_c")

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 11
        for i in range(num_children):
            assertiCmd(s.adminsession, "iput -R pt README foo%d" % i)

        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession, "itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8")
        assertiCmd(s.adminsession, "itrim -N1 -n 1 foo1 foo3 foo4 foo9")
        assertiCmd(s.adminsession, "itrim -N1 -n 2 foo2 foo4 foo5")

        # =-=-=-=-=-=-=-
        # visualize our pruning
        assertiCmd(s.adminsession, "ils -AL", "LIST", "foo")

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession, "iadmin modresc pt rebalance")

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 1 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 2 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 3 ", " foo0"])

        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 0 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 2 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 3 ", " foo1"])

        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 0 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 1 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 2 ", " foo2"])

        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 2 ", " foo3"])
        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 3 ", " foo3"])
        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 4 ", " foo3"])

        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 0 ", " foo4"])
        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 1 ", " foo4"])
        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 2 ", " foo4"])

        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 1 ", " foo5"])
        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 2 ", " foo5"])
        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 3 ", " foo5"])

        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 1 ", " foo6"])
        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 2 ", " foo6"])
        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 3 ", " foo6"])

        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 1 ", " foo7"])
        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 2 ", " foo7"])
        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 3 ", " foo7"])

        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 1 ", " foo8"])
        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 2 ", " foo8"])
        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 3 ", " foo8"])

        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 0 ", " foo9"])
        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 2 ", " foo9"])
        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 3 ", " foo9"])

        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 0 ", " foo10"])
        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 1 ", " foo10"])
        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 2 ", " foo10"])

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range(num_children):
            assertiCmd(s.adminsession, "irm -f foo%d" % i)

        assertiCmd(s.adminsession, "iadmin rmchildfromresc pt_c2 leaf_c")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc repl leaf_a")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc pt_b leaf_b")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc pt_c1 pt_c2")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc repl pt_c1")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc repl pt_b")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc pt repl")

        assertiCmd(s.adminsession, "iadmin rmresc leaf_c")
        assertiCmd(s.adminsession, "iadmin rmresc leaf_b")
        assertiCmd(s.adminsession, "iadmin rmresc leaf_a")
        assertiCmd(s.adminsession, "iadmin rmresc pt_c2")
        assertiCmd(s.adminsession, "iadmin rmresc pt_c1")
        assertiCmd(s.adminsession, "iadmin rmresc pt_b")
        assertiCmd(s.adminsession, "iadmin rmresc repl")
        assertiCmd(s.adminsession, "iadmin rmresc pt")

    def test_rebalance_for_repl_in_repl_node(self):
        output = commands.getstatusoutput("hostname")
        # =-=-=-=-=-=-=-
        # STANDUP
        h = get_hostname()
        # first tree standup
        assertiCmd(s.adminsession, "iadmin mkresc %s passthru %s:/tmp/irods/pydevtest_%s" %
                   ("pt", h, "pt"), "LIST", "Creating")  # passthru
        assertiCmd(s.adminsession, "iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" %
                   ("replA", h, "replA"), "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixA1", h, "unixA1"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixA2", h, "unixA2"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        assertiCmd(s.adminsession, "iadmin mkresc %s replication %s:/tmp/irods/pydevtest_%s" %
                   ("replB", h, "replB"), "LIST", "Creating")  # replication
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixB1", h, "unixB1"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc %s unixfilesystem %s:/tmp/irods/pydevtest_%s" %
                   ("unixB2", h, "unixB2"), "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # wire the repls together
        assertiCmd(s.adminsession, "iadmin addchildtoresc %s %s" % ("replA", "replB"))

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 11
        for i in range(num_children):
            assertiCmd(s.adminsession, "iput -R pt README foo%d" % i)

        # =-=-=-=-=-=-=-
        # visualize our replication
        assertiCmd(s.adminsession, "ils -AL", "LIST", "foo")

        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession, "itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8")
        assertiCmd(s.adminsession, "itrim -N1 -n 1 foo1 foo3 foo4 foo9")
        assertiCmd(s.adminsession, "itrim -N1 -n 2 foo2 foo4 foo5")

        # =-=-=-=-=-=-=-
        # dirty up a foo10 repl to ensure that code path is tested also
        assertiCmd(s.adminsession, "iadmin modresc unixA2 status down")
        assertiCmd(s.adminsession, "iput -fR pt test_allrules.py foo10")
        assertiCmd(s.adminsession, "iadmin modresc unixA2 status up")

        # =-=-=-=-=-=-=-
        # visualize our pruning
        assertiCmd(s.adminsession, "ils -AL", "LIST", "foo")

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession, "iadmin modresc pt rebalance")

        # =-=-=-=-=-=-=-
        # visualize our rebalance
        assertiCmd(s.adminsession, "ils -AL", "LIST", "foo")

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 1 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 2 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 3 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 4 ", " foo0"])

        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 0 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 2 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 3 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 4 ", " foo1"])

        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 0 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 1 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 3 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 4 ", " foo2"])

        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 2 ", " foo3"])
        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 3 ", " foo3"])
        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 4 ", " foo3"])
        assertiCmd(s.adminsession, "ils -AL foo3", "LIST", [" 5 ", " foo3"])

        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 0 ", " foo4"])
        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 3 ", " foo4"])
        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 4 ", " foo4"])
        assertiCmd(s.adminsession, "ils -AL foo4", "LIST", [" 5 ", " foo4"])

        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 1 ", " foo5"])
        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 3 ", " foo5"])
        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 4 ", " foo5"])
        assertiCmd(s.adminsession, "ils -AL foo5", "LIST", [" 5 ", " foo5"])

        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 1 ", " foo6"])
        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 2 ", " foo6"])
        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 3 ", " foo6"])
        assertiCmd(s.adminsession, "ils -AL foo6", "LIST", [" 4 ", " foo6"])

        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 1 ", " foo7"])
        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 2 ", " foo7"])
        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 3 ", " foo7"])
        assertiCmd(s.adminsession, "ils -AL foo7", "LIST", [" 4 ", " foo7"])

        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 1 ", " foo8"])
        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 2 ", " foo8"])
        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 3 ", " foo8"])
        assertiCmd(s.adminsession, "ils -AL foo8", "LIST", [" 4 ", " foo8"])

        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 0 ", " foo9"])
        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 2 ", " foo9"])
        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 3 ", " foo9"])
        assertiCmd(s.adminsession, "ils -AL foo9", "LIST", [" 4 ", " foo9"])

        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 0 ", " & ", " foo10"])
        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 1 ", " & ", " foo10"])
        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 2 ", " & ", " foo10"])
        assertiCmd(s.adminsession, "ils -AL foo10", "LIST", [" 3 ", " & ", " foo10"])

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range(num_children):
            assertiCmd(s.adminsession, "irm -f foo%d" % i)

        # unwire repl nods
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "replB"))

        # second tree teardown
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixB2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixB1")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "replB")
        # first tree teardown
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
        assertiCmd(s.adminsession, "iadmin rmchildfromresc %s %s" % ("pt", "replA"))
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixA2")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "unixA1")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "replA")
        assertiCmd(s.adminsession, "iadmin rmresc %s" % "pt")

    def test_iexecmd(self):
        assertiCmd(s.adminsession, "iput README foo")
        assertiCmd(s.adminsession, "iexecmd -p /tempZone/home/rods/" +
                   s.adminsession.sessionId + "/foo hello", "LIST", "Hello world  from irods")
        assertiCmd(s.adminsession, "irm -f foo")

    def test_ibun(self):
        cmd = "tar cf somefile.tar ./README"
        output = commands.getstatusoutput(cmd)

        tar_path = "/tempZone/home/rods/" + s.adminsession.sessionId + "/somefile.tar"
        dir_path = "/tempZone/home/rods/" + s.adminsession.sessionId + "/somedir"

        assertiCmd(s.adminsession, "iput somefile.tar")
        assertiCmd(s.adminsession, "imkdir " + dir_path)
        assertiCmd(s.adminsession, "iput README " + dir_path + "/foo0")
        assertiCmd(s.adminsession, "iput README " + dir_path + "/foo1")

        assertiCmd(s.adminsession, "ibun -cD tar " + tar_path + " " +
                   dir_path, "ERROR", "OVERWRITE_WITHOUT_FORCE_FLAG")

        assertiCmd(s.adminsession, "irm -rf " + dir_path)
        assertiCmd(s.adminsession, "irm -rf " + tar_path)

    def test_rebalance_for_repl_node_with_different_users(self):
        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        assertiCmd(s.adminsession, "iadmin mkresc repl replication", "LIST", "Creating")
        assertiCmd(s.adminsession, "iadmin mkresc leaf_a unixfilesystem " + hostname +
                   ":/tmp/irods/pydevtest_leaf_a", "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin mkresc leaf_b unixfilesystem " + hostname +
                   ":/tmp/irods/pydevtest_leaf_b", "LIST", "Creating")  # unix
        assertiCmd(s.adminsession, "iadmin addchildtoresc repl leaf_a")
        assertiCmd(s.adminsession, "iadmin addchildtoresc repl leaf_b")

        # =-=-=-=-=-=-=-
        # place data into the resource
        num_children = 3
        for i in range(num_children):
            assertiCmd(s.adminsession, "iput -R repl README foo%d" % i)
            assertiCmd(s.sessions[1], "iput -R repl README bar%d" % i)

        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        assertiCmd(s.adminsession, "itrim -N1 -n 0 foo1")
        assertiCmd(s.sessions[1], "itrim -N1 -n 0 bar0")

        # =-=-=-=-=-=-=-
        # dirty up a foo10 repl to ensure that code path is tested also
        assertiCmd(s.adminsession, "iadmin modresc leaf_a status down")
        assertiCmd(s.sessions[1], "iput -fR repl test_allrules.py bar2")
        assertiCmd(s.adminsession, "iadmin modresc leaf_a status up")

        # =-=-=-=-=-=-=-
        # visualize our pruning and dirtying
        assertiCmd(s.adminsession, "ils -ALr /", "LIST", "rods")

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        assertiCmd(s.adminsession, "iadmin modresc repl rebalance")

        # =-=-=-=-=-=-=-
        # visualize our rebalance
        assertiCmd(s.adminsession, "ils -ALr /", "LIST", "rods")

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 0 ", " foo0"])
        assertiCmd(s.adminsession, "ils -AL foo0", "LIST", [" 1 ", " foo0"])

        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 1 ", " foo1"])
        assertiCmd(s.adminsession, "ils -AL foo1", "LIST", [" 2 ", " foo1"])

        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 0 ", " foo2"])
        assertiCmd(s.adminsession, "ils -AL foo2", "LIST", [" 1 ", " foo2"])

        assertiCmd(s.sessions[1], "ils -AL bar0", "LIST", [" 1 ", " bar0"])
        assertiCmd(s.sessions[1], "ils -AL bar0", "LIST", [" 2 ", " bar0"])

        assertiCmd(s.sessions[1], "ils -AL bar1", "LIST", [" 0 ", " bar1"])
        assertiCmd(s.sessions[1], "ils -AL bar1", "LIST", [" 1 ", " bar1"])

        assertiCmd(s.sessions[1], "ils -AL bar2", "LIST", [" 0 ", " bar2"])
        assertiCmd(s.sessions[1], "ils -AL bar2", "LIST", [" 1 ", " bar2"])

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range(num_children):
            assertiCmd(s.adminsession, "irm -f foo%d" % i)
            assertiCmd(s.sessions[1], "irm -f bar%d" % i)

        assertiCmd(s.adminsession, "iadmin rmchildfromresc repl leaf_b")
        assertiCmd(s.adminsession, "iadmin rmchildfromresc repl leaf_a")
        assertiCmd(s.adminsession, "iadmin rmresc leaf_b")
        assertiCmd(s.adminsession, "iadmin rmresc leaf_a")
        assertiCmd(s.adminsession, "iadmin rmresc repl")

    def test_rule_engine_2242(self):
        assertiCmdFail(s.adminsession, "irule -F rule1_2242.r", "LIST", "failmsg")
        assertiCmd(s.adminsession, "irule -F rule2_2242.r", "EMPTY")

    def test_hosts_config(self):
        addy1 = {}
        addy1['address'] = socket.gethostname()

        addy2 = {}
        addy2['address'] = 'jimbo'

        addresses = [addy1, addy2]

        remote = {}
        remote['address_type'] = 'local'
        remote['addresses'] = addresses

        cfg = {}
        cfg['host_entries'] = [remote]

        hosts_config = ''
        if os.path.isfile('/etc/irods/hosts_config.json'):
            hosts_config = '/etc/irods/hosts_config.json'
        else:
            install_dir = os.path.dirname(
                os.path.dirname(
                    os.path.realpath(__file__)))
            hosts_config = install_dir + '/iRODS/server/config/hosts_config.json'

        orig_file = hosts_config + '.orig'
        os.system('cp %s %s' % (hosts_config, orig_file))
        with open(hosts_config, 'w') as f:
            json.dump(
                cfg,
                f,
                sort_keys=True,
                indent=4,
                ensure_ascii=False)

        hostuser = getpass.getuser()
        assertiCmd(s.adminsession, "iadmin mkresc jimboResc unixfilesystem jimbo:/tmp/%s/jimboResc" %
                   hostuser, "LIST", "jimbo")
        assertiCmd(s.adminsession, "iput -R jimboResc README jimbofile")
        assertiCmd(s.adminsession, "irm -f jimbofile")
        assertiCmd(s.adminsession, "iadmin rmresc jimboResc")

        os.system('mv %s %s' % (orig_file, hosts_config))

    def test_host_access_control(self):
        my_ip = socket.gethostbyname(socket.gethostname())

        # manipulate the core.re to enable host access control
        corefile = get_irods_config_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName
        shutil.copy(corefile, backupcorefile)
        os.system(
            '''sed -e '/^acChkHostAccessControl { }/i acChkHostAccessControl { msiCheckHostAccessControl; }' /etc/irods/core.re > /tmp/irods/core.re''')
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp /tmp/irods/core.re /etc/irods/core.re")
        time.sleep(1)  # remove once file hash fix is commited #2279

        # restart the server to reread the new core.re
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl start")

        host_access_control = ''
        if os.path.isfile('/etc/irods/host_access_control.json'):
            host_access_control = '/etc/irods/host_access_control.json'
        else:
            install_dir = os.path.dirname(
                os.path.dirname(
                    os.path.realpath(__file__)))
            host_access_control = install_dir + '/iRODS/server/config/host_access_control.json'

        orig_file = host_access_control + '.orig'
        os.system('cp %s %s' % (host_access_control, orig_file))

        write_host_access_control(host_access_control, 'nope', 'nope', '', '')

        assertiCmdFail(s.adminsession, "ils", "ERROR", "SYS_AGENT_INIT_ERR")

        write_host_access_control(host_access_control, 'all', 'all', my_ip, '255.255.255.255')

        assertiCmd(s.adminsession, "ils", "LIST", "tempZone")

        # restore the original host_access_control.json
        os.system('mv %s %s' % (orig_file, host_access_control))

        # restore the original core.re
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

    def test_issue_2420(self):
        # manipulate the core.re to enable host access control
        corefile = get_irods_config_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName
        shutil.copy(corefile, backupcorefile)
        os.system(
            '''sed -e '/^acAclPolicy {msiAclPolicy("STRICT"); }/iacAclPolicy {ON($userNameClient == "quickshare") { } }' /etc/irods/core.re > /tmp/irods/core.re''')
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp /tmp/irods/core.re /etc/irods/core.re")
        time.sleep(1)  # remove once file hash fix is commited #2279

        # restart the server to reread the new core.re
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl start")

        assertiCmd(s.adminsession, "ils", "LIST", "tempZone")

        # look for the error "unable to read session variable $userNameClient."
        p = subprocess.Popen(
            ['grep "unable to read session variable $userNameClient."  ../../iRODS/server/log/rodsLog.*'], shell=True, stdout=subprocess.PIPE)
        result = p.communicate()[0]

        # restore the original core.re
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

        # check the results for the error
        assert(-1 == result.find("userNameClient"))

    def test_server_config_environment_variables(self):
        # set log level to get all the things
        os.environ['spLogLevel'] = '11'

        # set a random environment value to find in the log
        svr_cfg_file = get_irods_config_dir() + "/server_config.json"
        os.system("cp %s %sOrig" % (svr_cfg_file, svr_cfg_file))

        with open(svr_cfg_file) as f:
            svr_cfg = json.load(f)
        the_value = 'THIS_IS_THE_VALUE'
        svr_cfg['environment_variables']['foo_bar'] = the_value
        mod_json_file(svr_cfg_file, svr_cfg)

        # bounce the server to get the new env variable
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl start")

        assertiCmd(s.adminsession, "ils", "LIST", "tempZone")

        # look for the error "unable to read session variable $userNameClient."
        p = subprocess.Popen(
            ['grep "' + the_value + '"  ../../iRODS/server/log/rodsLog.*'], shell=True, stdout=subprocess.PIPE)
        result = p.communicate()[0]

        del os.environ['spLogLevel']
        os.system("mv %sOrig %s" % (svr_cfg_file, svr_cfg_file))

        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl start")

        # check the results for the error
        assert(-1 != result.find(the_value))

    def test_set_resource_comment_to_emptystring_ticket_2434(self):
        mycomment = "notemptystring"
        assertiCmdFail(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)
        assertiCmd(s.adminsession, "iadmin modresc " + self.testresc + " comment '" + mycomment + "'")
        assertiCmd(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)
        assertiCmd(s.adminsession, "iadmin modresc " + self.testresc + " comment ''")
        assertiCmdFail(s.adminsession, "iadmin lr " + self.testresc, "LIST", mycomment)

    def test_irmtrash_admin_2461(self):
        # 'irmtrash -M' was not deleting the r_objt_metamap entries for  collections it was deleting
        #  leading to orphaned avu's that 'iadmin rum' could never remove
        collection_basename = sys._getframe().f_code.co_name
        assertiCmd(s.adminsession, 'imkdir {collection_basename}'.format(**vars()))
        file_basename = 'dummy_file_to_trigger_recursive_rm'
        pydevtest_common.make_file(file_basename, 10)
        file_irods_path = os.path.join(collection_basename, file_basename)
        assertiCmd(s.adminsession, 'iput {file_basename} {file_irods_path}'.format(**vars()))
        a, v, u = ('attribute_' + collection_basename, 'value_' + collection_basename, 'unit_' + collection_basename)
        assertiCmd(s.adminsession, 'imeta add -C {collection_basename} {a} {v} {u}'.format(**vars()))
        assertiCmd(s.adminsession, 'imeta ls -C {collection_basename}'.format(**vars()), 'STDOUT_MULTILINE', [a, v, u])
        assertiCmd(s.adminsession, 'irm -r {collection_basename}'.format(**vars()))
        assertiCmd(s.adminsession, 'irmtrash -M')
        assertiCmd(s.adminsession, 'iadmin rum')
        assertiCmdFail(s.adminsession, '''iquest "select META_DATA_ATTR_NAME where META_DATA_ATTR_NAME = '{a}'"'''.format(**vars()),
                       'STDOUT', a)
