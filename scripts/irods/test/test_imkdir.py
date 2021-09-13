from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session

class Test_imkdir(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    def setUp(self):
        super(Test_imkdir, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_imkdir, self).tearDown()

    def test_imkdir_does_not_create_collections_under_root_collection_due_to_substring_bug__issue_4621(self):
        rods = session.make_session_for_existing_admin()
        rods.assert_icommand_fail(['imkdir', os.path.join("/", rods.zone_name + 'Nopes')], 'STDOUT', [
            'SYS_INVALID_INPUT_PARAM',
            'a valid zone name does not appear at the root of the object path'
        ])

    def test_local_imkdir(self):
        mytestdir = "testingimkdir"
        self.admin.assert_icommand_fail("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir)  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should be listed

    def test_local_imkdir_with_trailing_slash(self):
        mytestdir = "testingimkdirwithslash"
        self.admin.assert_icommand_fail("ils -L " + mytestdir + "/", 'STDOUT_SINGLELINE', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir + "/")  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should be listed

    def test_local_imkdir_with_trailing_slash_already_exists(self):
        mytestdir = "testingimkdirwithslash"
        self.admin.assert_icommand("imkdir " + mytestdir + "/")  # imkdir
        self.admin.assert_icommand_fail("imkdir " + mytestdir)  # should fail, already exists
        self.admin.assert_icommand_fail("imkdir " + mytestdir + "/")  # should fail, already exists

    def test_local_imkdir_when_dir_already_exists(self):
        mytestdir = "testingimkdiralreadyexists"
        self.admin.assert_icommand("imkdir " + mytestdir)  # imkdir
        self.admin.assert_icommand_fail("imkdir " + mytestdir)  # should fail, already exists

    def test_local_imkdir_when_file_already_exists(self):
        testfile = 'test_local_imkdir_when_file_already_exists'
        self.admin.assert_icommand(['itouch', testfile])
        self.admin.assert_icommand_fail(['imkdir', testfile])  # should fail, filename already exists

    def test_local_imkdir_with_parent(self):
        mytestdir = "parent/testingimkdirwithparent"
        self.admin.assert_icommand_fail("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir -p " + mytestdir)  # imkdir with parent
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should be listed

    def test_local_imkdir_with_bad_option(self):
        self.admin.assert_icommand_fail("imkdir -z")  # run imkdir with bad option

    def test_imkdir_long_path(self):
        basecoll = "research-acceptatietestj"
        longpath = basecoll + "/gfasgasgfisjafipkaweopfkpwkpvfksawdogkvloafsadlkskdjfdsajfkl asjd" + \
                   "fuihbuihuiajujgfawnfuiaheuirfhsaduifhvnasindfuihawsuihseuihruiashdfjkashdjkfhajksdfdsaf/fa" + \
                   "sdfwaguwioefhiuawshfuiwheafuihaweuifhuiawehfuihuwaeihfuihweauihfuihguiwhauighuiewhguahwseu" + \
                   "ighuiaehguheuighuawghaeguihwguhuiwehheuahguiwsehghawsghuiashuigheui/sadgasdgadwsfjkasdj fh" + \
                   "jkasdhjfjkasdfjjesawedfjjfdijkasdfjsdafjsadjkjksdajkasdjklfsadjkjklifr weierwiorweiorweior" + \
                   "weiokjfdskjajlioeriorweiorwrewisdajkfdsfdasrweoiroerowakljsdlfklaskleiowiorewijasdfkljfdkl" + \
                   "saiowropiewkliakl dfas/dafsfasdfjkfdsajikuadsfjkdsafjkdfsajkdfseajerwrrwareafldfadffdsafdr" + \
                   "efwierioaiorioarifrfifdfakrw/dsdfsdf"

        self.admin.assert_icommand("ils -l " + basecoll, 'STDERR_SINGLELINE',
                   basecoll+" does not exist or user lacks access permission", desired_rc=4)
        self.admin.assert_icommand("imkdir -p " + longpath)
        self.admin.assert_icommand("ils -l " + basecoll, 'STDOUT_SINGLELINE', basecoll)  # should be listed

