import commands
import getpass
import os
import re
import shutil
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import pydevtest_common as pdc
import pydevtest_common
from pydevtest_common import assertiCmd, assertiCmdFail, getiCmdOutput, create_local_testfile, get_irods_config_dir, get_irods_top_level_dir
import pydevtest_sessions as s
from resource_suite import ResourceSuite, ShortAndSuite, ResourceBase
from test_chunkydevtest import ChunkyDevTest
import subprocess



class Test_Control_Plane(unittest.TestCase, ResourceBase):
    hostname = pdc.get_hostname()
    my_test_resource = {
        "setup": [
        ],
        "teardown": [
        ],
    }

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_all_option(self):
        out = subprocess.check_output( "irods-grid pause --all", shell=True)
        assert( out.find( "pausing" ) != -1 )
        
        out = subprocess.check_output( "irods-grid resume --all", shell=True)
        assert( out.find( "resuming" ) != -1 )
        
        out = subprocess.check_output( "irods-grid status --all", shell=True)
        assert( out.find( "hosts" ) != -1 )





