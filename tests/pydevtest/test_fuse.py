from __future__ import print_function

import commands
import contextlib
import distutils.spawn
import multiprocessing
import os
import shutil
import socket
import stat
import subprocess
import sys
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
import lib
import metaclass_unittest_test_case_generator
import resource_suite


def helper_irodsFs_iput_to_mv(self, filesize):
    with tempfile.NamedTemporaryFile(prefix=sys._getframe().f_code.co_name + '_0') as f:
        lib.make_file(f.name, filesize, 'arbitrary')
        hash0 = lib.md5_hex_file(f.name)
        self.admin.assert_icommand(['iput', f.name])
    basename = os.path.basename(f.name)
    self.helper_irodsFs_stat(basename, self.mount_point, self.admin.session_collection, filesize)
    with tempfile.NamedTemporaryFile(prefix=sys._getframe().f_code.co_name + '_1') as f:
        shutil.move(os.path.join(self.mount_point, basename), f.name)
        assert basename not in os.listdir(self.mount_point)
        self.admin.assert_icommand_fail(['ils'], 'STDOUT_SINGLELINE', basename)
        hash1 = lib.md5_hex_file(f.name)
        assert hash0 == hash1

def helper_irodsFs_cp_to_iget(self, filesize):
    fullpath, hash0 = self.helper_irodsFs_cp_into_mount_point(self.mount_point, filesize)
    basename = os.path.basename(fullpath)
    self.helper_irodsFs_stat(basename, self.mount_point, self.admin.session_collection, filesize)
    hash1 = self.helper_irodsFs_iget_and_hash(os.path.join(self.admin.session_collection, basename))
    assert hash0 == hash1
    self.helper_irodsFs_irm_and_confirm(basename, self.mount_point, self.admin.session_collection)

def helper_irodsFs_cp_to_cp_to_iget(self, filesize):
    fullpath0, hash0 = self.helper_irodsFs_cp_into_mount_point(self.mount_point, filesize)
    basename = os.path.basename(fullpath0)
    self.helper_irodsFs_stat(basename, self.mount_point, self.admin.session_collection, filesize)
    hash1 = self.helper_irodsFs_iget_and_hash(os.path.join(self.admin.session_collection, basename))
    assert hash0 == hash1
    deeper_directory = sys._getframe().f_code.co_name
    deeper_directory_fullpath = os.path.join(self.mount_point, deeper_directory)
    deeper_collection_fullpath = os.path.join(self.admin.session_collection, deeper_directory)
    os.mkdir(deeper_directory_fullpath)
    fullpath1 = os.path.join(deeper_directory_fullpath, basename)
    shutil.copyfile(fullpath0, fullpath1)
    self.helper_irodsFs_stat(basename, deeper_directory_fullpath, deeper_collection_fullpath, filesize)
    hash2 = self.helper_irodsFs_iget_and_hash(os.path.join(deeper_collection_fullpath, basename))
    assert hash1 == hash2
    self.helper_irodsFs_irm_and_confirm(basename, deeper_directory_fullpath, deeper_collection_fullpath)

@unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Skip for Topology Testing')
class Test_Fuse(resource_suite.ResourceBase, unittest.TestCase):
    __metaclass__ = metaclass_unittest_test_case_generator.MetaclassUnittestTestCaseGenerator

    def setUp(self):
        super(Test_Fuse, self).setUp()
        self.mount_point = tempfile.mkdtemp(prefix='irods-testing-fuse-mount-point')
        self.admin.assert_icommand(['irodsFs', self.mount_point])

    def tearDown(self):
        lib.assert_command(['fusermount', '-uz', self.mount_point])
        shutil.rmtree(self.mount_point)
        super(Test_Fuse, self).tearDown()

    def test_fusermount_permissions(self):
        # Check that fusermount is configured correctly for irodsFs
        fusermount_path = distutils.spawn.find_executable('fusermount')
        assert fusermount_path is not None, 'fusermount binary not found'
        assert os.stat(fusermount_path).st_mode & stat.S_ISUID, 'fusermount setuid bit not set'
        p = subprocess.Popen(['fusermount', '-V'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdoutdata, stderrdata = p.communicate()
        assert p.returncode == 0, '\n'.join(['fusermount not executable',
                                             'return code: ' + str(p.returncode),
                                             'stdout: ' + stdoutdata,
                                             'stderr: ' + stderrdata])

    def helper_irodsFs_stat(self, basename, parent_dir, parent_collection, filesize):
        assert basename in os.listdir(parent_dir)
        self.admin.assert_icommand(['ils', '-l', parent_collection], 'STDOUT_SINGLELINE', [basename, str(filesize)])
        assert os.stat(os.path.join(parent_dir, basename)).st_size == filesize

    def helper_irodsFs_cp_into_mount_point(self, target_dir, filesize):
        with tempfile.NamedTemporaryFile(prefix=sys._getframe().f_code.co_name) as f:
            lib.make_file(f.name, filesize, 'arbitrary')
            hash_ = lib.md5_hex_file(f.name)
            shutil.copy(f.name, target_dir)
        fullpath = os.path.join(target_dir, os.path.basename(f.name))
        assert os.stat(fullpath).st_size == filesize
        return fullpath, hash_

    def helper_irodsFs_iget_and_hash(self, data_object_path):
        basename = os.path.basename(data_object_path)
        with tempfile.NamedTemporaryFile(prefix=sys._getframe().f_code.co_name) as f:
            self.admin.assert_icommand(['iget', '-f', data_object_path, f.name])
            hash_ = lib.md5_hex_file(f.name)
        return hash_

    def helper_irodsFs_irm_and_confirm(self, basename, parent_dir, parent_collection):
        self.admin.assert_icommand(['irm', '-f', os.path.join(parent_collection, basename)])
        self.admin.assert_icommand_fail(['ils', parent_collection], 'STDOUT_SINGLELINE', basename)
        assert basename not in os.listdir(parent_dir)

    def generate_tests_for_each_file_size():
        # irodsFs uses aligned cache blocks of size 2**20, issue #2830
        file_sizes = [0, 1]
        for s in [2**10, 2**16, 2**20, 10**8, 10**9]:
            file_sizes.extend([s-1, s, s+1])

        def make_test(func, filesize):
            def test(self):
                func(self, filesize)
            return test

        for s in file_sizes:
            for f in [helper_irodsFs_cp_to_iget, helper_irodsFs_iput_to_mv, helper_irodsFs_cp_to_cp_to_iget]:
                name = 'test_' + f.__name__ + '_filesize_' + str(s)
                yield name, make_test(f, s)

    def test_irodsFs_bonnie(self):
        if os.path.isfile('/usr/sbin/bonnie++'):
            bonnie = '/usr/sbin/bonnie++'
        else:
            bonnie = '/usr/bin/bonnie++'

        rc, out, err = lib.run_command([bonnie, '-r', '1', '-c', '2', '-n', '10', '-d', self.mount_point])

        formatted_output = '\n'.join(['rc [{0}]'.format(rc),
                                      'stdout:\n{0}'.format(out),
                                      'stderr:\n{0}'.format(err)])

        assert rc == 0, formatted_output
        assert '-Per Chr- --Block-- -Rewrite- -Per Chr- --Block-- --Seeks--' in out, formatted_output
        assert '-Create-- --Read--- -Delete-- -Create-- --Read--- -Delete--' in out, formatted_output

    def test_parallel_write(self):
        num_files = 4
        files = [tempfile.NamedTemporaryFile(prefix='test_fuse.test_parallel_copy') for i in range(num_files)]
        with contextlib.nested(*files):
            hashes = []
            for f in files:
                lib.make_file(f.name, pow(10,8), 'arbitrary')
                hashes.append(lib.md5_hex_file(f.name))
            proc_pool = multiprocessing.Pool(len(files))
            proc_pool_results = [proc_pool.apply_async(shutil.copyfile, (f.name, os.path.join(self.mount_point, os.path.basename(f.name))))
                                 for f in files]
            for r in proc_pool_results:
                r.get()
            for f, h in zip(files, hashes):
                self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', os.path.basename(f.name))
                with tempfile.NamedTemporaryFile(prefix='test_fuse.test_parallel_copy_get') as fget:
                    self.admin.assert_icommand(['iget', '-f', os.path.basename(f.name), fget.name])
                    assert lib.md5_hex_file(fget.name) == h
