import os
import unittest

from . import session
from .. import lib

class Test_ilocate(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    def setUp(self):
        super(Test_ilocate, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_ilocate, self).tearDown()

    def test_ilocate_with_spaces_in_collection_name__3332(self):
        testfile = 'test_ilocate_with_spaces_in_collection_name__3332'
        collection_name = 'before after'
        logical_path = os.path.join(collection_name, testfile)

        self.admin.assert_icommand(['imkdir', collection_name])
        self.admin.assert_icommand(['itouch', logical_path])
        self.admin.assert_icommand(['ils', '-L', collection_name], 'STDOUT_SINGLELINE', testfile)
        self.admin.assert_icommand(['ilocate', testfile], 'STDOUT_SINGLELINE', 'after') # no longer truncating before the space

    def test_ilocate_with_spaces_in_data_object_name__4540(self):
        try:
            dirname = 'dir'
            file_with_spaces = 'file with spaces'
            lib.make_file(file_with_spaces, 15)
            self.admin.assert_icommand("imkdir '{0}'".format(dirname))
            self.admin.assert_icommand("iput '{0}' '{1}'".format(file_with_spaces, dirname))
            self.admin.assert_icommand("ils -L '{0}'".format(dirname), 'STDOUT_SINGLELINE', file_with_spaces)
            self.admin.assert_icommand("ilocate '{0}'".format(file_with_spaces), 'STDOUT_SINGLELINE', dirname+'/'+file_with_spaces) # no longer splitting input on spaces
            self.admin.assert_icommand("ilocate '%h s%'", 'STDOUT_SINGLELINE', dirname+'/'+file_with_spaces) # wildcard works with space as well
        finally:
            # cleanup
            os.unlink(file_with_spaces)

    def test_ilocate_with_spaces_in_data_object_name_and_multiple_arguments__4540(self):
        try:
            dirname = 'dir'
            file_with_spaces = 'file with spaces'
            otherfile = 'otherfile'
            lib.make_file(file_with_spaces, 15)
            lib.make_file(otherfile, 30)
            self.admin.assert_icommand("imkdir '{0}'".format(dirname))
            self.admin.assert_icommand("iput '{0}' '{1}'".format(file_with_spaces, dirname))
            self.admin.assert_icommand("iput '{0}' '{1}'".format(otherfile, dirname))
            self.admin.assert_icommand("ils -L '{0}'".format(dirname), 'STDOUT_SINGLELINE', file_with_spaces)
            self.admin.assert_icommand("ilocate '{0}' {1}".format(file_with_spaces, otherfile), 'STDOUT_SINGLELINE', dirname+'/'+file_with_spaces) # no longer splitting input on spaces
            self.admin.assert_icommand("ilocate '{0}' {1}".format(file_with_spaces, otherfile), 'STDOUT_SINGLELINE', dirname+'/'+otherfile) # no longer splitting input on spaces
            self.admin.assert_icommand("ilocate '%h s%'", 'STDOUT_SINGLELINE', dirname+'/'+file_with_spaces) # wildcard works with space as well
        finally:
            # cleanup
            os.unlink(file_with_spaces)
            os.unlink(otherfile)

    def test_ilocate_supports_case_insensitive_search__issue_4761(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='object 1')

        data_object_upper = data_object.upper()
        self.admin.assert_icommand(['istream', 'write', data_object_upper], input='object 2')

        data_object_mixed = 'fOo'
        self.admin.assert_icommand(['istream', 'write', data_object_mixed], input='object 3')

        expected_output = [
            os.path.join(self.admin.session_collection, data_object),
            os.path.join(self.admin.session_collection, data_object_upper),
            os.path.join(self.admin.session_collection, data_object_mixed)
        ]
        self.admin.assert_icommand(['ilocate', '-i', data_object], 'STDOUT', expected_output)

