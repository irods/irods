import copy
import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
import shutil

from .resource_suite import ResourceBase
from .. import lib
from .. import test
from . import settings
from ..configuration import IrodsConfig

class Test_iPhymv(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iPhymv, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-iphymv'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

        irods_config = IrodsConfig()
        self.admin.assert_icommand("iadmin mkresc randResc random", 'STDOUT_SINGLELINE', 'random')
        self.admin.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                      irods_config.irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                      irods_config.irods_directory + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin addchildtoresc randResc unix1Resc")
        self.admin.assert_icommand("iadmin addchildtoresc randResc unix2Resc")


    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        self.admin.assert_icommand("iadmin rmchildfromresc randResc unix2Resc")
        self.admin.assert_icommand("iadmin rmchildfromresc randResc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc unix2Resc")
        self.admin.assert_icommand("iadmin rmresc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc randResc")
        super(Test_iPhymv, self).tearDown()

    def test_iphymv_to_child_resource__2933(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        file_name = 'file'
        dest_path = os.path.join(self.admin.session_collection, file_name)
        self.admin.assert_icommand('iput -fR randResc ' + filepath + ' ' + dest_path)

        self.admin.assert_icommand('ils -L ' + dest_path, 'STDOUT_SINGLELINE', 'randResc')
        source_resc = self.admin.run_icommand(['iquest', '%s',
            '''"select RESC_NAME where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(os.path.dirname(dest_path), os.path.basename(dest_path))])[0].strip()
        source_resc = source_resc.strip()
        dest_resc = 'unix2Resc' if source_resc == 'unix1Resc' else 'unix1Resc'
        self.admin.assert_icommand(['iphymv', '-S', source_resc, '-R', dest_resc, dest_path])

        self.admin.assert_icommand('irm -f ' + dest_path)

    def test_iphymv_to_resc_hier__2933(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        file_name = 'file'
        dest_path = os.path.join(self.admin.session_collection, file_name)
        self.admin.assert_icommand('iput -fR randResc ' + filepath + ' ' + dest_path)

        self.admin.assert_icommand('ils -l ' + dest_path, 'STDOUT_SINGLELINE', 'randResc')
        source_resc = self.admin.run_icommand(['iquest', '%s',
            '''"select RESC_NAME where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(os.path.dirname(dest_path), os.path.basename(dest_path))])[0].strip()
        dest_resc = 'unix2Resc' if source_resc == 'unix1Resc' else 'unix1Resc'
        self.admin.assert_icommand(['iphymv', '-S', 'randResc;{}'.format(source_resc), '-R', 'randResc;{}'.format(dest_resc), dest_path])

        self.admin.assert_icommand('irm -f ' + dest_path)

    def test_iphymv_invalid_resource__2821(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('iphymv -S invalidResc -R demoResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('iphymv -S demoResc -R invalidResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for Topology Testing on Resource Server')
    def test_iphymv_unable_to_unlink__2820(self):
        filename = 'test_iphymv_unable_to_unlink__2820'
        localfile = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(localfile, 1)
        dest_path = os.path.join(self.admin.session_collection, filename)

        try:
            # put a data object so that a file appears in the vault
            self.admin.assert_icommand(['iput', localfile, dest_path])

            # modify the permissions on the physical file in the vault to deny unlinking to the service account
            filepath = os.path.join(self.admin.get_vault_session_path(), filename)
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', filepath)
            os.chmod(filepath, 0444)

            # attempt and fail to phymv the replica (due to no permissions for unlink)
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'TestResc', dest_path], 'STDERR_SINGLELINE', 'Permission denied')
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', 'demoResc')

            # ensure that physical file was not unlinked
            self.assertTrue(os.path.exists(filepath))

            # set the permissions on the physical file back to allow unlinking to the service account
            os.chmod(filepath, 0666)

            # attempt to phymv the replica (and succeed)
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'TestResc', dest_path])

            # ensure that file was unlinked
            self.assertFalse(os.path.exists(filepath))
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', 'TestResc')

        finally:
            os.unlink(localfile)
            self.admin.assert_icommand(['irm', '-f', dest_path])

    def test_iphymv_to_self(self):
        filename = 'test_iphymv_to_self'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)

        try:
            self.admin.assert_icommand(['iput', filepath, filename])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT', [' 0 ', 'demoResc', ' & ', filename])
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'demoResc', filename])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT', [' 0 ', 'demoResc', ' & ', filename])

        finally:
            self.admin.run_icommand(['irm', '-f', filename])
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iphymv_no_source(self):
        filename = 'test_iphymv_no_source'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        dest_path = os.path.join(self.admin.session_collection, filename)
        self.admin.assert_icommand(['iput', filepath])
        out, err, _ = self.admin.run_icommand(['iphymv', '-R', 'demoResc', dest_path])
        error_string = 'Source hierarchy or leaf resource or replica number required'
        self.assertIn(error_string, out, msg='Missing error message')
        self.assertIn('USER__NULL_INPUT_ERR', err, msg='Missing USER__NULL_INPUT_ERR')

