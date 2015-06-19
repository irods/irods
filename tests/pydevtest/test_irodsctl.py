import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

import lib


class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(lib.get_irods_top_level_dir(), 'iRODS', 'irodsctl')
        lib.assert_command([irodsctl_fullpath, 'stop'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')

        possible_shm_locations = ['/var/run/shm', '/dev/shm']
        for l in possible_shm_locations:
            try:
                files = os.listdir(l)
                for f in files:
                    assert 'irods' not in f.lower(), os.path.join(l, f)
            except OSError:
                pass

        lib.start_irods_server()
