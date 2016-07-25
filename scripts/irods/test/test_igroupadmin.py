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
                self.user_sessions[0].assert_icommand(['igroupadmin', 'atg', 'test_group', self.sessions[0].username])
                self.user_sessions[0].assert_icommand(['igroupadmin', 'atg', 'test_group', self.sessions[1].username])
                self.user_sessions[0].assert_icommand(['igroupadmin', 'lg', 'test_group'], STDOUT_SINGLELINE, self.user_sessions[0].username)
                self.user_sessions[0].assert_icommand(['igroupadmin', 'lg', 'test_group'], STDOUT_SINGLELINE, self.user_sessions[1].username)
                self.admin.assert_icommand(['iadmin', 'lg', 'test_group'], STDOUT_SINGLELINE, self.user_sessions[0].username)
                self.admin.assert_icommand(['iadmin', 'lg', 'test_group'], STDOUT_SINGLELINE, self.user_sessions[1].username)
                self.user_sessions[0].assert_icommand(['igroupadmin', 'rfg', 'test_group', self.sessions[1].username])

            finally :
                self.admin.assert_icommand(['iadmin', 'rmgroup', 'test_group'])
        finally :
            self.admin.assert_icommand(['iadmin', 'moduser', self.user_sessions[0].username, 'type', 'rodsuser'])

