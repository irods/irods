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

class Test_Irm(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Irm, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Irm, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_option_n_is_deprecated__issue_3451(self):
        filename = 'test_file_issue_3451.txt'
        lib.make_file(filename, 1024)
        self.admin.assert_icommand('iput {0}'.format(filename))
        self.admin.assert_icommand('irm -n0 {0}'.format(filename), 'STDOUT', '-n is deprecated.')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_disallow_simultaneous_usage_of_options_r_and_n__issue_3661(self):
        filename = 'test_file_issue_3661.txt'
        filename_path = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filename_path, 1024)

        self.admin.assert_icommand('iput {0}'.format(filename_path))
        self.admin.assert_icommand('irm -rn0 {0}'.format(filename), 'STDERR', 'USER_INCOMPATIBLE_PARAMS')

    def test_irm_delete_collection_with_ampersand_in_name_causes_error__issue_3398(self):
        collection = 'testDeleteACollectionWithAmpInTheNameBug170 && hail hail rock & roll  &'
        self.admin.assert_icommand("imkdir '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection), 'STDOUT', collection)
        self.admin.assert_icommand("irm -r '{0}'".format(collection))
        self.admin.assert_icommand("ils -l '{0}'".format(self.admin.session_collection_trash), 'STDOUT', collection)

