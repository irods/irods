import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
import shutil

import configuration
from resource_suite import ResourceBase
import lib
import copy

class Test_iPhymv(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iPhymv, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-iphymv'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_iPhymv, self).tearDown()

    def test_iphymv_invalid_resource__2821(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('iphymv -S invalidResc -R demoResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('iphymv -S demoResc -R invalidResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')

    def test_iphymv_unable_to_unlink__2820(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('ireg '+filepath+' '+dest_path)

        os.chmod(filepath, 0444)
        self.admin.assert_icommand('iphymv -S demoResc -R pydevtest_TestResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_USER_NO_PERMISSION')
        
        os.chmod(filepath, 0666)
        self.admin.assert_icommand('iphymv -S demoResc -R pydevtest_TestResc '+dest_path)

    def test_iphymv_to_child_resource__2933(self):
        self.admin.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
        self.admin.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                      lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                      lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin addchildtoresc rrResc unix1Resc")
        self.admin.assert_icommand("iadmin addchildtoresc rrResc unix2Resc")

        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('iput -fR rrResc ' + filepath + ' ' + dest_path)

        self.admin.assert_icommand('ils -L ' + dest_path, 'STDOUT_SINGLELINE', 'rrResc')
        self.admin.assert_icommand('iphymv -S unix1Resc -R unix2Resc ' + dest_path)

        self.admin.assert_icommand('irm -f ' + dest_path)
        self.admin.assert_icommand("iadmin rmchildfromresc rrResc unix2Resc")
        self.admin.assert_icommand("iadmin rmchildfromresc rrResc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc unix2Resc")
        self.admin.assert_icommand("iadmin rmresc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc rrResc")

    def test_iphymv_to_resc_hier__2933(self):
        self.admin.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
        self.admin.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                      lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                      lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin addchildtoresc rrResc unix1Resc")
        self.admin.assert_icommand("iadmin addchildtoresc rrResc unix2Resc")

        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('iput -fR rrResc ' + filepath + ' ' + dest_path)

        self.admin.assert_icommand('ils -L ' + dest_path, 'STDOUT_SINGLELINE', 'rrResc')
        self.admin.assert_icommand('iphymv -S "rrResc;unix1Resc" -R "rrResc;unix2Resc" ' + dest_path)

        self.admin.assert_icommand('irm -f ' + dest_path)
        self.admin.assert_icommand("iadmin rmchildfromresc rrResc unix2Resc")
        self.admin.assert_icommand("iadmin rmchildfromresc rrResc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc unix2Resc")
        self.admin.assert_icommand("iadmin rmresc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc rrResc")




