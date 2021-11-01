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
