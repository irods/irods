from __future__ import print_function
import datetime
import os
import time
import unittest

from . import session

class Test_isysmeta(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(Test_isysmeta, self).setUp()
        self.admin = self.admin_sessions[0]

        self.filename = 'testfile.txt'
        self.local_file = os.path.join(self.admin.local_session_dir, self.filename)
        self.testfile = os.path.join(self.admin.session_collection, self.filename)

        with open(self.local_file, 'wt') as f:
            print('I AM A TESTFILE -- [' + self.filename + ']', file=f, end='')

        self.admin.assert_icommand(['iput', self.local_file, self.testfile])

    def tearDown(self):
        super(Test_isysmeta, self).tearDown()

    def test_isysmeta_no_resc_group__2819(self):
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', self.filename)  # basic listing
        self.admin.assert_icommand_fail("isysmeta ls -l "+self.testfile, 'STDOUT_SINGLELINE',
                                   "resc_group_name:")  # should not exist

    def test_isysmeta_init_set_and_reset(self):
        self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', self.filename)  # basic listing
        self.admin.assert_icommand(['isysmeta', 'ls', self.testfile], 'STDOUT_SINGLELINE',
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

