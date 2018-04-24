# from __future__ import print_function
import sys
from .. import lib

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase

class Test_Util(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Util, self).setUp()

    def tearDown(self):
        super(Test_Util, self).tearDown()

    def test_stty(self):
        commands = [ "stty", "/bin/stty", "env stty" ]
        for cmd in commands:
            _, _, rc = lib.execute_command_permissive(cmd)
            self.assertEqual(rc, 0, "{0}: failed rc = {1} (expected 0)".format(cmd, rc))
