import commands
import os
import shlex
import datetime
import time
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from resource_suite import ResourceBase
import lib


class Test_CatalogSuite(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_CatalogSuite, self).setUp()

    def tearDown(self):
        super(Test_CatalogSuite, self).tearDown()

    ###################
    # icd
    ###################

    def test_empty_icd(self):
        self.admin.assert_icommand("ils -L", 'STDOUT', "test")  # whatever
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("icd")  # just go home
        self.admin.assert_icommand("ils", 'STDOUT', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_empty_icd_verbose(self):
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("icd -v", 'STDOUT', "Deleting (if it exists) session envFile:")
        self.admin.assert_icommand("ils", 'STDOUT', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_icd_to_subdir(self):
        self.admin.assert_icommand("icd " + self.testdir)  # get into subdir
        self.admin.assert_icommand("ils", 'STDOUT', "/" + self.admin.zone_name + "/home/" +
                           self.admin.username + "/" + self.admin._session_id + "/" + self.testdir + ":")

    def test_icd_to_parentdir(self):
        self.admin.assert_icommand("icd ..")  # go to parent
        self.admin.assert_icommand("ils", 'STDOUT', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_icd_to_root(self):
        self.admin.assert_icommand("icd /")  # go to root
        self.admin.assert_icommand("ils", 'STDOUT', "/:")  # listing

    def test_icd_to_root_with_badpath(self):
        # go to root with bad path
        self.admin.assert_icommand("icd /doesnotexist", 'STDOUT', "No such directory (collection):")

    ###################
    # iexit
    ###################

    def test_iexit(self):
        self.admin.assert_icommand("iexit")  # just go home

    def test_iexit_verbose(self):
        self.admin.assert_icommand("iexit -v", 'STDOUT', "Deleting (if it exists) session envFile:")  # home, verbose

    def test_iexit_with_bad_option(self):
        self.admin.assert_icommand_fail("iexit -z")  # run iexit with bad option

    def test_iexit_with_bad_parameter(self):
        self.admin.assert_icommand_fail("iexit badparameter")  # run iexit with bad parameter

    ###################
    # ihelp
    ###################

    def test_local_ihelp(self):
        self.admin.assert_icommand('ihelp', 'STDOUT', 'The iCommands and a brief description of each:')

    def test_local_ihelp_with_help(self):
        self.admin.assert_icommand("ihelp -h", 'STDOUT', "Display iCommands synopsis")  # run ihelp with help

    def test_local_ihelp_all(self):
        self.admin.assert_icommand("ihelp -a", 'STDOUT', "Usage")  # run ihelp on all icommands

    def test_local_ihelp_with_good_icommand(self):
        self.admin.assert_icommand("ihelp ils", 'STDOUT', "Usage")  # run ihelp with good icommand

    def test_local_ihelp_with_bad_icommand(self):
        self.admin.assert_icommand_fail("ihelp idoesnotexist")  # run ihelp with bad icommand

    def test_local_ihelp_with_bad_option(self):
        self.admin.assert_icommand_fail("ihelp -z")  # run ihelp with bad option

    ###################
    # imkdir
    ###################

    def test_local_imkdir(self):
        # local setup
        mytestdir = "testingimkdir"
        self.admin.assert_icommand_fail("ils -L " + mytestdir, 'STDOUT', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir)  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT', mytestdir)  # should be listed

    def test_local_imkdir_with_trailing_slash(self):
        # local setup
        mytestdir = "testingimkdirwithslash"
        self.admin.assert_icommand_fail("ils -L " + mytestdir + "/", 'STDOUT', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir + "/")  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT', mytestdir)  # should be listed

    def test_local_imkdir_with_trailing_slash_already_exists(self):
        # local setup
        mytestdir = "testingimkdirwithslash"
        self.admin.assert_icommand("imkdir " + mytestdir + "/")  # imkdir
        self.admin.assert_icommand_fail("imkdir " + mytestdir)  # should fail, already exists
        self.admin.assert_icommand_fail("imkdir " + mytestdir + "/")  # should fail, already exists

    def test_local_imkdir_when_dir_already_exists(self):
        # local setup
        mytestdir = "testingimkdiralreadyexists"
        self.admin.assert_icommand("imkdir " + mytestdir)  # imkdir
        self.admin.assert_icommand_fail("imkdir " + mytestdir)  # should fail, already exists

    def test_local_imkdir_when_file_already_exists(self):
        # local setup
        self.admin.assert_icommand_fail("imkdir " + self.testfile)  # should fail, filename already exists

    def test_local_imkdir_with_parent(self):
        # local setup
        mytestdir = "parent/testingimkdirwithparent"
        self.admin.assert_icommand_fail("ils -L " + mytestdir, 'STDOUT', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir -p " + mytestdir)  # imkdir with parent
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT', mytestdir)  # should be listed

    def test_local_imkdir_with_bad_option(self):
        self.admin.assert_icommand_fail("imkdir -z")  # run imkdir with bad option

    ###################
    # iquest
    ###################

    def test_iquest_totaldatasize(self):
        self.admin.assert_icommand("iquest \"select sum(DATA_SIZE) where COLL_NAME like '/" +
                   self.admin.zone_name + "/home/%'\"", 'STDOUT', "DATA_SIZE")  # selects total data size

    def test_iquest_bad_format(self):
        self.admin.assert_icommand("iquest \"bad formatting\"", 'STDERR',
                   "INPUT_ARG_NOT_WELL_FORMED_ERR")  # bad request

    ###################
    # isysmeta
    ###################

    def test_isysmeta_init_set_and_reset(self):
        self.admin.assert_icommand("ils -L", "STDOUT", "pydevtest_testfile.txt")  # basic listing
        self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                   "data_expiry_ts (expire time): 00000000000: None")  # initialized with zeros
        offset_seconds = 1
        expected_time_string = time.strftime('%Y-%m-%d.%H:%M:%S', time.localtime(offset_seconds))
        # set to 1 sec after epoch
        self.admin.assert_icommand('isysmeta mod pydevtest_testfile.txt {0}'.format(offset_seconds), "EMPTY")
        self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                   "data_expiry_ts (expire time): 00000000001: {0}".format(expected_time_string))  # confirm
        self.admin.assert_icommand("isysmeta mod pydevtest_testfile.txt 0", "EMPTY")  # reset to zeros
        self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                   "data_expiry_ts (expire time): 00000000000: None")  # confirm

    def test_isysmeta_relative_set(self):
        self.admin.assert_icommand("ils -L", "STDOUT", "pydevtest_testfile.txt")  # basic listing
        self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                   "data_expiry_ts (expire time): 00000000000: None")  # initialized with zeros

        def check_relative_expiry(offset_seconds):
            def get_future_time_string(t):
                return (t + datetime.timedelta(0, offset_seconds)).strftime('%Y-%m-%d.%H:%M:%S')
            current_time = datetime.datetime.now()
            # Race condition: first assert fails if second threshold crossed in between iCAT recording
            #  current time and this script recording current time
            try:
                self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                           get_future_time_string(current_time))
            # Back script's current_time off by a second, since iCAT command issued before script records
            #  current_time
            except AssertionError:
                self.admin.assert_icommand("isysmeta ls pydevtest_testfile.txt", "STDOUT",
                           get_future_time_string(current_time - datetime.timedelta(0, 1)))

        # test seconds syntax
        seconds_ahead = 10
        self.admin.assert_icommand("isysmeta mod pydevtest_testfile.txt +" + str(seconds_ahead), "EMPTY")
        check_relative_expiry(seconds_ahead)

        # test hours syntax
        seconds_ahead = 60 * 60  # 1 hour
        self.admin.assert_icommand("isysmeta mod pydevtest_testfile.txt +1h", "EMPTY")
        check_relative_expiry(seconds_ahead)



class Test_CatalogSuitePermissions(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_CatalogSuitePermissions, self).setUp()

    def tearDown(self):
        super(Test_CatalogSuitePermissions, self).tearDown()

    def test_isysmeta_no_permission(self):
        self.user0.assert_icommand('icd /' + self.user0.zone_name + '/home/public')  # get into public/
        self.user0.assert_icommand('ils -L ', 'STDOUT', 'pydevtest_testfile.txt')
        self.user0.assert_icommand('isysmeta ls pydevtest_testfile.txt', 'STDOUT',
                                              'data_expiry_ts (expire time): 00000000000: None')  # initialized with zeros
        self.user0.assert_icommand('isysmeta mod pydevtest_testfile.txt 1', 'STDERR', 'CAT_NO_ACCESS_PERMISSION')  # cannot set expiry
