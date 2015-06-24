import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

import lib


class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_creation(self):
        assert lib.re_shm_exists()

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(lib.get_irods_top_level_dir(), 'iRODS', 'irodsctl')
        lib.assert_command([irodsctl_fullpath, 'stop'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
        assert not lib.re_shm_exists(), lib.re_shm_exists()
        lib.start_irods_server()
