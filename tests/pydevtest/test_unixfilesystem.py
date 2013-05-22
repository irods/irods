import unittest
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail
import pydevtest_sessions as s
from resource_suite import ResourceSuite, ShortAndSuite

class Test_SaS(unittest.TestCase, ShortAndSuite):

    my_test_resource = {
        "setup"    : [
             "iadmin modresc demoResc name origResc",
             "iadmin mkresc demoResc unixfilesystem localhost:/tmp/demoRescVault",
        ],
        "teardown" : [
             "iadmin rmresc demoResc",
             "iadmin modresc origResc name demoResc",
        ]
    }

    def setUp(self):
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_usertwo(self):
        print s.sessions[2].getUserName()
        assert False, "usertwo forcefail"

    def test_skipper(self):
        print "I'm a captain!"
        raise SkipTest

class Test_UnixFileSystem(unittest.TestCase, ResourceSuite):

    my_test_resource = {
        "setup"    : [],
        "teardown" : []
    }

    def setUp(self):
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

