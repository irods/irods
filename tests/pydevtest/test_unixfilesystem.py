import unittest
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import pydevtest_sessions as s
from resource_suite import ResourceSuite

class Test_UnixFileSystem(unittest.TestCase, ResourceSuite):

    def setUp(self):
        s.twousers_up()

    def tearDown(self):
        s.twousers_down()

