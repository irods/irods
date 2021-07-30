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

