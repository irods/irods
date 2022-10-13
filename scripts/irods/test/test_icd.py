import os
import unittest

from . import session

class Test_icd(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    def setUp(self):
        super(Test_icd, self).setUp()
        self.admin = self.admin_sessions[0]
        self.testdir = os.path.join(self.admin.session_collection, 'testdir')
        self.admin.assert_icommand(['imkdir', self.testdir])

    def tearDown(self):
        super(Test_icd, self).tearDown()

    def test_empty_icd(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "test")  # whatever
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("icd")  # just go home
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_empty_icd_verbose(self):
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("icd -v", 'STDOUT_SINGLELINE', "Deleting (if it exists) session envFile:")
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_icd_to_subdir(self):
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', self.testdir + ':')

    def test_icd_to_parentdir(self):
        self.admin.assert_icommand("icd ..")  # go to parent
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_icd_to_root(self):
        self.admin.assert_icommand("icd /")  # go to root
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/:")  # listing

    def test_icd_to_root_with_badpath(self):
        # go to root with bad path
        self.admin.assert_icommand("icd /doesnotexist", 'STDOUT_SINGLELINE', "No such collection:")

    def test_icd_to_data_object(self):
        object_name = 'foo'
        logical_path = os.path.join(self.admin.session_collection, object_name)
        self.admin.assert_icommand(['itouch', logical_path])
        self.admin.assert_icommand(['ils', logical_path], 'STDOUT', object_name)
        self.admin.assert_icommand(['icd', logical_path], 'STDOUT', "No such collection:")

