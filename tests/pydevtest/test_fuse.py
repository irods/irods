from __future__ import print_function

import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import commands
import contextlib
import distutils.spawn
import multiprocessing
import os
import shutil
import socket
import stat
import subprocess
import tempfile

import configuration
import lib

from resource_suite import ResourceBase


@unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Skip for Topology Testing')
class Test_Fuse(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Fuse, self).setUp()
        self.mount_point = tempfile.mkdtemp(prefix='irods-testing-fuse-mount-point')
        self.admin.assert_icommand(['irodsFs', self.mount_point])

    def tearDown(self):
        lib.assert_command(['fusermount', '-u', self.mount_point])
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

    def helper_test_irodsFs_iput(self, filesize):
        with tempfile.NamedTemporaryFile(prefix='irods-test-fuse') as f:
            lib.make_file(f.name, filesize)
            self.admin.assert_icommand(['iput', f.name])
        basename = os.path.basename(f.name)

        assert basename in os.listdir(self.mount_point)
        self.admin.assert_icommand(['ils'], 'STDOUT_SINGLELINE', basename)
        self.admin.assert_icommand(['irm', '-f', basename])
        assert basename not in os.listdir(self.mount_point)
        self.admin.assert_icommand_fail(['ils'], 'STDOUT_SINGLELINE', basename)

    def helper_test_irodsFs_cp(self, filesize):
        with tempfile.NamedTemporaryFile(prefix='irods-test-fuse') as f:
            lib.make_file(f.name, filesize)
            shutil.copy(f.name, self.mount_point)
        basename = os.path.basename(f.name)

        assert basename in os.listdir(self.mount_point)
        self.admin.assert_icommand(['ils'], 'STDOUT_SINGLELINE', basename)
        os.unlink(os.path.join(self.mount_point, basename))
        assert basename not in os.listdir(self.mount_point)
        self.admin.assert_icommand_fail(['ils'], 'STDOUT_SINGLELINE', basename)

    def test_irodsFs_large_file_issue_2252(self):
        self.helper_test_irodsFs_cp(pow(10,8))
        self.helper_test_irodsFs_iput(pow(10,8))

    def test_irodsFs_small_file(self):
        self.helper_test_irodsFs_cp(10)
        self.helper_test_irodsFs_iput(10)

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

    def test_parallel_copy(self):
        num_files = 4
        files = [tempfile.NamedTemporaryFile(prefix='test_fuse.test_parallel_copy') for i in range(num_files)]
        with contextlib.nested(*files):
            hashes = []
            for f in files:
                lib.make_file(f.name, pow(10,8), '/dev/urandom')
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
