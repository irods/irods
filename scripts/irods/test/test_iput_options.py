import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase
from .. import lib


class Test_iPut_Options(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iPut_Options, self).setUp()

    def tearDown(self):
        super(Test_iPut_Options, self).tearDown()

    def test_iput_options(self):
        self.admin.assert_icommand('ichmod read ' + self.user0.username + ' ' + self.admin.session_collection)
        self.admin.assert_icommand('ichmod read ' + self.user1.username + ' ' + self.admin.session_collection)
        zero_filepath = os.path.join(self.admin.local_session_dir, 'zero')
        lib.touch(zero_filepath)
        self.admin.assert_icommand('iput --metadata "a;v;u;a0;v0" --acl "read ' + self.user0.username + ';'
                                   + 'write ' + self.user1.username + ';" -- ' + zero_filepath)
        self.admin.assert_icommand('imeta ls -d zero', 'STDOUT',
                                   '(attribute: *a0?\nvalue: *v0?\nunits: *u?(\n-+ *\n)?){2}', use_regex=True)
        self.admin.assert_icommand('iget -- ' + self.admin.session_collection + '/zero ' + self.admin.local_session_dir + '/newzero')
        self.user0.assert_icommand('iget -- ' + self.admin.session_collection + '/zero ' + self.user0.local_session_dir + '/newzero')
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u;a2;v2" --acl "read ' + self.user0.username + ';'
                                   + 'write ' + self.user1.username + ';" -- ' + filepath)
        self.admin.assert_icommand('imeta ls -d file', 'STDOUT',
                                   '(attribute: *a2?\nvalue: *v2?\nunits: *u?(\n-+ *\n)?){2}', use_regex=True)
        self.admin.assert_icommand('ils -l', 'STDOUT')
        self.admin.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.admin.local_session_dir + '/newfile')
        self.user0.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.user0.local_session_dir + '/newfile')
        new_filepath = os.path.join(self.user1.local_session_dir, 'file')
        # skip the end until the iput -f of unowned files is resolved
        lib.make_file(new_filepath, 2)
        self.admin.assert_icommand('iput -f -- ' + filepath + ' ' + self.admin.session_collection + '/file')
        self.user1.assert_icommand('iput -f -- ' + new_filepath + ' ' + self.admin.session_collection + '/file')
    def test_iput_with_metadata_overwrite__issue_3114(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('iput -f --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('iput -f --metadata "a;v1;u1" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'value: v1')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/file', 'STDOUT_SINGLELINE', 'units: u1')
    def test_iput_checksum_zero_length_file__issue_3275(self):
        filename = 'test_iput_checksum_zero_length_file__issue_3275'
        lib.touch(filename)
        self.user0.assert_icommand(['iput', '-K', filename])
        self.user0.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', 'sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=')
        os.unlink(filename)
    def test_iput_with_metadata_same_file_name__issue_3434(self):
        filename = 'test_iput_with_metadata_same_file_name__issue_3434'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/' + filename, 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/' + filename, 'STDOUT_SINGLELINE', 'units: u')
        self.admin.assert_icommand('imkdir foo')
        self.admin.assert_icommand('icd foo')
        self.admin.assert_icommand('iput --metadata "a;v;u" ' + filepath)
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/foo/' + filename, 'STDOUT_SINGLELINE', 'value: v')
        self.admin.assert_icommand('imeta ls -d ' + self.admin.session_collection + '/foo/' + filename, 'STDOUT_SINGLELINE', 'units: u')
