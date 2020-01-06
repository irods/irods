from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import commands
import contextlib
import copy
import getpass
import inspect
import json
import os
import shutil
import socket
import stat
import subprocess
import time
import tempfile
import ustrings

from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from .. import paths
from .. import test
from . import session
from . import settings
from .. import lib
from . import resource_suite
from .rule_texts_for_tests import rule_texts

def write_host_access_control(filename, username, group, address, mask):
    add_ent = {
        'user': username,
        'group': group,
        'address': address,
        'mask': mask,
    }

    address_entries = [add_ent]
    hac = {'access_entries': address_entries}

    with open(filename, 'wt') as f:
        json.dump(hac, f, sort_keys=True, indent=4, ensure_ascii=False)


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

    def test_ibun__issue_3571(self):
        test_file = "ibun_test_file"
        lib.make_file(test_file, 1000)

        tar_path = self.admin.session_collection + '/somefile.tar'
        dir_path = self.admin.session_collection + '/somedir'

        self.admin.assert_icommand("imkdir " + dir_path)
        for i in range(257):
            self.admin.assert_icommand("iput %s %s/foo%d" % (test_file, dir_path, i))

        self.admin.assert_icommand("ibun -cD tar " + tar_path + " " + dir_path)

        self.admin.assert_icommand("irm -rf " + dir_path)
        self.admin.assert_icommand("irm -rf " + tar_path)

    def test_tokens(self):
        self.admin.assert_icommand(['iadmin', 'at', 'user_type', 'rodstest', self.admin.username])
        self.admin.assert_icommand(['iadmin', 'lt', 'user_type'], 'STDOUT_SINGLELINE', 'rodstest')
        self.admin.assert_icommand(['iadmin', 'lt', 'user_type', 'rodstest'], 'STDOUT_SINGLELINE', 'token_name: rodstest')
        self.admin.assert_icommand(['iadmin', 'rt', 'user_type', 'rodstest'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_moduser(self):
        username = 'moduser_user'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        try :
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'type', 'invalid_user_type'], 'STDERR_SINGLELINE', 'CAT_INVALID_USER_TYPE')
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'type', 'rodsadmin'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'type', 'rodsuser'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'zone', 'tempZone'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'comment', 'this is a comment'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'comment', ''])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'info', 'this is the info field'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'info', ''])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'password', 'abc'])
            self.admin.assert_icommand(['iadmin', 'moduser', username, 'password', '1234'])
        finally :
            self.admin.assert_icommand(['iadmin', 'rmuser', username])

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

    def test_quota_actions_3509(self):
        self.admin.assert_icommand("iadmin suq " + self.user0.username + " " + self.testresc + " 50")
        self.admin.assert_icommand("iadmin suq " + self.user1.username + " " + self.testresc + " 60")
        self.admin.assert_icommand("iadmin cu")

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
        # duplicate parent
        self.admin.assert_icommand("iadmin addchildtoresc pt the_child")
        self.admin.assert_icommand("iadmin addchildtoresc pt the_child", 'STDERR_SINGLELINE', "CHILD_HAS_PARENT")
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
        self.admin.assert_icommand("iput -R %s -r %s" % ("pt", dir1), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        self.admin.assert_icommand("iput -R %s -r %s" % ("replB", dir2), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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

    # =-=-=-=-=-=-=-
    # REBALANCE
    def test_rebalance_for_invalid_data__ticket_3147(self):
        output = commands.getstatusoutput("hostname")
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
        self.admin.assert_icommand("itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8", 'STDOUT_SINGLELINE', "files trimmed")
        self.admin.assert_icommand("itrim -N1 -n 1 foo1 foo3 foo4 foo9", 'STDOUT_SINGLELINE', "files trimmed")
        self.admin.assert_icommand("itrim -N1 -n 2 foo2 foo4 foo5", 'STDOUT_SINGLELINE', "files trimmed")

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
        self.admin.assert_icommand("itrim -N1 -n 0 foo0 foo3 foo5 foo6 foo7 foo8", 'STDOUT_SINGLELINE', "files trimmed")
        self.admin.assert_icommand("itrim -N1 -n 1 foo1 foo3 foo4 foo9", 'STDOUT_SINGLELINE', "files trimmed")
        self.admin.assert_icommand("itrim -N1 -n 2 foo2 foo4 foo5", 'STDOUT_SINGLELINE', "files trimmed")

        # =-=-=-=-=-=-=-
        # dirty up a foo10 repl to ensure that code path is tested also
        self.admin.assert_icommand("iadmin modresc unixA2 status down")
        test1_path = os.path.join(self.admin.local_session_dir, 'test1')
        lib.make_file(test1_path, 1500, 'arbitrary')
        self.admin.assert_icommand("iput -fR pt %s foo10" % (test1_path))
        self.admin.assert_icommand("iadmin modresc unixA2 status up")

        # =-=-=-=-=-=-=-
        # visualize our pruning
        self.admin.assert_icommand("ils -AL", 'STDOUT_SINGLELINE', "foo")

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
        self.admin.assert_icommand(["imeta","add","-R","demoResc","rebalance_operation",hostname+":1234","20181004T123456Z"])
        # rebalance, and check for avu removal
        self.admin.assert_icommand(["iadmin","modresc",self.testresc,"rebalance"])
        self.admin.assert_icommand(["imeta","ls","-R",self.testresc], 'STDOUT_SINGLELINE', "None")
        # rebalance already running
        self.admin.assert_icommand(["imeta","add","-R",self.testresc,"rebalance_operation",hostname+":1234","20181004T123456Z"])
        self.admin.assert_icommand(["iadmin","modresc",self.testresc,"rebalance"], 'STDERR_SINGLELINE', "REBALANCE_ALREADY_ACTIVE_ON_RESOURCE")
        # clean up
        self.admin.assert_icommand(["imeta","rmw","-R",self.testresc,"rebalance_operation","%","%"])
        self.admin.assert_icommand(["imeta","rmw","-R","demoResc","rebalance_operation","%","%"])

    def test_iexecmd(self):
        test_file = "iput_test_file"
        lib.make_file(test_file, 10)
        self.admin.assert_icommand("iput %s foo" % test_file)
        self.admin.assert_icommand(['iexecmd', '-p', self.admin.session_collection + '/foo', 'hello'], 'STDOUT_SINGLELINE', "Hello world  from irods")
        self.admin.assert_icommand("irm -f foo")

    def test_ibun(self):
        test_file = "ibun_test_file"
        lib.make_file(test_file, 1000)
        cmd = "tar cf somefile.tar " + test_file
        lib.execute_command(['tar', 'cf', 'somefile.tar', test_file])

        tar_path = self.admin.session_collection + '/somefile.tar'
        dir_path = self.admin.session_collection + '/somedir'

        self.admin.assert_icommand("iput somefile.tar")
        self.admin.assert_icommand("imkdir " + dir_path)
        self.admin.assert_icommand("iput %s %s/foo0" % (test_file, dir_path))
        self.admin.assert_icommand("iput %s %s/foo1" % (test_file, dir_path))

        self.admin.assert_icommand("ibun -cD tar " + tar_path + " " +
                                   dir_path, 'STDERR_SINGLELINE', "OVERWRITE_WITHOUT_FORCE_FLAG")

        self.admin.assert_icommand("irm -rf " + dir_path)
        self.admin.assert_icommand("irm -rf " + tar_path)

    @unittest.skip( "configuration requires sudo to create the environment")
    def test_rebalance_for_repl_node_with_different_users_with_write_failure__issue_3674(self):
        #  sudo truncate -s 3M /tmp/irods/badfs_file
        #  sudo mkfs -t ext3 /tmp/irods/badfs_file
        #  sudo mount -t ext3 /tmp/irods/badfs_file /tmp/irods/bad_fs/
        #  sudo chmod 777 /tmp/irods/bad_fs/


        output = commands.getstatusoutput("hostname")
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
        self.admin.assert_icommand("itrim -N1 -n 0 foo1", 'STDOUT_SINGLELINE', "files trimmed")
        self.user0.assert_icommand("itrim -N1 -n 0 bar0", 'STDOUT_SINGLELINE', "files trimmed")

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

        # =-=-=-=-=-=-=-
        # TEARDOWN
        for i in range(num_children):
            self.admin.assert_icommand("irm -f foo%d" % i)
            self.user0.assert_icommand("irm -f bar%d" % i)

        self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_b")
        self.admin.assert_icommand("iadmin rmchildfromresc repl leaf_a")
        self.admin.assert_icommand("iadmin rmresc leaf_b")
        self.admin.assert_icommand("iadmin rmresc leaf_a")
        self.admin.assert_icommand("iadmin rmresc repl")

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
            hosts_config = os.path.join(IrodsConfig().config_directory, 'hosts_config.json')

        orig_file = hosts_config + '.orig'
        os.system('cp %s %s' % (hosts_config, orig_file))
        with open(hosts_config, 'wt') as f:
            json.dump(
                cfg,
                f,
                sort_keys=True,
                indent=4,
                ensure_ascii=False)

        hostuser = getpass.getuser()
        temp_file = 'file_to_test_hosts_config'
        lib.make_file(temp_file, 10)
        self.admin.assert_icommand("iadmin mkresc jimboResc unixfilesystem jimbo:/tmp/%s/jimboResc" %
                                   hostuser, 'STDOUT_SINGLELINE', "jimbo")
        self.admin.assert_icommand("iput -R jimboResc " + temp_file + " jimbofile")
        self.admin.assert_icommand("irm -f jimbofile")
        self.admin.assert_icommand("iadmin rmresc jimboResc")

        os.system('mv %s %s' % (orig_file, hosts_config))

    def test_host_access_control(self):
        my_ip = socket.gethostbyname(socket.gethostname())

        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # restart the server to reread the new core.re
            IrodsController().restart()

            host_access_control = os.path.join(paths.config_directory(), 'host_access_control_config.json')

            with lib.file_backed_up(host_access_control):
                write_host_access_control(host_access_control, 'nope', 'nope', '', '')
                self.admin.assert_icommand("ils", 'STDERR_SINGLELINE', "SYS_AGENT_INIT_ERR")
                write_host_access_control(host_access_control, 'all', 'all', my_ip, '255.255.255.255')
                self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', self.admin.zone_name)

    def test_issue_2420(self):

        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # restart the server to reread the new core.re
            IrodsController().restart()

            self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', self.admin.zone_name)

            # look for the error "unable to read session variable $userNameClient."
            out, _, _ = lib.execute_command_permissive(
                ['grep', 'unable to read session variable $userNameClient.', IrodsConfig().server_log_path])

        # check the results for the error
        assert(-1 == out.find("userNameClient"))

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
            IrodsController().restart()

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
        IrodsController().restart()

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
        hostname = lib.get_hostname()
        testresc1 = 'TestResc'
        with temporary_core_file() as core:
            time.sleep(2)  # remove once file hash fix is commited #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(2)  # remove once file hash fix is commited #2279

            trigger_file = 'file_to_trigger_acSetRescSchemeForCreate'
            lib.make_file(trigger_file, 10)
            initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_log_path)
            self.user0.assert_icommand(['iput', trigger_file])
            self.user0.assert_icommand(['ils', '-L', trigger_file], 'STDOUT_SINGLELINE', testresc1)

            os.unlink(trigger_file)
            self.user0.assert_icommand('irm -f ' + trigger_file)
        time.sleep(2)  # remove once file hash fix is commited #2279

    def test_storageadmin_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes storageadmin", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_domainadmin_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes domainadmin", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_rodscurators_role(self):
        self.admin.assert_icommand("iadmin mkuser nopes rodscurators", 'STDERR_SINGLELINE', "CAT_INVALID_USER_TYPE")

    def test_izonereport_key_sanitization(self):
        self.admin.assert_icommand("izonereport | grep key | grep -v XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                   'STDOUT_SINGLELINE', '"irods_encryption_key_size": 32,', use_unsafe_shell=True)

    def test_impostor_resource_debug_logging(self):
        irods_config = IrodsConfig()
        server_config_filename = irods_config.server_config_path
        with lib.file_backed_up(server_config_filename):
            server_config_update = {
                'environment_variables': {
                    'spLogLevel': '11'
                }
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)
            IrodsController().restart()

            name_of_bogus_resource = 'name_of_bogus_resource'
            name_of_missing_plugin = 'name_of_missing_plugin'
            self.admin.assert_icommand(['iadmin', 'mkresc', name_of_bogus_resource, name_of_missing_plugin], 'STDOUT_SINGLELINE', name_of_missing_plugin)

            initial_size_of_server_log = lib.get_file_size_by_path(irods_config.server_log_path)
            self.admin.assert_icommand(['ils'], 'STDOUT_SINGLELINE', self.admin.zone_name) # creates an agent, which instantiates the resource hierarchy

            debug_message = 'loading impostor resource for [{0}] of type [{1}] with context [] and load_plugin message'.format(name_of_bogus_resource, name_of_missing_plugin)
            debug_message_count = lib.count_occurrences_of_string_in_log(irods_config.server_log_path, debug_message, start_index=initial_size_of_server_log)
            self.assertTrue(1 == debug_message_count, msg='Found {} messages in log but expected 1'.format(debug_message_count))

        self.admin.assert_icommand(['iadmin', 'rmresc', name_of_bogus_resource])
        IrodsController().restart()

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

        assert 0 < lib.count_occurrences_of_string_in_log(irods_config.server_log_path, 'dlerror', start_index=initial_size_of_server_log)
        assert 0 < lib.count_occurrences_of_string_in_log(irods_config.server_log_path, 'PLUGIN_ERROR', start_index=initial_size_of_server_log)

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
        self.admin.assert_icommand(['iadmin', 'lg'], 'STDOUT_SINGLELINE', 'rodsadmin')
        self.admin.assert_icommand(['iadmin', 'lg', 'rodsadmin'], 'STDOUT_SINGLELINE')
        self.admin.assert_icommand(['iadmin', 'lgd', 'rodsadmin'], 'STDOUT_SINGLELINE', 'user_name: rodsadmin')
        self.admin.assert_icommand(['iadmin', 'lrg'], 'STDERR_SINGLELINE', 'Resource groups are deprecated.')

    def test_group_membership(self):
        self.admin.assert_icommand(["iadmin", "rmgroup", "g1"], 'STDERR_SINGLELINE', 'CAT_INVALID_USER')
        self.admin.assert_icommand(["iadmin", "mkgroup", "g1"])
        self.admin.assert_icommand(["iadmin", "atg", "g1", self.user0.username])
        self.admin.assert_icommand(["iadmin", "atg", "g1a", self.user0.username], 'STDERR_SINGLELINE', 'CAT_INVALID_GROUP')
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "atg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user0.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "rfg", "g1", self.user1.username])
        self.admin.assert_icommand(["iadmin", "rmgroup", "g1"])

    def test_idempotent_aua__issue_3104(self):
        username = 'issue_3104_user'
        authentication_name = '3104_user@TEST.AUTHENTICATION'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name])
        out, _, _ = self.admin.run_icommand(['iadmin', 'lua', username])
        self.assertEqual(len(out.splitlines()), 1, 'iadmin lua returned more than one line')

    def test_aua_multiple_distinguished_name__issue_3620(self):
        username = 'issue_3620_user'
        authentication_name_1 = '3620_user_1@TEST.AUTHENTICATION'
        authentication_name_2 = '3620_user_2@TEST.AUTHENTICATION'
        self.admin.assert_icommand(['iadmin', 'mkuser', username, 'rodsuser'])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_2])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_2])
        self.admin.assert_icommand(['iadmin', 'aua', username, authentication_name_1])
        _, out, _ = self.admin.assert_icommand(['iadmin', 'lua', username], 'STDOUT_MULTILINE', [username + ' ' + authentication_name_1, username + ' ' + authentication_name_2])
        self.assertEqual(len(out.splitlines()), 2, 'iadmin lua did not return exactly two lines')


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
        self.admin.assert_icommand(['iquest', '--sql', query_name], 'STDOUT', 'No rows found')

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

        output = commands.getstatusoutput("hostname")
        hostname = output[1]

        # =-=-=-=-=-=-=-
        # STANDUP
        self.admin.assert_icommand("iadmin mkresc repl replication", 'STDOUT_SINGLELINE', "Creating")

        self.admin.assert_icommand("iadmin mkresc leaf_a unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_a", 'STDOUT_SINGLELINE', "Creating")  # unix
        self.admin.assert_icommand("iadmin mkresc leaf_b unixfilesystem " + hostname +
                                   ":/tmp/irods/pydevtest_leaf_b", 'STDOUT_SINGLELINE', "Creating")  # unix

        # =-=-=-=-=-=-=-
        # place data into the leaf_a
        test_dir = 'issue3862'
        num_files = 400
        if not os.path.isdir(test_dir):
            os.mkdir(test_dir)
        for i in range(num_files):
            filename = test_dir + '/file_' + str(i)
            lib.create_local_testfile(filename)

        with session.make_session_for_existing_admin() as admin_session:

            # add files to leaf_a
            admin_session.run_icommand(['iput', '-r', '-R', 'leaf_a', test_dir])

            # now connect leaves to repl
            admin_session.run_icommand(['iadmin', 'addchildtoresc', 'repl', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'addchildtoresc', 'repl', 'leaf_b'])

    def tearDown(self):

        super(Test_Issue3862, self).tearDown()

        test_dir = 'issue3862'

        with session.make_session_for_existing_admin() as admin_session:

            admin_session.run_icommand(['irm', '-rf', test_dir])

            admin_session.run_icommand(['iadmin', 'rmchildfromresc', 'repl', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'rmchildfromresc', 'repl', 'leaf_b'])

            admin_session.run_icommand(['iadmin', 'rmresc', 'leaf_b'])
            admin_session.run_icommand(['iadmin', 'rmresc', 'leaf_a'])
            admin_session.run_icommand(['iadmin', 'rmresc', 'repl'])

    # Issue 3862:  test that CAT_STATEMENT_TABLE_FULL is not encountered during parallel rebalance
    def test_rebalance__ticket_3862(self):

        def check_and_remove_rebalance_visibility_metadata(attempts):
            print("checking rebalance visibility metadata [{0} attempts remaining]".format(attempts))
            self.assertTrue(attempts != 0, msg="rebalance visibility metadata not in catalog")
            out,_,_ = self.admin.run_icommand(['imeta','ls','-R','repl'])
            if "rebalance_operation" not in out:
                check_and_remove_rebalance_visibility_metadata(attempts - 1)
            else:
                self.admin.assert_icommand("imeta rmw -R repl rebalance_operation % %") # b/c #3683

        # =-=-=-=-=-=-=-
        # call two separate rebalances, the first in background
        # prior to the fix, the second would generate a CAT_STATEMENT_TABLE_FULL error
        subprocess.Popen(["iadmin", "modresc",  "repl", "rebalance"])
        # make sure rebalance visibility metadata has been populated before removing it
        check_and_remove_rebalance_visibility_metadata(10); # try 10 times
        # fire parallel rebalance
        self.admin.assert_icommand("iadmin modresc repl rebalance")
