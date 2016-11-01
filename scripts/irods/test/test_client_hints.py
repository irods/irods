from __future__ import print_function
import sys
import shutil
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import datetime
import socket

from .. import test
from . import settings
from .. import lib
from . import resource_suite
from ..configuration import IrodsConfig


class Test_ClientHints(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_ClientHints, self).setUp()

    def tearDown(self):
        super(Test_ClientHints, self).tearDown()

    def test_client_hints(self):
        self.admin.assert_icommand('iclienthints', 'STDOUT_SINGLELINE', 'plugins')

