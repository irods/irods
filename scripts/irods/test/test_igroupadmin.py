from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import contextlib
import copy
import getpass
import json
import os
import shutil
import socket
import stat
import subprocess
import time
import tempfile

from ..configuration import IrodsConfig
from ..controller import IrodsController
from .. import test
from . import session
from . import settings
from .. import lib
from . import resource_suite

class Test_Igroupadmin(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Igroupadmin, self).setUp()

    def tearDown(self):
        super(Test_Igroupadmin, self).tearDown()

    def test_atg(self):
        self.admin.assert_icommand(['iadmin', 'moduser', self.user_sessions[0].username, 'type', 'groupadmin'])
        try :
            self.user_sessions[0].assert_icommand(['igroupadmin', 'mkgroup', 'test_group'])
            try :
                self.user_sessions[0].assert_icommand(['igroupadmin', 'atg', 'test_group', self.user_sessions[0].username])
                self.user_sessions[0].assert_icommand(['igroupadmin', 'atg', 'test_group', self.user_sessions[1].username])
                self.user_sessions[0].assert_icommand(['igroupadmin', 'lg', 'test_group'], 'STDOUT', self.user_sessions[0].username)
                self.user_sessions[0].assert_icommand(['igroupadmin', 'lg', 'test_group'], 'STDOUT', self.user_sessions[1].username)
                self.admin.assert_icommand(['iadmin', 'lg', 'test_group'], 'STDOUT', self.user_sessions[0].username)
                self.admin.assert_icommand(['iadmin', 'lg', 'test_group'], 'STDOUT', self.user_sessions[1].username)
                self.user_sessions[0].assert_icommand(['igroupadmin', 'rfg', 'test_group', self.user_sessions[1].username])
            finally :
                self.admin.assert_icommand(['iadmin', 'rmgroup', 'test_group'])
        finally :
            self.admin.assert_icommand(['iadmin', 'moduser', self.user_sessions[0].username, 'type', 'rodsuser'])


class test_mkuser_group(unittest.TestCase):
    """Test making a group."""
    @classmethod
    def setUpClass(self):
        """Set up the test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        self.groupadmin = session.mkuser_and_return_session('groupadmin', 'agroupadmin', 'rods', lib.get_hostname())
        self.group = 'test_making_groups_group'


    @classmethod
    def tearDownClass(self):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            self.groupadmin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.groupadmin.username])

            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def test_create_user_with_password_in_igroupadmin__issue_6887(self):
        username = 'alice'
        # Passwords of length 42 (the current maximum) cannot be used as input for iinit
        # until resolution https://github.com/irods/irods/issues/6913
        password_length = 41
        try:
            # Create a user and assign a password of the given length.
            passwd = (username[0]+'pass_6887_the_quick_brown_fox_jumps_over_the_lazy_dog')[:password_length]
            self.groupadmin.assert_icommand(['igroupadmin', 'mkuser', username, passwd])

            # Test that we can log in as the user, with the assigned password.
            with session.make_session_for_existing_user(username, passwd, lib.get_hostname(), self.admin.zone_name) as user_session:
                user_session.assert_icommand('icd')
        finally:
            self.admin.run_icommand(['iadmin','rmuser',username])

    def test_group_administrator_scenarios__issue_6888(self):
        """Thoroughly test atg, rfg (remove from group) and transfer of responsiblity to another group administrator"""
        rodsuser_name = 'alice'
        try:
            # Use the groupadmin account to create group and set itself up as administrator.
            self.groupadmin.assert_icommand(['igroupadmin', 'mkgroup', self.group])
            self.groupadmin.assert_icommand(['igroupadmin', 'atg', self.group, self.groupadmin.username])

            # Another groupadmin is needed for this test.
            self.groupadmin2 = session.mkuser_and_return_session('groupadmin', 'groupadmin2', 'rods', lib.get_hostname())

            # Test another user (a rodsuser) can be created and added to the group by this.
            self.groupadmin.assert_icommand(['igroupadmin', 'mkuser', rodsuser_name])
            self.groupadmin.assert_icommand(['igroupadmin', 'atg', self.group, rodsuser_name])

            user_with_zone = lambda user_name,zn='': '{}#{}'.format(user_name, zn if zn else self.groupadmin.zone_name)

            # Test that the groupadmin itself, and the new rodsuser, are members of the group.
            self.groupadmin.assert_icommand(['igroupadmin', 'lg', self.group, ], 'STDOUT_MULTILINE', [user_with_zone(rodsuser_name),
                                                                                                      user_with_zone(self.groupadmin.username)])

            # Another groupadmin who tries adding themselves to a nonempty group must be unsuccessful, else they could hijack the group.
            self.groupadmin2.assert_icommand(['igroupadmin', 'atg', self.group, self.groupadmin2.username ],'STDERR', 'CAT_INSUFFICIENT_PRIVILEGE_LEVEL')
            rc, out, _ = self.groupadmin2.assert_icommand(['igroupadmin', 'lg', self.group], 'STDOUT')
            self.assertNotIn(user_with_zone(self.groupadmin2.username), out)

            # Test removal of users, and self, from a group, as well as assignment of a new admin for the group via "atg".
            self.groupadmin.assert_icommand(['igroupadmin', 'rfg', self.group, rodsuser_name])         # remove rodsuser
            self.groupadmin.assert_icommand(['igroupadmin', 'atg', self.group, self.groupadmin2.username])  # add other groupadmin
            self.groupadmin.assert_icommand(['igroupadmin', 'rfg', self.group, self.groupadmin.username])   # remove self

            # Test that groupadmin2 is now a member of the group, and that groupadmin and the rodsuser have successfully been removed.
            rc, out, _ = self.groupadmin2.assert_icommand(['igroupadmin', 'lg', self.group], 'STDOUT_MULTILINE', user_with_zone(self.groupadmin2.username))
            self.assertEqual(rc, 0)
            self.assertNotIn(user_with_zone(rodsuser_name), out)
            self.assertNotIn(user_with_zone(self.groupadmin.username), out)

            # Test that the new groupadmin can do some necessary things (add and remove a user).
            self.groupadmin2.assert_icommand(['igroupadmin', 'atg', self.group, rodsuser_name], desired_rc = 0)
            self.groupadmin2.assert_icommand(['igroupadmin', 'lg', self.group], 'STDOUT_MULTILINE', user_with_zone(rodsuser_name))

            self.groupadmin2.assert_icommand(['igroupadmin', 'rfg', self.group, rodsuser_name], desired_rc = 0)
            rc, out, _ = self.groupadmin2.assert_icommand(['igroupadmin', 'lg', self.group], 'STDOUT', desired_rc = 0)
            self.assertNotIn(user_with_zone(rodsuser_name), out)
        finally:
            self.groupadmin2.__exit__()
            self.admin.assert_icommand(['iadmin', 'rfg', self.group, self.groupadmin2.username])
            self.admin.assert_icommand(['iadmin', 'rmgroup', self.group])
            self.admin.assert_icommand(['iadmin', 'rmuser', rodsuser_name])
            self.admin.assert_icommand(['iadmin', 'rmuser', self.groupadmin2.username])

    def test_mkgroup_with_no_zone(self):
        """Test mkgroup with no zone name provided."""
        try:
            self.groupadmin.assert_icommand(['igroupadmin', 'mkgroup', self.group])
            self.assertEqual('rodsgroup', lib.get_user_type(self.groupadmin, self.group))
            self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.groupadmin, self.group))

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])


    def test_mkgroup_with_local_zone(self):
        """Test mkgroup with local zone name provided."""
        try:
            self.groupadmin.assert_icommand(['igroupadmin', 'mkgroup', '#'.join([self.group, self.admin.zone_name])])
            self.assertEqual('rodsgroup', lib.get_user_type(self.groupadmin, self.group))
            self.assertEqual(self.admin.zone_name, lib.get_user_zone(self.groupadmin, self.group))

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])


    def test_mkgroup_with_remote_zone(self):
        """Test mkgroup with remote zone name provided."""
        remote_zone = 'somezone'
        try:
            group_with_zone = '#'.join([self.group, remote_zone])

            # remote zones for non-existent zones are not allowed
            self.groupadmin.assert_icommand(['igroupadmin', 'mkgroup', group_with_zone],
                                       'STDERR', 'SYS_NOT_ALLOWED')

            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_user_type(self.admin, self.group))

            # remote zones for existing remote zones are not allowed
            self.admin.assert_icommand(['iadmin', 'mkzone', remote_zone, 'remote', 'localhost:1247'])
            self.groupadmin.assert_icommand(['igroupadmin', 'mkgroup', group_with_zone],
                                       'STDERR', 'SYS_NOT_ALLOWED')

            self.assertIn('CAT_NO_ROWS_FOUND', lib.get_user_type(self.admin, self.group))

        finally:
            self.admin.run_icommand(['iadmin', 'rmgroup', self.group])
            self.admin.run_icommand(['iadmin', 'rmzone', remote_zone])
