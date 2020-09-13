from __future__ import print_function

import os
import sys
import shutil

from time import sleep

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from .. import paths

class Test_Collection_MTime(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Collection_MTime, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Collection_MTime, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_mtime_of_parent_collection_is_updated_following_collection_operations(self):
        collection = 'mtime_col.d'
        copied_collection = 'mtime_col.copied.d'

        self.run_test(lambda: self.admin.assert_icommand(['imkdir', collection]))
        self.run_test(lambda: self.admin.assert_icommand(['icp', '-r', collection, copied_collection]))
        self.run_test(lambda: self.admin.assert_icommand(['irmdir', collection]))
        self.run_test(lambda: self.admin.assert_icommand(['irmdir', copied_collection]))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_mtime_of_parent_collection_is_updated_following_data_object_operations(self):
        filename = 'mtime_file.txt'
        new_filename = 'mtime_file.renamed.txt'
        copied_filename = 'mtime_file.copied.txt'

        try:
            lib.make_file(filename, 1)
            self.run_test(lambda: self.admin.assert_icommand(['iput', filename]))
            self.run_test(lambda: self.admin.assert_icommand(['imv', filename, new_filename]))
            self.run_test(lambda: self.admin.assert_icommand(['icp', new_filename, copied_filename]))
            self.run_test(lambda: self.admin.assert_icommand(['irm', '-f', new_filename]))
            self.run_test(lambda: self.admin.assert_icommand(['irm', '-f', copied_filename]))

            # Creates a new data object.
            self.run_test(lambda: self.admin.assert_icommand(['istream', 'write', filename], input='the data'))
        finally:
            os.remove(filename)

    def run_test(self, trigger_mtime_update_func):
        old_mtime = self.get_mtime(self.admin.session_collection)
        sleep(2) # Guarantees that the following operation produces a different mtime.
        trigger_mtime_update_func()
        self.assertNotEqual(self.get_mtime(self.admin.session_collection), old_mtime)

    def get_mtime(self, coll_path):
        mtime, ec, rc = self.admin.run_icommand('iquest %s "select COLL_MODIFY_TIME where COLL_NAME = \'{0}\'"'.format(coll_path))
        return int(mtime)

