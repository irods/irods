import os
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
from .resource_suite import ResourceBase
import shutil
from .. import lib

class Test_Faulty_Filesystem(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Faulty_Filesystem, self).setUp()

        # make test directory
        self.testing_tmp_dir = '/tmp/irods_test_faulty_fs'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

        # mount point should be there
        self.fs_mount_point = '/tmp/bad_fs'
        assert os.path.isdir(self.fs_mount_point)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_Faulty_Filesystem, self).tearDown()

    def test_iget_large_file(self):
        # make large test file
        file_name = 'test_iget_large_file'
        file_path = os.path.join(self.testing_tmp_dir, file_name)
        lib.make_file(file_path, '448M')

        # iput file
        self.user0.assert_icommand("iput -fK {file_path}".format(**locals()), "EMPTY")

        # iget file into bad_fs
        target_dir = self.fs_mount_point
        self.user0.assert_icommand("iget -fK {file_name} {target_dir}/{file_name}".format(**locals()),
                                   'STDERR_SINGLELINE','USER_CHKSUM_MISMATCH')
