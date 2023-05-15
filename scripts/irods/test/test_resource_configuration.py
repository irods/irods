import time
import sys
import shutil
import os
import socket
import datetime
import imp
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import test
from . import settings
from . import session
from .. import lib
from ..configuration import IrodsConfig
from . import resource_suite

class Test_DeferredToDeferred(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            context_prefix = lib.get_hostname() + ':' + IrodsConfig().irods_directory
            admin_session.assert_icommand('iadmin modresc demoResc name origResc', 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand('iadmin mkresc demoResc deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc1 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc2 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc3 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc4 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc rescA "unixfilesystem" ' + context_prefix + '/rescAVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc rescB "unixfilesystem" ' + context_prefix + '/rescBVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin addchildtoresc defResc3 rescA')
            admin_session.assert_icommand('iadmin addchildtoresc defResc4 rescB')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc defResc1')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc defResc2')
            admin_session.assert_icommand('iadmin addchildtoresc defResc1 defResc3')
            admin_session.assert_icommand('iadmin addchildtoresc defResc2 defResc4')
        super(Test_DeferredToDeferred, self).setUp()

    def tearDown(self):
        super(Test_DeferredToDeferred, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc defResc3 rescA")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc4 rescB")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc1 defResc3")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc2 defResc4")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc defResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc defResc2")
            admin_session.assert_icommand("iadmin rmresc rescA")
            admin_session.assert_icommand("iadmin rmresc rescB")
            admin_session.assert_icommand("iadmin rmresc defResc1")
            admin_session.assert_icommand("iadmin rmresc defResc2")
            admin_session.assert_icommand("iadmin rmresc defResc3")
            admin_session.assert_icommand("iadmin rmresc defResc4")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/rescAVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/rescBVault", ignore_errors=True)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iput_irm(self):
            # =-=-=-=-=-=-=-
            # build a logical path for putting a file
            test_file = self.admin.session_collection + "/test_file.txt"

            # =-=-=-=-=-=-=-
            # put a test_file.txt - should be on rescA given load table values
            self.admin.assert_icommand("iput -f %s %s" % (self.testfile, test_file))
            self.admin.assert_icommand("irm -f " + test_file)


class test_configuring_operations_to_fail(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.user0 = session.mkuser_and_return_session('rodsuser', 'bilbo', 'bpass', lib.get_hostname())
        self.user1 = session.mkuser_and_return_session('rodsuser', 'frodo', 'fpass', lib.get_hostname())


    @classmethod
    def tearDownClass(self):
        with session.make_session_for_existing_admin() as admin_session:
            username0 = self.user0.username
            username1 = self.user1.username
            self.user0.__exit__()
            self.user1.__exit__()
            admin_session.run_icommand(['iadmin', 'rmuser', username0])
            admin_session.run_icommand(['iadmin', 'rmuser', username1])


    def test_operation_close_failure_on_data_object_creation__issue_6154(self):
        operation = 'resource_close'
        error_code = -169000
        error_name = 'SYS_NOT_ALLOWED'
        context_string = 'munge_operations={}:{}'.format(operation, error_code)
        filename = 'test_operation_close_fails_on_data_object_creation__issue_6154'
        logical_path = os.path.join(self.user0.session_collection, filename)
        local_file = os.path.join(self.user0.local_session_dir, filename)
        file_size_in_bytes = 10

        target_resource = 'test_configuring_operations_to_fail_resource'

        with session.make_session_for_existing_admin() as admin_session:
            try:
                lib.make_file(local_file, file_size_in_bytes)
                lib.create_ufs_resource(target_resource, admin_session, hostname=test.settings.HOSTNAME_2)

                # Successfully create a new data object, targeting target_resource
                self.user0.assert_icommand(['iput', '-R', target_resource, local_file, logical_path])
                self.assertTrue(lib.replica_exists(self.user0, logical_path, 0))
                self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

                # Configure the resource such that resource close operations fail
                admin_session.assert_icommand(['iadmin', 'modresc', target_resource, 'context', context_string])

                # Try to overwrite the existing replica and observe the failure
                self.user0.assert_icommand(['iput', '-f', '-R', target_resource, local_file, logical_path],
                                          'STDERR', error_name)
                self.assertTrue(lib.replica_exists(self.user0, logical_path, 0))
                self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

            finally:
                admin_session.assert_icommand(['iadmin', 'modresc', target_resource, 'context', 'null'])
                lib.set_replica_status(admin_session, logical_path, 0, 0)

                self.user0.assert_icommand(['irm', '-f', logical_path])

                lib.remove_resource(target_resource, admin_session)


    def test_operation_stat_failure_on_data_object_creation__issue_6479(self):
        operation = 'resource_stat'
        error_code = -169000
        error_name = 'SYS_NOT_ALLOWED'
        context_string = 'munge_operations={}:{}'.format(operation, error_code)
        filename = 'test_operation_stat_failure_on_data_object_creation__issue_6479'
        logical_path = os.path.join(self.user0.session_collection, filename)
        local_file = os.path.join(self.user0.local_session_dir, filename)
        file_size_in_bytes = 10

        target_resource = 'test_configuring_operations_to_fail_resource'

        with session.make_session_for_existing_admin() as admin_session:
            try:
                lib.make_file(local_file, file_size_in_bytes)
                lib.create_ufs_resource(target_resource, admin_session, hostname=test.settings.HOSTNAME_2)

                # Successfully create a new data object, targeting target_resource
                self.user0.assert_icommand(['iput', '-R', target_resource, local_file, logical_path])
                self.assertTrue(lib.replica_exists(self.user0, logical_path, 0))
                self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

                # Configure the resource such that resource stat operations fail
                admin_session.assert_icommand(['iadmin', 'modresc', target_resource, 'context', context_string])

                # Try to overwrite the existing replica and observe the failure
                self.user0.assert_icommand(['iput', '-f', '-R', target_resource, local_file, logical_path],
                                          'STDERR', error_name)
                self.assertTrue(lib.replica_exists(self.user0, logical_path, 0))
                self.assertEqual(str(0), lib.get_replica_status(self.user0, os.path.basename(logical_path), 0))

            finally:
                admin_session.assert_icommand(['ils', '-LA', os.path.dirname(logical_path)], 'STDOUT') # Debugging
                admin_session.assert_icommand(['ilsresc', '-l', target_resource], 'STDOUT') # Debugging

                admin_session.assert_icommand(['iadmin', 'modresc', target_resource, 'context', 'null'])
                lib.set_replica_status(admin_session, logical_path, 0, 0)

                self.user0.assert_icommand(['irm', '-f', logical_path])

                lib.remove_resource(target_resource, admin_session)


    def test_iget_data_object_as_user_with_read_only_access_and_replica_only_in_glacier_archive__issue_6697(self):
        def assert_permissions_on_data_object_for_user(session, username, logical_path, permission_value):
            data_access_type = session.run_icommand(['iquest', '%s',
                'select DATA_ACCESS_TYPE where COLL_NAME = \'{}\' and DATA_NAME = \'{}\' and USER_NAME = \'{}\''.format(
                    os.path.dirname(logical_path), os.path.basename(logical_path), username)
                ])[0].strip()

            self.assertEqual(str(data_access_type), str(permission_value))

        compound_resource = 'compResc'
        cache_resource = 'cacheResc'
        archive_resource = 'archiveResc'
        cache_hierarchy = compound_resource + ';' + cache_resource
        archive_hierarchy = compound_resource + ';' + archive_resource

        operation = 'resource_stagetocache'
        error_code = -721000 # REPLICA_IS_BEING_STAGED
        context_string = 'munge_operations={}:{}'.format(operation, error_code)
        filename = 'test_iget_data_object_as_user_with_read_only_access_and_replica_only_in_glacier_archive__issue_6697'
        logical_path = os.path.join(self.user0.session_collection, filename)

        owner_user = self.user0
        readonly_user = self.user1
        contents = 'jimbo'
        logical_path = os.path.join(owner_user.session_collection, filename)

        with session.make_session_for_existing_admin() as admin_session:
            try:
                # Construct compound resource hierarchy
                admin_session.assert_icommand(['iadmin', 'mkresc', compound_resource, 'compound'], 'STDOUT')
                lib.create_ufs_resource(cache_resource, admin_session, hostname=test.settings.HOSTNAME_2)
                lib.create_ufs_resource(archive_resource, admin_session, hostname=test.settings.HOSTNAME_2)
                lib.add_child_resource(compound_resource, cache_resource, admin_session, parent_context='cache')
                lib.add_child_resource(compound_resource, archive_resource, admin_session, parent_context='archive')

                # Create a data object which should appear under the compound resource.
                owner_user.assert_icommand(['istream', 'write', '-R', compound_resource, logical_path], input=contents)
                self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
                self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

                # Grant read access to another user, ensuring that the other user can see the data object.
                owner_user.assert_icommand(
                    ['ichmod', '-r', 'read', readonly_user.username, os.path.dirname(logical_path)])

                # Ensure that the read-only user has read-only permission on the data object.
                assert_permissions_on_data_object_for_user(admin_session, readonly_user.username, logical_path, 1050)

                # Trim the replica on the cache resource so that only the replica in the archive remains. Replica 0
                # resides on the cache resource at this point.
                owner_user.assert_icommand(['itrim', '-N1', '-n0', logical_path], 'STDOUT')
                self.assertFalse(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
                self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

                # Configure the resource such that resource close operations fail
                admin_session.assert_icommand(['iadmin', 'modresc', archive_resource, 'context', context_string])

                # As the user with read-only access, attempt to get the data object. The get should fail because the
                # archive resource has been configured to fail on resource_stagetocache. The result should be that no
                # replica exists in the cache and the replica in the archive should be good. The replica in the archive
                # should be number 1. The HIERARCHY_ERROR is expected here because the stage-to-cache operation will
                # fail during the hierarchy resolution for the get.
                _, err, rc = readonly_user.run_icommand(['iget', logical_path])
                self.assertNotEqual(0, rc)
                self.assertIn('HIERARCHY_ERROR', err)
                self.assertFalse(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
                self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))
                self.assertEqual(str(1), lib.get_replica_status(readonly_user, os.path.basename(logical_path), 1))

                # Ensure that the user has the same permissions on the data object as before getting it.
                assert_permissions_on_data_object_for_user(admin_session, readonly_user.username, logical_path, 1050)

                # Set the archive resource context string to "null" so that stage-to-cache operations succeed again.
                admin_session.assert_icommand(['iadmin', 'modresc', archive_resource, 'context', 'null'])

                # As the user with read-only access, attempt to get the data object. Replica 1 resides on the archive
                # resource, so the replica on the cache resource which results from the stage-to-cache should be number 2.
                readonly_user.assert_icommand(['iget', logical_path, '-'], 'STDOUT', contents)
                self.assertEqual(str(1), lib.get_replica_status(readonly_user, os.path.basename(logical_path), 1))
                self.assertEqual(str(1), lib.get_replica_status(readonly_user, os.path.basename(logical_path), 2))

            finally:
                admin_session.assert_icommand(['ils', '-LA', os.path.dirname(logical_path)], 'STDOUT') # Debugging
                admin_session.assert_icommand(['ilsresc', '-l', archive_resource], 'STDOUT') # Debugging

                admin_session.assert_icommand(['iadmin', 'modresc', archive_resource, 'context', 'null'])
                lib.set_replica_status(admin_session, logical_path, 0, 0)
                lib.set_replica_status(admin_session, logical_path, 0, 1)
                lib.set_replica_status(admin_session, logical_path, 0, 2)

                self.user0.assert_icommand(['irm', '-f', logical_path])

                lib.remove_child_resource(compound_resource, cache_resource, admin_session)
                lib.remove_child_resource(compound_resource, archive_resource, admin_session)
                lib.remove_resource(cache_resource, admin_session)
                lib.remove_resource(archive_resource, admin_session)
                lib.remove_resource(compound_resource, admin_session)
