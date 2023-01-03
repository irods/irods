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

    def test_pattern_and_ignore_options_together__6791_6806(self):
        patterns = ['foo', 'bar', 'bat']
        for i in range(10):
            for p in patterns:
                self.admin.assert_icommand(['itouch', p + str(i)])

        to_catch = [r'\b{}\b'.format('bat'+str(i)) for i in range(10)]

        # Using forward and reverse combination of -P and -I options, catch 'bat*' objects as result
        self.admin.assert_icommand(["itree", "-P", "ba*", "-I", "*ar?"], "STDOUT_MULTILINE", to_catch, use_regex = True)
        self.admin.assert_icommand(["itree", "-I", "*ar?", "-P", "ba*"], "STDOUT_MULTILINE", to_catch, use_regex = True)

        # Using regular expressions, catch objects matching /bat[0-9]/
        self.admin.assert_icommand(["itree", "--ignore-regex", ".*ar.", "--pattern-regex", "ba.*"], "STDOUT_MULTILINE", to_catch, use_regex = True)

        # Using regular expressions with extended syntax, catch objects matching /(bar|bat)[0-2]/x
        to_catch = [r'\b{}\b'.format(prefix+str(i)) for i in range(3) for prefix in ('bar','bat')]
        self.admin.assert_icommand(["itree",  "--regex-extended", "--ignore-regex", ".*[3-9]", "--pattern-regex", "ba(r|t)[[:digit:]]"],
                                    "STDOUT_MULTILINE", to_catch, use_regex = True)

    def test_pattern_and_ignore_options(self):
        patterns = ['foo', 'bar', 'bat']
        for i in range(10):
            for p in patterns:
                self.admin.assert_icommand(['itouch', p + str(i)])

        for i in patterns:
            i_star = i + '*'
            positive_space = [r'\b{}\b'.format(i+str(n)) for n in range(10)]
            negative_space = [r'\b{}\b'.format(P+str(n)) for n in range(10) for P in set(patterns) - {i}]
            self.admin.assert_icommand(["itree", "-P", i_star], "STDOUT_MULTILINE", positive_space, use_regex = True)
            self.admin.assert_icommand(["itree", "-I", i_star], "STDOUT_MULTILINE", negative_space, use_regex = True)

        for i in patterns:
            i_dot_star = i + '.*'
            positive_space = [r'\b{}\b'.format(i+str(n)) for n in range(10)]
            negative_space = [r'\b{}\b'.format(P+str(n)) for n in range(10) for P in set(patterns) - {i}]
            self.admin.assert_icommand(["itree", "--pattern-regex", i_dot_star], "STDOUT_MULTILINE", positive_space, use_regex = True)
            self.admin.assert_icommand(["itree", "--ignore-regex", i_dot_star], "STDOUT_MULTILINE", negative_space, use_regex = True)

    def test_collections_only(self):
        for i in range(10):
            self.admin.assert_icommand(['itouch', str(i) + ".file"])
            self.admin.assert_icommand(['imkdir', str(i)])

        self.admin.assert_icommand(["itree", "-c"], "STDOUT_SINGLELINE", "Found 10 collections and 0 data objects")

    def test_relative_paths__issue_6075(self):
        collection_name = os.path.basename(self.admin.session_collection)
        self.admin.assert_icommand(["itree", os.path.join("..", collection_name)], "STDOUT_SINGLELINE", "Found 0 collections and 0 data objects")
        self.assertEqual(self.admin.run_icommand(["itree"]), self.admin.run_icommand(["itree", "."]))

