from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

from . import session
from . import settings
from . import resource_suite
from .. import lib

class Test_Irmdir(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Irmdir, self).setUp()

    def tearDown(self):
        super(Test_Irmdir, self).tearDown()

    def test_irmdir_of_nonexistent_collection(self):
        self.admin.assert_icommand(['irmdir', 'garbage_dir'], 'STDOUT_SINGLELINE', 'Collection does not exist')

    def test_irmdir_of_dataobj(self):
        filename = 'test_irmdir_of_dataobj'
        lib.make_file(filename, 1024, 'arbitrary')
        rods_filename = self.admin.session_collection + '/' + filename
        self.admin.assert_icommand(['iput', filename, rods_filename])
        self.admin.assert_icommand(['irmdir', rods_filename], 'STDOUT_SINGLELINE', 'Collection does not exist')
        self.admin.assert_icommand(['irm', '-f', rods_filename])
        os.unlink(filename)

    def test_irmdir_of_collection_containing_dataobj(self):
        filename = 'test_dataobj'
        collname = 'test_collection'
        lib.make_file(filename, 1024, 'arbitrary')
        rods_collname = self.admin.session_collection + '/' + collname
        rods_filename = rods_collname + '/' + filename
        self.admin.assert_icommand(['imkdir', rods_collname])
        self.admin.assert_icommand(['iput', filename, rods_filename])
        self.admin.assert_icommand(['irmdir', rods_collname], 'STDOUT_SINGLELINE', 'Collection is not empty')
        os.unlink(filename)

    def test_irmdir_of_collection_containing_collection(self):
        collname_1 = 'test_collection_1'
        collname_2 = 'test_collection_2'
        rods_collname_1 = self.admin.session_collection + '/' + collname_1
        rods_collname_2 = rods_collname_1 + '/' + collname_2
        self.admin.assert_icommand(['imkdir', rods_collname_1])
        self.admin.assert_icommand(['imkdir', rods_collname_2])
        self.admin.assert_icommand(['irmdir', rods_collname_1], 'STDOUT_SINGLELINE', 'Collection is not empty')

    def test_irmdir_of_empty_collection(self):
        collname = 'test_collection'
        rods_collname = self.admin.session_collection + '/' + collname
        self.admin.assert_icommand(['imkdir', rods_collname])
        self.admin.assert_icommand(['irmdir', rods_collname])
        # If irmdir failed, attempting to make a directory with the same name will also fail
        self.admin.assert_icommand(['imkdir', rods_collname])

    def test_irmdir_dash_p(self):
        path = 'a/b/c'
        col_d = 'd'
        col_e = 'd/e'

        abs_path = os.path.join(self.admin.session_collection, path)
        abs_path_to_col_d = os.path.join(self.admin.session_collection, path, col_d)

        self.admin.assert_icommand(['imkdir', '-p', os.path.join(path, col_e)])
        self.admin.assert_icommand(['icd', path])
        self.admin.assert_icommand(['ils'], 'STDOUT_MULTILINE', [abs_path, 'C- {0}'.format(abs_path_to_col_d)])
        self.admin.assert_icommand(['irmdir', '-p', col_e])
        self.admin.assert_icommand(['ils', col_e], 'STDERR', '/{0} does not exist '.format(col_e))
        self.admin.assert_icommand(['icd', self.admin.session_collection])
        self.admin.assert_icommand(['irmdir', '-p', path])
        self.admin.assert_icommand(['ils', path], 'STDERR', '/{0} does not exist '.format(path))

        # Trying to remove a collection that does not exist produces an error.
        self.admin.assert_icommand(['irmdir', '-p', 'x/y/z'], 'STDERR', 'Collection does not exist')

        # Trying to remove a collection that is not empty produces an error.
        self.admin.assert_icommand(['imkdir', '-p', 'a/b/c'])
        self.admin.assert_icommand(['imkdir', '-p', 'a/b/d'])
        self.admin.assert_icommand(['irmdir', '-p', 'a/b'], 'STDERR', 'Collection is not empty')
        self.admin.assert_icommand(['irmdir', 'a/b/c'])
        self.admin.assert_icommand(['irmdir', 'a/b/d'])
        self.admin.assert_icommand(['irmdir', '-p', 'a/b'])

        # Trying to remove a data object produces an error.
        filename = 'test_irmdir_of_dataobj'
        lib.make_file(filename, 1024, 'arbitrary')
        rods_filename = os.path.join(self.admin.session_collection, filename)
        self.admin.assert_icommand(['iput', filename, rods_filename])
        self.admin.assert_icommand(['irmdir', '-p', rods_filename], 'STDERR', 'Path does not point to a collection')
        self.admin.assert_icommand(['irm', '-f', rods_filename])
        os.unlink(filename)

    def test_irmdir_no_input(self):
        self.admin.assert_icommand('irmdir', 'STDOUT_SINGLELINE', 'No collection names specified.')

    def test_irmdir_removes_collection_even_if_sibling_exists__issue_4788(self):
        col_a = 'foo'
        self.admin.assert_icommand(['imkdir', col_a])

        col_b = 'foot'
        self.admin.assert_icommand(['imkdir', col_b])

        filename = 'issue_4788'
        file_path = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(file_path, 1024, 'arbitrary')
        self.admin.assert_icommand(['iput', file_path, os.path.join(col_b, filename)])

        self.admin.assert_icommand(['irmdir', col_a])
        self.admin.assert_icommand(['ils', col_a], 'STDERR', ['{0} does not exist'.format(os.path.join(self.admin.session_collection, col_a))])

