from __future__ import print_function
import os
import sys
import tempfile
import socket
import copy

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import paths
from .. import test

class Test_Istream(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Istream, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Istream, self).tearDown()

    def test_istream_empty_file(self):
        filename = 'test_istream_empty_file.txt'
        contents = '''don't look at what I'm doing'''
        lib.cat(filename, contents)
        try:
            logical_path = os.path.join(self.user.session_collection, filename)
            self.user.assert_icommand(['iput', filename])
            self.user.assert_icommand(['istream', 'read', filename], 'STDOUT', contents)
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path, 'replica_number', '0', 'DATA_SIZE', '0'])
            # The catalog says the file is empty, so nothing should come back from a read
            self.user.assert_icommand(['istream', 'read', filename])
            self.user.assert_icommand(['irm', '-f', filename])
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_creating_data_objects_using_replica_numbers_is_not_allowed__issue_5107(self):
        data_object = 'cannot_create_data_objects'

        # Show that the targeted data object does not exist.
        self.admin.assert_icommand(['ils', '-l', data_object], 'STDERR', ['does not exist or user lacks access permission'])

        # Show that attempts to write to a non-existent data object using replica numbers does
        # not result in creation of the data object.
        self.admin.assert_icommand(['istream', 'write', '-n0', data_object], 'STDERR', ['Error: Cannot open data object'], input='some data')
        self.admin.assert_icommand(['istream', 'write', '-n4', data_object], 'STDERR', ['Error: Cannot open data object'], input='some data')

        # Show that the targeted data object was not created.
        self.admin.assert_icommand(['ils', '-l', data_object], 'STDERR', ['does not exist or user lacks access permission'])

    def test_writing_to_existing_replicas_using_replica_numbers_is_supported__issue_5107(self):
        resc = 'resc_issue_5107'
        vault = '{0}:{1}'.format(lib.get_hostname(), tempfile.mkdtemp())
        data_object = 'data_object.test'

        try:
            # Create a new resource.
            self.admin.assert_icommand(['iadmin', 'mkresc', resc, 'unixfilesystem', vault], 'STDOUT', ['unixfilesystem'])

            # Write a data object into iRODS.
            replica_0_contents = 'some data 0'
            self.admin.assert_icommand(['istream', 'write', data_object], input=replica_0_contents)

            # Create another replica and verify that it exists.
            self.admin.assert_icommand(['irepl', '-R', resc, data_object])
            self.admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', ['0 demoResc', '1 {0}'.format(resc)])

            # Write to the second replica.
            replica_1_contents = 'some data 1'
            self.admin.assert_icommand(['istream', 'write', '-n1', data_object], input=replica_1_contents)

            # Show that each replica contains different information.
            self.assertNotEqual(replica_0_contents, replica_1_contents)
            self.admin.assert_icommand(['istream', 'read', '-n0', data_object], 'STDOUT', [replica_0_contents])
            self.admin.assert_icommand(['istream', 'read', '-n1', data_object], 'STDOUT', [replica_1_contents])
        finally:
            self.admin.assert_icommand(['irm', '-f', data_object])
            self.admin.assert_icommand(['iadmin', 'rmresc', resc])

    def test_writing_to_non_existent_replica_of_existing_data_object_is_not_allowed__issue_5107(self):
        data_object = 'data_object.test'

        # Write a data object into iRODS.
        contents = 'totally worked!'
        self.admin.assert_icommand(['istream', 'write', data_object], input=contents)
        self.admin.assert_icommand(['istream', 'read', '-n0', data_object], 'STDOUT', [contents])

        # Attempt to write to a non-existent replica.
        self.admin.assert_icommand(['istream', 'write', '-n1', data_object], 'STDERR', ['Error: Cannot open data object'], input='nope')

    def test_invalid_integer_arguments_are_handled_appropriately__issue_5112(self):
        data_object = 'data_object.test'

        # Write a data object into iRODS.
        contents = 'totally worked!'
        self.admin.assert_icommand(['istream', 'write', data_object], input=contents)
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', [contents])

        # The following error messages prove that the invalid integer arguments are being handled by
        # the correct code.

        self.admin.assert_icommand(['istream', 'read', '-o', '-1', data_object], 'STDERR', ['Error: Invalid byte offset.'])
        self.admin.assert_icommand(['istream', 'read', '-c', '-1', data_object], 'STDERR', ['Error: Invalid byte count.'])

        self.admin.assert_icommand(['istream', 'write', '-o', '-1', data_object], 'STDERR', ['Error: Invalid byte offset.'], input='data')
        self.admin.assert_icommand(['istream', 'write', '-c', '-1', data_object], 'STDERR', ['Error: Invalid byte count.'], input='data')

    def test_provides_ipc_functionality__issue_4698(self):
        data_object = 'data_object.test'

        # Write a data object into iRODS.
        contents = 'The fox jumped over the box.'
        self.admin.assert_icommand(['istream', 'write', data_object], input=contents)
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', [contents])

        # Show that partial reads are supported.
        self.admin.assert_icommand(['istream', 'read', '-o4', '-c3', data_object], 'STDOUT', ['fox'])

        # Show that partial writes are supported.
        self.admin.assert_icommand(['istream', 'write', '-o4', '--no-trunc', data_object], input='cat')
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', ['The cat jumped over the box'])

        # Show that append is supported.
        self.admin.assert_icommand(['istream', 'write', '--append', data_object], input='  This was appended!')
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', ['The cat jumped over the box.  This was appended'])

    def test_openmode_is_honored_when_replica_number_is_specified_during_writes__issue_5279(self):
        data_object = 'data_object.test'

        # Write a data object into iRODS.
        contents = 'The fox jumped over the box.'
        self.admin.assert_icommand(['istream', 'write', data_object], input=contents)
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', [contents])

        # Show that using -n and --no-trunc are honored by istream. These flags prove that
        # the openmode is being adjusted and honored by the server. If the openmode was not being
        # passed to the underlying dstream object, then the replica would be truncated on open.
        self.admin.assert_icommand(['istream', 'write', '-n0', '-o4', '--no-trunc', data_object], input='cat')
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', ['The cat jumped over the box.'])

        # Show that using -n and --append results in the openmode being updated and passed to
        # the server correctly. This combination of options follows the same path as -n and --no-trunc.
        msg = ' This was appended.'
        self.admin.assert_icommand(['istream', 'write', '-n0', '--append', data_object], input=msg)
        self.admin.assert_icommand(['istream', 'read', data_object], 'STDOUT', ['The cat jumped over the box.' + msg])

    def test_client_side_configured_default_resource_is_honored_on_writes__issue_5294(self):
        resc_name = 'other_resc'
        self.create_ufs_resource(resc_name)

        old_env_config = copy.deepcopy(self.user.environment_file_contents)
        self.user.environment_file_contents.update({'irods_default_resource': resc_name})

        data_object = 'foo'

        try:
            # Write a data object into iRODS.
            self.user.assert_icommand(['istream', 'write', data_object], input='some data')

            # Show that the data object resides on the new resource.
            gql = "select RESC_NAME where COLL_NAME = '{0}' and DATA_NAME = '{1}'".format(self.user.session_collection, data_object)
            out, err, ec = self.user.run_icommand(['iquest', '%s', gql])
            self.assertEqual(ec, 0)
            self.assertEqual(resc_name, out.strip())
            self.assertEqual(len(err), 0)
        finally:
            self.user.run_icommand(['irm', '-f', data_object])
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_name])
            self.user.environment_file_contents = old_env_config

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_opening_a_non_existent_data_object_does_not_cause_a_stacktrace_to_be_logged__issue_5419(self):
        # Capture the current log file size. This will serve as an offset for when we inspect
        # the log file for messages.
        log_offset = lib.get_file_size_by_path(paths.server_log_path())

        # The following causes a stacktrace to be logged in iRODS v4.2.8 because the data object does not exist.
        self.admin.assert_icommand(['istream', 'read', 'does_not_exist'], 'STDERR', ['Error: Cannot open data object.'])

        # Show that the log file does not contain any errors or stacktraces.
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='Dumping stack trace', count=0, start_index=log_offset))
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='Data object or replica does not exist [error_code=', count=0, start_index=log_offset))
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='Could not open replica [error_code=', count=0, start_index=log_offset))
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='CAT_NO_ROWS_FOUND', count=0, start_index=log_offset))
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='OBJ_PATH_DOES_NOT_EXIST', count=0, start_index=log_offset))
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='-808000', count=0, start_index=log_offset)) # CAT_NO_ROWS_FOUND
        self.assertTrue(lib.log_message_occurrences_equals_count(msg='-358000', count=0, start_index=log_offset)) # OBJ_PATH_DOES_NOT_EXIST

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_istream_honors_data_size_in_the_catalog_on_reads__issue_5477(self):
        # Put a data object into iRODS.
        data_object = os.path.join(self.admin.session_collection, 'foo')
        contents = 'some important data'
        self.admin.assert_icommand(['istream', 'write', data_object], input=contents)

        # Show that istream wrote the bytes to disk.
        out, err, ec = self.admin.run_icommand(['istream', 'read', data_object])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(out, contents)

        # Update the data object's size in the catalog.
        size_in_catalog = 3
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_SIZE', str(size_in_catalog)])

        # Show that istream reads up to the number of bytes defined in the catalog for the data object.
        out, err, ec = self.admin.run_icommand(['istream', 'read', data_object])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(out, contents[:size_in_catalog])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_writing_zero_bytes_to_replica_does_not_result_in_checksum_being_erased__issue_5496(self):
        data_object = 'foo'

        # Create a data object and checksum it.
        self.admin.assert_icommand(['istream', 'write', '-k', data_object], input='some data')

        # Verify that the replica has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        original_checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertGreater(len(original_checksum.strip()), len('sha2:'))

        # Write zero bytes to the replica (i.e. open and immediately close the replica).
        self.admin.assert_icommand(['istream', 'write', '-a', data_object], input='')

        # Show that the checksum still exists and matches the original.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(checksum.strip(), original_checksum.strip())

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_overwriting_replica_erases_checksum__issue_5496(self):
        data_object = 'foo'

        #
        # TEST #1: Checksum is erased when the replica is truncated.
        #

        # Create a data object and checksum it.
        self.admin.assert_icommand(['istream', 'write', '-k', data_object], input='some data')

        # Verify that the replica has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertGreater(len(checksum.strip()), len('sha2:'))

        # Show that truncation results in the checksum being erased.
        # "istream write" truncates a replica's data by default.
        self.admin.assert_icommand(['istream', 'write', data_object], input='new data')

        # Show that the checksum has been erased.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(len(checksum.strip()), 0)

        #
        # TEST #2: Checksum is erased when the bytes in the middle of the replica
        #          are overwritten.
        #

        # Checksum the replica and store it in the catalog.
        self.admin.assert_icommand(['ichksum', data_object], 'STDOUT', ['sha2:'])

        # Verify that the replica has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertGreater(len(checksum.strip()), len('sha2:'))

        # Overwrite the three bytes in the middle of the replica.
        self.admin.assert_icommand(['istream', 'write', '-o', '3', '--no-trunc', data_object], input='XYZ')

        # Show that the checksum has been erased.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(len(checksum.strip()), 0)

        #
        # TEST #3: Checksum is erased when bytes are appended to the replica.
        #

        # Checksum the replica and store it in the catalog.
        self.admin.assert_icommand(['ichksum', data_object], 'STDOUT', ['sha2:'])

        # Verify that the replica has a checksum.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertGreater(len(checksum.strip()), len('sha2:'))

        # Overwrite the three bytes in the middle of the replica's data.
        self.admin.assert_icommand(['istream', 'write', '-a', data_object], input='APPENDED DATA')

        # Show that the checksum has been erased.
        gql = "select DATA_CHECKSUM where COLL_NAME = '{0}' and DATA_NAME = '{1}'"
        checksum, err, ec = self.admin.run_icommand(['iquest', '%s', gql.format(self.admin.session_collection, data_object)])
        self.assertEqual(ec, 0)
        self.assertEqual(len(err), 0)
        self.assertEqual(len(checksum.strip()), 0)

    def create_ufs_resource(self, resource_name):
        vault_name = resource_name + '_vault'
        vault_directory = os.path.join(self.admin.local_session_dir, vault_name)
        if not os.path.exists(vault_directory):
            os.mkdir(vault_directory)
        vault = socket.gethostname() + ':' + vault_directory
        self.admin.assert_icommand(['iadmin', 'mkresc', resource_name, 'unixfilesystem', vault], 'STDOUT', [resource_name])

