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
        collname_1 = 'test_collection_1'
        collname_2 = 'test_collection_2'
        collname_3 = 'subdir'
        collname_4 = 'subsubdir'
        collname_5 = 'subsubsubdir'
        rods_collname_1 = self.admin.session_collection + '/' + collname_1
        rods_collname_2 = self.admin.session_collection + '/' + collname_2
        rods_collname_3 = rods_collname_2 + '/' + collname_3
        rods_collname_4 = rods_collname_3 + '/' + collname_4
        rods_collname_5 = rods_collname_4 + '/' + collname_5
        self.admin.assert_icommand(['imkdir', rods_collname_1])
        self.admin.assert_icommand(['imkdir', rods_collname_2])
        self.admin.assert_icommand(['imkdir', rods_collname_3])
        self.admin.assert_icommand(['imkdir', rods_collname_4])
        self.admin.assert_icommand(['imkdir', rods_collname_5])
        self.admin.assert_icommand(['irmdir', '-p', rods_collname_5], 'STDOUT_SINGLELINE', [ '[' + self.admin.session_collection + ']', 'Collection is not empty'])
        # If irmdir failed, attempting to make a directory with the same name will also fail
        self.admin.assert_icommand(['imkdir', rods_collname_2])
