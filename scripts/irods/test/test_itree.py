from __future__ import print_function

import sys
import os

from . import session

if sys.version_info < (2,7):
    import unittest2 as unittest
else:
    import unittest

class Test_ITree(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_ITree, self).setUp()
        self.admin = self.admin_sessions[0]
        self.previous_dir,_, _ = self.admin.run_icommand(["ipwd"])
        self.admin.run_icommand(["icd", self.admin.session_collection])

    def tearDown(self):
        super(Test_ITree, self).tearDown()
        self.admin.run_icommand(['icd',self.previous_dir.strip()])

    def test_itree_correctly_counts_data_objects_and_collections(self):
        self.admin.assert_icommand("itree", 'STDOUT_SINGLELINE', "Found 0 collections and 0 data objects")

        for i in range(10):
            self.admin.assert_icommand("imkdir {}".format(i))
            self.admin.assert_icommand("itree",'STDOUT_SINGLELINE',"Found {} collections and 0 data objects".format(i + 1))

        for i in range(10):
            self.admin.assert_icommand("itouch {}.file".format(i))

        self.admin.assert_icommand('itree','STDOUT_SINGLELINE','Found 10 collections and 10 data objects')

    def test_depth(self):
        directory_names = ["foo", 'bar', 'bat', 'bamus', 'batis', 'bant']
        accumulated = ""

        for index,i in enumerate(directory_names):
            accumulated += (i + '/')
            self.admin.assert_icommand(["imkdir", accumulated])
            self.admin.assert_icommand("itree", 'STDOUT_SINGLELINE', ["  " * index + i])

        for index,item in enumerate(directory_names):
            self.admin.assert_icommand(["itree", "-L", str(1 + index)], 'STDOUT_SINGLELINE', ["  " * (1 + index) + item])

    def test_pattern_and_ignore_options(self):
        patterns = ['foo', 'bar', 'bat']
        for i in range(10):
            for p in patterns:
                self.admin.assert_icommand(['itouch', p + str(i)])

        for i in patterns:
            i = i + '.*'
            self.admin.assert_icommand(["itree", "-P", i], "STDOUT_SINGLELINE", "Found 0 collections and 10 data objects")
            self.admin.assert_icommand(["itree", "-I", i], "STDOUT_SINGLELINE", "Found 0 collections and 20 data objects")

    def test_collections_only(self):
        for i in range(10):
            self.admin.assert_icommand(['itouch', str(i) + ".file"])
            self.admin.assert_icommand(['imkdir', str(i)])

        self.admin.assert_icommand(["itree", "-c"], "STDOUT_SINGLELINE", "Found 10 collections and 0 data objects")

    def test_relative_paths__issue_6075(self):
        collection_name = os.path.basename(self.admin.session_collection)
        self.admin.assert_icommand(["itree", os.path.join("..", collection_name)], "STDOUT_SINGLELINE", "Found 0 collections and 0 data objects")
        self.assertEqual(self.admin.run_icommand(["itree"]), self.admin.run_icommand(["itree", "."]))

