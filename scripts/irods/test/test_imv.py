from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from ..configuration import IrodsConfig

class Test_Imv(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Imv, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Imv, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_imv_boost_any_cast_exception_on_rename__issue_4301(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                with open(core_re_path, 'a') as core_re:
                    core_re.write('pep_api_data_obj_rename_post(*a, *b, *c) {}\n')

                src = 'test_file_issue_4301_a.txt'
                lib.make_file(src, 1024, 'arbitrary')
                self.admin.assert_icommand(['iput', src])

                dst = 'test_file_issue_4301_b.txt'
                self.admin.assert_icommand(['imv', src, dst])
                self.admin.assert_icommand('ils', 'STDOUT', dst)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_imv_prints_error_on_non_existent_collection__issue_4414(self):
        # Put a file into iRODS.
        filename = os.path.join(self.admin.local_session_dir, 'foo')
        lib.make_file(filename, 1024, 'arbitrary')
        self.admin.assert_icommand(['iput', filename])

        # Try to move the data object under a non-existent collection.
        data_object = os.path.basename(filename)
        new_path = os.path.join(self.admin.session_collection, 'non_existent_collection.d', data_object)
        _, stderr, ec = self.admin.run_icommand(['imv', data_object, new_path])
        self.assertTrue('-814000 CAT_UNKNOWN_COLLECTION' in stderr)
        self.assertTrue(ec != 0)

