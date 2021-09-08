from __future__ import print_function
import os
import sys
import socket

from time import sleep

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test

class Test_Itouch(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Itouch, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Itouch, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_creates_non_existent_data_objects(self):
        data_object = 'foo'
        self.admin.assert_icommand(['itouch', data_object])
        self.admin.assert_icommand(['ils', data_object], 'STDOUT', [data_object])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_no_create_option_is_honored(self):
        data_object = 'foo'
        self.admin.assert_icommand(['itouch', '--no-create', data_object])
        self.admin.assert_icommand(['ils', '-l', data_object], 'STDERR', ['does not exist'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_updates_mtime_of_latest_good_replica_when_target_replica_is_not_specified(self):
        resc_0 = 'resc_0'
        self.create_resource_ufs(resc_0)

        resc_1 = 'resc_1'
        self.create_resource_ufs(resc_1)

        try:
            data_object = os.path.join(self.admin.session_collection, 'foo')
            self.admin.assert_icommand(['itouch', data_object])
            replica_0_mtime = self.get_replica_mtime(data_object, 0)

            sleep(2)
            self.admin.assert_icommand(['irepl', '-R', resc_0, data_object])
            replica_1_mtime = self.get_replica_mtime(data_object, 1)

            sleep(2)
            self.admin.assert_icommand(['irepl', '-R', resc_1, data_object])
            replica_2_mtime = self.get_replica_mtime(data_object, 2)

            # We now have three replicas.
            # Make the original replica stale.
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', data_object, 'replica_number', '0', 'DATA_REPL_STATUS', '0'])

            # Show that updating the data object without targeting a specific replica
            # results in the latest good replica being updated. In this particular scenario,
            # the third replica will have the latest mtime.
            sleep(2)
            self.admin.assert_icommand(['itouch', data_object])
            replica_2_new_mtime = self.get_replica_mtime(data_object, 2)
            self.assertGreater(replica_2_new_mtime, replica_0_mtime)
            self.assertGreater(replica_2_new_mtime, replica_1_mtime)
            self.assertGreater(replica_2_new_mtime, replica_2_mtime)

            # Clean up.
            self.admin.assert_icommand(['irm', '-f', data_object])

        except Exception as e:
            print(e)

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_0])
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_1])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_updates_mtime_of_replica_using_replica_number_option(self):
        resc_0 = 'resc_0'
        self.create_resource_ufs(resc_0)

        try:
            data_object = os.path.join(self.admin.session_collection, 'foo')
            self.admin.assert_icommand(['itouch', data_object])

            # Guarantees that the mtime of the new replica is different from the
            # original replica.
            sleep(2)

            # Replicate the original replica and show that their mtimes are different.
            self.admin.assert_icommand(['irepl', '-R', resc_0, data_object])
            mtime = self.get_replica_mtime(data_object, 0)
            self.assertNotEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Make the new replica have the same mtime as the original.
            self.admin.assert_icommand(['itouch', '-n', '1', '-s', str(mtime), data_object])
            self.assertEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Clean up.
            self.admin.assert_icommand(['irm', '-f', data_object])

        except Exception as e:
            print(e)

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_0])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_updates_mtime_of_replica_using_leaf_resource_option(self):
        resc_0 = 'resc_0'
        self.create_resource_ufs(resc_0)

        pt = 'pt'
        self.create_resource_pt(pt)

        try:
            data_object = os.path.join(self.admin.session_collection, 'foo')
            self.admin.assert_icommand(['itouch', data_object])

            # Guarantees that the mtime of the new replica is different from the
            # original replica.
            sleep(2)

            # Replicate the original replica and show that their mtimes are different.
            self.admin.assert_icommand(['irepl', '-R', resc_0, data_object])
            mtime = self.get_replica_mtime(data_object, 0)
            self.assertNotEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Make the new replica have the same mtime as the original. This also shows
            # that resources without a parent or children are considered leaf resources.
            self.admin.assert_icommand(['itouch', '-R', resc_0, '-s', str(mtime), data_object])
            self.assertEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Trim the second replica and add a passthru resource as the parent of resc_0.
            self.admin.assert_icommand(['itrim', '-N', '1', '-n', '1', data_object], 'STDOUT', ['trimmed = 1'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', pt, resc_0])

            # Replicate the original replica and show that their mtimes are different.
            self.admin.assert_icommand(['irepl', '-R', pt, data_object])
            mtime = self.get_replica_mtime(data_object, 0)
            self.assertNotEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Make the new replica have the same mtime as the original. This shows that
            # leaf resources are truly supported by itouch.
            self.admin.assert_icommand(['itouch', '-R', resc_0, '-s', str(mtime), data_object])
            self.assertEqual(mtime, self.get_replica_mtime(data_object, 1))

            # Clean up.
            self.admin.assert_icommand(['irm', '-f', data_object])
            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', pt, resc_0])

        except Exception as e:
            print(e)

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', pt])
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_0])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_reference_option_is_honored(self):
        try:
            data_object_1 = 'foo'
            self.admin.assert_icommand(['itouch', data_object_1])

            # Guarantees that the mtime of the new replica is different from the
            # original replica.
            sleep(2)

            data_object_2 = 'bar'
            self.admin.assert_icommand(['itouch', data_object_2])

            # Show that the mtimes for the data objects are different.
            mtime = self.get_replica_mtime(data_object_2, 0)
            self.assertNotEqual(self.get_replica_mtime(data_object_1, 0), mtime)

            # Make the first data object's mtime match the second data object's mtime.
            # This also demonstrates that the reference path is relative to the current
            # working collection.
            self.admin.assert_icommand(['itouch', '-r', data_object_2, data_object_1])
            self.assertEqual(self.get_replica_mtime(data_object_1, 0), mtime)

        except Exception as e:
            print(e)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_seconds_since_epoch_is_honored(self):
        data_object = os.path.join(self.admin.session_collection, 'foo')
        seconds_since_epoch = 584582400 # 1988-07-11 00:00:00.0 UTC

        self.admin.assert_icommand(['itouch', '-s', str(seconds_since_epoch), data_object])
        self.assertEqual(seconds_since_epoch, self.get_replica_mtime(data_object, 0))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_collections_are_supported(self):
        try:
            old_mtime = self.get_collection_mtime(self.admin.session_collection)
            sleep(2)
            self.admin.assert_icommand(['itouch', self.admin.session_collection])
            self.assertNotEqual(old_mtime, self.get_collection_mtime(self.admin.session_collection))

        except Exception as e:
            print(e)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_mutually_exclusive_options_are_handled(self):
        error_msg = 'USER_INCOMPATIBLE_PARAMS: [replica_number] and [leaf_resource_name] cannot be used together.'
        self.admin.assert_icommand(['itouch', '-n', '0', '-R', 'demoResc', 'foo'], 'STDOUT', [error_msg])

        error_msg = 'USER_INCOMPATIBLE_PARAMS: [reference] and [seconds_since_epoch] cannot be used together.'
        self.admin.assert_icommand(['itouch', '-r', 'source', '-s', '1', 'foo'], 'STDOUT', [error_msg])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_replica_number_cannot_be_used_to_create_new_replicas(self):
        error_msg = 'OBJ_PATH_DOES_NOT_EXIST: Replica numbers cannot be used when creating new data objects.'
        self.admin.assert_icommand(['itouch', '-n', '3', 'foo'], 'STDOUT', [error_msg])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_replica_number_option_and_leaf_resource_option_cannot_be_used_to_create_additional_replicas(self):
        resc_0 = 'resc_0'
        self.create_resource_ufs(resc_0)

        try:
            data_object = os.path.join(self.admin.session_collection, 'foo')
            self.admin.assert_icommand(['itouch', data_object])

            # Show that using the leaf resource option to create an additional replica
            # results in an error.
            error_msg = 'SYS_REPLICA_DOES_NOT_EXIST: Replica does not exist matching that replica number.'
            self.admin.assert_icommand(['itouch', '-n', '1', data_object], 'STDOUT', [error_msg])

            # Show that using the leaf resource option to create an additional replica
            # results in an error.
            error_msg = 'SYS_REPLICA_DOES_NOT_EXIST: Replica does not exist in resource.'
            self.admin.assert_icommand(['itouch', '-R', resc_0, data_object], 'STDOUT', [error_msg])

        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_0])

    def test_itouch_returns_better_error_message_when_given_a_coordinating_resource__issue_5771(self):
        resc = 'resc_5771'
        self.create_resource_ufs(resc)

        pt = 'pt_5771'
        self.create_resource_pt(pt)

        try:
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', pt, resc])

            # Show that passing a coordinating resource as a leaf resource results in an error.
            data_object = os.path.join(self.admin.session_collection, 'foo')
            error_msg = 'USER_INVALID_RESC_INPUT: [{0}] is not a leaf resource.'.format(pt)
            self.admin.assert_icommand(['itouch', '-R', pt, data_object], 'STDOUT', [error_msg])

        finally:
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', pt, resc])
            self.admin.run_icommand(['iadmin', 'rmresc', resc])
            self.admin.run_icommand(['iadmin', 'rmresc', pt])

    def get_replica_mtime(self, path, replica_number):
        collection = os.path.dirname(path)
        if len(collection) == 0:
            collection = self.admin.session_collection
        data_object = os.path.basename(path)
        res = self.admin.run_icommand('iquest %s "select DATA_MODIFY_TIME where COLL_NAME = \'{0}\' and DATA_NAME = \'{1}\' and DATA_REPL_NUM = \'{2}\'"'
            .format(collection, data_object, replica_number))
        return int(res[0])

    def get_collection_mtime(self, path):
        res = self.admin.run_icommand('iquest %s "select COLL_MODIFY_TIME where COLL_NAME = \'{0}\'"'.format(path))
        return int(res[0])

    def create_resource_ufs(self, resource_name):
        vault_name = resource_name + '_vault'
        vault_directory = os.path.join(self.admin.local_session_dir, vault_name)
        if not os.path.exists(vault_directory):
            os.mkdir(vault_directory)
        vault = socket.gethostname() + ':' + vault_directory
        self.admin.assert_icommand(['iadmin', 'mkresc', resource_name, 'unixfilesystem', vault], 'STDOUT', [resource_name])

    def create_resource_pt(self, resource_name):
        self.admin.assert_icommand(['iadmin', 'mkresc', resource_name, 'passthru'], 'STDOUT', [resource_name])

