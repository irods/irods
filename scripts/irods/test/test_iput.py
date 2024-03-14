from __future__ import print_function
import json
import os
import sys
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..test.command import assert_command

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]

class Test_Iput(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(Test_Iput, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Iput, self).tearDown()

    def test_multithreaded_iput__issue_5675(self):
        user0 = self.user_sessions[0]

        def do_test(size, threads):
            file_name = 'test_multithreaded_iput__issue_5675'
            local_file = os.path.join(user0.local_session_dir, file_name)
            dest_path = os.path.join(user0.session_collection, file_name)

            try:
                lib.make_file(local_file, size)

                put_thread_count = user0.run_icommand(['iput', '-v', local_file, dest_path])[0].split('|')[2].split()[0]
                self.assertEqual(int(put_thread_count), threads)

                put_overwrite_thread_count = user0.run_icommand(['iput', '-f', '-v', local_file, dest_path])[0].split('|')[2].split()[0]
                self.assertEqual(int(put_overwrite_thread_count), threads)

            finally:
                user0.assert_icommand(['irm', '-f', dest_path])

        server_config_filename = paths.server_config_path()
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)
            default_max_threads = svr_cfg['advanced_settings']['default_number_of_transfer_threads']

        default_buffer_size_in_bytes = 4 * (1024 ** 2)
        cases = [
            {
                'size':     0,
                'threads':  0
            },
            {
                'size':     34603008,
                'threads':  min(default_max_threads, (34603008 / default_buffer_size_in_bytes) + 1)
            }
        ]

        for case in cases:
            do_test(size=case['size'], threads=case['threads'])


    def test_parallel_put_does_not_generate_SYS_COPY_LEN_ERR_when_overwriting_large_file_with_small_file__issue_5505(self):
        file_40mb = os.path.join(tempfile.gettempdir(), '40mb.txt')
        lib.make_file(file_40mb, 40000000, 'arbitrary')

        file_50mb = os.path.join(tempfile.gettempdir(), '50mb.txt')
        lib.make_file(file_50mb, 50000000, 'arbitrary')

        data_object = 'foo'
        self.admin.assert_icommand(['iput', file_50mb, data_object])

        # The following command should not result in a SYS_COPY_LEN_ERR error.
        self.admin.assert_icommand(['iput', '-f', file_40mb, data_object])

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_data_transfer_for_overwrite(self):
        def do_test_failure_in_data_transfer_for_overwrite(self, file_size_in_bytes):
            target_resc_1 = 'otherresc'
            target_resc_2 = 'anotherresc'
            target_resc_vault_1 = os.path.join(self.user.local_session_dir, target_resc_1 + '_vault')
            target_resc_vault_2 = os.path.join(self.user.local_session_dir, target_resc_2 + '_vault')
            munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
            munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
            munge_resc = 'munge_resc'
            local_file = os.path.join(self.admin.local_session_dir, 'local_file')
            logical_path = os.path.join(self.admin.session_collection, 'foo')
            question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
                os.path.basename(logical_path), os.path.dirname(logical_path))

            try:
                for resc, vault, host in [(target_resc_1, target_resc_vault_1, test.settings.HOSTNAME_1),
                                          (target_resc_2, target_resc_vault_2, test.settings.HOSTNAME_2)]:
                    self.admin.assert_icommand(
                        ['iadmin', 'mkresc', resc, 'unixfilesystem',
                         ':'.join([host, vault])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                lib.make_dir_p(munge_mount)
                lib.make_dir_p(munge_target)
                assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

                self.admin.assert_icommand(
                    ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                     ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size_in_bytes)

                self.admin.assert_icommand(['iput', '-R', munge_resc, local_file, logical_path])
                self.admin.assert_icommand(['irepl', '-R', target_resc_1, logical_path])
                self.admin.assert_icommand(['irepl', '-R', target_resc_2, logical_path])
                print(self.admin.run_icommand(['ils', '-l', logical_path])[0])

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)

                # there should be 3 replicas...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 3)

                # ensure the all replicas are good
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 1)          # DATA_REPL_STATUS

                repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
                self.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_1),   target_resc_1) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_1), 1)          # DATA_REPL_STATUS

                repl_num_2, resc_name_2, repl_status_2 = out_lines[2].split()
                self.assertEqual(int(repl_num_2),    2)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_2),   target_resc_2) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_2), 1)          # DATA_REPL_STATUS

                # make attempts to write to this resource return an error code
                assert_command(['mungefsctl', '--operations', 'write', '--err_no=28'])

                self.admin.assert_icommand(
                    ['iput', '-f', '-R', munge_resc, local_file, logical_path],
                    'STDERR', 'UNIX_FILE_WRITE_ERR')

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)

                # there should be 3 replicas...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 3)

                # ensure the targeted replica is marked stale
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 0)          # DATA_REPL_STATUS

                # ensure the other replicas unlocked to their original statuses
                repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
                self.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_1),   target_resc_1) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_1), 1)          # DATA_REPL_STATUS

                repl_num_2, resc_name_2, repl_status_2 = out_lines[2].split()
                self.assertEqual(int(repl_num_2),    2)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_2),   target_resc_2) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_2), 1)          # DATA_REPL_STATUS

            finally:
                print(self.admin.run_icommand(['ils', '-l', logical_path])[0])
                self.admin.run_icommand(['irm', '-f', logical_path])
                assert_command(['mungefsctl', '--operations', 'write'])
                assert_command(['fusermount', '-u', munge_mount])
                self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])
                for resc in [target_resc_1, target_resc_2]:
                    self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        # run the tests for different file sizes (0 is not a valid size because rsDataObjWrite will not trip the error provided by mungefs)
        do_test_failure_in_data_transfer_for_overwrite(self, 1024)
        do_test_failure_in_data_transfer_for_overwrite(self, 40000000)

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_data_transfer_for_create(self):
        def do_test_failure_in_data_transfer_for_create(self, file_size_in_bytes):
            munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
            munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
            munge_resc = 'munge_resc'
            local_file = os.path.join(self.admin.local_session_dir, 'local_file')
            logical_path = os.path.join(self.admin.session_collection, 'foo')
            question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
                os.path.basename(logical_path), os.path.dirname(logical_path))

            try:
                lib.make_dir_p(munge_mount)
                lib.make_dir_p(munge_target)
                assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

                self.admin.assert_icommand(
                    ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                     ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size_in_bytes)

                # make attempts to write to this resource return an error code
                assert_command(['mungefsctl', '--operations', 'write', '--err_no=28'])

                self.admin.assert_icommand(
                    ['iput', '-f', '-R', munge_resc, local_file, logical_path],
                    'STDERR', 'UNIX_FILE_WRITE_ERR')

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)
                self.assertNotIn('CAT_NO_ROWS_FOUND', out)

                # there should be 1 replica...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 1)

                # ensure the targeted replica is marked stale
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 0)          # DATA_REPL_STATUS

            finally:
                print(self.admin.run_icommand(['ils', '-l', logical_path])[0])
                self.admin.run_icommand(['irm', '-f', logical_path])
                assert_command(['mungefsctl', '--operations', 'write'])
                assert_command(['fusermount', '-u', munge_mount])
                self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

        # run the tests for different file sizes (0 is not a valid size because rsDataObjWrite will not trip the error provided by mungefs)
        do_test_failure_in_data_transfer_for_create(self, 1024)
        do_test_failure_in_data_transfer_for_create(self, 40000000)

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_verification_for_overwrite(self):
        def do_test_failure_in_verification_for_overwrite(self, file_size_in_bytes):
            target_resc_1 = 'otherresc'
            target_resc_2 = 'anotherresc'
            target_resc_vault_1 = os.path.join(self.user.local_session_dir, target_resc_1 + '_vault')
            target_resc_vault_2 = os.path.join(self.user.local_session_dir, target_resc_2 + '_vault')
            munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
            munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
            munge_resc = 'munge_resc'
            local_file = os.path.join(self.admin.local_session_dir, 'local_file')
            logical_path = os.path.join(self.admin.session_collection, 'foo')
            question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
                os.path.basename(logical_path), os.path.dirname(logical_path))

            try:
                for resc, vault, host in [(target_resc_1, target_resc_vault_1, test.settings.HOSTNAME_1),
                                          (target_resc_2, target_resc_vault_2, test.settings.HOSTNAME_2)]:
                    self.admin.assert_icommand(
                        ['iadmin', 'mkresc', resc, 'unixfilesystem',
                         ':'.join([host, vault])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                lib.make_dir_p(munge_mount)
                lib.make_dir_p(munge_target)
                assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

                self.admin.assert_icommand(
                    ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                     ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size_in_bytes)

                self.admin.assert_icommand(['iput', '-K', '-R', munge_resc, local_file, logical_path])
                self.admin.assert_icommand(['irepl', '-R', target_resc_1, logical_path])
                self.admin.assert_icommand(['irepl', '-R', target_resc_2, logical_path])
                print(self.admin.run_icommand(['ils', '-L', logical_path])[0])

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)

                # there should be 3 replicas...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 3)

                # ensure the all replicas are good
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 1)          # DATA_REPL_STATUS

                repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
                self.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_1),   target_resc_1) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_1), 1)          # DATA_REPL_STATUS

                repl_num_2, resc_name_2, repl_status_2 = out_lines[2].split()
                self.assertEqual(int(repl_num_2),    2)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_2),   target_resc_2) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_2), 1)          # DATA_REPL_STATUS

                # make attempts to write to this resource return an error code
                assert_command(['mungefsctl', '--operations', 'write', '--corrupt_data'])

                self.admin.assert_icommand(
                    ['iput', '-f', '-K', '-R', munge_resc, local_file, logical_path],
                    'STDERR', 'USER_CHKSUM_MISMATCH')

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)

                # there should be 3 replicas...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 3)

                # ensure the targeted replica is marked stale
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 0)          # DATA_REPL_STATUS

                # ensure the other replicas unlocked to their original statuses
                repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
                self.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_1),   target_resc_1) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_1), 1)          # DATA_REPL_STATUS

                repl_num_2, resc_name_2, repl_status_2 = out_lines[2].split()
                self.assertEqual(int(repl_num_2),    2)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_2),   target_resc_2) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_2), 1)          # DATA_REPL_STATUS

            finally:
                print(self.admin.run_icommand(['ils', '-L', logical_path])[0])
                self.admin.run_icommand(['irm', '-f', logical_path])
                assert_command(['mungefsctl', '--operations', 'write'])
                assert_command(['fusermount', '-u', munge_mount])
                self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])
                for resc in [target_resc_1, target_resc_2]:
                    self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        # run the tests for different file sizes (0 is not valid because mungefs cannot corrupt data which is not there)
        do_test_failure_in_verification_for_overwrite(self, 1024)
        do_test_failure_in_verification_for_overwrite(self, 40000000)

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_verification_for_create(self):
        def do_test_failure_in_verification_for_create(self, file_size_in_bytes):
            munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
            munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
            munge_resc = 'munge_resc'
            local_file = os.path.join(self.admin.local_session_dir, 'local_file')
            logical_path = os.path.join(self.admin.session_collection, 'foo')
            question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
                os.path.basename(logical_path), os.path.dirname(logical_path))

            try:
                lib.make_dir_p(munge_mount)
                lib.make_dir_p(munge_target)
                assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

                self.admin.assert_icommand(
                    ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                     ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                assert_command(['mungefsctl', '--operations', 'read', '--corrupt_data'])

                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size_in_bytes)

                self.admin.assert_icommand(
                    ['iput', '-K', '-R', munge_resc, local_file, logical_path],
                    'STDERR', 'USER_CHKSUM_MISMATCH')

                # check on the replicas after the put...
                out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
                self.assertEqual(0, ec)
                self.assertEqual(len(err), 0)

                # there should be 1 replica...
                out_lines = out.splitlines()
                self.assertEqual(len(out_lines), 1)

                # ensure the new replica is marked stale
                repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
                self.assertEqual(int(repl_num_0),    0)          # DATA_REPL_NUM
                self.assertEqual(str(resc_name_0),   munge_resc) # DATA_RESC_NAME
                self.assertEqual(int(repl_status_0), 0)          # DATA_REPL_STATUS

            finally:
                self.admin.run_icommand(['irm', '-f', logical_path])
                assert_command(['mungefsctl', '--operations', 'write'])
                assert_command(['fusermount', '-u', munge_mount])
                self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

        # run the tests for different file sizes (0 is not valid because mungefs cannot corrupt data which is not there)
        do_test_failure_in_verification_for_create(self, 1024)
        do_test_failure_in_verification_for_create(self, 40000000)

    def test_force_iput_to_new_resource_results_in_error__issue_5575(self):
        def do_test_force_iput_to_new_resource_results_in_error__issue_5575(self, file_size_in_bytes):
            target_resc_1 = 'otherresc'
            target_resc_2 = 'anotherresc'
            target_resc_3 = 'yetanotherresc'
            target_resc_vault_1 = os.path.join(self.user.local_session_dir, target_resc_1 + '_vault')
            target_resc_vault_2 = os.path.join(self.user.local_session_dir, target_resc_2 + '_vault')
            target_resc_vault_3 = os.path.join(self.user.local_session_dir, target_resc_3 + '_vault')
            logical_path = os.path.join(self.user.session_collection, 'foo')
            local_file = os.path.join(self.user.local_session_dir, 'local_file')

            try:
                # create test resources
                for resc, vault, host in [(target_resc_1, target_resc_vault_1, test.settings.HOSTNAME_1),
                                          (target_resc_2, target_resc_vault_2, test.settings.HOSTNAME_2),
                                          (target_resc_3, target_resc_vault_3, test.settings.HOSTNAME_3)]:
                    self.admin.assert_icommand(
                        ['iadmin', 'mkresc', resc, 'unixfilesystem',
                         ':'.join([host, vault])], 'STDOUT_SINGLELINE', 'unixfilesystem')

                # create some test data if it's not already there
                if not os.path.exists(local_file):
                    lib.make_file(local_file, file_size_in_bytes)

                # put the test data into irods on a specific resource
                self.user.assert_icommand(['iput', '-R', target_resc_1, local_file, logical_path])
                self.user.assert_icommand(['irepl', '-R', target_resc_3, logical_path])

                def assert_object_is_good():
                    # make sure the object put at the beginning has its original replicas and they are good
                    out, err, rc = self.user.run_icommand(['iquest', '%s %s',
                        "select RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"
                        .format(os.path.basename(logical_path), os.path.dirname(logical_path))])
                    self.assertEqual(0, rc)
                    self.assertEqual(0, len(err))

                    results = out.splitlines()
                    self.assertEqual(2, len(results))
                    result_0 = results[0].split()
                    result_1 = results[1].split()

                    self.assertEqual(target_resc_1, result_0[0])
                    self.assertEqual(1, int(result_0[1]))
                    self.assertEqual(target_resc_3, result_1[0])
                    self.assertEqual(1, int(result_1[1]))

                assert_object_is_good()

                # TODO: This overwrites some existing replica but will not emit an error. Might change in future.
                # implicitly target default resource, which has no replica
                #self.user.assert_icommand(['iput', '-f', local_file, logical_path], 'STDERR', 'HIERARCHY_ERROR')
                #assert_object_is_good()

                # target a specific resource which has no replica
                self.user.assert_icommand(['iput', '-R', target_resc_2, '-f', local_file, logical_path], 'STDERR', 'HIERARCHY_ERROR')
                assert_object_is_good()

            finally:
                self.user.run_icommand(['irm', '-f', logical_path])
                for resc in [target_resc_1, target_resc_2, target_resc_3]:
                    self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        # run the tests for different file sizes
        do_test_force_iput_to_new_resource_results_in_error__issue_5575(self, 0)
        do_test_force_iput_to_new_resource_results_in_error__issue_5575(self, 1024)
        do_test_force_iput_to_new_resource_results_in_error__issue_5575(self, 40000000)


    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_checksum_is_erased_on_single_buffer_overwrite__issue_5496(self):
        self.do_test_issue_5496_impl(500)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_checksum_is_erased_on_parallel_overwrite__issue_5496(self):
        self.do_test_issue_5496_impl(40000000)

    # A helper function for testing changes that resolve issue 5496.
    def do_test_issue_5496_impl(self, file_size_in_bytes):
        data_object = 'foo'

        # Create a new data object.
        self.admin.assert_icommand(['istream', 'write', '-k', data_object], input='small amount of data')

        # Verify that the replica has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertGreater(len(checksum.strip()), len('sha2:'))

        # Overwrite the data object via a force-put.
        local_file = os.path.join(tempfile.gettempdir(), 'issue_5496.txt')
        lib.make_file(local_file, file_size_in_bytes, 'arbitrary')
        self.admin.assert_icommand(['iput', '-f', local_file, data_object])

        # Show that the replica no longer has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(len(checksum.strip()), 0)

    def test_iput_with_apostrophe_logical_path__issue_5759(self):
        """Test iput with apostrophes in the logical path.

        For each iput, the logical path will contain an apostrophe in either the collection
        name, data object name, both, or neither.
        """

        local_file = os.path.join(self.user.local_session_dir, 'test_iput_with_apostrophe_logical_path__issue_5759')
        lib.make_file(local_file, 1024, 'arbitrary')

        collection_names = ["collection", "collect'ion"]

        data_names = ["data_object", "data'_object"]

        for coll in collection_names:
            collection_path = os.path.join(self.user.session_collection, coll)

            self.user.assert_icommand(['imkdir', collection_path])

            for name in data_names:
                logical_path = os.path.join(collection_path, name)

                self.user.assert_icommand(['iput', local_file, logical_path])

                self.user.assert_icommand(['ils', '-l', logical_path], 'STDOUT', name)

            self.user.assert_icommand(['ils', '-l', collection_path], 'STDOUT', coll)

    def test_recursive_bulk_overwrite_is_not_allowed__issue_7110(self):
        filenames = ['file1', 'file2']
        bonus_filename = 'file3'

        dirname = 'bulkputme'
        directory_path = os.path.join(self.user.local_session_dir, dirname)

        collection_path = os.path.join(self.user.session_collection, dirname)
        logical_paths = [os.path.join(collection_path, f) for f in filenames]
        bonus_logical_path = os.path.join(collection_path, bonus_filename)

        random_resource = 'rand'
        ufs_resource_1 = 'ufs1'
        ufs_resource_2 = 'ufs2'

        try:
            # Create the two files so that we can perform the bulk put.
            os.mkdir(directory_path)
            for f in filenames:
                physical_path = os.path.join(directory_path, f)
                lib.make_arbitrary_file(physical_path, 2)

            # Set up random resource hierarchy such that only 1 replica will appear.
            lib.create_random_resource(self.admin, random_resource)
            lib.create_ufs_resource(self.admin, ufs_resource_1, hostname=test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, ufs_resource_2, hostname=test.settings.HOSTNAME_3)
            lib.add_child_resource(self.admin, random_resource, ufs_resource_1)
            lib.add_child_resource(self.admin, random_resource, ufs_resource_2)

            # Bulk put the directory ensure that the objects appear as expected.
            self.user.assert_icommand(
                ['iput', '-R', random_resource, '-br', directory_path, self.user.session_collection], 'STDOUT')

            for lp in logical_paths:
                self.assertFalse(lib.replica_exists(self.user, lp, 1))
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(lp), 0))

            # Add a bonus file to the directory so that the overwrite has something to do.
            lib.make_arbitrary_file(os.path.join(directory_path, bonus_filename), 2)

            # Bulk put the directory with force flag to overwrite and ensure that the action is disallowed.
            self.user.assert_icommand(
                ['iput', '-R', random_resource, '-brf', directory_path, self.user.session_collection],
                'STDERR', '-402000 USER_INCOMPATIBLE_PARAMS')

            # Ensure that no extra replicas are created.
            for lp in logical_paths:
                self.assertFalse(lib.replica_exists(self.user, lp, 1))
                self.assertEqual(str(1), lib.get_replica_status(self.user, os.path.basename(lp), 0))

            # Ensure that the overwrite did not occur as evidenced by the lack of the bonus file.
            self.assertFalse(lib.replica_exists(self.user, bonus_logical_path, 0))

        finally:
            self.user.assert_icommand(['ils', '-lr', self.user.session_collection], 'STDOUT') # debugging

            # Just in case, make sure any existing replicas are stale before removing. The issue results in replicas
            # being stuck in the intermediate status.
            for lp in logical_paths:
                self.admin.run_icommand(
                    ['iadmin', 'modrepl', 'logical_path', lp, 'replica_number', str(0), 'DATA_REPL_STATUS', str(0)])
                self.admin.run_icommand(
                    ['iadmin', 'modrepl', 'logical_path', lp, 'replica_number', str(1), 'DATA_REPL_STATUS', str(0)])

            self.user.run_icommand(['irm', '-rf', collection_path])

            self.admin.run_icommand(['iadmin', 'rmchildfromresc', random_resource, ufs_resource_1])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', random_resource, ufs_resource_2])
            self.admin.run_icommand(['iadmin', 'rmresc', random_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', ufs_resource_1])
            self.admin.run_icommand(['iadmin', 'rmresc', ufs_resource_2])

    def test_iput_link_option_ignores_symlinks__issue_5359(self):
        """Creates a directory to target for iput -r --link with good and bad symlinks which should all be ignored."""

        import shutil

        dirname = 'test_iput_link_option_ignores_broken_symlinks__issue_5359'
        collection_path = os.path.join(self.user.session_collection, dirname)

        # Put all the files/dirs and symlinks in the test dir so it all gets cleaned up "for free".
        put_dir = os.path.join(self.user.local_session_dir, dirname)

        # These are the broken symlink files and directories.
        deleted_file_path = os.path.join(put_dir, 'this_file_will_be_deleted')
        deleted_subdir_path = os.path.join(put_dir, 'this_dir_will_be_deleted')
        broken_symlink_file_path = os.path.join(put_dir, 'broken_symlink_file')
        broken_symlink_dir_path = os.path.join(put_dir, 'broken_symlink_dir')

        # These are the data objects and collections which shouldn't exist for the broken symlink files and directories.
        nonexistent_data_object_path = os.path.join(collection_path, os.path.basename(deleted_file_path))
        nonexistent_subcollection_path = os.path.join(collection_path, os.path.basename(deleted_subdir_path))
        broken_symlink_data_object_path = os.path.join(collection_path, os.path.basename(broken_symlink_file_path))
        broken_symlink_collection_path = os.path.join(collection_path, os.path.basename(broken_symlink_dir_path))

        # These are the good symlink files and directories. The symlink target file and subdirectory actually have to
        # exist outside of the target directory because the symlinks will be followed and it cannot make multiples of
        # the same data object or collection.
        good_symlink_target_file_path = os.path.join(os.path.dirname(put_dir), 'this_symlink_target_file_will_exist')
        good_symlink_target_dir_path = os.path.join(os.path.dirname(put_dir), 'this_symlink_target_dir_will_exist')
        good_symlink_file_path = os.path.join(put_dir, 'not_actually_broken_symlink_file')
        good_symlink_dir_path = os.path.join(put_dir, 'not_actually_broken_symlink_dir')

        # These are the data objects and collections which shouldn't exist for the good symlink files and directories.
        good_symlink_target_file_data_object_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_file_path))
        good_symlink_target_dir_subcollection_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_dir_path))
        good_symlink_data_object_path = os.path.join(collection_path, os.path.basename(good_symlink_file_path))
        good_symlink_collection_path = os.path.join(collection_path, os.path.basename(good_symlink_dir_path))

        # These are the normal files and directories. These will be added so that SOMETHING is in iRODS at the end.
        existent_file_path = os.path.join(put_dir, 'this_file_will_exist')
        existent_subdir_path = os.path.join(put_dir, 'this_dir_will_exist')

        # These are the data objects and collections which should exist for the normal files and directories.
        existent_data_object_path = os.path.join(collection_path, os.path.basename(existent_file_path))
        existent_subcollection_path = os.path.join(collection_path, os.path.basename(existent_subdir_path))

        file_size = 1024 # This value has no particular significance.

        try:
            # Create the directory which will contain all the broken links...
            os.mkdir(put_dir)
            self.assertTrue(os.path.exists(put_dir))

            # Create subdirectories and symlinks to the subdirectories.
            os.mkdir(deleted_subdir_path)
            self.assertTrue(os.path.exists(deleted_subdir_path))
            os.mkdir(existent_subdir_path)
            self.assertTrue(os.path.exists(existent_subdir_path))
            os.mkdir(good_symlink_target_dir_path)
            self.assertTrue(os.path.exists(good_symlink_target_dir_path))
            os.symlink(deleted_subdir_path, broken_symlink_dir_path)
            self.assertTrue(os.path.exists(broken_symlink_dir_path))
            os.symlink(good_symlink_target_dir_path, good_symlink_dir_path)
            self.assertTrue(os.path.exists(good_symlink_dir_path))

            # Create files and symlinks to the files.
            lib.make_file(deleted_file_path, file_size)
            self.assertTrue(os.path.exists(deleted_file_path))
            lib.make_file(good_symlink_target_file_path, file_size)
            self.assertTrue(os.path.exists(good_symlink_target_file_path))
            lib.make_file(existent_file_path, file_size)
            self.assertTrue(os.path.exists(existent_file_path))
            os.symlink(deleted_file_path, broken_symlink_file_path)
            self.assertTrue(os.path.exists(broken_symlink_file_path))
            os.symlink(good_symlink_target_file_path, good_symlink_file_path)
            self.assertTrue(os.path.exists(good_symlink_file_path))

            # Now break the symlinks that are supposed to be broken by removing their target files/directories.
            # Note: os.path.lexists returns True for broken symbolic links.
            shutil.rmtree(deleted_subdir_path)
            self.assertFalse(os.path.exists(deleted_subdir_path))
            self.assertTrue(os.path.lexists(broken_symlink_dir_path))

            os.remove(deleted_file_path)
            self.assertFalse(os.path.exists(deleted_file_path))
            self.assertTrue(os.path.lexists(broken_symlink_file_path))

            # Run iput to upload the directory. Use the --link option to instruct that symlinks are to be ignored,
            # broken or not.
            self.user.assert_icommand(['iput', '--link', '-r', put_dir, collection_path])

            # Confirm that the main collection was created - no funny business there.
            self.assertTrue(lib.collection_exists(self.user, collection_path))

            # Confirm that a subcollection was created for the existent subcollection, but that no subcollection was
            # created for the symlink or the collection to which it points because iput was supposed to ignore symlinks.
            self.assertTrue(lib.collection_exists(self.user, existent_subcollection_path))
            self.assertFalse(lib.collection_exists(self.user, good_symlink_collection_path))
            self.assertFalse(lib.collection_exists(self.user, good_symlink_target_dir_subcollection_path))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_collection_path, 0))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_target_dir_subcollection_path, 0))

            # Confirm that a subcollection was not created for the nonexistent subdirectory, and that no subcollection
            # was created for the symlink because iput was supposed to ignore symlinks. Also make sure a data object
            # was not created.
            self.assertFalse(lib.collection_exists(self.user, nonexistent_subcollection_path))
            self.assertFalse(lib.collection_exists(self.user, broken_symlink_collection_path))
            self.assertFalse(lib.replica_exists(self.user, broken_symlink_collection_path, 0))

            # Confirm that a data object was not created for the nonexistent file, and that no data object was created
            # for the symlink because iput was supposed to ignore symlinks.
            self.assertFalse(lib.replica_exists(self.user, nonexistent_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user, broken_symlink_data_object_path, 0))

            # Confirm that a data object was created for the existent file, but that no data object was created for the
            # symlink because iput was supposed to ignore symlinks.
            self.assertTrue(lib.replica_exists(self.user, existent_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_target_file_data_object_path, 0))

        finally:
            self.user.assert_icommand(['ils', '-lr'], 'STDOUT', self.user.session_collection) # debugging

    def test_iput_does_not_ignore_symlinks_by_default__issue_5359(self):
        """Creates a directory to target for iput -r with good symlinks, all of which should be sync'd."""

        dirname = 'test_iput_does_not_ignore_symlinks_by_default__issue_5359'
        collection_path = os.path.join(self.user.session_collection, dirname)

        # Put all the files/dirs and symlinks in the test dir so it all gets cleaned up "for free".
        put_dir = os.path.join(self.user.local_session_dir, dirname)

        # These are the good symlink files and directories. The symlink target file and subdirectory actually have to
        # exist outside of the target directory because the symlinks will be followed and it cannot make multiples of
        # the same data object or collection.
        good_symlink_target_file_path = os.path.join(os.path.dirname(put_dir), 'this_symlink_target_file_will_exist')
        good_symlink_target_dir_path = os.path.join(os.path.dirname(put_dir), 'this_symlink_target_dir_will_exist')
        good_symlink_file_path = os.path.join(put_dir, 'not_actually_broken_symlink_file')
        good_symlink_dir_path = os.path.join(put_dir, 'not_actually_broken_symlink_dir')

        # These are the data objects and collections which shouldn't exist for the good symlink target files and
        # directories. The symlink files and directories themselves will be the names used for the data objects and
        # collections.
        good_symlink_target_file_data_object_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_file_path))
        good_symlink_target_dir_subcollection_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_dir_path))

        # These are the data objects and collections which should exist for the good symlink files and directories
        # because, while the symlinks will be followed when they are sync'd for the actual data, the symlink file names
        # will be used.
        good_symlink_data_object_path = os.path.join(collection_path, os.path.basename(good_symlink_file_path))
        good_symlink_collection_path = os.path.join(collection_path, os.path.basename(good_symlink_dir_path))

        # These are the normal files and directories.
        existent_file_path = os.path.join(put_dir, 'this_file_will_exist')
        existent_subdir_path = os.path.join(put_dir, 'this_dir_will_exist')

        # These are the data objects and collections which should exist for the normal files and directories.
        existent_data_object_path = os.path.join(collection_path, os.path.basename(existent_file_path))
        existent_subcollection_path = os.path.join(collection_path, os.path.basename(existent_subdir_path))

        file_size = 1024 # This value has no particular significance.

        try:
            # Create the directory which will contain all the broken links...
            os.mkdir(put_dir)
            self.assertTrue(os.path.exists(put_dir))

            # Create subdirectories and symlinks to the subdirectories.
            os.mkdir(existent_subdir_path)
            self.assertTrue(os.path.exists(existent_subdir_path))
            os.mkdir(good_symlink_target_dir_path)
            self.assertTrue(os.path.exists(good_symlink_target_dir_path))
            os.symlink(good_symlink_target_dir_path, good_symlink_dir_path)
            self.assertTrue(os.path.exists(good_symlink_dir_path))

            # Create files and symlinks to the files.
            lib.make_file(good_symlink_target_file_path, file_size)
            self.assertTrue(os.path.exists(good_symlink_target_file_path))
            lib.make_file(existent_file_path, file_size)
            self.assertTrue(os.path.exists(existent_file_path))
            os.symlink(good_symlink_target_file_path, good_symlink_file_path)
            self.assertTrue(os.path.exists(good_symlink_file_path))

            # Run iput to upload the directory.
            self.user.assert_icommand(['iput', '-r', put_dir, collection_path])

            # Confirm that the main collection was created - no funny business there.
            self.assertTrue(lib.collection_exists(self.user, collection_path))

            # Confirm that a subcollection was created for the existent subdirectory and the symlinked subdirectory, but
            # that no subcollection was created for the symlink itself because it should have been followed.
            self.assertTrue(lib.collection_exists(self.user, existent_subcollection_path))
            self.assertTrue(lib.collection_exists(self.user, good_symlink_collection_path))
            self.assertFalse(lib.collection_exists(self.user, good_symlink_target_dir_subcollection_path))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_target_dir_subcollection_path, 0))

            # Confirm that a data object was created for the existent file and the symlinked file, but that no data
            # object was created for the symlink itself because it should have been followed.
            self.assertTrue(lib.replica_exists(self.user, existent_data_object_path, 0))
            self.assertTrue(lib.replica_exists(self.user, good_symlink_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user, good_symlink_target_file_data_object_path, 0))

        finally:
            self.user.assert_icommand(['ils', '-lr'], 'STDOUT', self.user.session_collection) # debugging

    def test_iput_ignore_symlinks_option_is_an_alias_of_the_link_option__issue_7537(self):
        # Create a file.
        test_file_basename = 'iput_issue_7537.txt'
        test_file = f'{self.user.local_session_dir}/{test_file_basename}'
        with open(test_file, 'w') as f:
            f.write('This file will be ignored by iput due to the symlink.')

        # Create a directory for the symlink.
        # The symlink file must be placed in a separate directory due to the recursive flag for iput.
        symlink_dir_basename = 'iput_issue_7537_symlink_dir'
        symlink_dir = f'{self.user.local_session_dir}/{symlink_dir_basename}'
        os.mkdir(symlink_dir)

        # Create a symlink to the file.
        symlink_file_basename = f'{test_file_basename}.symlink'
        symlink_file = f'{symlink_dir}/{symlink_file_basename}'
        os.symlink(test_file, symlink_file)
        self.assertTrue(os.path.exists(symlink_file))

        # Try to upload the symlink file into iRODS.
        self.user.assert_icommand(['iput', '--ignore-symlinks', symlink_file])
        self.user.assert_icommand(['ils', '-lr'], 'STDOUT') # Debugging

        # Show no data object exists, proving the symlink file was ignored.
        logical_path = f'{self.user.session_collection}/{symlink_file_basename}'
        self.assertFalse(lib.replica_exists(self.user, logical_path, 0))

        # Try to upload the symlink file again using the recursive flag and the parent directory.
        self.user.assert_icommand(['iput', '--ignore-symlinks', '-r', symlink_dir])
        self.user.assert_icommand(['ils', '-lr'], 'STDOUT') # Debugging

        # Show no data object exists, proving the symlink file was ignored.
        logical_path = f'{self.user.session_collection}/{symlink_dir_basename}/{symlink_file_basename}'
        self.assertFalse(lib.replica_exists(self.user, logical_path, 0))

class test_iput_with_checksums(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    def setUp(self):
        super(test_iput_with_checksums, self).setUp()
        self.admin = self.admin_sessions[0]

        self.resource = 'test_iput_with_checksums_resource'
        self.vault_path = os.path.join(self.admin.local_session_dir, self.resource + '_storage_vault')
        self.admin.assert_icommand(
            ['iadmin', 'mkresc', self.resource, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, self.vault_path])],
            'STDOUT', test.settings.HOSTNAME_2)

    def tearDown(self):
        self.admin.run_icommand(['irm', '-f', self.object_name])
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource])
        super(test_iput_with_checksums, self).tearDown()

    def test_zero_length_put(self):
        self.object_name = 'test_zero_length_put'
        self.run_tests(self.object_name, 0)

    def test_small_put(self):
        self.object_name = 'test_small_put'
        self.run_tests(self.object_name, 512)

    @unittest.skip('parallel transfer not yet working')
    def test_large_put(self):
        self.object_name = 'test_large_put'
        self.run_tests(self.object_name, 5120000)

    def get_checksum(self, object_name):
        iquest_result,_,ec = self.admin.run_icommand(['iquest', '%s', '"select DATA_CHECKSUM where DATA_NAME = \'{}\'"'.format(object_name)])
        self.assertEqual(0, ec)

        print(iquest_result)
        self.assertEqual(1, len(iquest_result.splitlines()))

        return '' if u'\n' == iquest_result else iquest_result

    def run_tests(self, filename, file_size):
        lib.make_file(filename, file_size, 'arbitrary')
        filename_tmp = filename + '_tmp'
        self.admin.assert_icommand(['iput', '-K', filename, filename_tmp])
        expected_checksum = self.get_checksum(filename_tmp)

        try:
            self.new_register(filename, expected_checksum)
            self.new_verify(filename, expected_checksum)
            self.overwrite_register_good_no_checksum(filename, expected_checksum)
            self.overwrite_register_good_with_checksum(filename, expected_checksum)
            self.overwrite_register_stale_no_checksum(filename, expected_checksum)
            self.overwrite_register_stale_with_checksum(filename, expected_checksum)
            self.overwrite_verify_good_no_checksum(filename, expected_checksum)
            self.overwrite_verify_good_with_checksum(filename, expected_checksum)
            self.overwrite_verify_stale_no_checksum(filename, expected_checksum)
            self.overwrite_verify_stale_with_checksum(filename, expected_checksum)
        finally:
            os.unlink(filename)
            self.admin.run_icommand(['irm', '-f', filename_tmp])

    def new_register(self, filename, expected_checksum):
        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def new_verify(self, filename, expected_checksum):
        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_register_good_no_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), '')

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_register_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-k', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_no_checksum_flag_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_register_stale_no_checksum(self, filename, expected_checksum):
        pass

    def overwrite_register_stale_with_checksum(self, filename, expected_checksum):
        pass

    def overwrite_verify_good_no_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), '')

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])

    def overwrite_verify_good_with_checksum(self, filename, expected_checksum):
        filename_tmp = filename + '_for_overwrite'
        lib.make_file(filename_tmp, 4444, 'arbitrary')
        self.admin.assert_icommand(['iput', '-R', self.resource, '-K', filename_tmp])
        tmp_checksum = self.get_checksum(filename_tmp)

        try:
            self.admin.assert_icommand(['iput', '-R', self.resource, '-k', filename_tmp, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), tmp_checksum)

            self.admin.assert_icommand(['iput', '-R', self.resource, '-f', '-K', filename, self.object_name])
            self.assertEqual(self.get_checksum(self.object_name), expected_checksum)
        finally:
            os.unlink(filename_tmp)
            self.admin.assert_icommand(['irm', '-f', self.object_name])
            self.admin.assert_icommand(['irm', '-f', filename_tmp])

    def overwrite_verify_stale_no_checksum(self, filename, expected_checksum):
        pass

    def overwrite_verify_stale_with_checksum(self, filename, expected_checksum):
        pass
