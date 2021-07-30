import copy
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
import json
import time

import replica_status_test
from . import session
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .resource_suite import ResourceBase
from ..test.command import assert_command

def assert_two_replicas_where_original_is_good_and_new_is_stale(cls, logical_path, original_resc, munge_resc):
    question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"'''.format(
        os.path.basename(logical_path), os.path.dirname(logical_path))

    # check on the replicas after the phymv...
    out, err, ec = cls.admin.run_icommand(['iquest', '%s %s %s', question])
    cls.assertEqual(0, ec)
    cls.assertEqual(len(err), 0)

    # there should be 2 replicas now...
    out_lines = out.splitlines()
    cls.assertEqual(len(out_lines), 2)

    # ensure the original replica remains in tact (important!)
    repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
    cls.assertEqual(int(repl_num_0),    0)             # DATA_REPL_NUM
    cls.assertEqual(str(resc_name_0),   original_resc) # DATA_RESC_NAME
    cls.assertEqual(int(repl_status_0), 1)             # DATA_REPL_STATUS

    # ensure the new replica is marked stale
    repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
    cls.assertEqual(int(repl_num_1),    1)          # DATA_REPL_NUM
    cls.assertEqual(str(resc_name_1),   munge_resc) # DATA_RESC_NAME
    cls.assertEqual(int(repl_status_1), 0)          # DATA_REPL_STATUS

class Test_iPhymv(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iPhymv, self).setUp()

        irods_config = IrodsConfig()

        self.resource_1 = 'unix1Resc'
        self.resource_2 = 'unix2Resc'

        self.admin.assert_icommand("iadmin mkresc randResc random", 'STDOUT_SINGLELINE', 'random')

        self.admin.assert_icommand(
            ['iadmin', 'mkresc', self.resource_1, 'unixfilesystem',
                ':'.join([
                    test.settings.HOSTNAME_1, os.path.join(irods_config.irods_directory, 'unix1RescVault')
                ])
            ], 'STDOUT_SINGLELINE', 'unixfilesystem')

        self.admin.assert_icommand(
            ['iadmin', 'mkresc', self.resource_2, 'unixfilesystem',
                ':'.join([
                    test.settings.HOSTNAME_2, os.path.join(irods_config.irods_directory, 'unix2RescVault')
                ])
            ], 'STDOUT_SINGLELINE', 'unixfilesystem')

        self.admin.assert_icommand("iadmin addchildtoresc randResc unix1Resc")
        self.admin.assert_icommand("iadmin addchildtoresc randResc unix2Resc")


    def tearDown(self):
        self.admin.assert_icommand("iadmin rmchildfromresc randResc unix2Resc")
        self.admin.assert_icommand("iadmin rmchildfromresc randResc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc unix2Resc")
        self.admin.assert_icommand("iadmin rmresc unix1Resc")
        self.admin.assert_icommand("iadmin rmresc randResc")
        super(Test_iPhymv, self).tearDown()

    def get_source_resource(self, object_path):
        return self.admin.run_icommand(['iquest', '%s',
            '''"select RESC_NAME where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(
                os.path.dirname(object_path), os.path.basename(object_path))])[0].strip().strip()

    def get_destination_resource(self, source_resc):
        return self.resource_2 if source_resc == self.resource_1 else self.resource_1

    def get_source_and_destination_resources(self, object_path):
        source_resource = self.get_source_resource(object_path)
        destination_resource = self.get_destination_resource(source_resource)
        return (source_resource, destination_resource)

    def test_iphymv_to_child_resource__2933(self):
        file_name = 'file'
        filepath = os.path.join(self.user0.local_session_dir, file_name)
        lib.make_file(filepath, 1)
        dest_path = os.path.join(self.user0.session_collection, file_name)

        question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(os.path.dirname(dest_path), os.path.basename(dest_path))

        try:
            # put a new data object
            self.user0.assert_icommand(['iput', '-f', '-R', 'randResc', filepath, dest_path])
            self.user0.assert_icommand(['ils', '-L', dest_path], 'STDOUT_SINGLELINE', 'randResc')
            source_resc, dest_resc = self.get_source_and_destination_resources(dest_path)

            # get information from catalog about the new object
            out, err, ec = self.user0.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)

            # there should only be 1 replica to worry about
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure replica information is correct in the catalog
            repl_num, resc_name, repl_status = out_lines[0].split()
            self.assertEqual(int(repl_num),    0)           # DATA_REPL_NUM
            self.assertEqual(str(resc_name),   source_resc) # DATA_RESC_NAME
            self.assertEqual(int(repl_status), 1)           # DATA_REPL_STATUS

            # move the replica to the other resource, targeting child resources
            self.user0.assert_icommand(['iphymv', '-S', source_resc, '-R', dest_resc, dest_path])

            out, err, ec = self.user0.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)

            # there should only be 1 replica to worry about
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure moved replica information is correct in the catalog
            repl_num, resc_name, repl_status = out_lines[0].split()
            self.assertEqual(int(repl_num),    0)           # DATA_REPL_NUM
            self.assertEqual(str(resc_name),   dest_resc)   # DATA_RESC_NAME
            self.assertEqual(int(repl_status), 1)           # DATA_REPL_STATUS

        finally:
            self.user0.assert_icommand(['irm', '-f', dest_path])

    def test_iphymv_to_resc_hier__2933(self):
        file_name = 'file'
        root_resc = 'randResc'
        filepath = os.path.join(self.user0.local_session_dir, file_name)
        lib.make_file(filepath, 1)
        dest_path = os.path.join(self.user0.session_collection, file_name)

        question = '''"select DATA_REPL_NUM, DATA_RESC_NAME, DATA_REPL_STATUS where COLL_NAME = '{0}' and DATA_NAME = '{1}'"'''.format(os.path.dirname(dest_path), os.path.basename(dest_path))

        try:
            # put a new data object
            self.user0.assert_icommand(['iput', '-f', '-R', root_resc, filepath, dest_path])
            self.user0.assert_icommand(['ils', '-L', dest_path], 'STDOUT_SINGLELINE', root_resc)
            source_resc, dest_resc = self.get_source_and_destination_resources(dest_path)

            # get information from catalog about the new object
            out, err, ec = self.user0.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)

            # there should only be 1 replica to worry about
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure replica 0 information is correct in the catalog
            repl_num, resc_name, repl_status = out_lines[0].split()
            self.assertEqual(int(repl_num),    0)           # DATA_REPL_NUM
            self.assertEqual(str(resc_name),   source_resc) # DATA_RESC_NAME
            self.assertEqual(int(repl_status), 1)           # DATA_REPL_STATUS

            # move the replica to the other resource, targeting child resources
            self.user0.assert_icommand(['iphymv', '-S', ';'.join([root_resc, source_resc]), '-R', ';'.join([root_resc, dest_resc]), dest_path])

            out, err, ec = self.user0.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(ec, 0)
            self.assertEqual(len(err), 0)

            # there should only be 1 replica to worry about
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 1)

            # ensure replica 0 information is correct in the catalog
            repl_num, resc_name, repl_status = out_lines[0].split()
            self.assertEqual(int(repl_num),    0)           # DATA_REPL_NUM
            self.assertEqual(str(resc_name),   dest_resc)   # DATA_RESC_NAME
            self.assertEqual(int(repl_status), 1)           # DATA_REPL_STATUS

        finally:
            self.user0.assert_icommand(['irm', '-f', dest_path])

    def test_iphymv_invalid_resource__2821(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        dest_path = self.admin.session_collection + '/file'
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('iphymv -S invalidResc -R demoResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')
        self.admin.assert_icommand('iphymv -S demoResc -R invalidResc '+dest_path, 'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for Topology Testing on Resource Server')
    def test_iphymv_unable_to_unlink__2820(self):
        filename = 'test_iphymv_unable_to_unlink__2820'
        localfile = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(localfile, 1)
        dest_path = os.path.join(self.admin.session_collection, filename)

        try:
            # put a data object so that a file appears in the vault
            self.admin.assert_icommand(['iput', localfile, dest_path])

            # modify the permissions on the physical file in the vault to deny unlinking to the service account
            filepath = os.path.join(self.admin.get_vault_session_path(), filename)
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', filepath)
            os.chmod(filepath, 0444)

            # attempt and fail to phymv the replica (due to no permissions for unlink)
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'TestResc', dest_path], 'STDERR_SINGLELINE', 'Permission denied')
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', 'demoResc')

            # ensure that physical file was not unlinked
            self.assertTrue(os.path.exists(filepath))

            # set the permissions on the physical file back to allow unlinking to the service account
            os.chmod(filepath, 0666)

            # attempt to phymv the replica (and succeed)
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'TestResc', dest_path])

            # ensure that file was unlinked
            self.assertFalse(os.path.exists(filepath))
            self.admin.assert_icommand(['ils', '-L', dest_path], 'STDOUT', 'TestResc')

        finally:
            os.unlink(localfile)
            self.admin.assert_icommand(['irm', '-f', dest_path])

    def test_iphymv_to_self(self):
        filename = 'test_iphymv_to_self'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1)

        try:
            self.admin.assert_icommand(['iput', filepath, filename])
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT', [' 0 ', 'demoResc', ' & ', filename])
            self.admin.assert_icommand(['iphymv', '-S', 'demoResc', '-R', 'demoResc', filename], 'STDERR', 'SYS_NOT_ALLOWED')
            self.admin.assert_icommand(['ils', '-L'], 'STDOUT', [' 0 ', 'demoResc', ' & ', filename])

        finally:
            self.admin.run_icommand(['irm', '-f', filename])
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iphymv_retains_system_metadata(self):
        file_name = 'test_iphymv_retains_system_metadata'
        filepath = os.path.join(self.admin.local_session_dir, file_name)
        lib.make_file(filepath, 1)
        dest_path = os.path.join(self.admin.session_collection, file_name)
        source_resource = self.admin.default_resource
        destination_resource = 'unix1Resc'

        attributes = [
            'DATA_CHECKSUM',
            'DATA_CREATE_TIME',
            'DATA_MODIFY_TIME',
            'DATA_REPL_NUM',
            'DATA_REPL_STATUS',
            'DATA_SIZE'
        ]

        system_metadata_genquery = '''"select {0} where COLL_NAME = '{1}' and DATA_NAME = '{2}'"'''.format(
            ','.join(attributes), os.path.dirname(dest_path), os.path.basename(dest_path))

        try:
            # put in a file and get the system metadata
            self.admin.assert_icommand(['iput', '-K', '-R', source_resource, filepath, dest_path])
            self.admin.assert_icommand(['ils', '-l', dest_path], 'STDOUT_SINGLELINE', source_resource)
            source_system_metadata = self.admin.run_icommand(['iquest', '%s' * len(attributes), system_metadata_genquery])[0].strip().split()

            # sleep to make sure the modify time would change (but doesn't!)
            time.sleep(2)

            # phymv the replica and get the system metadata
            self.admin.assert_icommand(['iphymv', '-S', source_resource, '-R', destination_resource, dest_path])
            self.admin.assert_icommand(['ils', '-l', dest_path], 'STDOUT_SINGLELINE', destination_resource)
            destination_system_metadata = self.admin.run_icommand(['iquest', '%s' * len(attributes), system_metadata_genquery])[0].strip().split()

            # assert that everything lines up
            self.assertEqual(len(source_system_metadata), len(destination_system_metadata))
            for i,_ in enumerate(source_system_metadata):
                self.assertEqual(source_system_metadata[i], destination_system_metadata[i])

        finally:
            self.admin.assert_icommand(['irm', '-f', file_name])

    def test_multithreaded_iphymv__issue_5478(self):
        def do_test(size, threads):
            file_name = 'test_multithreaded_iphymv__issue_5478'
            local_file = os.path.join(self.user0.local_session_dir, file_name)
            dest_path = os.path.join(self.user0.session_collection, file_name)

            try:
                lib.make_file(local_file, size)

                self.user0.assert_icommand(['iput', '-R', 'randResc', local_file, dest_path])

                source_resc, dest_resc = self.get_source_and_destination_resources(dest_path)

                phymv_thread_count = self.user0.run_icommand(['iphymv', '-v', '-S', source_resc, '-R', dest_resc, dest_path])[0].split('|')[2].split()[0]
                self.assertEqual(int(phymv_thread_count), threads)

            finally:
                self.user0.assert_icommand(['irm', '-f', dest_path])

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
    def test_failure_to_unlink_source_replica(self):
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

            assert_command(['mungefsctl', '--operations', 'unlink', '--err_no=9'])

            if not os.path.exists(local_file):
                lib.make_file(local_file, 1024)

            self.admin.assert_icommand(['iput', '-K', '-R', munge_resc, local_file, logical_path])
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', munge_resc)

            ec, _, _ = self.admin.assert_icommand(
                ['iphymv', '-S', munge_resc, '-R', self.admin.default_resource, logical_path],
                'STDERR', 'UNIX_FILE_UNLINK_ERR')

            # check on the replicas after the phymv...
            out, err, ec = self.admin.run_icommand(['iquest', '%s %s %s', question])
            self.assertEqual(0, ec)
            self.assertEqual(len(err), 0)

            # there should be 2 replicas...
            out_lines = out.splitlines()
            self.assertEqual(len(out_lines), 2)

            # ensure the new replica is marked good and has the original replica number...
            repl_num_0, resc_name_0, repl_status_0 = out_lines[0].split()
            self.assertEqual(int(repl_num_0),    0)                             # DATA_REPL_NUM
            self.assertEqual(str(resc_name_0),   self.admin.default_resource)   # DATA_RESC_NAME
            self.assertEqual(int(repl_status_0), 1)                             # DATA_REPL_STATUS

            # ensure the original replica has moved to a new replica, and is stale
            repl_num_1, resc_name_1, repl_status_1 = out_lines[1].split()
            self.assertEqual(int(repl_num_1),    2)          # DATA_REPL_NUM
            self.assertEqual(str(resc_name_1),   munge_resc) # DATA_RESC_NAME
            self.assertEqual(int(repl_status_1), 0)          # DATA_REPL_STATUS

        finally:
            print(self.admin.run_icommand(['ils', '-l', logical_path])[0])
            assert_command(['mungefsctl', '--operations', 'unlink'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

class test_iphymv_with_two_basic_ufs_resources(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    """Test suite which provides 2 basic unixfilesystem resources.

    In this suite:
       - rodsadmin otherrods
       - rodsuser alice
       - 2 unixfilesystem resources
    """

    def setUp(self):
        super(test_iphymv_with_two_basic_ufs_resources, self).setUp()

        self.admin = self.admin_sessions[0]

        self.resource_1 = 'resource1'
        resource_1_vault = os.path.join(self.admin.local_session_dir, self.resource_1 + 'vault')
        self.resource_2 = 'resource2'
        resource_2_vault = os.path.join(self.admin.local_session_dir, self.resource_2 + 'vault')

        self.admin.assert_icommand(
                ['iadmin', 'mkresc', self.resource_1, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_1, resource_1_vault])],
                'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand(
                ['iadmin', 'mkresc', self.resource_2, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, resource_2_vault])],
                'STDOUT_SINGLELINE', 'unixfilesystem')

        self.user = self.user_sessions[0]

    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource_1])
        self.admin.assert_icommand(['iadmin', 'rmresc', self.resource_2])
        super(test_iphymv_with_two_basic_ufs_resources, self).tearDown()

    def test_iphymv_with_apostrophe_logical_path__issue_5759(self):
        """Test iphymv with apostrophes in the logical path.

        For each iphymv, the logical path will contain an apostrophe in either the collection
        name, data object name, both, or neither.
        """

        local_file = os.path.join(self.user.local_session_dir, 'test_iphymv_with_apostrophe_logical_path__issue_5759')
        lib.make_file(local_file, 1024, 'arbitrary')

        collection_names = ["collection", "collect'ion"]

        data_names = ["data_object", "data'_object"]

        for coll in collection_names:
            collection_path = os.path.join(self.user.session_collection, coll)

            self.user.assert_icommand(['imkdir', collection_path])

            try:
                for name in data_names:
                    logical_path = os.path.join(collection_path, name)

                    self.user.assert_icommand(['iput', '-R', self.resource_1, local_file, logical_path])
                    self.user.assert_icommand(['ils', '-l', logical_path], 'STDOUT', self.resource_1)

                    self.user.assert_icommand(['iphymv', '-S', self.resource_1, '-R', self.resource_2, logical_path])
                    self.user.assert_icommand(['ils', '-l', logical_path], 'STDOUT', self.resource_2)

                self.user.assert_icommand(['ils', '-l', collection_path], 'STDOUT', coll)

            finally:
                self.user.run_icommand(['irm', '-r', '-f', collection_path])


class test_iphymv_exit_codes(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_iphymv_exit_codes, self).setUp()

        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(test_iphymv_exit_codes, self).tearDown()

    def test_exit_code_with_no_input(self):
        rc, _, _ = self.admin.assert_icommand(['iphymv'], 'STDERR', 'no input')
        self.assertEqual(1, rc)

    def test_exit_code_with_unsupported_option(self):
        rc, _, _ = self.admin.assert_icommand(['iphymv', '-Y', 'whatever'], 'STDERR', 'USER_INPUT_OPTION_ERR')
        self.assertEqual(1, rc)

    def test_exit_code_when_connection_fails(self):
        try:
            IrodsController().stop()
            rc, _, _ = self.admin.assert_icommand(['iphymv', 'whatever'], 'STDERR', 'is probably down')
            self.assertEqual(2, rc)

        finally:
            IrodsController().start()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Delete on resolution of #5690')
    def test_exit_code_for_authentication_failure(self):
        config = IrodsConfig()

        with lib.file_backed_up(config.client_environment_path):
            session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
            self.admin.environment_file_contents.update({'irods_user_name': 'test_exit_code_for_authentication_failure'})

            rc, _, _ = self.admin.assert_icommand(['iphymv', 'whatever'], 'STDERR', 'CAT_INVALID_USER')
            self.assertEqual(3, rc)

            self.admin.environment_file_contents = session_env_backup

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_exit_code_for_mismatched_checksum(self):
        munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
        munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
        munge_resc = 'munge_resc'
        local_file = os.path.join(self.admin.local_session_dir, 'local_file')
        logical_path = os.path.join(self.admin.session_collection, 'foo')

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

            self.admin.assert_icommand(['iput', '-K', local_file, logical_path])
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', self.admin.default_resource)

            ec, _, _ = self.admin.assert_icommand(
                ['iphymv', '-S', self.admin.default_resource, '-R', munge_resc, logical_path],
                'STDERR', 'USER_CHKSUM_MISMATCH')

            self.assertEqual(4, ec)

            assert_two_replicas_where_original_is_good_and_new_is_stale(self, logical_path, self.admin.default_resource, munge_resc)

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            assert_command(['mungefsctl', '--operations', 'write'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

    def test_exit_code_for_phymv_of_nonexistent_object(self):
        rc, _, _ = self.admin.assert_icommand(['iphymv', 'whatever'], 'STDERR', 'does not exist')
        self.assertEqual(5, rc)

    @unittest.skipIf(False == test.settings.USE_MUNGEFS, "This test requires mungefs")
    def test_exit_code_for_open_error(self):
        munge_mount = os.path.join(self.admin.local_session_dir, 'munge_mount')
        munge_target = os.path.join(self.admin.local_session_dir, 'munge_target')
        munge_resc = 'munge_resc'
        local_file = os.path.join(self.admin.local_session_dir, 'local_file')
        logical_path = os.path.join(self.admin.session_collection, 'foo')

        try:
            lib.make_dir_p(munge_mount)
            lib.make_dir_p(munge_target)
            assert_command(['mungefs', munge_mount, '-omodules=subdir,subdir={}'.format(munge_target)])

            self.admin.assert_icommand(
                ['iadmin', 'mkresc', munge_resc, 'unixfilesystem',
                 ':'.join([test.settings.HOSTNAME_1, munge_mount])], 'STDOUT_SINGLELINE', 'unixfilesystem')

            if not os.path.exists(local_file):
                lib.make_file(local_file, 1024)

            self.admin.assert_icommand(['iput', '-K', local_file, logical_path])
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', self.admin.default_resource)

            # ENOSPC is a special case where the resource storage is full
            assert_command(['mungefsctl', '--operations', 'write', '--err_no=28'])

            ec, _, _ = self.admin.assert_icommand(
                ['iphymv', '-S', self.admin.default_resource, '-R', munge_resc, logical_path],
                'STDERR', 'UNIX_FILE_WRITE_ERR')

            self.assertEqual(6, ec)

            assert_two_replicas_where_original_is_good_and_new_is_stale(self, logical_path, self.admin.default_resource, munge_resc)

            # remove the new replica
            self.admin.assert_icommand(['itrim', '-N', '1', '-n', '1', logical_path], 'STDOUT', ['trimmed = 1'])

            # Anything else should result in a different error code
            assert_command(['mungefsctl', '--operations', 'write', '--err_no=16'])

            ec, _, _ = self.admin.assert_icommand(
                ['iphymv', '-S', self.admin.default_resource, '-R', munge_resc, logical_path],
                'STDERR', 'UNIX_FILE_WRITE_ERR')

            self.assertEqual(7, ec)

            assert_two_replicas_where_original_is_good_and_new_is_stale(self, logical_path, self.admin.default_resource, munge_resc)

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            assert_command(['mungefsctl', '--operations', 'write'])
            assert_command(['fusermount', '-u', munge_mount])
            self.admin.assert_icommand(['iadmin', 'rmresc', munge_resc])

class test_invalid_parameters(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_invalid_parameters, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_iphymv_repl_status_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_iphymv_repl_status_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_iphymv_repl_status_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            },
            'f' : {
                'name': 'test_iphymv_repl_status_f',
                'vault': os.path.join(self.admin.local_session_dir, 'f'),
                'host': test.settings.HOSTNAME_1
            }
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.parent_rescs = {
            'd' : 'test_iphymv_repl_status_d',
            'e' : 'test_iphymv_repl_status_e',
        }

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc, 'passthru'], 'STDOUT', resc)

        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        self.admin.environment_file_contents.update({'irods_default_resource': self.leaf_rescs['a']['name']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_invalid_parameters, self).tearDown()

    def test_invalid_parameters(self):
        nonexistent_resource_name = 'nope'

        # put a file
        if not os.path.exists(self.local_path):
            lib.make_file(self.local_path, 1024)
        self.admin.assert_icommand(['imkdir', os.path.dirname(self.logical_path)])
        self.admin.assert_icommand(['iput', '-R', self.leaf_rescs['a']['name'], self.local_path, self.logical_path])

        try:
            # USER__NULL_INPUT_ERR
            # iphymv foo
            self.admin.assert_icommand(['iphymv', self.logical_path],
                'STDERR', 'USER__NULL_INPUT_ERR')

            # iphymv -S a foo
            self.admin.assert_icommand(['iphymv', '-S', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER__NULL_INPUT_ERR')

            # iphymv -R a foo
            self.admin.assert_icommand(['iphymv', '-R', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER__NULL_INPUT_ERR')

            # USER_INCOMPATIBLE_PARAMS
            # iphymv -S a -n 0 foo
            self.admin.assert_icommand(['iphymv', '-S', self.leaf_rescs['a']['name'], '-n0', self.logical_path],
                'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # USER_INVALID_RESC_INPUT
            # iphymv -S d -R a foo
            self.admin.assert_icommand(
                ['iphymv', '-S', self.parent_rescs['d'], '-R', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S a -R d foo
            self.admin.assert_icommand(
                ['iphymv', '-R', self.leaf_rescs['a']['name'], '-S', self.parent_rescs['d'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S e -R a foo
            self.admin.assert_icommand(
                ['iphymv', '-S', self.parent_rescs['e'], '-R', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S a -R e foo
            self.admin.assert_icommand(
                ['iphymv', '-S', self.leaf_rescs['a']['name'], '-R', self.parent_rescs['e'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S 'd;e' -R a foo
            hier = ';'.join([self.parent_rescs['d'], self.parent_rescs['e']])
            self.admin.assert_icommand(
                ['iphymv', '-S', hier, '-R', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S a -R 'd;e' foo
            self.admin.assert_icommand(
                ['iphymv', '-S', self.leaf_rescs['a']['name'], '-R', hier, self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S 'e;f' -R a foo
            hier = ';'.join([self.parent_rescs['e'], self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(
                ['iphymv', '-S', hier, '-R', self.leaf_rescs['a']['name'], self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # iphymv -S a -R 'e;f' foo
            self.admin.assert_icommand(
                ['iphymv', '-S', self.leaf_rescs['a']['name'], '-R', hier, self.logical_path],
                'STDERR', 'USER_INVALID_RESC_INPUT')

            # SYS_RESC_DOES_NOT_EXIST
            # iphymv -S nope -R b foo
            self.admin.assert_icommand(['iphymv', '-S', nonexistent_resource_name, '-R', self.leaf_rescs['b']['name'], self.logical_path],
                'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # iphymv -S a -R nope foo
            self.admin.assert_icommand(['iphymv', '-S', self.leaf_rescs['a']['name'], '-R', nonexistent_resource_name, self.logical_path],
                'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # ensure no replications took place
            out, _, _ = self.admin.run_icommand(['ils', '-l', self.logical_path])
            print(out)
            for resc in self.leaf_rescs.values()[1:]:
                self.assertNotIn(resc['name'], out, msg='found unwanted replica on [{}]'.format(resc['name']))
            for resc in self.parent_rescs.values():
                self.assertNotIn(resc, out, msg='found replica on coordinating resc [{}]'.format(resc))

        finally:
            self.admin.assert_icommand(['irm', '-rf', os.path.dirname(self.logical_path)])

class test_iphymv_repl_status(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    def setUp(self):
        super(test_iphymv_repl_status, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_iphymv_repl_status_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_iphymv_repl_status_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_iphymv_repl_status_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            },
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.admin.environment_file_contents.update({'irods_default_resource': self.leaf_rescs['a']['name']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_iphymv_repl_status, self).tearDown()

    def test_iphymv_Sa_Rb_foo(self):
        # iphymv -S a -R b foo (source default resource, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-S', self.leaf_rescs['a']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'iphymv -S a -R b foo')

    def test_iphymv_Sc_Rb_foo(self):
        # iphymv -S c -R b foo (source non-default resource, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-S', self.leaf_rescs['c']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'iphymv -S c -R b foo')

    def test_iphymv_Sc_Ra_foo(self):
        # iphymv -S c -R a foo (source non-default resource, destination default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-S', self.leaf_rescs['c']['name'], '-R', self.leaf_rescs['a']['name'], self.logical_path],
            scenarios, 'iphymv -S c -R a foo')

    def test_iphymv_n0_Ra_foo(self):
        # iphymv -n 0 -R a foo (source replica 0, destination default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-n', '0', '-R', self.leaf_rescs['a']['name'], self.logical_path],
            scenarios, 'iphymv -n 0 -R a foo')

    def test_iphymv_n0_Rb_foo(self):
        # iphymv -n 0 -R b foo (source replica 0, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-n', '0', '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'iphymv -n 0 -R b foo')

    def test_iphymv_n1_Ra_foo(self):
        # iphymv -n 1 -R a foo (source replica 1, destination default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-n', '1', '-R', self.leaf_rescs['a']['name'], self.logical_path],
            scenarios, 'iphymv -n 1 -R a foo')

    def test_iphymv_n1_Rb_foo(self):
        # iphymv -n 1 -R b foo (source replica 1, destination non-default resource)
        scenarios = [
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_REPLICA_DOES_NOT_EXIST', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NOT_ALLOWED', 'rc':None}}
        ]

        replica_status_test.run_command_against_scenarios(self,
            ['iphymv', '-n', '1', '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'iphymv -n 1 -R b foo')
