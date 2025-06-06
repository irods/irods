import contextlib
import copy
import getpass
import json
import os
import shutil
import socket
import stat
import subprocess
import sys
import tempfile
import textwrap
import time
import unittest

from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from .. import paths
from .. import test
from . import session
from . import settings
from .. import lib
from . import resource_suite

class Test_Iadmin(resource_suite.ResourceBase, unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Iadmin'

    def setUp(self):
        super(Test_Iadmin, self).setUp()

    def tearDown(self):
        super(Test_Iadmin, self).tearDown()

    ###################
    # iadmin
    ###################

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'msiDeleteUnusedAVUs does not redirect appropriately. See #6989.')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Applies to the NREP only')
    def test_non_admins_are_not_allowed_to_delete_unused_metadata__issue_6183(self):
        attr_name  = 'issue_6183_attr'
        attr_value = 'issue_6183_value'

        # As the administrator, generate unused metadata (i.e. add/remove metadata).
        # Removing the metadata doesn't delete it from the catalog. It is detached from
        # the collection and left in the r_meta_main table.
        self.admin.assert_icommand(['imeta', 'add', '-C', '.', attr_name, attr_value])
        self.admin.assert_icommand(['imeta', 'rm', '-C', '.', attr_name, attr_value])

        # Show that non-admins are not allowed to remove unused metadata AVUs via the
        # NREP and msiDeleteUnusedAVUs().
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.user0.assert_icommand(['irule', '-r', rep_name, 'msiDeleteUnusedAVUs()', 'null', 'ruleExecOut'],
                                   'STDERR', ['CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

        # Clean up.
        self.admin.assert_icommand(['irule', '-r', rep_name, 'msiDeleteUnusedAVUs()', 'null', 'ruleExecOut'])
        self.admin.assert_icommand(['imeta', 'ls', '-C', '.'], 'STDOUT', ['None\n'])

    def test_iadmin_rmresc_failures__5545(self):
        test_file = "foo"
        lib.make_file(test_file, 50)

        ptresc = "pt"
        newresc = "newResc"
        h = lib.get_hostname()

        try:
            # rmresc standalone resource, with data
            self.admin.assert_icommand(['iadmin', 'mkresc', newresc, 'unixfilesystem', '{}:/tmp/{}Vault'.format(h,newresc)], 'STDOUT_SINGLELINE', 'Creating')
            self.admin.assert_icommand(['iput', '-R', newresc, test_file])
            self.admin.assert_icommand(['iadmin', 'rmresc', newresc], 'STDERR_SINGLELINE', 'CAT_RESOURCE_NOT_EMPTY')

            # rmresc parent and child resource, without data
            self.admin.assert_icommand(['iadmin', 'mkresc', ptresc, 'passthru'], 'STDOUT_SINGLELINE', 'Creating')
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', ptresc, newresc])
            self.admin.assert_icommand(['irm', '-f', test_file])
            self.admin.assert_icommand(['iadmin', 'rmresc', ptresc], 'STDERR_SINGLELINE', 'CHILD_EXISTS')
            self.admin.assert_icommand(['iadmin', 'rmresc', newresc], 'STDERR_SINGLELINE', 'CHILD_EXISTS')

        finally:
            if (os.path.exists(test_file)):
                os.unlink(test_file)

            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', ptresc, newresc])
            self.admin.assert_icommand(['iadmin', 'rmresc', ptresc])
            self.admin.assert_icommand(['iadmin', 'rmresc', newresc])

    def test_tokens(self):
        self.admin.assert_icommand(['iadmin', 'at', 'user_type', 'rodstest', self.admin.username])
        self.admin.assert_icommand(['iadmin', 'lt', 'user_type'], 'STDOUT_SINGLELINE', 'rodstest')
        self.admin.assert_icommand(['iadmin', 'lt', 'user_type', 'rodstest'], 'STDOUT_SINGLELINE', 'token_name: rodstest')
        self.admin.assert_icommand(['iadmin', 'rt', 'user_type', 'rodstest'])

    def test_authentication_name(self):
        username = 'moduser_user'
        authentication_name = 'asdfsadfsadf/dfadsf/dadf/d/'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        try :
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            self.admin.assert_icommand(['iadmin', 'lua', username], 'STDOUT_SINGLELINE', authentication_name)
            self.admin.assert_icommand(['iadmin', 'lua', '#'.join([username, 'tempZone'])], 'STDOUT_SINGLELINE', authentication_name)
            self.admin.assert_icommand(['iadmin', 'lua'], 'STDOUT_SINGLELINE', username)
            self.admin.assert_icommand(['iadmin', 'luan', authentication_name], 'STDOUT_SINGLELINE', username)
            self.admin.assert_icommand(['iadmin', 'rua', username, authentication_name])
        finally :
            self.admin.assert_icommand(['iadmin', 'rmuser', username])

    def test_suq_only_accepts_zero_as_the_quota_limit__issue_4481(self):
        self.admin.assert_icommand(['iadmin', 'suq', self.user0.username, 'demoResc', '0'])

    def test_suq_no_longer_accepts_non_zero_quota_limits__issue_4481(self):
        self.admin.assert_icommand(['iadmin', 'suq', self.user0.username, 'demoResc',        '50'], 'STDERR', ['SYS_NOT_ALLOWED'])
        self.admin.assert_icommand(['iadmin', 'suq', self.user0.username, 'demoResc', 'bad_input'], 'STDERR', ['SYS_INVALID_INPUT_PARAM'])
        self.admin.assert_icommand(['iadmin', 'suq', self.user0.username, 'demoResc',          ''], 'STDERR', ['SYS_INVALID_INPUT_PARAM'])

    # PASSWORDS

    def test_iadmin_scrambling_and_descrambling(self):
        # spass
        self.admin.assert_icommand("iadmin spass gibberish 456", 'STDOUT_SINGLELINE', 'Scrambled form is:VMd96$wwJ')
        self.admin.assert_icommand("iadmin spass '-startswithhyphen' 123", 'STDERR_SINGLELINE', 'parseCmdLineOpt: Option not supported.')
        self.admin.assert_icommand("iadmin -- spass '-startswithhyphen' 123", 'STDOUT_SINGLELINE', 'Scrambled form is:9u&sabM*+#ht+ahb!')
        # dspass
        self.admin.assert_icommand("iadmin dspass VMd96$wwJ 456", 'STDOUT_SINGLELINE', 'Unscrambled form is:gibberish')
        self.admin.assert_icommand("iadmin dspass '-startswithhyphen' 123", 'STDERR_SINGLELINE', 'parseCmdLineOpt: Option not supported.')
        self.admin.assert_icommand("iadmin -- dspass '-startswithhyphen' 123", 'STDOUT_SINGLELINE', 'Unscrambled form is:!qhI),9jGkhVm%hha')

    # LISTS

    def test_list_zones(self):
        self.admin.assert_icommand("iadmin lz", 'STDOUT_SINGLELINE', self.admin.zone_name)
        self.admin.assert_icommand("iadmin lz notazone", 'STDOUT_SINGLELINE', "No rows found")

    def test_list_resources(self):
        self.admin.assert_icommand("iadmin lr", 'STDOUT_SINGLELINE', self.testresc)
        self.admin.assert_icommand("iadmin lr notaresource", 'STDOUT_SINGLELINE', "No rows found")

    def test_list_users(self):
        self.admin.assert_icommand("iadmin lu", 'STDOUT_SINGLELINE', [self.admin.username + "#" + self.admin.zone_name])
        self.admin.assert_icommand("iadmin lu notauser", 'STDOUT_SINGLELINE', "No rows found")

    def test_list_groups(self):
        group_name = 'test_group'
        session.mkgroup_and_add_users(group_name, [self.admin.username, self.user0.username])
        self.admin.assert_icommand('iadmin lg', 'STDOUT_SINGLELINE', group_name)
        self.admin.assert_icommand_fail('iadmin lg', 'STDOUT_SINGLELINE', 'notagroup')
        self.admin.assert_icommand('iadmin lg ' + group_name, 'STDOUT_SINGLELINE', self.user0.username)
        self.admin.assert_icommand_fail('iadmin lg ' + group_name, 'STDOUT_SINGLELINE', 'notauser')
        self.admin.assert_icommand(['iadmin', 'rmgroup', group_name])

    def test_list_users__issue_6624(self):
        created_user_count = 15
        username_prefix = 'a' * 57
        usernames = ['{}_{:02}#{}'.format(username_prefix, i, self.admin.zone_name) for i in range(created_user_count)]

        # Get the initial list of users for proper reset after the test
        initial_user_list = self.admin.run_icommand(['iadmin', 'lu'])[0].splitlines()

        try:
            # Create the users with long usernames
            for username in usernames:
                self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
                self.admin.assert_icommand(['iadmin', 'lu', username], 'STDOUT', 'rodsuser')

            # List all users after making the big list and ensure that they are all present
            new_user_list = self.admin.assert_icommand(['iadmin', 'lu'], 'STDOUT', self.admin.username)[1].splitlines()
            for username in initial_user_list + usernames:
                self.assertIn(username, new_user_list)

        finally:
            for username in usernames:
                self.admin.run_icommand(['iadmin', 'rmuser', username])

            self.assertEqual(
                len(initial_user_list),
                len(self.admin.run_icommand(['iadmin', 'lu'])[0].splitlines()))

    # USERS

    def test_modify_user_info__ticket_3245(self):
        user_info_string = 'USER_INFO_STRING'
        self.admin.assert_icommand("iadmin moduser %s info %s" % ( self.user0.username, user_info_string))
        self.admin.assert_icommand('iadmin lu %s' % self.user0.username, 'STDOUT_SINGLELINE', user_info_string)

        self.admin.assert_icommand("iadmin moduser %s info ''" % (self.user0.username))
        self.admin.assert_icommand_fail('iadmin lu %s' % self.user0.username, 'STDOUT_SINGLELINE', user_info_string)

    # RESOURCES

    def test_resource_name_restrictions(self):
        h = lib.get_hostname()
        oversize_name = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"  # longer than NAME_LEN
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("?/=*", h, "junk"), 'STDERR_SINGLELINE', "SYS_INVALID_INPUT_PARAM")  # invalid char
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("replication.B", h, "junk"), 'STDERR_SINGLELINE', "SYS_INVALID_INPUT_PARAM")  # invalid char
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("replication{", h, "junk"), 'STDERR_SINGLELINE', "SYS_INVALID_INPUT_PARAM")  # invalid char
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   (oversize_name, h, "junk"), 'STDERR_SINGLELINE', "SYS_INVALID_INPUT_PARAM")  # too long

    def test_no_longer_show_resc_objcount__issue_5099(self):
        self.admin.assert_icommand_fail(['iadmin','lr','demoResc'],'STDOUT_SINGLELINE','resc_objcount: ')

    @unittest.skip("deprecated due to resc id")
    def test_modify_resource_name(self):
        h = lib.get_hostname()
        # tree standup
        self.admin.assert_icommand("iadmin mkresc %s passthru %s:/tmp/irods/test_%s" %
                                   ("pt1", h, "pt1"), 'STDOUT_SINGLELINE', "Creating")  # passthru
        self.admin.assert_icommand("iadmin mkresc %s replication %s:/tmp/irods/test_%s" %
                                   ("repl", h, "repl"), 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unix1", h, "unix1"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc %s passthru %s:/tmp/irods/test_%s" %
                                   ("pt2", h, "pt2"), 'STDOUT_SINGLELINE', "Creating")  # passthru
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unix2", h, "unix2"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("pt1",  "repl"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("repl", "unix1"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("repl", "pt2"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("pt2",  "unix2"))

        # rename repl node
        newnodename = "replwithmoreletters"
        self.admin.assert_icommand("iadmin modresc %s name %s" %
                                   ("repl", newnodename), 'STDOUT_SINGLELINE', 'OK, performing the resource rename', input='yes\n')

        # confirm parent of newnodename is still pt1
        self.admin.assert_icommand("iadmin lr %s" % newnodename, 'STDOUT_SINGLELINE', "resc_parent: %s" % "pt1")
        # confirm children of newnodename is unix1 and pt2
        self.admin.assert_icommand("iadmin lr %s" % newnodename, 'STDOUT_SINGLELINE', "resc_children: %s" % "unix1{};pt2{}")
        # confirm parent of pt2 is newnodename
        self.admin.assert_icommand("iadmin lr %s" % "pt2", 'STDOUT_SINGLELINE', "resc_parent: %s" % newnodename)
        # confirm parent of unix2 is pt2
        self.admin.assert_icommand("iadmin lr %s" % "unix2", 'STDOUT_SINGLELINE', "resc_parent: %s" % "pt2")
        # confirm parent of unix1 is newnodename
        self.admin.assert_icommand("iadmin lr %s" % "unix1", 'STDOUT_SINGLELINE', "resc_parent: %s" % newnodename)

        # tree teardown
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("pt2", "unix2"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % (newnodename, "unix1"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % (newnodename, "pt2"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("pt1", newnodename))
        self.admin.assert_icommand("iadmin rmresc %s" % "unix2")
        self.admin.assert_icommand("iadmin rmresc %s" % "unix1")
        self.admin.assert_icommand("iadmin rmresc %s" % "pt2")
        self.admin.assert_icommand("iadmin rmresc %s" % newnodename)
        self.admin.assert_icommand("iadmin rmresc %s" % "pt1")

    def test_resource_hierarchy_errors(self):
        # prep
        self.admin.assert_icommand("iadmin mkresc %s passthru" %
                                   ("pt"), 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc %s passthru" %
                                   ("the_child"), 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc %s passthru" %
                                   ("grandchild"), 'STDOUT_SINGLELINE', "Creating")
        # bad parent
        self.admin.assert_icommand("iadmin addchildtoresc non_existent_resource %s" %
                                   ("pt"), 'STDERR_SINGLELINE', "CAT_INVALID_RESOURCE")
        # bad child
        self.admin.assert_icommand("iadmin addchildtoresc %s non_existent_resource" %
                                   ("pt"), 'STDERR_SINGLELINE', "CHILD_NOT_FOUND")
        # create a working hierarchy
        self.admin.assert_icommand("iadmin addchildtoresc pt the_child")
        # parent and child are the same
        self.admin.assert_icommand("iadmin addchildtoresc pt pt", 'STDERR_SINGLELINE', "HIERARCHY_ERROR")
        # adding ancestor as a child to a descendant
        self.admin.assert_icommand("iadmin addchildtoresc the_child grandchild")
        self.admin.assert_icommand("iadmin addchildtoresc grandchild pt", 'STDERR_SINGLELINE', "HIERARCHY_ERROR")
        # cleanup
        self.admin.assert_icommand("iadmin rmchildfromresc the_child grandchild")
        self.admin.assert_icommand("iadmin rmchildfromresc pt the_child")
        self.admin.assert_icommand("iadmin rmresc grandchild")
        self.admin.assert_icommand("iadmin rmresc the_child")
        self.admin.assert_icommand("iadmin rmresc pt")

    def test_resource_hierarchy_manipulation(self):
        h = lib.get_hostname()
        # first tree standup
        self.admin.assert_icommand("iadmin mkresc %s passthru" %
                                   ("pt"), 'STDOUT_SINGLELINE', "Creating")  # passthru
        self.admin.assert_icommand("iadmin mkresc %s replication" %
                                   ("replA"), 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixA1", h, "unixA1"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixA2", h, "unixA2"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("pt", "replA"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        self.admin.assert_icommand("iadmin mkresc %s replication" %
                                   ("replB"), 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixB1", h, "unixB1"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixB2", h, "unixB2"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # create some files
        dir1 = "for_pt"
        dir2 = "for_replB"
        tree1 = 5
        tree2 = 8
        doubletree1 = 2 * tree1  # 10
        doubletree2 = 2 * tree2  # 16
        totaltree = doubletree1 + doubletree2  # 26
        lib.create_directory_of_small_files(dir1, tree1)
        lib.create_directory_of_small_files(dir2, tree2)

        # add files
        self.admin.assert_icommand("iput -R %s -r %s" % ("pt", dir1))
        self.admin.assert_icommand("iput -R %s -r %s" % ("replB", dir2))

        # debugging
        self.admin.assert_icommand("ils -L %s" % dir1, 'STDOUT_SINGLELINE', dir1)
        self.admin.assert_icommand("ils -L %s" % dir2, 'STDOUT_SINGLELINE', dir2)

        # add tree2 to tree1
        # add replB to replA
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "replB"))

        # debugging
        self.admin.assert_icommand("ils -L %s" % dir1, 'STDOUT_SINGLELINE', dir1)
        self.admin.assert_icommand("ils -L %s" % dir2, 'STDOUT_SINGLELINE', dir2)

        # remove child
        # rm replB from replA
        self.admin.assert_icommand("iadmin lr %s" % "replA", 'STDOUT_SINGLELINE', "replA")  # debugging
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "replB"))

        # delete files
        self.admin.assert_icommand("irm -rf %s" % dir1)
        self.admin.assert_icommand("irm -rf %s" % dir2)

        # local cleanup
        shutil.rmtree(dir1)
        shutil.rmtree(dir2)

        # second tree teardown
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
        self.admin.assert_icommand("iadmin rmresc %s" % "unixB2")
        self.admin.assert_icommand("iadmin rmresc %s" % "unixB1")
        self.admin.assert_icommand("iadmin rmresc %s" % "replB")
        # first tree teardown
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
        self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("pt", "replA"))
        self.admin.assert_icommand("iadmin rmresc %s" % "unixA2")
        self.admin.assert_icommand("iadmin rmresc %s" % "unixA1")
        self.admin.assert_icommand("iadmin rmresc %s" % "replA")
        self.admin.assert_icommand("iadmin rmresc %s" % "pt")

    def test_create_and_remove_unixfilesystem_resource(self):
        testresc1 = "testResc1"
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should not be listed
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin mkresc " + testresc1 + " unixfilesystem " +
                                   hostname + ":/tmp/irods/test_" + testresc1, 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be listed
        self.admin.assert_icommand_fail("iadmin rmresc notaresource")  # bad remove
        self.admin.assert_icommand("iadmin rmresc " + testresc1)  # good remove
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be gone

    def test_create_and_remove_unixfilesystem_resource_without_spaces(self):
        testresc1 = "testResc1"
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should not be listed
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin mkresc " + testresc1 + " unixfilesystem " +
                                   hostname + ":/tmp/irods/test_" + testresc1, 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be listed
        self.admin.assert_icommand("iadmin rmresc " + testresc1)  # good remove
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be gone

    def test_create_and_remove_coordinating_resource(self):
        testresc1 = "testResc1"
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should not be listed
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin mkresc " + testresc1 + " replication", 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be listed
        # should have empty host
        self.admin.assert_icommand("iadmin lr " + testresc1, 'STDOUT_SINGLELINE', ["resc_net", "EMPTY_RESC_HOST"])
        # should have empty path
        self.admin.assert_icommand("iadmin lr " + testresc1, 'STDOUT_SINGLELINE', ["resc_def_path", "EMPTY_RESC_PATH"])
        self.admin.assert_icommand("iadmin rmresc " + testresc1)  # good remove
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be gone

    def test_create_and_remove_coordinating_resource_with_explicit_contextstring(self):
        testresc1 = "testResc1"
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should not be listed
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin mkresc " + testresc1 +
                                   " replication '' Context:String", 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be listed
        # should have empty host
        self.admin.assert_icommand("iadmin lr " + testresc1, 'STDOUT_SINGLELINE', ["resc_net", "EMPTY_RESC_HOST"])
        # should have empty path
        self.admin.assert_icommand("iadmin lr " + testresc1, 'STDOUT_SINGLELINE', ["resc_def_path", "EMPTY_RESC_PATH"])
        # should have contextstring
        self.admin.assert_icommand("iadmin lr " + testresc1, 'STDOUT_SINGLELINE', ["resc_context", "Context:String"])
        self.admin.assert_icommand("iadmin rmresc " + testresc1)  # good remove
        self.admin.assert_icommand_fail("iadmin lr", 'STDOUT_SINGLELINE', testresc1)  # should be gone

    def test_modify_resource_comment(self):
        mycomment = "thisisacomment with some spaces"
        self.admin.assert_icommand_fail("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)
        self.admin.assert_icommand("iadmin modresc " + self.testresc + " comment '" + mycomment + "'")
        self.admin.assert_icommand("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)
        self.admin.assert_icommand("iadmin modresc " + self.testresc + " comment 'none'")
        self.admin.assert_icommand_fail("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)

    def test_modify_resource_data_paths__3598(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "Vault")
        self.admin.assert_icommand("iadmin modrescdatapaths demoResc /var/lib/irods/Vault/ /var/lib/irods/NEWVAULT/", 'STDOUT_SINGLELINE', 'Warning', input='yes\n')
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', 'NEWVAULT')
        self.admin.assert_icommand("iadmin modrescdatapaths demoResc /var/lib/irods/NEWVAULT/ /var/lib/irods/Vault/", 'STDOUT_SINGLELINE', 'Warning', input='yes\n')
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "Vault")

    def test_create_and_remove_new_user(self):
        testuser1 = "testaddandremoveuser"
        # should not be listed
        self.admin.assert_icommand_fail("iadmin lu", 'STDOUT_SINGLELINE', [testuser1 + "#" + self.admin.zone_name])
        self.admin.assert_icommand("iadmin mkuser " + testuser1 + " rodsuser")  # add rodsuser
        # should be listed
        self.admin.assert_icommand("iadmin lu", 'STDOUT_SINGLELINE', [testuser1 + "#" + self.admin.zone_name])
        self.admin.assert_icommand("iadmin rmuser notauser", 'STDERR_SINGLELINE', 'CAT_INVALID_USER')  # bad remove
        self.admin.assert_icommand("iadmin rmuser " + testuser1)  # good remove
        # should be gone
        self.admin.assert_icommand_fail("iadmin lu", 'STDOUT_SINGLELINE', [testuser1 + "#" + self.admin.zone_name])

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

        # Test valid names
        for name in valid:
            self.admin.assert_icommand("iadmin mkuser " + name + " rodsuser")  # should be accepted
            # should be listed
            self.admin.assert_icommand("iadmin lu", 'STDOUT_SINGLELINE', [name + "#" + self.admin.zone_name])
            self.admin.assert_icommand("iadmin rmuser " + name)  # remove user
            # should be gone
            self.admin.assert_icommand_fail("iadmin lu", 'STDOUT_SINGLELINE', [name + "#" + self.admin.zone_name])

        # Test invalid names
        for name in invalid:
            self.admin.assert_icommand("iadmin mkuser " + name + " rodsuser",
                                       'STDERR_SINGLELINE', "Invalid username format")  # should be rejected

        # Invalid names with special characters
        self.admin.assert_icommand(r"iadmin mkuser hawai\'i rodsuser",
                                   'STDERR_SINGLELINE', "Invalid username format")  # should be rejected
        self.admin.assert_icommand(r"iadmin mkuser \\\/\!\*\?\|\$ rodsuser",
                                   'STDERR_SINGLELINE', "Invalid username format")  # should be rejected

        #Invalid user type
        self.admin.assert_icommand("iadmin mkuser lando invalid_user_type",
                                   'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")  # should be rejected

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 3, 2), "Only applies to 4.3.2 and above.")
    def test_iadmin_rmuser_rmgroup_report_error_on_mismatch_entity_type__issue_7380(self):
        user_name = "test_user_mismatch_entity_issue_7380"
        group_name = "test_group_mismatch_entity_issue_7380"
        self.admin.assert_icommand("iadmin mkuser %s rodsuser" % user_name)
        self.admin.assert_icommand("iadmin mkgroup %s" % group_name)

        try:
            self.admin.assert_icommand("iadmin rmuser %s" % group_name,
                                       'STDERR_SINGLELINE', "rcGeneralAdmin failed with error -827000 CAT_INVALID_USER")
            self.admin.assert_icommand("iadmin rmgroup %s" % user_name,
                                       'STDERR_SINGLELINE', "rcGeneralAdmin failed with error -829000 CAT_INVALID_GROUP")
        finally:
            self.admin.assert_icommand("iadmin rmuser %s" % user_name)
            self.admin.assert_icommand("iadmin rmgroup %s" % group_name)

    # =-=-=-=-=-=-=-
    # REBALANCE
    def test_rebalance_for_invalid_data__ticket_3147(self):
        output = subprocess.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc pt passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_b passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_c1 passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_c2 passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")

        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_c unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_c", 'STDOUT_SINGLELINE', "Creating")  # unix

        self.admin.assert_icommand("iadmin addchildtoresc pt repl")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_a")
        self.admin.assert_icommand("iadmin addchildtoresc repl pt_b")
        self.admin.assert_icommand("iadmin addchildtoresc repl pt_c1")
        self.admin.assert_icommand("iadmin addchildtoresc pt_b leaf_b")
        self.admin.assert_icommand("iadmin addchildtoresc pt_c1 pt_c2")
        self.admin.assert_icommand("iadmin addchildtoresc pt_c2 leaf_c")

        try:
            # =-=-=-=-=-=-=-
            # place data into the resource
            test_file = "iput_test_file"
            lib.make_file(test_file, 10)
            num_children = 11
            for i in range(num_children):
                self.admin.assert_icommand("iput -R pt %s foo%d" % (test_file, i))

            # =-=-=-=-=-=-=-
            # surgically trim repls so we can rebalance
            for i in range(num_children):
                self.admin.assert_icommand("itrim -N1 -n 0 foo%d" % (i), 'STDOUT_SINGLELINE', 'Total size trimmed')

            # =-=-=-=-=-=-=-
            # visualize our pruning
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # remove physical data in order to test durability of the rebalance
            path = self.admin.get_vault_session_path('leaf_a')
            print(path)
            print(os.listdir(path))
            if os.path.isfile(os.path.join(path,'foo1')):
                os.unlink(os.path.join(path,'foo1'))
            if os.path.isfile(os.path.join(path,'foo3')):
                os.unlink(os.path.join(path,'foo3'))
            if os.path.isfile(os.path.join(path,'foo5')):
                os.unlink(os.path.join(path,'foo5'))

            path = self.admin.get_vault_session_path('leaf_b')
            print(path)
            print(os.listdir(path))
            if os.path.isfile(os.path.join(path,'foo1')):
                os.unlink(os.path.join(path,'foo1'))
            if os.path.isfile(os.path.join(path,'foo3')):
                os.unlink(os.path.join(path,'foo3'))
            if os.path.isfile(os.path.join(path,'foo5')):
                os.unlink(os.path.join(path,'foo5'))

            path = self.admin.get_vault_session_path('leaf_c')
            print(path)
            print(os.listdir(path))
            if os.path.isfile(os.path.join(path,'foo1')):
                os.unlink(os.path.join(path,'foo1'))
            if os.path.isfile(os.path.join(path,'foo3')):
                os.unlink(os.path.join(path,'foo3'))
            if os.path.isfile(os.path.join(path,'foo5')):
                os.unlink(os.path.join(path,'foo5'))

            # =-=-=-=-=-=-=-
            # visualize our pruning
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # call rebalance function - the thing were actually testing... finally.
            self.admin.assert_icommand("iadmin modresc pt rebalance", 'STDERR_SINGLELINE', 'UNIX_FILE_OPEN_ERR')

            # =-=-=-=-=-=-=-
            # visualize our rebalance
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # assert that all the appropriate repl numbers exist for all the children
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', [" 1 ", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', [" 2 ", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', [" 3 ", " foo0"])

            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', [" 1 ", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', [" 2 ", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', [" 3 ", " foo2"])

            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', [" 1 ", " foo4"])
            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', [" 2 ", " foo4"])
            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', [" 3 ", " foo4"])

            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', [" 1 ", " foo6"])
            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', [" 2 ", " foo6"])
            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', [" 3 ", " foo6"])

            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', [" 1 ", " foo7"])
            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', [" 2 ", " foo7"])
            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', [" 3 ", " foo7"])

            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', [" 1 ", " foo8"])
            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', [" 2 ", " foo8"])
            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', [" 3 ", " foo8"])

            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', [" 1 ", " foo9"])
            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', [" 2 ", " foo9"])
            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', [" 3 ", " foo9"])

            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 1 ", " foo10"])
            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 2 ", " foo10"])
            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 3 ", " foo10"])

        finally:
            # =-=-=-=-=-=-=-
            # TEARDOWN
            for i in range(num_children):
                self.admin.assert_icommand("irm -f foo%d" % i)

            self.admin.assert_icommand("iadmin rmchildfromresc pt_c2 leaf_c")
            self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_a")
            self.admin.assert_icommand("iadmin rmchildfromresc pt_b leaf_b")
            self.admin.assert_icommand("iadmin rmchildfromresc pt_c1 pt_c2")
            self.admin.assert_icommand("iadmin rmchildfromresc repl pt_c1")
            self.admin.assert_icommand("iadmin rmchildfromresc repl pt_b")
            self.admin.assert_icommand("iadmin rmchildfromresc pt repl")

            self.admin.assert_icommand("iadmin rmresc leaf_c")
            self.admin.assert_icommand("iadmin rmresc leaf_b")
            self.admin.assert_icommand("iadmin rmresc leaf_a")
            self.admin.assert_icommand("iadmin rmresc pt_c2")
            self.admin.assert_icommand("iadmin rmresc pt_c1")
            self.admin.assert_icommand("iadmin rmresc pt_b")
            self.admin.assert_icommand("iadmin rmresc repl")
            self.admin.assert_icommand("iadmin rmresc pt")

    def test_rebalance_for_repl_node(self):
        hostname = lib.get_hostname()

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc pt passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_b passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_c1 passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc pt_c2 passthru", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")

        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_c unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_c", 'STDOUT_SINGLELINE', "Creating")  # unix

        self.admin.assert_icommand("iadmin addchildtoresc pt repl")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_a")
        self.admin.assert_icommand("iadmin addchildtoresc repl pt_b")
        self.admin.assert_icommand("iadmin addchildtoresc repl pt_c1")
        self.admin.assert_icommand("iadmin addchildtoresc pt_b leaf_b")
        self.admin.assert_icommand("iadmin addchildtoresc pt_c1 pt_c2")
        self.admin.assert_icommand("iadmin addchildtoresc pt_c2 leaf_c")

        # =-=-=-=-=-=-=-
        # place data into the resource
        test_file = "iput_test_file"
        lib.make_file(test_file, 10)
        num_children = 11
        for i in range(num_children):
            self.admin.assert_icommand("iput -R pt %s foo%d" % (test_file, i))

        # =-=-=-=-=-=-=-
        # surgically trim repls so we can rebalance
        self.admin.assert_icommand("itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8", 'STDOUT', "data objects trimmed")
        self.admin.assert_icommand("itrim -N1 -n 1 foo1 foo3 foo4 foo9", 'STDOUT', "data objects trimmed")
        self.admin.assert_icommand("itrim -N1 -n 2 foo2 foo4 foo5", 'STDOUT', "data objects trimmed")

        # =-=-=-=-=-=-=-
        # visualize our pruning
        self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        self.admin.assert_icommand("iadmin modresc pt rebalance")

        # =-=-=-=-=-=-=-
        # assert that all the appropriate repl numbers exist for all the children
        self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["leaf_a", " foo0"])
        self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["leaf_b", " foo0"])
        self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["leaf_c", " foo0"])

        self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["leaf_a", " foo1"])
        self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["leaf_b", " foo1"])
        self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["leaf_c", " foo1"])

        self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["leaf_a", " foo2"])
        self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["leaf_b", " foo2"])
        self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["leaf_c", " foo2"])

        self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["leaf_a", " foo3"])
        self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["leaf_b", " foo3"])
        self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["leaf_c", " foo3"])

        self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["leaf_a", " foo4"])
        self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["leaf_b", " foo4"])
        self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["leaf_c", " foo4"])

        self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["leaf_a", " foo5"])
        self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["leaf_b", " foo5"])
        self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["leaf_c", " foo5"])

        self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["leaf_a", " foo6"])
        self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["leaf_b", " foo6"])
        self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["leaf_c", " foo6"])

        self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["leaf_a", " foo7"])
        self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["leaf_b", " foo7"])
        self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["leaf_c", " foo7"])

        self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["leaf_a", " foo8"])
        self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["leaf_b", " foo8"])
        self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["leaf_c", " foo8"])

        self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["leaf_a", " foo9"])
        self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["leaf_b", " foo9"])
        self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["leaf_c", " foo9"])

        self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', ["leaf_a", " foo10"])
        self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', ["leaf_b", " foo10"])
        self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', ["leaf_c", " foo10"])

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range(num_children):
            self.admin.assert_icommand("irm -f foo%d" % i)

        self.admin.assert_icommand("iadmin rmchildfromresc pt_c2 leaf_c")
        self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_a")
        self.admin.assert_icommand("iadmin rmchildfromresc pt_b leaf_b")
        self.admin.assert_icommand("iadmin rmchildfromresc pt_c1 pt_c2")
        self.admin.assert_icommand("iadmin rmchildfromresc repl pt_c1")
        self.admin.assert_icommand("iadmin rmchildfromresc repl pt_b")
        self.admin.assert_icommand("iadmin rmchildfromresc pt repl")

        self.admin.assert_icommand("iadmin rmresc leaf_c")
        self.admin.assert_icommand("iadmin rmresc leaf_b")
        self.admin.assert_icommand("iadmin rmresc leaf_a")
        self.admin.assert_icommand("iadmin rmresc pt_c2")
        self.admin.assert_icommand("iadmin rmresc pt_c1")
        self.admin.assert_icommand("iadmin rmresc pt_b")
        self.admin.assert_icommand("iadmin rmresc repl")
        self.admin.assert_icommand("iadmin rmresc pt")

    def test_rebalance_for_tree_with_many_resources__issue_5110(self):
        hostname = lib.get_hostname()

        try:
            # =-=-=-=-=-=-=-
            # STANDUP
            lib.create_replication_resource(self.admin, 'repl')

            lib.create_random_resource(self.admin, 'red')
            lib.create_random_resource(self.admin, 'green')

            for i in range(1,301):
                lib.create_ufs_resource(self.admin, 'ufs{0}'.format(i))

            lib.add_child_resource(self.admin, 'repl', 'red')
            lib.add_child_resource(self.admin, 'repl', 'green')

            for i in range(1,151):
                lib.add_child_resource(self.admin, 'red', 'ufs{0}'.format(i))
            for i in range(151,301):
                lib.add_child_resource(self.admin, 'green', 'ufs{0}'.format(i))

            # =-=-=-=-=-=-=-
            # call rebalance function - confirm no error returned (USER_STRLEN_TOOLONG)
            self.admin.assert_icommand("iadmin modresc repl rebalance")

        finally:
            # =-=-=-=-=-=-=-
            # TEARDOWN
            for i in range(1,151):
                lib.remove_child_resource(self.admin, 'red', 'ufs{0}'.format(i))
            for i in range(151,301):
                lib.remove_child_resource(self.admin, 'green', 'ufs{0}'.format(i))
            lib.remove_child_resource(self.admin, 'repl', 'red')
            lib.remove_child_resource(self.admin, 'repl', 'green')

            for i in range(1,301):
                lib.remove_resource(self.admin, 'ufs{0}'.format(i))
            lib.remove_resource(self.admin, 'red')
            lib.remove_resource(self.admin, 'green')
            lib.remove_resource(self.admin, 'repl')

    def test_rebalance_for_repl_in_repl_node(self):
        # =-=-=-=-=-=-=-
        # STANDUP
        h = lib.get_hostname()
        # first tree standup
        self.admin.assert_icommand("iadmin mkresc %s passthru %s:/tmp/irods/test_%s" %
                                   ("pt", h, "pt"), 'STDOUT_SINGLELINE', "Creating")  # passthru
        self.admin.assert_icommand("iadmin mkresc %s replication %s:/tmp/irods/test_%s" %
                                   ("replA", h, "replA"), 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixA1", h, "unixA1"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixA2", h, "unixA2"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("pt", "replA"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "unixA1"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "unixA2"))
        # second tree standup
        self.admin.assert_icommand("iadmin mkresc %s replication %s:/tmp/irods/test_%s" %
                                   ("replB", h, "replB"), 'STDOUT_SINGLELINE', "Creating")  # replication
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixB1", h, "unixB1"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc %s unixfilesystem %s:/tmp/irods/test_%s" %
                                   ("unixB2", h, "unixB2"), 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replB", "unixB1"))
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replB", "unixB2"))

        # wire the repls together
        self.admin.assert_icommand("iadmin addchildtoresc %s %s" % ("replA", "replB"))

        # =-=-=-=-=-=-=-
        # place data into the resource
        try:
            test_file = "iput_test_file"
            lib.make_file(test_file, 10)
            num_children = 11
            for i in range(num_children):
                self.admin.assert_icommand("iput -R pt %s foo%d" % (test_file, i))

            # =-=-=-=-=-=-=-
            # visualize our replication
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # surgically trim repls so we can rebalance
            self.admin.assert_icommand("itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8", 'STDOUT', "data objects trimmed")
            self.admin.assert_icommand("itrim -N1 -n 1 foo1 foo3 foo4 foo9", 'STDOUT', "data objects trimmed")
            self.admin.assert_icommand("itrim -N1 -n 2 foo2 foo4 foo5", 'STDOUT', "data objects trimmed")

            # =-=-=-=-=-=-=-
            # visualize our pruning
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # dirty up a foo10 repl to ensure that code path is tested also
            down_resource = 'unixA2'
            resources = ['unixA1', 'unixA2', 'unixB1', 'unixB2']
            self.admin.assert_icommand(['iadmin', 'modresc', down_resource, 'status', 'down'])
            test1_path = os.path.join(self.admin.local_session_dir, 'test1')
            lib.make_file(test1_path, 1500, 'arbitrary')
            self.admin.assert_icommand("iput -fR pt %s foo10" % (test1_path))
            self.admin.assert_icommand(['iadmin', 'modresc', down_resource, 'status', 'up'])
            # Ensure resource marked down does not receive an update
            for resc in resources:
                desired_status = 'X' if resc is down_resource else '&'
                self.admin.assert_icommand(['ils', '-l', 'foo10'], 'STDOUT_SINGLELINE', [resc, desired_status, "foo10"])

            # =-=-=-=-=-=-=-
            # call rebalance function - the thing were actually testing... finally.
            self.admin.assert_icommand("iadmin modresc pt rebalance")

            # =-=-=-=-=-=-=-
            # visualize our rebalance
            self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

            # =-=-=-=-=-=-=-
            # assert that all the appropriate repl numbers exist for all the children
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["unixA1", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["unixA2", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["unixB1", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', ["unixB2", " foo0"])

            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["unixA1", " foo1"])
            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["unixA2", " foo1"])
            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["unixB1", " foo1"])
            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', ["unixB2", " foo1"])

            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["unixA1", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["unixA2", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["unixB1", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', ["unixB2", " foo2"])

            self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["unixA1", " foo3"])
            self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["unixA2", " foo3"])
            self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["unixB1", " foo3"])
            self.admin.assert_icommand("ils -AL foo3", 'STDOUT_SINGLELINE', ["unixB2", " foo3"])

            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["unixA1", " foo4"])
            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["unixA2", " foo4"])
            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["unixB1", " foo4"])
            self.admin.assert_icommand("ils -AL foo4", 'STDOUT_SINGLELINE', ["unixB2", " foo4"])

            self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["unixA1", " foo5"])
            self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["unixA2", " foo5"])
            self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["unixB1", " foo5"])
            self.admin.assert_icommand("ils -AL foo5", 'STDOUT_SINGLELINE', ["unixB2", " foo5"])

            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["unixA1", " foo6"])
            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["unixA2", " foo6"])
            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["unixB1", " foo6"])
            self.admin.assert_icommand("ils -AL foo6", 'STDOUT_SINGLELINE', ["unixB2", " foo6"])

            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["unixA1", " foo7"])
            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["unixA2", " foo7"])
            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["unixB1", " foo7"])
            self.admin.assert_icommand("ils -AL foo7", 'STDOUT_SINGLELINE', ["unixB2", " foo7"])

            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["unixA1", " foo8"])
            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["unixA2", " foo8"])
            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["unixB1", " foo8"])
            self.admin.assert_icommand("ils -AL foo8", 'STDOUT_SINGLELINE', ["unixB2", " foo8"])

            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["unixA1", " foo9"])
            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["unixA2", " foo9"])
            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["unixB1", " foo9"])
            self.admin.assert_icommand("ils -AL foo9", 'STDOUT_SINGLELINE', ["unixB2", " foo9"])

            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 0 ", " & ", " foo10"])
            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 1 ", " & ", " foo10"])
            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 2 ", " & ", " foo10"])
            self.admin.assert_icommand("ils -AL foo10", 'STDOUT_SINGLELINE', [" 3 ", " & ", " foo10"])

        finally:
            self.admin.assert_icommand("iadmin modresc unixA2 status up")

            # =-=-=-=-=-=-=-
            # TEARDOWN
            for i in range(num_children):
                self.admin.assert_icommand("irm -f foo%d" % i)

            # unwire repl nods
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "replB"))

            # second tree teardown
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replB", "unixB2"))
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replB", "unixB1"))
            self.admin.assert_icommand("iadmin rmresc %s" % "unixB2")
            self.admin.assert_icommand("iadmin rmresc %s" % "unixB1")
            self.admin.assert_icommand("iadmin rmresc %s" % "replB")
            # first tree teardown
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "unixA2"))
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("replA", "unixA1"))
            self.admin.assert_icommand("iadmin rmchildfromresc %s %s" % ("pt", "replA"))
            self.admin.assert_icommand("iadmin rmresc %s" % "unixA2")
            self.admin.assert_icommand("iadmin rmresc %s" % "unixA1")
            self.admin.assert_icommand("iadmin rmresc %s" % "replA")
            self.admin.assert_icommand("iadmin rmresc %s" % "pt")

    def test_rebalance_visibility_in_resource_avu__3683(self):
        hostname = lib.get_hostname()
        # populate demoResc with an AVU, to confirm we're matching the right resource below
        value = f"{hostname}:1234"
        unit = "20181004T123456Z"
        self.admin.assert_icommand(["imeta","add","-R","demoResc","rebalance_operation", value, unit])
        # rebalance, and check for avu removal
        self.admin.assert_icommand(["iadmin","modresc",self.testresc,"rebalance"])
        self.admin.assert_icommand(["imeta","ls","-R",self.testresc], 'STDOUT_SINGLELINE', "None")
        # rebalance already running
        self.admin.assert_icommand(["imeta","add","-R",self.testresc,"rebalance_operation", value, unit])
        self.admin.assert_icommand(["iadmin","modresc",self.testresc,"rebalance"], 'STDERR_SINGLELINE', "REBALANCE_ALREADY_ACTIVE_ON_RESOURCE")
        # clean up
        self.admin.assert_icommand(["imeta","rm","-R",self.testresc,"rebalance_operation", value, unit])
        self.admin.assert_icommand(["imeta","rm","-R","demoResc","rebalance_operation", value, unit])

    @unittest.skip( "configuration requires sudo to create the environment")
    def test_rebalance_for_repl_node_with_different_users_with_write_failure__issue_3674(self):
        #  sudo truncate -s 3M /tmp/irods/badfs_file
        #  sudo mkfs -t ext3 /tmp/irods/badfs_file
        #  sudo mount -t ext3 /tmp/irods/badfs_file /tmp/irods/bad_fs/
        #  sudo chmod 777 /tmp/irods/bad_fs/


        output = subprocess.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/bad_fs/pydevtest_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_a")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_b")

        # =-=-=-=-=-=-=-
        # place data into the resource
        test_file = "rebalance_test_file"
        # test for single buffer put
        #lib.make_file(test_file, 2000000)

        # test for parallel transfer
        lib.make_file(test_file, 40000000)

        self.admin.assert_icommand("iadmin modresc leaf_b status down")

        self.user0.assert_icommand("iput -R repl %s foo" % (test_file))

        self.admin.assert_icommand("ils -Lr /", 'STDOUT_SINGLELINE', self.admin.username)

        self.admin.assert_icommand("iadmin modresc leaf_b status up")

        # =-=-=-=-=-=-=-
        # call rebalance function - the thing were actually testing... finally.
        # test for single buffer put
        #self.admin.assert_icommand("iadmin modresc repl rebalance", 'STDERR_SINGLELINE', 'SYS_COPY_LEN_ERR')

        # test for parallel transfer
        self.admin.assert_icommand("iadmin modresc repl rebalance", 'STDERR_SINGLELINE', 'UNIX_FILE_WRITE_ERR')

        # =-=-=-=-=-=-=-
        # visualize our rebalance
        self.admin.assert_icommand("ils -Lr /", 'STDOUT_SINGLELINE', self.admin.username)

        # =-=-=-=-=-=-=-
        # TEARDOWN
        self.user0.assert_icommand("irm -f foo")

        self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_b")
        self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_a")
        self.admin.assert_icommand("iadmin rmresc leaf_b")
        self.admin.assert_icommand("iadmin rmresc leaf_a")
        self.admin.assert_icommand("iadmin rmresc repl")

    def test_rebalance_for_repl_node_with_different_users(self):
        hostname = lib.get_hostname()

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_a")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_b")

        try:
            # =-=-=-=-=-=-=-
            # place data into the resource
            test_file = "rebalance_test_file"
            lib.make_file(test_file, 100)
            num_children = 3
            for i in range(num_children):
                self.admin.assert_icommand("iput -R repl %s foo%d" % (test_file, i))
                self.user0.assert_icommand("iput -R repl %s bar%d" % (test_file, i))

            # =-=-=-=-=-=-=-
            # surgically trim repls so we can rebalance
            self.admin.assert_icommand("itrim -N1 -n 0 foo1", 'STDOUT', "data objects trimmed")
            self.user0.assert_icommand("itrim -N1 -n 0 bar0", 'STDOUT', "data objects trimmed")

            # =-=-=-=-=-=-=-
            # dirty up a foo10 repl to ensure that code path is tested also
            self.admin.assert_icommand("iadmin modresc leaf_a status down")
            test1_path = os.path.join(self.admin.local_session_dir, 'test1')
            lib.make_file(test1_path, 1500, 'arbitrary')
            self.user0.assert_icommand("iput -fR repl %s bar2" % (test1_path))
            self.admin.assert_icommand("iadmin modresc leaf_a status up")

            # =-=-=-=-=-=-=-
            # visualize our pruning and dirtying
            self.admin.assert_icommand("ils -ALr /", 'STDOUT_SINGLELINE', self.admin.username)

            # =-=-=-=-=-=-=-
            # call rebalance function - the thing were actually testing... finally.
            self.admin.assert_icommand("iadmin modresc repl rebalance")

            # =-=-=-=-=-=-=-
            # visualize our rebalance
            self.admin.assert_icommand("ils -ALr /", 'STDOUT_SINGLELINE', self.admin.username)

            # =-=-=-=-=-=-=-
            # assert that all the appropriate repl numbers exist for all the children
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', [" 0 ", " foo0"])
            self.admin.assert_icommand("ils -AL foo0", 'STDOUT_SINGLELINE', [" 1 ", " foo0"])

            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', [" 1 ", " foo1"])
            self.admin.assert_icommand("ils -AL foo1", 'STDOUT_SINGLELINE', [" 2 ", " foo1"])

            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', [" 0 ", " foo2"])
            self.admin.assert_icommand("ils -AL foo2", 'STDOUT_SINGLELINE', [" 1 ", " foo2"])

            self.user0.assert_icommand("ils -AL bar0", 'STDOUT_SINGLELINE', [" 1 ", " bar0"])
            self.user0.assert_icommand("ils -AL bar0", 'STDOUT_SINGLELINE', [" 2 ", " bar0"])

            self.user0.assert_icommand("ils -AL bar1", 'STDOUT_SINGLELINE', [" 0 ", " bar1"])
            self.user0.assert_icommand("ils -AL bar1", 'STDOUT_SINGLELINE', [" 1 ", " bar1"])

            self.user0.assert_icommand("ils -AL bar2", 'STDOUT_SINGLELINE', [" 0 ", " bar2"])
            self.user0.assert_icommand("ils -AL bar2", 'STDOUT_SINGLELINE', [" 1 ", " bar2"])

        finally:
            # =-=-=-=-=-=-=-
            # TEARDOWN
            for i in range(num_children):
                self.admin.run_icommand("irm -f foo%d" % i)
                self.user0.run_icommand("irm -f bar%d" % i)

            self.admin.run_icommand("iadmin rmchildfromresc repl leaf_b")
            self.admin.run_icommand("iadmin rmchildfromresc repl leaf_a")
            self.admin.run_icommand("iadmin rmresc leaf_b")
            self.admin.run_icommand("iadmin rmresc leaf_a")
            self.admin.run_icommand("iadmin rmresc repl")

    def test_rebalance_for_child_with_substring_in_name(self):
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin mkresc rootrepl replication", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_c unixfilesystem " + hostname +
                                   ":/tmp/irods/test_leaf_c", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin addchildtoresc rootrepl repl")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_a")
        self.admin.assert_icommand("iadmin addchildtoresc repl leaf_b")

        temp_file = 'test_rebalance_for_child_with_substring_in_name'
        lib.make_file(temp_file, 10)
        try:
            # put a file in replication hierarchy
            self.admin.assert_icommand(['iput', '-R', 'rootrepl', temp_file])
            # force put to standalone resource with a replica
            self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_b")
            self.admin.assert_icommand(['iput', '-f', '-R', 'leaf_b', temp_file])
            # put standalone resource in replication hierarchy
            # put back removed child in replication hierarchy
            self.admin.assert_icommand("iadmin addchildtoresc repl leaf_c")
            self.admin.assert_icommand("iadmin addchildtoresc repl leaf_b")

            self.admin.assert_icommand(['ils', '-l', temp_file], 'STDOUT', '&')
            self.admin.assert_icommand(['ils', '-l', temp_file], 'STDOUT', 'X')
            self.admin.assert_icommand_fail(['ils', '-l', temp_file], 'STDOUT', ' 2 ')
            # rebalance
            self.admin.assert_icommand("iadmin modresc rootrepl rebalance")
            # ensure everything went well
            self.admin.assert_icommand(['ils', '-l', temp_file], 'STDOUT', [' 0 ', '&'])
            self.admin.assert_icommand(['ils', '-l', temp_file], 'STDOUT', [' 1 ', '&'])
            self.admin.assert_icommand(['ils', '-l', temp_file], 'STDOUT', [' 2 ', '&'])

        finally:
            self.admin.assert_icommand(['irm', '-f', temp_file])
            if (os.path.exists(temp_file)):
                os.unlink(temp_file)

            self.admin.assert_icommand("iadmin rmchildfromresc rootrepl repl")
            self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_c")
            self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_b")
            self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_a")
            self.admin.assert_icommand("iadmin rmresc leaf_c")
            self.admin.assert_icommand("iadmin rmresc leaf_b")
            self.admin.assert_icommand("iadmin rmresc leaf_a")
            self.admin.assert_icommand("iadmin rmresc repl")
            self.admin.assert_icommand("iadmin rmresc rootrepl")

    def test_hosts_config(self):
        config = IrodsConfig()

        try:
            with lib.file_backed_up(config.server_config_path):
                config.server_config['host_resolution'] = {
                    'host_entries': [
                        {
                            'address_type': 'local',
                            'addresses': [
                                socket.gethostname(),
                                'jimbo',
                                'larry'
                            ]
                        }
                    ]
                }
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)
                IrodsController().reload_configuration()

                try:
                    hostuser = getpass.getuser()
                    temp_file = 'file_to_test_hosts_config'
                    lib.make_file(temp_file, 10)
                    self.admin.assert_icommand("iadmin mkresc jimboResc unixfilesystem jimbo:/tmp/%s/jimboResc" %
                                               hostuser, 'STDOUT_SINGLELINE', "jimbo")
                    self.admin.assert_icommand("iput -R jimboResc " + temp_file + " jimbofile")
                    self.admin.assert_icommand("irm -f jimbofile")

                    big_file = 'big_file_to_test_hosts_config'
                    lib.make_file(big_file, 35000000)
                    self.admin.assert_icommand("iadmin mkresc larryResc unixfilesystem larry:/tmp/%s/larryResc" %
                                               hostuser, 'STDOUT_SINGLELINE', "larry")
                    self.admin.assert_icommand("iput -R larryResc " + big_file + " biglarryfile")
                    self.admin.assert_icommand("ils -L biglarryfile", 'STDOUT_SINGLELINE', 'biglarryfile')
                    self.admin.assert_icommand("irm -f biglarryfile")

                finally:
                    self.admin.run_icommand("iadmin rmresc jimboResc")
                    self.admin.run_icommand("iadmin rmresc larryResc")
        finally:
            IrodsController().reload_configuration()

    def test_host_access_control(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acChkHostAccessControl {
                    msiCheckHostAccessControl;
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acChkHostAccessControl(rule_args, callback, rei):
                    callback.msiCheckHostAccessControl()
            ''')
        }

        my_ip = socket.gethostbyname(socket.gethostname())

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])

                config = IrodsConfig()

                def update_host_access_control_options(username, group, address, mask):
                    config.server_config['host_access_control'] = {
                        'access_entries': [
                            {
                                'user': username,
                                'group': group,
                                'address': address,
                                'mask': mask
                            }
                        ]
                    }
                    lib.update_json_file_from_dict(config.server_config_path, config.server_config)
                    IrodsController().reload_configuration()

                with lib.file_backed_up(config.server_config_path):
                    update_host_access_control_options('nope', 'nope', '', '')
                    self.admin.assert_icommand("ils", 'STDERR_SINGLELINE', "CONNECTION_REFUSED")

                    update_host_access_control_options('all', 'all', my_ip, '255.255.255.255')
                    self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', self.admin.zone_name)

        finally:
            IrodsController().reload_configuration()

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'python does not yet support msiGetStdoutInExecCmdOut - RTS')
    def test_server_config_environment_variables(self):
        irods_config = IrodsConfig()
        with lib.file_backed_up(irods_config.server_config_path):
            # set an arbitrary environment value to find in the log
            the_value = 'THIS_IS_THE_VALUE'
            server_config = copy.deepcopy(irods_config.server_config)
            server_config['environment_variables']['foo_bar'] = the_value
            irods_config.commit(server_config, irods_config.server_config_path)

            # bounce the server to get the new env variable
            IrodsController().restart(test_mode=True)

            # look for the error "unable to read session variable $userNameClient."
            cmd_directory = os.path.join(irods_config.irods_directory, 'msiExecCmd_bin')
            with contextlib.closing(tempfile.NamedTemporaryFile(mode='wt', dir=cmd_directory, delete=False)) as env_script:
                print('#!/bin/sh\nenv\n', file=env_script, end='')
            os.chmod(env_script.name, 0o700)
            self.admin.assert_icommand('irule \'msiExecCmd("{0}", "", "", "", "", *param); '
                    'msiGetStdoutInExecCmdOut(*param, *out); '
                    'writeLine("stdout", "*out")\' null ruleExecOut'.format(os.path.basename(env_script.name)),
                'STDOUT_SINGLELINE', the_value)
            os.unlink(env_script.name)
        IrodsController().restart(test_mode=True)

    def test_set_resource_comment_to_emptystring_ticket_2434(self):
        mycomment = "notemptystring"
        self.admin.assert_icommand_fail("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)
        self.admin.assert_icommand("iadmin modresc " + self.testresc + " comment '" + mycomment + "'")
        self.admin.assert_icommand("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)
        self.admin.assert_icommand("iadmin modresc " + self.testresc + " comment ''")
        self.admin.assert_icommand_fail("iadmin lr " + self.testresc, 'STDOUT_SINGLELINE', mycomment)

    def test_irmtrash_admin_2461(self):
        # 'irmtrash -M' was not deleting the r_objt_metamap entries for  collections it was deleting
        #  leading to orphaned avu's that 'iadmin rum' could never remove
        collection_basename = sys._getframe().f_code.co_name
        self.admin.assert_icommand('imkdir {collection_basename}'.format(**vars()))
        file_basename = 'dummy_file_to_trigger_recursive_rm'
        lib.make_file(file_basename, 10)
        file_irods_path = os.path.join(collection_basename, file_basename)
        self.admin.assert_icommand('iput {file_basename} {file_irods_path}'.format(**vars()))
        a, v, u = ('attribute_' + collection_basename, 'value_' + collection_basename, 'unit_' + collection_basename)
        self.admin.assert_icommand('imeta add -C {collection_basename} {a} {v} {u}'.format(**vars()))
        self.admin.assert_icommand('imeta ls -C {collection_basename}'.format(**vars()), 'STDOUT_MULTILINE', [a, v, u])
        self.admin.assert_icommand('irm -r {collection_basename}'.format(**vars()))
        self.admin.assert_icommand('irmtrash -M')
        self.admin.assert_icommand('iadmin rum')
        self.admin.assert_icommand_fail('''iquest "select META_DATA_ATTR_NAME where META_DATA_ATTR_NAME = '{a}'"'''.format(**vars()), 'STDOUT_SINGLELINE', a)

    def test_msiset_default_resc__2712(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acSetRescSchemeForCreate{
                    msiSetDefaultResc('TestResc', 'forced');
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acSetRescSchemeForCreate(rule_args, callback, rei):
                    callback.msiSetDefaultResc('TestResc', 'forced');
            ''')
        }

        hostname = lib.get_hostname()
        testresc1 = 'TestResc'

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])
                IrodsController().reload_configuration()

                trigger_file = 'file_to_trigger_acSetRescSchemeForCreate'
                lib.make_file(trigger_file, 10)
                initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_log_path)
                self.user0.assert_icommand(['iput', trigger_file])
                self.user0.assert_icommand(['ils', '-L', trigger_file], 'STDOUT_SINGLELINE', testresc1)

                os.unlink(trigger_file)
                self.user0.assert_icommand('irm -f ' + trigger_file)

        finally:
            IrodsController().reload_configuration()

    def test_storageadmin_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes storageadmin", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_domainadmin_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes domainadmin", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_rodscurators_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes rodscurators", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_impostor_resource_debug_logging(self):
        irods_config = IrodsConfig()
        server_config_filename = irods_config.server_config_path

        try:
            with lib.file_backed_up(server_config_filename):
                server_config_update = {
                    'log_level': {
                        'agent': 'info',
                        'agent_factory': 'info',
                        'api': 'info',
                        'authentication': 'info',
                        'database': 'info',
                        'delay_server': 'info',
                        'legacy': 'info',
                        'microservice': 'info',
                        'network': 'info',
                        'resource': 'debug',
                        'rule_engine': 'info',
                        'server': 'info'
                    }
                }
                lib.update_json_file_from_dict(server_config_filename, server_config_update)
                IrodsController().reload_configuration()

                name_of_bogus_resource = 'name_of_bogus_resource'
                name_of_missing_plugin = 'name_of_missing_plugin'
                self.admin.assert_icommand(['iadmin', 'mkresc', name_of_bogus_resource, name_of_missing_plugin], 'STDOUT_SINGLELINE', name_of_missing_plugin)

                initial_size_of_server_log = lib.get_file_size_by_path(irods_config.server_log_path)
                self.admin.assert_icommand(['ils'], 'STDOUT_SINGLELINE', self.admin.zone_name) # creates an agent, which instantiates the resource hierarchy

                debug_message = 'loading impostor resource for [{0}] of type [{1}] with context [] and load_plugin message'.format(name_of_bogus_resource, name_of_missing_plugin)
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_greater_than_count(
                        msg=debug_message,
                        count=0,
                        server_log_path=irods_config.server_log_path,
                        start_index=initial_size_of_server_log))

        finally:
            self.admin.run_icommand(['iadmin', 'rmresc', name_of_bogus_resource])
            IrodsController().reload_configuration()

    @unittest.skipIf(True, "Activate once fixed issue#3387")
    def test_dlopen_failure_error_message(self):
        irods_config = IrodsConfig()
        plugin_dir = os.path.join(paths.plugins_directory(), 'resources')
        name_of_corrupt_plugin = 'name_of_corrupt_plugin'
        name_of_corrupt_so = 'lib' + name_of_corrupt_plugin + '.so'
        path_of_corrupt_so = os.path.join(plugin_dir, name_of_corrupt_so)
        lib.make_file(path_of_corrupt_so, 500)

        name_of_bogus_resource = 'name_of_bogus_resource'
        self.admin.assert_icommand(['iadmin', 'mkresc', name_of_bogus_resource, name_of_corrupt_plugin], 'STDOUT_SINGLELINE', name_of_corrupt_plugin)

        initial_size_of_server_log = lib.get_file_size_by_path(irods_config.server_log_path)
        self.admin.assert_icommand(['ils'], 'STDOUT_SINGLELINE', self.admin.zone_name) # creates an agent, which instantiates the resource hierarchy

        for msg in ['dlerror', 'PLUGIN_ERROR']:
            lib.delayAssert(
                lambda: lib.log_message_occurrences_greater_than_count(
                    msg=msg,
                    count=0,
                    server_log_path=irods_config.server_log_path,
                    start_index=initial_size_of_server_log))

        self.admin.assert_icommand(['iadmin', 'rmresc', name_of_bogus_resource])
        os.remove(path_of_corrupt_so)

    def test_admin_listings(self):
        self.admin.assert_icommand(['iadmin', 'lt'], 'STDOUT_SINGLELINE', 'zone_type')
        self.admin.assert_icommand(['iadmin', 'lt', 'zone_type'], 'STDOUT_SINGLELINE', 'local')
        self.admin.assert_icommand(['iadmin', 'lt', 'zone_type', 'local'], 'STDOUT_SINGLELINE', 'token_namespace: zone_type')
        self.admin.assert_icommand(['iadmin', 'lr'], 'STDOUT_SINGLELINE', 'demoResc')
        self.admin.assert_icommand(['iadmin', 'lr', 'demoResc'], 'STDOUT_SINGLELINE', 'resc_name: demoResc')
        self.admin.assert_icommand(['iadmin', 'lz'], 'STDOUT_SINGLELINE', self.admin.zone_name)
        self.admin.assert_icommand(['iadmin', 'lz', self.admin.zone_name], 'STDOUT_SINGLELINE', 'zone_type_name: local')
        self.admin.assert_icommand(['iadmin', 'lg'], 'STDOUT_SINGLELINE', 'public')
        self.admin.assert_icommand(['iadmin', 'lg', 'public'], 'STDOUT_SINGLELINE', ['rods#tempZone'])
        self.admin.assert_icommand(['iadmin', 'lgd', 'public'], 'STDOUT_SINGLELINE', 'user_name: public')
        self.admin.assert_icommand(['iadmin', 'lrg'], 'STDERR_SINGLELINE', 'Resource groups are deprecated.')

    def test_group_membership(self):
        self.admin.assert_icommand(["iadmin", "rmgroup", "g1"], 'STDERR_SINGLELINE', 'CAT_INVALID_GROUP')
        self.admin.assert_icommand(["iadmin", "mkgroup", "g1"])
        self.admin.assert_icommand(["iadmin", "atg", "g1", self.user0.username])
        self.admin.assert_icommand(["iadmin", "atg", "g1a", self.user0.username], 'STDERR_SINGLELINE', 'CAT_INVALID_GROUP')
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username], 'STDERR', '-1830000 USER_NOT_IN_GROUP')
        self.admin.assert_icommand(["iadmin", "atg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user0.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username], 'STDERR', '-1830000 USER_NOT_IN_GROUP')
        self.admin.assert_icommand(["iadmin", "rmgroup", "g1"])

    def test_idempotent_aua__issue_3104(self):
        username = 'issue_3104_user'
        authentication_name = '3104_user@TEST.AUTHENTICATION'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        try:
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
            out, _, _ = self.admin.run_icommand(['iadmin', 'lua', username])
            self.assertEqual(len(out.splitlines()), 1, 'iadmin lua returned more than one line')
        finally:
            self.admin.assert_icommand(['iadmin', 'rmuser', username])

    def test_aua_multiple_distinguished_name__issue_3620(self):
        username = 'issue_3620_user'
        authentication_name_1 = '3620_user_1@TEST.AUTHENTICATION'
        authentication_name_2 = '3620_user_2@TEST.AUTHENTICATION'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        try:
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_2])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_2])
            self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'lua', username], 'STDOUT_MULTILINE', [username + ' ' + authentication_name_1, username + ' ' + authentication_name_2])
            self.assertEqual(len(out.splitlines()), 2, 'iadmin lua did not return exactly two lines')
        finally:
            self.admin.assert_icommand(['iadmin', 'rmuser', username])


    def test_addchildtoresc_forbidden_characters_3449(self):
        self.admin.assert_icommand(['iadmin', 'mkresc', 'parent', 'passthru'], 'STDOUT_SINGLELINE', 'passthru')
        self.admin.assert_icommand(['iadmin', 'mkresc', 'child', 'passthru'], 'STDOUT_SINGLELINE', 'passthru')
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'parent', 'child', ';'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM')
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'parent', 'child', '}'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM')
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'parent', 'child', '{'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM')
        self.admin.assert_icommand(['iadmin', 'rmresc', 'parent'])
        self.admin.assert_icommand(['iadmin', 'rmresc', 'child'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_silent_failure_when_removing_non_child_from_parent__issue_3859(self):
        # Create resource tree.
        self.admin.assert_icommand('iadmin mkresc pt0 passthru', 'STDOUT', 'passthru')
        self.admin.assert_icommand('iadmin mkresc pt1 passthru', 'STDOUT', 'passthru')
        self.admin.assert_icommand('iadmin addchildtoresc pt0 demoResc')

        # The tests we care about.
        self.admin.assert_icommand('iadmin rmchildfromresc pt1 demoResc', 'STDERR', '-857000 CAT_INVALID_CHILD')
        self.admin.assert_icommand('iadmin rmchildfromresc invalid_resc demoResc', 'STDERR', '-78000 SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('iadmin rmchildfromresc demoResc invalid_resc', 'STDERR', '-78000 SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('iadmin rmchildfromresc pt0 demoResc')

        # Clean-up.
        self.admin.assert_icommand('iadmin rmresc pt0')
        self.admin.assert_icommand('iadmin rmresc pt1')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_print_full_username__issue_3170(self):
        username = 'issue_3170_username_ts'
        self.admin.assert_icommand('iadmin mkuser {0} rodsuser'.format(username))
        self.admin.assert_icommand('iadmin lu {0}'.format(username), 'STDOUT', 'user_name: {0}'.format(username))
        self.admin.assert_icommand('iadmin rmuser {0}'.format(username))

    def test_empty_data_mode_does_not_cause_INVALID_LEXICAL_CAST_on_rebalance__issue_5227(self):
        filename = os.path.join(self.admin.local_session_dir, 'issue_5227')
        data_object = os.path.join(self.admin.session_collection, os.path.basename(filename))

        try:
            # Create resources.
            self.admin.assert_icommand(['iadmin', 'mkresc', 'root_resc', 'passthru'], 'STDOUT', ['root_resc'])
            self.admin.assert_icommand(['iadmin', 'mkresc', 'pt0', 'passthru'], 'STDOUT', ['pt0'])
            self.admin.assert_icommand(['iadmin', 'mkresc', 'pt1', 'passthru'], 'STDOUT', ['pt1'])
            self.admin.assert_icommand(['iadmin', 'mkresc', 'repl_resc', 'replication'], 'STDOUT', ['repl_resc'])
            lib.create_ufs_resource(self.admin, 'ufs0')
            lib.create_ufs_resource(self.admin, 'ufs1')

            # Create resource hierarchy.
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'root_resc', 'repl_resc'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'repl_resc', 'pt0'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'repl_resc', 'pt1'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'pt0', 'ufs0'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'pt1', 'ufs1'])

            # Create a new data object in the hierarchy.
            lib.make_file(filename, 0)
            self.admin.assert_icommand(['iput', '-R', 'root_resc', filename, data_object])
            self.admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', [' root_resc;repl_resc;pt0;ufs0 ', ' root_resc;repl_resc;pt1;ufs1 '])

            # The GenQuery string used to verify that the replicas are marked as good.
            gql = "select DATA_REPL_STATUS where DATA_NAME = '{0}'".format(os.path.basename(data_object))

            #
            # Test 1: The UPDATE Case
            #

            # While the issue was encountered via an empty string, we don't have any tools that
            # allow us to create the empty string scenario. Instead, we know the empty string scenario
            # is not any different then the data mode containing a non-integer value. Therefore, using
            # an invalid string for the data mode satisfies the test.
            for invalid_data_mode in [' ', '-', 'X']:
                # Set the source replica's data mode to an invalid value.
                self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_MODE', invalid_data_mode])

                # Create work for the rebalance operation by setting the second replica's status to stale.
                self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '1', 'DATA_REPL_STATUS', '0'])

                # Show that doing a rebalance does not trigger an INVALID_LEXICAL_CAST error.
                self.admin.assert_icommand(['iadmin', 'modresc', 'root_resc', 'rebalance'])

                # Show that each replica is now marked as good.
                _, out, _ = self.admin.assert_icommand(['iquest', '%s', gql], 'STDOUT', ['1'])
                self.assertEqual(out.strip(), '1') # This guarantees the string is indeed '1'.

            #
            # Test 2: The CREATE Case
            #

            for invalid_data_mode in [' ', '-', 'X']:
                # Set the source replica's data mode to an invalid value. At this point, replica zero should
                # be marked as good, therefore it will be used as the source replica when creating the second replica.
                self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_MODE', invalid_data_mode])

                # Create work for the rebalance operation by trimming the second replica.
                self.admin.assert_icommand(['itrim', '-n1', '-N1', data_object], 'STDOUT', ['trimmed = 1'])

                # Show that doing a rebalance does not trigger an INVALID_LEXICAL_CAST error.
                self.admin.assert_icommand(['iadmin', 'modresc', 'root_resc', 'rebalance'])

                # Show that each replica is now marked as good.
                _, out, _ = self.admin.assert_icommand(['iquest', '%s', gql], 'STDOUT', ['1'])
                self.assertEqual(out.strip(), '1') # This guarantees the string is indeed '1'.

        finally:
            # Remove the data object in case one of the previous assertions failed.
            # This is required so that the resources created for this test can be removed.
            self.admin.run_icommand(['irm', '-f', data_object])

            # Tear down resource hierarchy.
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'root_resc', 'repl_resc'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'repl_resc', 'pt0'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'repl_resc', 'pt1'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'pt0', 'ufs0'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'pt1', 'ufs1'])

            # Remove resources.
            self.admin.run_icommand(['iadmin', 'rmresc', 'root_resc'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'pt0'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'pt1'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'repl_resc'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'ufs0'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'ufs1'])

            # Remove the test file.
            if os.path.exists(filename):
                os.remove(filename)

    def test_addchildtoresc_atomically_updates_the_parent_child_resource_relationship__issue_5934(self):
        def assert_parent_resource(_parent_resc):
            # Get the resource id of the parent resource.
            gql = "select RESC_ID where RESC_NAME = '{0}'".format(_parent_resc)
            resc_id, err, ec = self.admin.run_icommand(['iquest', '%s', gql])

            # We should not see any errors.
            resc_id = resc_id.strip()
            self.assertEqual(ec, 0)
            self.assertEqual(len(err.strip()), 0)
            self.assertGreater(len(resc_id), 0)

            # Show that the child resource has the expected parent resource.
            self.admin.assert_icommand(['ilsresc', '-l', 'pt_5934_2'], 'STDOUT', ['parent: {0}\n'.format(resc_id)])

        try:
            # Create some passthru resources.
            self.admin.assert_icommand(['iadmin', 'mkresc', 'pt_5934_0', 'passthru'], 'STDOUT', ['pt_5934_0'])
            self.admin.assert_icommand(['iadmin', 'mkresc', 'pt_5934_1', 'passthru'], 'STDOUT', ['pt_5934_1'])
            self.admin.assert_icommand(['iadmin', 'mkresc', 'pt_5934_2', 'passthru'], 'STDOUT', ['pt_5934_2'])

            # Create a resource hierarchy.
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'pt_5934_0', 'pt_5934_2'])
            assert_parent_resource('pt_5934_0')

            # Show that changing the parent of a child resource no longer requires that
            # the existing parent-child relationship be severed via a call to "rmchildfromresc".
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'pt_5934_1', 'pt_5934_2'])
            assert_parent_resource('pt_5934_1')

        finally:
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'pt_5934_0', 'pt_5934_2'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', 'pt_5934_1', 'pt_5934_2'])

            self.admin.run_icommand(['iadmin', 'rmresc', 'pt_5934_0'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'pt_5934_1'])
            self.admin.run_icommand(['iadmin', 'rmresc', 'pt_5934_2'])

    def test_non_admins_are_not_allowed_to_invoke_iadmin_lg__issue_6188(self):
        self.user0.assert_icommand(['iadmin', 'lg'], 'STDERR', ['-830000 CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

    def test_non_admins_are_not_allowed_to_invoke_iadmin_set_delay_server__issue_5249(self):
        self.user0.assert_icommand(['iadmin', 'set_delay_server', lib.get_hostname()], 'STDERR', ['-830000 CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

    def test_non_admins_are_not_allowed_to_invoke_iadmin_get_delay_server_info__issue_5249(self):
        self.user0.assert_icommand(['iadmin', 'get_delay_server_info'], 'STDERR', ['-830000 CAT_INSUFFICIENT_PRIVILEGE_LEVEL'])

    def test_iadmin_mkuser_returns_correct_error_on_duplicate_user__issue_7599(self):
        test_user_name = 'duplicate_user_issue_7599'
        try:
            self.admin.assert_icommand(['iadmin', 'mkuser', test_user_name, 'rodsuser'])
            self.admin.assert_icommand(['iadmin', 'mkuser', test_user_name, 'rodsuser'], 'STDERR', ['-809000 CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME'])
        finally:
            self.admin.run_icommand(['iadmin', 'rmuser', test_user_name])

    def test_iadmin_mkgroup_returns_correct_error_on_duplicate_group__issue_7599(self):
        test_group_name = 'duplicate_group_issue_7599'
        try:
            self.admin.assert_icommand(['iadmin', 'mkgroup', test_group_name])
            self.admin.assert_icommand(['iadmin', 'mkgroup', test_group_name], 'STDERR', ['-809000 CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME'])
        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', test_group_name])

    def test_iadmin_exits_nonzero_on_nonexistent_command__issue_7734(self):
        # Nonexistent command
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'nonexistentcommand'], 'STDERR');
        self.assertNotEqual(ec, 0)

    def test_iadmin_rmuser_removes_associated_acls__issue_7778(self):
        test_user_name = "test_user_issue_7778"
        test_group_name = "test_group_issue_7778"
        test_collection_name = "test_collection_issue_7778"
        test_dataobj_name = "test_data_object_issue_7778"
        try:
            self.admin.assert_icommand(["imkdir", test_collection_name])
            self.admin.assert_icommand(["itouch", test_dataobj_name])
            self.admin.assert_icommand(["iadmin", "mkgroup", test_group_name])
            self.admin.assert_icommand(["iadmin", "mkuser", test_user_name, "rodsuser"])
            self.admin.assert_icommand(
                ["iadmin", "atg", test_group_name, test_user_name]
            )
            self.admin.assert_icommand(
                ["ichmod", "read_object", test_group_name, test_collection_name]
            )
            self.admin.assert_icommand(
                ["ichmod", "modify_object", test_user_name, test_collection_name]
            )
            self.admin.assert_icommand(
                ["ichmod", "read_object", test_group_name, test_dataobj_name]
            )
            self.admin.assert_icommand(
                ["ichmod", "modify_object", test_user_name, test_dataobj_name]
            )

            # Collect IDs of user and group
            _, out, _ = self.admin.assert_icommand(
                [
                    "iquest",
                    "%s",
                    "SELECT USER_ID WHERE USER_NAME = '%s' || = '%s'"
                    % (test_user_name, test_group_name),
                ],
                "STDOUT_MULTILINE",
                r"^[0-9]+$",
                use_regex=True,
            )

            # Create iquest query string for the IDs-- we don't care about order, we just want both
            query_string = "= '%s' || = '%s'" % tuple(out.strip().split('\n'))

            # Should see the permissions as expected
            self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT COLL_ACCESS_USER_ID, COLL_ACCESS_NAME WHERE COLL_NAME = '%s' AND COLL_ACCESS_USER_ID %s"
                    % (
                        os.path.join(
                            self.admin.session_collection, test_collection_name
                        ),
                        query_string,
                    ),
                ],
                "STDOUT",
                ["COLL_ACCESS_NAME = read_object", "COLL_ACCESS_NAME = modify_object"],
            )
            # As above, but for data objects
            self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT DATA_ACCESS_USER_ID, DATA_ACCESS_NAME WHERE DATA_NAME = '%s' AND COLL_NAME = '%s' AND DATA_ACCESS_USER_ID %s"
                    % (test_dataobj_name, self.admin.session_collection, query_string),
                ],
                "STDOUT",
                ["DATA_ACCESS_NAME = read_object", "DATA_ACCESS_NAME = modify_object"],
            )

            self.admin.assert_icommand(["iadmin", "rmuser", test_user_name])

            # Should only see read_object, since that was the perm given to the group
            _, out, _ = self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT COLL_ACCESS_USER_ID, COLL_ACCESS_NAME WHERE COLL_NAME = '%s' AND COLL_ACCESS_USER_ID %s"
                    % (
                        os.path.join(
                            self.admin.session_collection, test_collection_name
                        ),
                        query_string,
                    ),
                ],
                "STDOUT",
                ["COLL_ACCESS_NAME = read_object"],
            )
            self.assertNotIn("COLL_ACCESS_NAME = modify_object", out)

            # As above, but for data objects
            _, out, _ = self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT DATA_ACCESS_USER_ID, DATA_ACCESS_NAME WHERE DATA_NAME = '%s' AND COLL_NAME = '%s' AND DATA_ACCESS_USER_ID %s"
                    % (test_dataobj_name, self.admin.session_collection, query_string),
                ],
                "STDOUT",
                ["DATA_ACCESS_NAME = read_object"],
            )
            self.assertNotIn("DATA_ACCESS_NAME = modify_object", out)

            self.admin.assert_icommand(["iadmin", "rmgroup", test_group_name])

            # Perms should vanish after deletion of associated group
            self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT COLL_ACCESS_USER_ID, COLL_ACCESS_NAME WHERE COLL_NAME = '%s' AND COLL_ACCESS_USER_ID %s"
                    % (
                        os.path.join(
                            self.admin.session_collection, test_collection_name
                        ),
                        query_string,
                    ),
                ],
                "STDOUT_SINGLELINE",
                "CAT_NO_ROWS_FOUND: Nothing was found matching your query",
            )

            # As above, but for data objects
            self.admin.assert_icommand(
                [
                    "iquest",
                    "SELECT DATA_ACCESS_USER_ID, DATA_ACCESS_NAME WHERE DATA_NAME = '%s' AND COLL_NAME = '%s' AND DATA_ACCESS_USER_ID %s"
                    % (test_dataobj_name, self.admin.session_collection, query_string),
                ],
                "STDOUT_SINGLELINE",
                "CAT_NO_ROWS_FOUND: Nothing was found matching your query",
            )
        finally:
            self.admin.run_icommand(["iadmin", "rmuser", test_user_name])
            self.admin.run_icommand(["iadmin", "rmgroup", test_group_name])
            self.admin.run_icommand(["irmdir", test_collection_name])
            self.admin.run_icommand(["irm", "-f", test_dataobj_name])

    def test_modzone_does_not_accept_invalid_zone_names_for_the_local_zone__issue_7722(self):
        # Show that "iadmin modzone name" no longer accepts invalid zone names.
        invalid_zone_name = 'überZone'
        ec, _, _ = self.admin.assert_icommand(
            ['iadmin', 'modzone', self.admin.zone_name, 'name', invalid_zone_name],
            'STDERR',
            ['-130000 SYS_INVALID_INPUT_PARAM'],
            input='yes')
        self.assertNotEqual(ec, 0)

        # Show the zone name was not updated.
        _, out, err = self.admin.assert_icommand(['iadmin', 'lz'], 'STDOUT')
        self.assertNotIn(invalid_zone_name, out)
        self.assertIn(self.admin.zone_name, out)
        self.assertEqual(len(err), 0)

        # Show the zone is still functional.
        self.admin.assert_icommand(['ils'], 'STDOUT', [self.admin.session_collection + ':'])

    def test_mkzone_does_not_accept_invalid_zone_names__issue_7722(self):
        # Show that "iadmin mkzone" no longer accepts invalid zone names.
        invalid_zone_name = 'überZone'
        ec, _, _ = self.admin.assert_icommand(
            ['iadmin', 'mkzone', invalid_zone_name, 'remote', 'localhost:1247'], 'STDERR', ['-130000 SYS_INVALID_INPUT_PARAM'])
        self.assertNotEqual(ec, 0)

        # Show the zone was not added.
        _, out, err = self.admin.assert_icommand(['iadmin', 'lz'], 'STDOUT')
        self.assertNotIn(invalid_zone_name, out)
        self.assertIn(self.admin.zone_name, out)
        self.assertEqual(len(err), 0)

    def test_modzone_does_not_accept_invalid_zone_names_for_remote_zones__issue_7722(self):
        good_zone_name = 'goodZoneIssue7722'

        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', good_zone_name, 'remote', 'localhost:1247'])

            # Show that "iadmin modzone name" no longer accepts invalid zone names.
            # Modification of a zone name takes different code paths depending on whether
            # the zone name matches the local zone or a remote zone.
            invalid_zone_name = 'überZone'
            ec, _, _ = self.admin.assert_icommand(
                ['iadmin', 'modzone', good_zone_name, 'name', invalid_zone_name],
                'STDERR',
                ['-130000 SYS_INVALID_INPUT_PARAM'],
                input='yes')
            self.assertNotEqual(ec, 0)

            # Show the zone name was not updated.
            _, out, err = self.admin.assert_icommand(['iadmin', 'lz'], 'STDOUT')
            self.assertNotIn(invalid_zone_name, out)
            self.assertIn(good_zone_name, out)
            self.assertEqual(len(err), 0)

            # Show the local zone is still functional.
            self.admin.assert_icommand(['ils'], 'STDOUT', [self.admin.session_collection + ':'])

        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', good_zone_name])

class Test_Iadmin_Resources(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        self.resc_name = 'warningresc'
        self.host_warning_message = 'Warning, resource host address \'localhost\' will not work properly'
        self.vault_error_message = 'ERROR: rcGeneralAdmin failed with error -813000 CAT_INVALID_RESOURCE_VAULT_PATH'
        self.vault_warning_message = 'root directory cannot be used as vault path.'

        # Ensure vault does not exist so the stat fails
        self.good_vault = tempfile.mkdtemp()
        shutil.rmtree(self.good_vault)

        with session.make_session_for_existing_admin() as admin_session:
            # Make resource with good host and vault path
            admin_session.assert_icommand(['iadmin', 'mkresc', self.resc_name, 'unixfilesystem',
                                           '{0}:{1}'.format(lib.get_hostname(), self.good_vault)],
                                          'STDOUT_SINGLELINE', self.resc_name)
        super(Test_Iadmin_Resources, self).setUp()

    def tearDown(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand(['iadmin', 'rmresc', self.resc_name])
        super(Test_Iadmin_Resources, self).tearDown()

    def test_mkresc_host_path_warning_messages(self):
        # Make resc with localhost and root vault
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resc_name])
        stdout,stderr,rc = self.admin.run_icommand(['iadmin', 'mkresc', self.resc_name, 'unixfilesystem', 'localhost:/'])
        # Should produce warnings and errors
        self.assertTrue(0 != rc)
        self.assertTrue(self.vault_error_message in stderr)
        self.assertTrue(self.vault_warning_message in stdout)
        self.assertTrue(self.host_warning_message in stdout)
        # Should fail to create resource
        self.admin.assert_icommand_fail(['ilsresc', '--ascii'], 'STDOUT_SINGLELINE', self.resc_name)

        # Make resc with good vault path and localhost and ensure it is created
        self.admin.assert_icommand(['iadmin', 'mkresc', self.resc_name, 'unixfilesystem', 'localhost:' + self.good_vault],
                                   'STDOUT_SINGLELINE', self.host_warning_message)
        self.admin.assert_icommand(['ilsresc', '--ascii'], 'STDOUT_SINGLELINE', self.resc_name)

    def test_modresc_host_path_warning_messages(self):
        # Change host to localhost and ensure that it was changed (check for warning)
        self.admin.assert_icommand(['iadmin', 'modresc', self.resc_name, 'host', 'localhost'], 'STDOUT_SINGLELINE', self.host_warning_message)
        self.admin.assert_icommand(['ilsresc', '-l', self.resc_name], 'STDOUT_SINGLELINE', 'location: localhost')

        # Change vault to root and check for errors/warnings
        stdout,stderr,rc = self.admin.run_icommand(['iadmin', 'modresc', self.resc_name, 'path', '/'])
        self.assertTrue(0 != rc)
        self.assertTrue(self.vault_error_message in stderr)
        self.assertTrue(self.vault_warning_message in stdout)
        # Changing vault should have failed, so make sure it has not changed
        self.admin.assert_icommand(['ilsresc', '-l', self.resc_name], 'STDOUT_SINGLELINE', 'vault: ' + self.good_vault)

    def generate_invalid_resource_names(self, prefix):
        return [
            '{0} Resc'.format(prefix),
            '{0}\fResc'.format(prefix),
            '{0}\nResc'.format(prefix),
            '{0}\rResc'.format(prefix),
            '{0}\tResc'.format(prefix),
            '{0}\vResc'.format(prefix)
        ]

    def test_creating_a_resource_with_whitespace_is_not_allowed__issue_5861(self):
        expected_output = [' -859000 CAT_INVALID_RESOURCE_NAME']
        for resc_name in self.generate_invalid_resource_names(prefix='ufs_5861'):
            self.admin.assert_icommand(['iadmin', 'mkresc', resc_name, 'compound'], 'STDERR', expected_output)

    def test_adding_a_child_resource_with_whitespace_is_not_allowed__issue_5861(self):
        expected_output = [' -859000 CAT_INVALID_RESOURCE_NAME']
        for resc_name in self.generate_invalid_resource_names(prefix='ufs_5861'):
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'demoResc', resc_name], 'STDERR', expected_output)
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', resc_name, 'demoResc'], 'STDERR', expected_output)

    def test_removing_a_child_resource_with_whitespace_is_not_allowed__issue_5861(self):
        expected_output = [' -859000 CAT_INVALID_RESOURCE_NAME']
        for resc_name in self.generate_invalid_resource_names(prefix='ufs_5861'):
            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', 'demoResc', resc_name], 'STDERR', expected_output)
            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', resc_name, 'demoResc'], 'STDERR', expected_output)

    def test_removing_a_resource_with_whitespace_is_not_allowed__issue_5861(self):
        expected_output = [' -859000 CAT_INVALID_RESOURCE_NAME']
        for resc_name in self.generate_invalid_resource_names(prefix='ufs_5861'):
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_name], 'STDERR', expected_output)

    def test_renaming_resource_to_name_containing_whitespace_is_not_allowed__issue_5861(self):
        try:
            resc_name = 'ufs_5861_Resc'
            lib.create_ufs_resource(self.admin, resc_name)

            expected_output = [' -859000 CAT_INVALID_RESOURCE_NAME']

            # Show that the following BAD resource names are not allowed.
            for new_name in self.generate_invalid_resource_names(prefix='ufs_5861'):
                self.admin.assert_icommand(['iadmin', 'modresc', resc_name, 'name', new_name], 'STDERR', expected_output, input='yes')

            # Show that leading/trailing whitespace does not cause any issues.
            for new_name in ['  ufs_5861_Resc', 'ufs_5861_Resc  ', '  ufs_5861_Resc  ',
                             '\fufs_5861_Resc', 'ufs_5861_Resc\f', '\fufs_5861_Resc\f',
                             '\nufs_5861_Resc', 'ufs_5861_Resc\n', '\nufs_5861_Resc\n',
                             '\rufs_5861_Resc', 'ufs_5861_Resc\r', '\rufs_5861_Resc\r',
                             '\tufs_5861_Resc', 'ufs_5861_Resc\t', '\tufs_5861_Resc\t',
                             '\vufs_5861_Resc', 'ufs_5861_Resc\v', '\vufs_5861_Resc\v']:
                # Rename the resource.
                self.admin.assert_icommand(['iadmin', 'modresc', resc_name, 'name', new_name], 'STDOUT', [' '], input='yes')

                # Restore the resource's name for the next assertion.
                old_name = 'ufs_5861_Resc'
                self.admin.assert_icommand(['iadmin', 'modresc', new_name, 'name', old_name], 'STDOUT', [' '], input='yes')

        finally:
            self.admin.run_icommand(['iadmin', 'rmresc', resc_name])

class Test_Iadmin_Queries(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Iadmin_Queries, self).setUp()

    def tearDown(self):
        super(Test_Iadmin_Queries, self).tearDown()

    def test_add_and_remove_specific_query(self):
        # setup
        test_file = 'test_specific_query_file'
        lib.make_file(test_file, 500)
        specific_query = 'SELECT data_name FROM R_DATA_MAIN WHERE DATA_NAME = \'{}\';'.format(test_file)
        query_name = 'test_asq'

        # Make specific query and run before it returns any results
        self.admin.assert_icommand(['iadmin', 'asq', specific_query, query_name])
        self.admin.assert_icommand(['iquest', '--sql', 'ls'], 'STDOUT', query_name)
        self.admin.assert_icommand(['iquest', '--sql', query_name], 'STDOUT', 'CAT_NO_ROWS_FOUND: Nothing was found matching your query')

        # Run specific query after results can be found
        self.admin.assert_icommand(['iput', test_file])
        self.admin.assert_icommand(['iquest', '--sql', query_name], 'STDOUT', test_file)

        # Make sure specific query can be removed
        self.admin.assert_icommand(['iadmin', 'rsq', query_name])
        self.admin.assert_icommand_fail(['iquest', '--sql', 'ls'], 'STDOUT', query_name)
        self.admin.assert_icommand(['iquest', '--sql', query_name], 'STDERR', 'CAT_UNKNOWN_SPECIFIC_QUERY')

        # cleanup
        self.admin.assert_icommand(['irm', '-f', test_file])
        os.unlink(test_file)

    def test_add_non_unique_specific_query(self):
        specific_query = 'select * from r_data_main;'
        query_name = 'unique_asq'

        # Add specific query and confirm
        self.admin.assert_icommand(['iadmin', 'asq', specific_query, query_name])
        self.admin.assert_icommand(['iquest', '--sql', 'ls'], 'STDOUT', query_name)

        # Try adding specific query with same name
        _,out,_=self.admin.assert_icommand(['iadmin', 'asq', specific_query, query_name], 'STDERR', 'CAT_INVALID_ARGUMENT')
        self.assertTrue('Alias is not unique' in out)
        self.admin.assert_icommand(['iadmin', 'rsq', query_name])

    def test_unused_r_data_main_columns(self):
        # setup
        test_file = 'test_unused_r_data_main_columns_file'
        column_dict = {'resc_name': 'EMPTY_RESC_NAME',
                       'resc_hier': 'EMPTY_RESC_HIER',
                       'resc_group_name': 'EMPTY_RESC_GROUP_NAME'}
        specific_query = 'SELECT {0} FROM R_DATA_MAIN WHERE data_name = \'{1}\';'.format(', '.join([key for key in column_dict.keys()]), test_file)
        query_name = 'unused_columns_asq'
        lib.make_file(test_file, 500)

        # Run query and verify that output matches expected values for columns
        self.admin.assert_icommand(['iput', test_file])
        self.admin.assert_icommand(['iadmin', 'asq', specific_query, query_name])
        self.admin.assert_icommand(['iquest', '--sql', 'ls'], 'STDOUT', query_name)
        out,_,_=self.admin.run_icommand(['iquest', '--sql', query_name])
        for key in column_dict.keys():
            self.assertTrue(column_dict[key] in out)

        # teardown
        self.admin.assert_icommand(['iadmin', 'rsq', query_name])
        self.admin.assert_icommand(['irm', '-f', test_file])
        os.unlink(test_file)

class Test_Issue3862(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Issue3862, self).setUp()

        output = subprocess.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand(['iadmin', 'modresc', 'repl', 'context', 'retry_attempts=0'])

        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix

        # =-=-=-=-=-=-=-
        # place data into the leaf_a
        self.test_dir = 'issue3862'
        num_files = 400
        if not os.path.isdir(self.test_dir):
            os.mkdir(self.test_dir)
        for i in range(num_files):
            filename = self.test_dir + '/file_' + str(i)
            lib.create_local_testfile(filename)

        with session.make_session_for_existing_admin() as admin_session:

            # add files to leaf_a
            admin_session.run_icommand(['iput', '-r', '-R', 'leaf_a', self.test_dir])

            # now connect leaves to repl
            admin_session.run_icommand(['iadmin', 'addchildtoresc', 'repl', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'addchildtoresc', 'repl', 'leaf_b'])

    def tearDown(self):

        super(Test_Issue3862, self).tearDown()

        with session.make_session_for_existing_admin() as admin_session:

            admin_session.run_icommand(['irm', '-rf', self.test_dir])

            admin_session.run_icommand(['iadmin', 'rmchildfromresc', 'repl', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'rmchildfromresc', 'repl', 'leaf_b'])

            admin_session.run_icommand(['iadmin', 'rmresc', 'leaf_b'])
            admin_session.run_icommand(['iadmin', 'rmresc', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'rmresc', 'repl'])

    # Issue 3862:  test that CAT_STATEMENT_TABLE_FULL is not encountered during parallel rebalance
    def test_rebalance__ticket_3862(self):
        resource_name = "repl"
        def check_and_remove_rebalance_visibility_metadata(attempts):
            print("checking rebalance visibility metadata [{0} attempts remaining]".format(attempts))
            self.assertGreater(attempts, 0, msg="rebalance visibility metadata not in catalog")
            resource_metadata_query = "select META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS " \
                                      f"where RESC_NAME = '{resource_name}'"
            resource_avus = json.loads(self.admin.run_icommand(["iquery", resource_metadata_query])[0].strip())
            # There should only ever be, at most, one AVU on this resource for this test.
            self.assertLess(len(resource_avus), 2)
            for a, v, u in resource_avus:
                if "rebalance_operation" == a:
                    self.admin.assert_icommand(["imeta", "rm", "-R", resource_name, a, v, u])
                    return
            check_and_remove_rebalance_visibility_metadata(attempts - 1)

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand(['ils', '-l', '-r', self.test_dir], 'STDOUT_SINGLELINE', self.test_dir)

        # =-=-=-=-=-=-=-
        # call two separate rebalances, the first in background
        # prior to the fix, the second would generate a CAT_STATEMENT_TABLE_FULL error
        subprocess.Popen(["iadmin", "modresc",  resource_name, "rebalance"])
        # make sure rebalance visibility metadata has been populated before removing it
        check_and_remove_rebalance_visibility_metadata(10); # try 10 times
        # fire parallel rebalance - this will encounter some errors due to self-inflicted race condition
        out,err,_ = self.admin.run_icommand(['iadmin', 'modresc', resource_name, 'rebalance'])
        self.assertNotIn('CAT_STATEMENT_TABLE_FULL', out)
        self.assertNotIn('CAT_STATEMENT_TABLE_FULL', err)

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand(['ils', '-l', '-r', self.test_dir], 'STDOUT_SINGLELINE', self.test_dir)

class Test_Iadmin_modrepl(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Iadmin_modrepl, self).setUp()

    def tearDown(self):
        super(Test_Iadmin_modrepl, self).tearDown()

    # TODO: Add tests with invalid inputs to various columns
    def test_modifying_columns(self):
        dumb_string = 'garbage'
        dumb_int = 16
        cols = {
            #"COLL_ID" : dumb_int,
            "DATA_CREATE_TIME" : dumb_string,
            "DATA_CHECKSUM" : dumb_string,
            "DATA_EXPIRY" : '00000000016',
            #"DATA_ID" : dumb_string,
            "DATA_REPL_STATUS" : dumb_int,
            #"DATA_MAP_ID" : dumb_int,
            "DATA_MODE" : dumb_string,
            #"DATA_NAME" : dumb_string,
            "DATA_OWNER_NAME" : dumb_string,
            "DATA_OWNER_ZONE" : dumb_string,
            "DATA_PATH" : dumb_string,
            #"DATA_REPL_NUM" : dumb_int, # TODO: CAPTURE IN A SPECIAL TEST
            "DATA_SIZE" : dumb_int,
            "DATA_STATUS" : dumb_string,
            "DATA_TYPE_NAME" : 'image',
            "DATA_VERSION" : dumb_string,
            "DATA_MODIFY_TIME" : '00000000016',
            "DATA_ACCESS_TIME" : '00000000027',
            "DATA_COMMENTS" : dumb_string,
            #"DATA_RESC_GROUP_NAME" : dumb_string,
            #"DATA_RESC_HIER" : dumb_string,
            #"DATA_RESC_ID" : dumb_int # TODO: CAPTURE IN A SPECIAL TEST
            #"DATA_RESC_NAME" : dumb_string
        }

        filename = 'test_modifying_columns'
        original_file_path = os.path.join(self.admin.local_session_dir, filename)
        lib.touch(original_file_path)
        try:
            object_path = os.path.join(self.admin.session_collection, filename)
            self.admin.assert_icommand(['iput', original_file_path, object_path])
            repl_num = 0
            for k in cols:
                self.assertIn(str(k), lib.get_replica_full_row(self.admin, object_path, repl_num))
                self.admin.assert_icommand(
                    ['iadmin', 'modrepl', 'logical_path', object_path, 'replica_number', str(repl_num), str(k), str(cols[k])])
                self.assertEqual(str(cols[k]), lib.get_replica_full_row(self.admin, object_path, repl_num)[k])
        finally:
            if os.path.exists(original_file_path):
                os.unlink(original_file_path)

    def test_input_parameters_valid(self):
        filename = 'test_input_parameters_valid'
        original_file_path = os.path.join(self.admin.local_session_dir, filename)
        lib.touch(original_file_path)
        try:
            object_path = os.path.join(self.admin.session_collection, filename)
            self.admin.assert_icommand(['iput', original_file_path, object_path])
            self.admin.assert_icommand(['irepl', '-R', 'TestResc', object_path])
            query = "select DATA_ID where DATA_NAME = '{}'".format(filename)
            data_id,_,_ = self.admin.run_icommand(['iquest', '%s', query])
            data_object_option = {
                'logical_path' : object_path,
                'data_id' : data_id.rstrip()
            }
            replica_option = {
                'replica_number' : '1',
                'resource_hierarchy' : 'TestResc'
            }

            for d in data_object_option:
                for r in replica_option:
                    value = '_'.join([data_object_option[d], replica_option[r]])
                    self.assertIn("DATA_COMMENTS", lib.get_replica_full_row(self.admin, object_path, "1"))
                    self.admin.assert_icommand(
                        ['iadmin', 'modrepl', d, data_object_option[d], r, replica_option[r], 'DATA_COMMENTS', value])
                    self.assertEqual(str(value), lib.get_replica_full_row(self.admin, object_path, "1")["DATA_COMMENTS"])

        finally:
            if os.path.exists(original_file_path):
                os.unlink(original_file_path)

    def test_input_parameters_invalid(self):
        filename = 'test_input_parameters_invalid'
        original_file_path = os.path.join(self.admin.local_session_dir, filename)
        lib.touch(original_file_path)
        try:
            object_path = os.path.join(self.admin.session_collection, filename)
            self.admin.assert_icommand(['iput', original_file_path, object_path])

            initial_full_row = lib.get_replica_full_row(self.admin, object_path, "0")

            garbage_str = 'nopes'
            self.admin.assert_icommand(
                ['iadmin', 'modrepl', 'data_id', garbage_str, 'replica_number', '0', 'DATA_COMMENTS', 'nopes'],
                'STDERR', 'Invalid input [{}] for data_id.'.format(garbage_str))
            self.admin.assert_icommand(
                ['iadmin', 'modrepl', 'logical_path', object_path, 'replica_number', garbage_str, 'DATA_COMMENTS', 'nopes'],
                'STDERR', 'Invalid input [{}] for replica_number.'.format(garbage_str))

            ending_full_row = lib.get_replica_full_row(self.admin, object_path, "0")
            self.assertEqual(initial_full_row, ending_full_row)
        finally:
            if os.path.exists(original_file_path):
                os.unlink(original_file_path)

    def test_modifying_restricted_columns(self):
        dumb_string = 'garbage'
        dumb_int = 99999
        cols = {
            "COLL_ID" : dumb_int,
            #"DATA_CREATE_TIME" : dumb_string,
            #"DATA_CHECKSUM" : dumb_string,
            #"DATA_EXPIRY" : '00000000016',
            "DATA_ID" : dumb_string,
            #"DATA_REPL_STATUS" : dumb_int,
            "DATA_MAP_ID" : dumb_int,
            #"DATA_MODE" : dumb_string,
            "DATA_NAME" : dumb_string,
            #"DATA_OWNER_NAME" : dumb_string,
            #"DATA_OWNER_ZONE" : dumb_string,
            #"DATA_PATH" : dumb_string,
            #"DATA_REPL_NUM" : dumb_int,
            #"DATA_SIZE" : dumb_int,
            #"DATA_STATUS" : dumb_string,
            #"DATA_TYPE_NAME" : 'image',
            #"DATA_VERSION" : dumb_string,
            #"DATA_MODIFY_TIME" : '00000000016',
            #"DATA_COMMENTS" : dumb_string,
            "DATA_RESC_GROUP_NAME" : dumb_string,
            "DATA_RESC_HIER" : dumb_string,
            #"DATA_RESC_ID" : dumb_string
            "DATA_RESC_NAME" : dumb_string
        }

        filename = 'test_modifying_restricted_columns'
        original_file_path = os.path.join(self.admin.local_session_dir, filename)
        lib.touch(original_file_path)

        try:
            object_path = os.path.join(self.admin.session_collection, filename)
            self.admin.assert_icommand(['iput', original_file_path, object_path])
            for k in cols:
                query = "select {0} where COLL_NAME = '{1}' and DATA_NAME = '{2}'".format(
                    k, os.path.dirname(object_path), os.path.basename(object_path))
                self.admin.assert_icommand_fail(['iquest', query], 'STDOUT', str(cols[k]))
                self.admin.assert_icommand(
                    ['iadmin', 'modrepl', 'logical_path', object_path, 'replica_number', '0', str(k), str(cols[k])],
                    'STDERR', 'Invalid attribute specified.')
                self.admin.assert_icommand_fail(['iquest', query], 'STDOUT', str(cols[k]))
        finally:
            if os.path.exists(original_file_path):
                os.unlink(original_file_path)

    @unittest.skip(("Fails if run under an account with read permission on the server config. "
                    "Requires a pure client environment."))
    def test_modrepl_as_rodsuser(self):
        filename = 'test_modrepl_as_rodsuser'
        original_file_path = os.path.join(self.user0.local_session_dir, filename)
        lib.touch(original_file_path)
        try:
            object_path = os.path.join(self.user0.session_collection, filename)
            self.user0.assert_icommand(['iput', original_file_path, object_path])
            self.user0.assert_icommand(
                ['iadmin', 'modrepl', 'logical_path', object_path, 'replica_number', '0', 'DATA_COMMENTS', 'nopes'],
                'STDERR', 'SYS_NO_API_PRIV')
        finally:
            if os.path.exists(original_file_path):
                os.unlink(original_file_path)

    def test_iadmin_modrepl_exits_nonzero_on_failure__issue_7734(self):
        # Bad parameters to modrepl
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'foo', 'bar', 'baz'], 'STDERR');
        self.assertNotEqual(ec, 0)

        # Nonexistent data object
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', '/tempZone/object/does/not/exist', 'replica_number', '0', 'DATA_COMMENTS', 'something'], 'STDERR');
        self.assertNotEqual(ec, 0)

        test_file_name = 'test_file_name_issue_7734'
        self.admin.assert_icommand(['itouch', test_file_name])

        # Nonexistent replica number
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', self.admin.session_collection + '/' + test_file_name, 'replica_number', '123', 'DATA_COMMENTS', 'something'], 'STDERR');
        self.assertNotEqual(ec, 0)

        # Invalid replica number
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', self.admin.session_collection + '/' + test_file_name, 'replica_number', 'abc', 'DATA_COMMENTS', 'something'], 'STDERR');
        self.assertNotEqual(ec, 0)

        # Invalid data object ID
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'data_id', 'abc', 'replica_number', '123456789', 'DATA_COMMENTS', 'something'], 'STDERR');
        self.assertNotEqual(ec, 0)

        # Nonexistent data object ID
        ec, _, _ = self.admin.assert_icommand(['iadmin', 'modrepl', 'data_id', '1234567', 'replica_number', '123456789', 'DATA_COMMENTS', 'something'], 'STDERR');
        self.assertNotEqual(ec, 0)

class test_moduser_user(unittest.TestCase):
    """Test modifying a user."""
    @classmethod
    def setUpClass(self):
        """Set up the test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        self.username = 'moduser_user'
        self.admin.assert_icommand(['iadmin', 'mkuser', self.username, 'rodsuser'])


    @classmethod
    def tearDownClass(self):
        """Tear down the test class."""
        self.admin.assert_icommand(['iadmin', 'rmuser', self.username])
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])


    def test_moduser_type_rodsadmin_rodsuser(self):
        """Test modifying the user's type to and from supported types."""
        # rodsuser -> rodsuser
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsuser'])
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))

        # rodsuser -> rodsadmin
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsadmin'])
        self.assertEqual('rodsadmin', lib.get_user_type(self.admin, self.username))

        # rodsadmin -> rodsadmin
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsadmin'])
        self.assertEqual('rodsadmin', lib.get_user_type(self.admin, self.username))

        # rodsadmin -> rodsuser
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsuser'])
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))

        # rodsuser -> groupadmin
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'groupadmin'])
        self.assertEqual('groupadmin', lib.get_user_type(self.admin, self.username))

        # groupadmin -> groupadmin
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'groupadmin'])
        self.assertEqual('groupadmin', lib.get_user_type(self.admin, self.username))

        # groupadmin -> rodsadmin
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsadmin'])
        self.assertEqual('rodsadmin', lib.get_user_type(self.admin, self.username))

        # rodsadmin -> groupadmin
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'groupadmin'])
        self.assertEqual('groupadmin', lib.get_user_type(self.admin, self.username))

        # groupadmin -> rodsuser
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'rodsuser'])
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))

    def moduser_to_rodsgroup_is_not_allowed_test_impl(self, target_username, user_type):
        """Test modifying the user's type to a group (not allowed)."""
        self.assertEqual(user_type, lib.get_user_type(self.admin, target_username))
        self.admin.assert_icommand(
            ["iadmin", "moduser", target_username, "type", "rodsgroup"], "STDERR", "-169000 SYS_NOT_ALLOWED")
        self.assertEqual(user_type, lib.get_user_type(self.admin, target_username))

    def test_moduser_type_rodsuser_rodsgroup__issue_2978(self):
        """Test modifying the rodsuser's type to a group (not allowed)."""
        self.moduser_to_rodsgroup_is_not_allowed_test_impl(self.username, "rodsuser")

    def test_moduser_type_rodsadmin_rodsgroup__issue_2978(self):
        """Test modifying the rodsadmin's type to a group (not allowed)."""
        self.moduser_to_rodsgroup_is_not_allowed_test_impl(self.admin.username, "rodsadmin")

    def test_moduser_type_groupadmin_rodsgroup__issue_2978(self):
        """Test modifying the groupadmin's type to a group (not allowed)."""
        groupadmin_username = "groupadmin_of_the_year"
        try:
            self.admin.run_icommand(["iadmin", "mkuser", groupadmin_username, "groupadmin"])
            self.moduser_to_rodsgroup_is_not_allowed_test_impl(groupadmin_username, "groupadmin")

        finally:
            self.admin.run_icommand(["iadmin", "rmuser", groupadmin_username])

    def test_moduser_type_invalid_type(self):
        """Test modifying the user's type to something that is not supported."""
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'type', 'invalid_user_type'],
                                   'STDERR_SINGLELINE', 'CAT_INVALID_USER_TYPE')
        self.assertEqual('rodsuser', lib.get_user_type(self.admin, self.username))

    def test_downgrade_of_rodsadmin_user_managing_server_is_not_allowed__issue_6127_8012(self):
        """Test downgrading of service account user's type from rodsadmin to other supported types is not allowed."""
        def get_first_hostname_from_zone_report():
            """Get the hostname of the first "servers" entry of the zone report."""
            zr = json.loads(self.admin.assert_icommand(['izonereport'], 'STDOUT')[1].strip())
            return zr['zones'][0]['servers'][0]['server_config']['host']

        try:
            # Hide the service account user's irods_environment.json file. This helps to prove the
            # server no longer depends a irods_environment.json file.
            new_environment_file_path = paths.default_client_environment_path() + '.issue_8012'
            os.rename(paths.default_client_environment_path(), new_environment_file_path)
            self.assertFalse(os.path.exists(paths.default_client_environment_path()))
            self.assertTrue(os.path.exists(new_environment_file_path))

            # rodsadmin -> rodsuser
            self.assertEqual('rodsadmin', lib.get_user_type(self.admin, 'rods'))
            hostname = get_first_hostname_from_zone_report()
            error_msg = f'Cannot downgrade another rodsadmin [rods] running another server [{hostname}] in this zone.'
            out, err, ec = self.admin.run_icommand(['iadmin', 'moduser', 'rods', 'type', 'rodsuser'])
            self.assertNotEqual(ec, 0)
            self.assertIn(error_msg, out)
            self.assertIn('SYS_NOT_ALLOWED', err)
            self.assertEqual('rodsadmin', lib.get_user_type(self.admin, 'rods'))

            # rodsadmin -> groupadmin
            out, err, ec = self.admin.run_icommand(['iadmin', 'moduser', 'rods', 'type', 'groupadmin'])
            self.assertNotEqual(ec, 0)
            self.assertIn(error_msg, out)
            self.assertIn('SYS_NOT_ALLOWED', err)
            self.assertEqual('rodsadmin', lib.get_user_type(self.admin, 'rods'))

        finally:
            os.rename(new_environment_file_path, paths.default_client_environment_path())
            self.assertTrue(os.path.exists(paths.default_client_environment_path()))
            self.assertFalse(os.path.exists(new_environment_file_path))

    def test_moduser_zone(self):
        """Test modifying the user's zone (not supported)."""
        # Make sure local users cannot have the zone changed
        self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'zone', 'nopes'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.username))

        # Make sure remote users cannot have the zone changed
        remote_zone = 'somezone'
        user_and_zone = '#'.join([self.username, remote_zone])

        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', remote_zone, 'remote', 'localhost:1247'])
            self.admin.assert_icommand(['iadmin', 'mkuser', user_and_zone, 'rodsuser'])

            self.assertEqual(remote_zone, lib.get_user_zone(self.admin, self.username, remote_zone))
            self.admin.assert_icommand(['iadmin', 'moduser', user_and_zone, 'zone', 'nopes'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.assertEqual(remote_zone, lib.get_user_zone(self.admin, self.username, remote_zone))

        finally:
            self.admin.assert_icommand(['iadmin', 'rmuser', user_and_zone])
            self.admin.assert_icommand(['iadmin', 'rmzone', remote_zone])


    def test_moduser_comment(self):
        """Test modifying the user's comment field."""
        comment = 'this is a comment'
        self.assertEqual('', lib.get_user_comment(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'comment', comment])
        self.assertEqual(comment, lib.get_user_comment(self.admin, self.username))

        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'comment', ''])
        self.assertEqual('', lib.get_user_comment(self.admin, self.username))

    def test_moduser_info(self):
        """Test modifying the user's info field."""
        info = 'this is the info field'
        self.assertEqual('', lib.get_user_info(self.admin, self.username))
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'info', info])
        self.assertEqual(info, lib.get_user_info(self.admin, self.username))

        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'info', ''])
        self.assertEqual('', lib.get_user_info(self.admin, self.username))

    def test_moduser_password(self):
        """Test modifying the user's password."""
        host = socket.gethostname()

        # Make sure no password is set
        pw1 = 'abc'
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw1, host, self.admin.zone_name):
                pass

        # Add password
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw1])
        with session.make_session_for_existing_user(self.username, pw1, host, self.admin.zone_name):
            pass

        # Change password
        pw2 = '1234'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw2])
        with session.make_session_for_existing_user(self.username, pw2, host, self.admin.zone_name):
            pass

        # Make sure old password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw1, host, self.admin.zone_name):
                pass

        # Change password to 41 bytes (should succeed)
        pw3 = '12345678901234567890123456789012345678901'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw3])
        with session.make_session_for_existing_user(self.username, pw3, host, self.admin.zone_name):
            pass

        # Make sure old password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw2, host, self.admin.zone_name):
                pass

        # Change password to 42 bytes (should fail)
        pw4 = '123456789012345678901234567890123456789012'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw3])

        # Make sure new password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw4, host, self.admin.zone_name):
                pass

        # Change password to 43 bytes (should fail)
        pw4 = '1234567890123456789012345678901234567890123'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw4], 'STDERR', 'PASSWORD_EXCEEDS_MAX_SIZE')

        # Make sure new password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw4, host, self.admin.zone_name):
                pass

        # Change password to 45 bytes (should fail)
        pw4 = '123456789012345678901234567890123456789012345'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw4], 'STDERR', 'PASSWORD_EXCEEDS_MAX_SIZE')

        # Make sure new password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw4, host, self.admin.zone_name):
                pass

        # Change password to 46 bytes (should fail)
        pw4 = '1234567890123456789012345678901234567890123456'
        self.admin.assert_icommand(['iadmin', 'moduser', self.username, 'password', pw4], 'STDERR', 'PASSWORD_EXCEEDS_MAX_SIZE')

        # Make sure new password does not work
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.username, pw4, host, self.admin.zone_name):
                pass

        # Make sure last successful password setting works
        with session.make_session_for_existing_user(self.username, pw3, host, self.admin.zone_name) as user_session:
            user_session.assert_icommand(['ils'], 'STDOUT_SINGLELINE', '/tempZone/home/%s' % self.username)

class test_moduser_group(unittest.TestCase):
    """Test modifying a group."""
    @classmethod
    def setUpClass(self):
        """Set up the test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        self.group = 'moduser_group'
        self.admin.assert_icommand(['iadmin', 'mkgroup', self.group])

    @classmethod
    def tearDownClass(self):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmgroup', self.group])
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def moduser_type_on_rodsgroup_is_not_allowed_test_impl(self, new_type):
        """Test modifying the group's type (not allowed)."""
        self.assertEqual('rodsgroup', lib.get_user_type(self.admin, self.group))
        self.admin.assert_icommand(
            ['iadmin', 'moduser', self.group, 'type', new_type], 'STDERR', '-169000 SYS_NOT_ALLOWED')
        self.assertEqual('rodsgroup', lib.get_user_type(self.admin, self.group))

    def test_moduser_type_invalid_user_type_is_not_allowed__issue_2978(self):
        self.moduser_type_on_rodsgroup_is_not_allowed_test_impl("invalid_user_type")

    def test_moduser_type_rodsadmin_is_not_allowed__issue_2978(self):
        self.moduser_type_on_rodsgroup_is_not_allowed_test_impl("rodsadmin")

    def test_moduser_type_rodsuser_is_not_allowed__issue_2978(self):
        self.moduser_type_on_rodsgroup_is_not_allowed_test_impl("rodsuser")

    def test_moduser_type_rodsgroup_is_not_allowed__issue_2978(self):
        self.moduser_type_on_rodsgroup_is_not_allowed_test_impl("rodsgroup")

    def test_moduser_type_rodsgroup_is_not_allowed__issue_2978(self):
        self.moduser_type_on_rodsgroup_is_not_allowed_test_impl("groupadmin")

    def test_moduser_zone_for_rodsgroup_is_not_allowed__issue_2978(self):
        """Test modifying the user's zone (not allowed)."""
        self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.group))
        self.admin.assert_icommand(
            ['iadmin', 'moduser', self.group, 'zone', 'nopes'], 'STDERR', '-169000 SYS_NOT_ALLOWED')
        self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.group))

    def test_moduser_comment_for_rodsgroup_is_not_allowed__issue_2978(self):
        """Test modifying the group's comment field (not allowed)."""
        comment = 'this is a comment'
        self.assertEqual('', lib.get_user_comment(self.admin, self.group))
        self.admin.assert_icommand(
            ['iadmin', 'moduser', self.group, 'comment', comment], 'STDERR', '-169000 SYS_NOT_ALLOWED')
        self.assertEqual('', lib.get_user_comment(self.admin, self.group))

    def test_moduser_info_for_rodsgroup_is_not_allowed__issue_2978(self):
        """Test modifying the group's info field (not allowed)."""
        info = 'this is the info field'
        self.assertEqual('', lib.get_user_info(self.admin, self.group))
        self.admin.assert_icommand(['iadmin', 'moduser', self.group, 'info', info], 'STDERR', '-169000 SYS_NOT_ALLOWED')
        self.assertEqual('', lib.get_user_info(self.admin, self.group))

    def test_moduser_password_for_rodsgroup_is_not_allowed__issue_2978(self):
        """Test modifying the group's password (not allowed)."""
        host = socket.gethostname()

        # Make sure no password is set
        pw = 'abc'
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.group, pw, host, self.admin.zone_name) as s:
                pass

        # Add password (and fail)
        self.admin.assert_icommand(
            ['iadmin', 'moduser', self.group, 'password', pw], 'STDERR', '-169000 SYS_NOT_ALLOWED')

        # Make sure it didn't stick
        with self.assertRaises(Exception):
            with session.make_session_for_existing_user(self.group, pw, host, self.admin.zone_name) as s:
                pass


class test_making_groups(unittest.TestCase):
    """Test making a group."""

    @classmethod
    def setUpClass(self):
        """Set up the test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        self.group = 'test_making_groups_group'

    @classmethod
    def tearDownClass(self):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def test_mkuser_is_not_allowed_with_groups__issue_2978(self):
        """Test that mkuser with type rodsgroup is not allowed."""
        self.admin.assert_icommand(["iadmin", "mkuser", self.group, "rodsgroup"], "STDERR", "-169000 SYS_NOT_ALLOWED")
        self.assertIn('CAT_NO_ROWS_FOUND', lib.get_user_type(self.admin, self.group))

    def test_mkgroup_with_no_zone(self):
        """Test mkgroup with no zone name provided."""
        try:
            self.admin.assert_icommand(['iadmin', 'mkgroup', self.group])
            self.assertEqual('rodsgroup', lib.get_user_type(self.admin, self.group))
            self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.group))

            # From issue #2978: Ensure that the group does not add itself as a member on creation.
            self.admin.assert_icommand(
                ["iadmin", "lg", self.group], "STDOUT", f"Members of group {self.group}:\nNo rows found")

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])

    def test_mkgroup_with_local_zone(self):
        """Test mkgroup with local zone name provided."""
        try:
            self.admin.assert_icommand(['iadmin', 'mkgroup', '#'.join([self.group, self.admin.zone_name])])
            self.assertEqual('rodsgroup', lib.get_user_type(self.admin, self.group))
            self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.admin, self.group))

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])

    def test_mkgroup_with_remote_zone(self):
        """Test mkgroup with remote zone name provided."""
        remote_zone = 'somezone'
        try:
            group_with_zone = '#'.join([self.group, remote_zone])

            # remote zones for non-existent zones are not allowed
            self.admin.assert_icommand(['iadmin', 'mkgroup', group_with_zone],
                                       'STDERR', 'SYS_NOT_ALLOWED')

            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_user_type(self.admin, self.group))

            # remote zones for existing remote zones are not allowed
            self.admin.assert_icommand(['iadmin', 'mkzone', remote_zone, 'remote', 'localhost:1247'])
            self.admin.assert_icommand(['iadmin', 'mkgroup', group_with_zone],
                                       'STDERR', 'SYS_NOT_ALLOWED')

            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_user_type(self.admin, self.group))

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])
            self.admin.run_icommand(['iadmin', 'rmzone', remote_zone])


class test_mkzone_conn_str_validation(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """Set up the test class."""
        cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())

    @classmethod
    def tearDownClass(cls):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            cls.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

    def test_good_localhost(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'goodzone', 'remote', 'localhost:1247'], 'EMPTY')
            self.admin.assert_icommand(['iadmin', 'lz', 'goodzone'],
                                       'STDOUT_MULTILINE', r'^zone_conn_string:\s+localhost:1247$', use_regex=True)
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'goodzone'])

    def test_good_blank(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'goodzone', 'remote', ''], 'EMPTY')
            self.admin.assert_icommand(['iadmin', 'lz', 'goodzone'],
                                       'STDOUT_MULTILINE', r'^zone_conn_string:\s*$', use_regex=True)
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'goodzone'])

    def test_bad_noport(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_0port(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:0'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_nohost(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', ':1234'],
                                       'STDERR', 'CAT_HOSTNAME_INVALID')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_negport(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:-1234'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_bigport(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:1234567890'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_NaNportNaN(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:asdf1234asdf'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_NaNport(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:asdf1234'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

    def test_bad_portNaN(self):
        try:
            self.admin.assert_icommand(['iadmin', 'mkzone', 'badzone', 'remote', 'hostname:1234asdf'],
                                       'STDERR', 'CAT_INVALID_ARGUMENT')
            self.admin.assert_icommand(['iadmin', 'lz', 'badzone'],
                                       'STDOUT', 'No rows found')
        finally:
            self.admin.run_icommand(['iadmin', 'rmzone', 'badzone'])

class test_modzone_conn_str_validation(unittest.TestCase):
    localhost_zone = 'goodzone_localhost'
    localhost_connstr = 'localhost:1247'
    localhost_test_re = r'^zone_conn_string:\s+' + localhost_connstr + r'$'
    blank_zone = 'goodzone_blank'
    blank_test_re = r'^zone_conn_string:\s*$'

    @classmethod
    def setUpClass(cls):
        """Set up the test class."""
        cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())

    @classmethod
    def tearDownClass(cls):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            cls.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

    def setUp(self):
        self.admin.assert_icommand(['iadmin', 'mkzone', self.localhost_zone, 'remote', self.localhost_connstr], 'EMPTY')
        self.admin.assert_icommand(['iadmin', 'mkzone', self.blank_zone, 'remote', ''], 'EMPTY')

    def tearDown(self):
        self.admin.run_icommand(['iadmin', 'rmzone', self.localhost_zone])
        self.admin.run_icommand(['iadmin', 'rmzone', self.blank_zone])

    def test_good_localhost(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', self.localhost_connstr], 'EMPTY')
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)

    def test_bad_noport(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_0port(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:0'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:0'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_nohost(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', ':1234'],
                                   'STDERR', 'CAT_HOSTNAME_INVALID')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', ':1234'],
                                   'STDERR', 'CAT_HOSTNAME_INVALID')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_negport(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:-1234'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:-1234'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_bigport(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:1234567890'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:1234567890'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_NaNportNaN(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:asdf1234asdf'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:asdf1234asdf'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_NaNport(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:asdf1234'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:asdf1234'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)

    def test_bad_portNaN(self):
        self.admin.assert_icommand(['iadmin', 'modzone', self.localhost_zone, 'conn', 'hostname:1234asdf'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'modzone', self.blank_zone, 'conn', 'hostname:1234asdf'],
                                   'STDERR', 'CAT_INVALID_ARGUMENT')
        self.admin.assert_icommand(['iadmin', 'lz', self.localhost_zone],
                                   'STDOUT_MULTILINE', self.localhost_test_re, use_regex=True)
        self.admin.assert_icommand(['iadmin', 'lz', self.blank_zone],
                                   'STDOUT_MULTILINE', self.blank_test_re, use_regex=True)
