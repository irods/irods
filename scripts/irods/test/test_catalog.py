import datetime
import os
import platform
import sys
import time
import json

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .resource_suite import ResourceBase
from .. import lib
from .. import paths
from ..test.command import assert_command
from ..configuration import IrodsConfig


class Test_Catalog(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Catalog, self).setUp()

    def tearDown(self):
        super(Test_Catalog, self).tearDown()

    def test_case_sensitivity(self):
        self.user0.assert_icommand(['iput', self.testfile, 'a'])
        self.user0.assert_icommand(['iput', self.testfile, 'A'])
        self.user0.assert_icommand(['irm', 'a'])
        self.user0.assert_icommand(['irm', 'A'])

    def test_no_distinct(self):
        self.admin.assert_icommand(['iquest', 'no-distinct', 'select RESC_ID'], 'STDOUT_SINGLELINE', 'RESC_ID = ');

    ###################
    # icd
    ###################

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
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/" + self.admin.zone_name + "/home/" +
                                   self.admin.username + "/" + self.admin._session_id + "/" + self.testdir + ":")

    def test_icd_to_parentdir(self):
        self.admin.assert_icommand("icd ..")  # go to parent
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/" + self.admin.zone_name + "/home/" + self.admin.username + ":")

    def test_icd_to_root(self):
        self.admin.assert_icommand("icd /")  # go to root
        self.admin.assert_icommand("ils", 'STDOUT_SINGLELINE', "/:")  # listing

    def test_icd_to_root_with_badpath(self):
        # go to root with bad path
        self.admin.assert_icommand("icd /doesnotexist", 'STDOUT_SINGLELINE', "No such directory (collection):")

    ###################
    # iexit
    ###################

    def test_iexit(self):
        self.admin.assert_icommand('iexit')  # remove scrambled password file
        self.admin.assert_icommand(['iinit', self.admin.password]) # restore session for remaining tests

    def test_iexit_verbose(self):
        self.admin.assert_icommand('iexit -v', 'STDOUT_SINGLELINE', 'Deleting (if it exists) session envFile:', input='y\n')  # remove scrambled password file, verbose
        self.admin.assert_icommand(['iinit', self.admin.password]) # restore session for remaining tests

    def test_iexit_with_bad_option(self):
        self.admin.assert_icommand('iexit -z', 'STDERR_SINGLELINE', 'USER_INPUT_OPTION_ERR')  # run iexit with bad option

    ###################
    # ihelp
    ###################

    def test_local_ihelp(self):
        self.admin.assert_icommand('ihelp', 'STDOUT_SINGLELINE', 'The iCommands and a brief description of each:')

    def test_local_ihelp_with_help(self):
        self.admin.assert_icommand("ihelp -h", 'STDOUT_SINGLELINE', "Display iCommands synopsis")  # run ihelp with help

    def test_local_ihelp_all(self):
        self.admin.assert_icommand("ihelp -a", 'STDOUT_SINGLELINE', "Usage")  # run ihelp on all icommands

    def test_local_ihelp_with_good_icommand(self):
        self.admin.assert_icommand("ihelp ils", 'STDOUT_SINGLELINE', "Usage")  # run ihelp with good icommand

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
        self.admin.assert_icommand_fail("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir)  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should be listed

    def test_local_imkdir_with_trailing_slash(self):
        # local setup
        mytestdir = "testingimkdirwithslash"
        self.admin.assert_icommand_fail("ils -L " + mytestdir + "/", 'STDOUT_SINGLELINE', mytestdir)  # should not be listed
        self.admin.assert_icommand("imkdir " + mytestdir + "/")  # imkdir
        self.admin.assert_icommand("ils -L " + mytestdir, 'STDOUT_SINGLELINE', mytestdir)  # should be listed

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

    ###################
    # ilocate
    ###################

    def test_ilocate_with_spaces_in_collection_name__3332(self):
        self.admin.assert_icommand("imkdir 'before after'")
        self.admin.assert_icommand("iput {0} 'before after/'".format(self.testfile))
        self.admin.assert_icommand("ils -L 'before after'", 'STDOUT_SINGLELINE', self.testfile)
        self.admin.assert_icommand("ilocate {0}".format(self.testfile), 'STDOUT_SINGLELINE', 'after') # no longer truncating before the space

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

    ###################
    # iquest
    ###################

    def test_iquest_totaldatasize(self):
        self.admin.assert_icommand("iquest \"select sum(DATA_SIZE) where COLL_NAME like '/" +
                                   self.admin.zone_name + "/home/%'\"", 'STDOUT_SINGLELINE', "DATA_SIZE")  # selects total data size

    def test_iquest_bad_format(self):
        self.admin.assert_icommand("iquest \"bad formatting\"", 'STDERR_SINGLELINE',
                                   "INPUT_ARG_NOT_WELL_FORMED_ERR")  # bad request

    def test_iquest_incorrect_format_count(self):
        self.admin.assert_icommand("iquest \"%s %s\" \"select COLL_NAME where COLL_NAME like '%home%'\"",
                                   'STDERR_SINGLELINE', 'boost::too_few_args: format-string referred to more arguments than were passed')

    ###################
    # isysmeta
    ###################

    def test_isysmeta_no_resc_group__2819(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', self.testfile)  # basic listing
        self.admin.assert_icommand_fail("isysmeta ls -l "+self.testfile, 'STDOUT_SINGLELINE',
                                   "resc_group_name:")  # should not exist

    def test_isysmeta_init_set_and_reset(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "testfile.txt")  # basic listing
        self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                   "data_expiry_ts (expire time): 00000000000: None")  # initialized with zeros
        offset_seconds = 1
        expected_time_string = time.strftime('%Y-%m-%d.%H:%M:%S', time.localtime(offset_seconds))
        # set to 1 sec after epoch
        self.admin.assert_icommand('isysmeta mod testfile.txt {0}'.format(offset_seconds), "EMPTY")
        self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                   "data_expiry_ts (expire time): 00000000001: {0}".format(expected_time_string))  # confirm
        self.admin.assert_icommand("isysmeta mod testfile.txt 0", "EMPTY")  # reset to zeros
        self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                   "data_expiry_ts (expire time): 00000000000: None")  # confirm

    def test_isysmeta_relative_set(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "testfile.txt")  # basic listing
        self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                   "data_expiry_ts (expire time): 00000000000: None")  # initialized with zeros

        def check_relative_expiry(offset_seconds):
            def get_future_time_string(t):
                return (t + datetime.timedelta(0, offset_seconds)).strftime('%Y-%m-%d.%H:%M:%S')
            current_time = datetime.datetime.now()
            # Race condition: first assert fails if second threshold crossed in between iCAT recording
            #  current time and this script recording current time
            try:
                self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                           get_future_time_string(current_time))
            # Back script's current_time off by a second, since iCAT command issued before script records
            #  current_time
            except AssertionError:
                self.admin.assert_icommand("isysmeta ls testfile.txt", 'STDOUT_SINGLELINE',
                                           get_future_time_string(current_time - datetime.timedelta(0, 1)))

        # test seconds syntax
        seconds_ahead = 10
        self.admin.assert_icommand("isysmeta mod testfile.txt +" + str(seconds_ahead), "EMPTY")
        check_relative_expiry(seconds_ahead)

        # test hours syntax
        seconds_ahead = 60 * 60  # 1 hour
        self.admin.assert_icommand("isysmeta mod testfile.txt +1h", "EMPTY")
        check_relative_expiry(seconds_ahead)

class Test_CatalogPermissions(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_CatalogPermissions, self).setUp()

    def tearDown(self):
        super(Test_CatalogPermissions, self).tearDown()

    def test_isysmeta_no_permission(self):
        self.user0.assert_icommand('icd /' + self.user0.zone_name + '/home/public')  # get into public/
        self.user0.assert_icommand('ils -L ', 'STDOUT_SINGLELINE', 'testfile.txt')
        self.user0.assert_icommand('isysmeta ls testfile.txt', 'STDOUT_SINGLELINE',
                                   'data_expiry_ts (expire time): 00000000000: None')  # initialized with zeros
        self.user0.assert_icommand('isysmeta mod testfile.txt 1', 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')  # cannot set expiry
