from __future__ import print_function
import inspect
import json
import os
import shutil
import sys
import textwrap

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import lib
from . import session
from .. import core_file
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..test.command import assert_command

class Test_Icp(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Icp'
    def setUp(self):
        super(Test_Icp, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Icp, self).tearDown()

    def test_icp_closes_file_descriptors__4862(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acSetRescSchemeForCreate {
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acSetRescSchemeForCreate(rule_args, callback, rei):
                    pass
            ''')
        }
        test_dir_path = 'test_icp_closes_file_descriptors__4862'
        logical_put_path = 'iput_large_dir'
        logical_cp_path = 'icp_large_dir'
        try:
            with lib.file_backed_up(self.user._environment_file_path):
                del self.user.environment_file_contents['irods_default_resource']
                with core_file.temporary_core_file(self.plugin_name) as core:
                    core.add_rule(pep_map[self.plugin_name])
                    IrodsController().reload_configuration()

                    lib.make_large_local_tmp_dir(test_dir_path, 1024, 1024)

                    self.user.assert_icommand(['iput', os.path.join(test_dir_path, 'junk0001')], 'STDERR', 'SYS_INVALID_RESC_INPUT')

                    self.user.assert_icommand(['iput', '-R', 'demoResc', '-r', test_dir_path, logical_put_path])
                    _,_,err = self.user.assert_icommand(['icp', '-r', logical_put_path, logical_cp_path], 'STDERR', 'SYS_INVALID_RESC_INPUT')
                    self.assertNotIn('SYS_OUT_OF_FILE_DESC', err, 'SYS_OUT_OF_FILE_DESC found in output.')
        finally:
            shutil.rmtree(test_dir_path, ignore_errors=True)

            IrodsController().reload_configuration()

    def test_multithreaded_icp__issue_5478(self):
        def do_test(size, threads):
            file_name = 'test_multithreaded_icp__issue_5478'
            local_file = os.path.join(self.user.local_session_dir, file_name)
            dest_path = os.path.join(self.user.session_collection, file_name)
            another_dest_path = os.path.join(self.user.session_collection, file_name + '_copy')

            try:
                lib.make_file(local_file, size)

                self.user.assert_icommand(['iput', local_file, dest_path])

                cp_thread_count = self.user.run_icommand(['icp', '-v', dest_path, another_dest_path])[0].split('|')[2].split()[0]

                self.assertEqual(int(cp_thread_count), threads)

            finally:
                self.user.assert_icommand(['irm', '-f', dest_path])
                self.user.assert_icommand(['irm', '-f', another_dest_path])

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

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_failure_in_data_transfer_for_write(self):
        munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
        munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
        munge_resc = 'munge_resc'
        local_file = os.path.join(self.admin.local_session_dir, 'local_file')
        source_logical_path = os.path.join(self.admin.session_collection, 'foo')
        dest_logical_path = os.path.join(self.admin.session_collection, 'goo')

        def get_question(logical_path):
            return "select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'".format(
                os.path.basename(logical_path), os.path.dirname(logical_path))

        try:
            lib.make_dir_p(munge_mount)
            lib.make_dir_p(munge_target)
            assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

            self.admin.assert_icommand(
                ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                 ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

            assert_command(['mungefsctl', '--operations', 'write', '--corrupt_data'])

            if not os.path.exists(local_file):
                lib.make_file(local_file, 1024)

            self.admin.assert_icommand(['iput', '-K', local_file, source_logical_path])
            self.admin.assert_icommand(['ils', '-l', source_logical_path], 'STDOUT', self.admin.default_resource)

            self.admin.assert_icommand(
                ['icp', '-K', '-R', munge_resc, source_logical_path, dest_logical_path],
                'STDERR', 'USER_CHKSUM_MISMATCH')

            # check on the source object after the copy...
            out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', get_question(source_logical_path)])
            self.assertEqual(0, ec)
            self.assertEqual(len(err), 0)

            # there should be 1 replica...
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure the original object is in tact
            repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
            self.assertEqual(int(repl_num_0),    0)                             # DATA_REPL_NUM
            self.assertEqual(str(resc_name_0),   self.admin.default_resource)   # DATA_RESC_NAME
            self.assertEqual(int(repl_status_0), 1)                             # DATA_REPL_STATUS

            # check on the destination object after the copy...
            out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', get_question(dest_logical_path)])
            self.assertEqual(0, ec)
            self.assertEqual(len(err), 0)

            # there should be 1 replica...
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure the new object is marked stale due to failure
            repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
            self.assertEqual(int(repl_num_0),    0)             # DATA_REPL_NUM
            self.assertEqual(str(resc_name_0),   munge_resc)    # DATA_RESC_NAME
            self.assertEqual(int(repl_status_0), 0)             # DATA_REPL_STATUS

        finally:
            self.admin.assert_icommand(['ils', '-l', dest_logical_path], 'STDOUT', munge_resc)
            self.admin.run_icommand(['irm', '-f', source_logical_path])
            self.admin.run_icommand(['irm', '-f', dest_logical_path])
            assert_command(['mungefsctl', '--operations', 'write'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

    def test_icp_with_apostrophe_logical_path__issue_5759(self):
        """Test icp with apostrophes in the logical path.

        For each icp, the logical path of the source and destination data objects, will contain
        an apostrophe in either the collection name, data object name, both, or neither.
        """

        local_file = os.path.join(self.user.local_session_dir, 'test_icp_with_apostrophe_logical_path__issue_5759')
        lib.make_file(local_file, 1024, 'arbitrary')

        collection_names = ["collection", "collect'ion"]

        data_names = ["data_object", "data'_object"]

        for source_coll in collection_names:
            source_collection_path = os.path.join(self.user.session_collection, 'source', source_coll)
            self.user.assert_icommand(['imkdir', '-p', source_collection_path])

            for destination_coll in collection_names:
                destination_collection_path = os.path.join(self.user.session_collection, 'destination', destination_coll)
                self.user.assert_icommand(['imkdir', '-p', destination_collection_path])

                for source_name in data_names:
                    source_logical_path = os.path.join(source_collection_path, source_name)
                    self.user.assert_icommand(['iput', local_file, source_logical_path])
                    self.user.assert_icommand(['ils', '-l', source_collection_path], 'STDOUT', source_name)

                    for destination_name in data_names:
                        destination_logical_path = os.path.join(destination_collection_path, destination_name)
                        print('source=[{0}], destination=[{1}]'.format(source_logical_path, destination_logical_path))
                        self.user.assert_icommand(['icp', source_logical_path, destination_logical_path])
                        self.user.assert_icommand(['ils', '-l', destination_collection_path], 'STDOUT', destination_name)

                    self.user.assert_icommand(['irm', '-rf', destination_collection_path])
                    self.user.assert_icommand(['imkdir', '-p', destination_collection_path])

                self.user.assert_icommand(['irm', '-rf', destination_collection_path])

                self.user.assert_icommand(['irm', '-rf', source_collection_path])
                self.user.assert_icommand(['imkdir', '-p', source_collection_path])

    def test_icp_does_not_break_when_data_object_name_contains_certain_character_sequence__issue_4983(self):
        data_object = "' and '"
        self.user.assert_icommand(['itouch', data_object])
        self.user.assert_icommand(['ils', data_object], 'STDOUT', [data_object])
        new_data_object = 'issue_4983'
        self.user.assert_icommand(['icp', data_object, new_data_object])
        self.user.assert_icommand(['ils', new_data_object], 'STDOUT', [new_data_object])

class test_overwriting(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.user = session.mkuser_and_return_session('rodsuser', 'alice', 'apass', lib.get_hostname())

        self.target_resource = 'target_resource'
        self.other_resource = 'other_resource'

        with session.make_session_for_existing_admin() as admin_session:
            lib.create_ufs_resource(admin_session, self.target_resource, hostname=test.settings.HOSTNAME_2)
            lib.create_ufs_resource(admin_session, self.other_resource, hostname=test.settings.HOSTNAME_3)

    @classmethod
    def tearDownClass(self):
        self.user.__exit__()

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand(['iadmin', 'rmuser', self.user.username])
            lib.remove_resource(admin_session, self.target_resource)
            lib.remove_resource(admin_session, self.other_resource)

    def test_overwrites_replica_on_target_resource__issue_6497(self):
        object_name = 'test_overwrites_replica_on_target_resource__issue_6497'
        other_object_name = 'test_overwrites_replica_on_target_resource__issue_6497_other'
        logical_path = os.path.join(self.user.session_collection, object_name)
        other_logical_path = os.path.join(self.user.session_collection, other_object_name)

        original_content = 'goin down to cowtown'
        new_content = 'gonna see the cow beneath the sea'

        try:
            # Make an object and replicate to some target resource...
            self.user.assert_icommand(['istream', 'write', logical_path], 'STDOUT', input=original_content)
            self.user.assert_icommand(['irepl', '-R', self.target_resource, logical_path])

            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.target_resource))
            self.assertFalse(lib.replica_exists_on_resource(self.user, logical_path, self.other_resource))

            # Make a new object with different content and copy over the first object (the copy should succeed).
            self.user.assert_icommand(['istream', 'write', other_logical_path], 'STDOUT', input=new_content)
            self.user.assert_icommand(['icp', '-f', '-R', self.target_resource, other_logical_path, logical_path])

            # Assert that the copy occurred and the existing replicas have been updated appropriately.
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.target_resource))
            self.assertFalse(lib.replica_exists_on_resource(self.user, logical_path, self.other_resource))

            self.assertEqual(
                str(0), lib.get_replica_status_for_resource(self.user, logical_path, self.user.default_resource))
            self.assertEqual(str(1), lib.get_replica_status_for_resource(self.user, logical_path, self.target_resource))

            self.assertEqual(
                original_content,
                self.user.assert_icommand(
                    ['istream', '-R', self.user.default_resource, 'read', logical_path], 'STDOUT')[1].strip())

            self.assertEqual(
                new_content,
                self.user.assert_icommand(
                    ['istream', '-R', self.target_resource, 'read', logical_path], 'STDOUT')[1].strip())

        finally:
            self.user.assert_icommand(['irm', '-f', logical_path])
            self.user.assert_icommand(['irm', '-f', other_logical_path])

    def test_overwrite_fails_when_target_resource_has_no_replica__issue_6497(self):
        object_name = 'test_overwrite_fails_when_target_resource_has_no_replica__issue_6497'
        other_object_name = 'test_overwrite_fails_when_target_resource_has_no_replica__issue_6497_other'
        logical_path = os.path.join(self.user.session_collection, object_name)
        other_logical_path = os.path.join(self.user.session_collection, other_object_name)

        original_content = "you're older than you've ever been"
        new_content = "and now you're even older"

        try:
            # Make an object and replicate to some target resource...
            self.user.assert_icommand(['istream', 'write', logical_path], 'STDOUT', input=original_content)
            self.user.assert_icommand(['irepl', '-R', self.target_resource, logical_path])

            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.target_resource))
            self.assertFalse(lib.replica_exists_on_resource(self.user, logical_path, self.other_resource))

            # Make a new object with different content and attempt to copy over the first object (the copy should fail).
            self.user.assert_icommand(['istream', 'write', other_logical_path], 'STDOUT', input=new_content)
            self.user.assert_icommand(
                ['icp', '-f', '-R', self.other_resource, other_logical_path, logical_path],
                'STDERR', '-1803000 HIERARCHY_ERROR')

            # Assert that no copy occurred and the existing replicas remain untouched.
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.target_resource))
            self.assertFalse(lib.replica_exists_on_resource(self.user, logical_path, self.other_resource))

            self.assertEqual(
                str(1), lib.get_replica_status_for_resource(self.user, logical_path, self.user.default_resource))
            self.assertEqual(str(1), lib.get_replica_status_for_resource(self.user, logical_path, self.target_resource))

            self.assertEqual(
                original_content,
                self.user.assert_icommand(
                    ['istream', '-R', self.user.default_resource, 'read', logical_path], 'STDOUT')[1].strip())

            self.assertEqual(
                original_content,
                self.user.assert_icommand(
                    ['istream', '-R', self.target_resource, 'read', logical_path], 'STDOUT')[1].strip())

        finally:
            self.user.assert_icommand(['irm', '-f', logical_path])
            self.user.assert_icommand(['irm', '-f', other_logical_path])

    def test_overwrite_fails_when_source_path_matches_destination_path(self):
        object_name = 'test_overwrite_fails_when_source_path_matches_destination_path__issue_6497'
        logical_path = os.path.join(self.user.session_collection, object_name)

        original_content = 'we will copy the object over itself!'

        try:
            # Make an object...
            self.user.assert_icommand(['istream', 'write', logical_path], 'STDOUT', input=original_content)

            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))

            # Attempt to overwrite the object with itself -- this should fail.
            self.user.assert_icommand(
                ['icp', '-f', logical_path, logical_path], 'STDERR', '-317000 USER_INPUT_PATH_ERR')

            # Assert that no copy occurred and the existing replica remains untouched.
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))

            self.assertEqual(
                str(1), lib.get_replica_status_for_resource(self.user, logical_path, self.user.default_resource))

            self.assertEqual(
                original_content,
                self.user.assert_icommand(
                    ['istream', 'read', logical_path], 'STDOUT')[1].strip())

        finally:
            self.user.assert_icommand(['irm', '-f', logical_path])

    def test_overwrite_fails_when_force_flag_is_not_specified(self):
        object_name = 'test_overwrite_fails_when_force_flag_is_not_specified__issue_6497'
        other_object_name = 'test_overwrite_fails_when_force_flag_is_not_specified__issue_6497_other'
        logical_path = os.path.join(self.user.session_collection, object_name)
        other_logical_path = os.path.join(self.user.session_collection, other_object_name)

        original_content = 'you should always see this'
        new_content = 'you should never see this'

        try:
            # Make an object...
            self.user.assert_icommand(['istream', 'write', logical_path], 'STDOUT', input=original_content)

            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))

            # Make a new object with different content and attempt to copy over the first object (the copy should fail).
            self.user.assert_icommand(['istream', 'write', other_logical_path], 'STDOUT', input=new_content)
            self.user.assert_icommand(['icp', other_logical_path, logical_path],
                                      'STDERR', '-312000 OVERWRITE_WITHOUT_FORCE_FLAG')

            # Assert that no copy occurred and the existing replica remains untouched.
            self.assertTrue(lib.replica_exists_on_resource(self.user, logical_path, self.user.default_resource))

            self.assertEqual(
                str(1), lib.get_replica_status_for_resource(self.user, logical_path, self.user.default_resource))

            self.assertEqual(
                original_content,
                self.user.assert_icommand(
                    ['istream', 'read', logical_path], 'STDOUT')[1].strip())

        finally:
            self.user.assert_icommand(['irm', '-f', logical_path])
            self.user.assert_icommand(['irm', '-f', other_logical_path])
