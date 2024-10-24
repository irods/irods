import unittest

from subprocess import PIPE
import os

from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..test.command import assert_command

class Test_Irodsctl(unittest.TestCase):

    @unittest.skipUnless('irods_rule_engine_plugin-irods_rule_language' in IrodsConfig().configured_rule_engine_plugins,
                         'Requires the iRODS Rule Language plugin')
    def test_re_shm_creation(self):
        self.assertTrue(lib.re_shm_exists())

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(IrodsConfig().irods_directory, 'irodsctl')
        assert_command([irodsctl_fullpath, 'stop', '-v'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
        self.assertFalse(lib.re_shm_exists(), msg=lib.re_shm_exists())
        IrodsController().start(test_mode=True)
