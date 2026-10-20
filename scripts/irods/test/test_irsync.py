import copy
import filecmp
import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
import shutil

from .resource_suite import ResourceBase
from .. import lib
from .. import paths
from .. import test

class Test_iRsync(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iRsync, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-irsync'

        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_iRsync, self).tearDown()

    def test_multi_1247_syncs_local_files_without_a_portal(self):
        source = os.path.join(self.testing_tmp_dir, 'multi_1247_source')
        other = os.path.join(self.testing_tmp_dir, 'multi_1247_other')
        downloaded = os.path.join(self.testing_tmp_dir, 'multi_1247_downloaded')
        obj = os.path.join(self.user0.session_collection, 'multi_1247_object')
        unchecked_obj = os.path.join(self.user0.session_collection, 'multi_1247_unchecked')
        lib.make_file(source, 34 * 1024 * 1024, 'random')
        lib.make_file(other, 34 * 1024 * 1024, 'zero')

        def sync_and_check_marker(command, expected=True):
            stdout, stderr, status = self.user0.run_icommand(command)
            self.assertEqual(0, status, stderr)
            self.assertEqual(expected, 'multi-1247 transfer' in stdout, stdout)
            return stdout

        # New destination: the direct PUT/GET calls in rsyncUtil.
        sync_and_check_marker(['irsync', '-V', '-K', '-N', '3', '-R',
                               self.user0.default_resource, source, 'i:' + obj])
        self.user0.assert_icommand(['ichksum', '-K', obj])
        sync_and_check_marker(['irsync', '-V', '-K', '-N', '2', 'i:' + obj, downloaded])
        self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))
        os.unlink(downloaded)
        sync_and_check_marker(['irsync', '-V', 'i:' + obj, downloaded])
        self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))

        # Existing destination without a catalog checksum: the server-requested
        # PUT/GET actions inside rcDataObjRsync, not the direct calls above.
        self.user0.assert_icommand(['iput', '-N', '0', '-f', other, obj])
        sync_and_check_marker(['irsync', '-V', '-K', '-N', '2', source, 'i:' + obj])
        self.user0.assert_icommand(['iget', '-N', '0', '-f', '-K', obj, downloaded])
        self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))

        self.user0.assert_icommand(['iput', '-N', '0', other, unchecked_obj])
        stdout = sync_and_check_marker(['irsync', '-V', '-l', '-N', '2', 'i:' + unchecked_obj, downloaded], False)
        self.assertIn('   N', stdout)
        self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))
        stdout = sync_and_check_marker(['irsync', '-V', '-l', '-N', '2', 'i:' + unchecked_obj, other], False)
        self.assertNotIn('   N', stdout)
        sync_and_check_marker(['irsync', '-V', '-N', '2', 'i:' + unchecked_obj, downloaded])
        self.assertTrue(filecmp.cmp(other, downloaded, shallow=False))

        # Matches and dry runs do not transfer; -N 0 keeps the legacy path.
        sync_and_check_marker(['irsync', '-V', '-N', '2', 'i:' + unchecked_obj, downloaded], False)
        sync_and_check_marker(['irsync', '-V', '-s', '-N', '2', source, 'i:' + unchecked_obj], False)
        sync_and_check_marker(['irsync', '-V', '-l', '-N', '2', source, 'i:' + unchecked_obj], False)
        sync_and_check_marker(['irsync', '-V', '-N', '0', source, 'i:' + unchecked_obj], False)

    def test_large_server_to_server_copies_keep_the_server_side_path(self):
        source = os.path.join(self.testing_tmp_dir, 'server_copy_source')
        other = os.path.join(self.testing_tmp_dir, 'server_copy_other')
        downloaded = os.path.join(self.testing_tmp_dir, 'server_copy_downloaded')
        source_resc = 'server_copy_source_resc'
        dest_resc = 'server_copy_dest_resc'
        source_obj = os.path.join(self.user0.session_collection, 'server_copy_source')
        copy_obj = os.path.join(self.user0.session_collection, 'server_copy_icp')
        new_sync_obj = os.path.join(self.user0.session_collection, 'server_copy_new_sync')
        existing_sync_obj = os.path.join(self.user0.session_collection, 'server_copy_existing_sync')
        legacy_obj = os.path.join(self.user0.session_collection, 'server_copy_legacy')
        same_host_obj = os.path.join(self.user0.session_collection, 'server_copy_same_host')
        move_obj = os.path.join(self.user0.session_collection, 'server_copy_phymv')
        lib.make_file(source, 34 * 1024 * 1024, 'random')
        lib.make_file(other, 34 * 1024 * 1024, 'zero')

        def assert_server_copy(command, logical_path):
            log_offset = lib.get_file_size_by_path(paths.server_log_path())
            stdout, stderr, status = self.user0.run_icommand(command)
            self.assertEqual(0, status, stderr)
            # The client-side multi-1247 PUT/GET marker must not appear for
            # copies whose source and destination are both iRODS objects.
            self.assertNotIn('multi-1247 transfer', stdout)
            lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(
                msg='multi-1247 server copy: ' + logical_path,
                count=0, start_index=log_offset))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, logical_path, dest_resc))
            self.user0.assert_icommand(['ichksum', '-K', logical_path])
            self.user0.assert_icommand(['iget', '-N', '0', '-f', logical_path, downloaded])
            self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))

        # On a topology, force the source onto consumer 1 and the targets
        # onto consumer 2, rather than merely copying within one server.
        lib.create_ufs_resource(self.admin, source_resc, hostname=test.settings.HOSTNAME_1)
        try:
            lib.create_ufs_resource(self.admin, dest_resc, hostname=test.settings.HOSTNAME_2)
            try:
                self.user0.assert_icommand(['iput', '-N', '0', '-K', '-R', source_resc,
                                            source, source_obj])
                self.assertTrue(lib.replica_exists_on_resource(self.user0, source_obj, source_resc))

                # icp and a missing irsync target take the DATA_OBJ_COPY path.
                assert_server_copy(['icp', '-V', '-K', '-N', '3', '-R', dest_resc,
                                    source_obj, copy_obj], copy_obj)
                assert_server_copy(['irsync', '-V', '-K', '-N', '3', '-R', dest_resc,
                                    'i:' + source_obj, 'i:' + new_sync_obj], new_sync_obj)

                # An existing target without a catalog checksum delegates the
                # comparison and transfer to DATA_OBJ_RSYNC instead.
                self.user0.assert_icommand(['iput', '-N', '0', '-R', dest_resc,
                                            other, existing_sync_obj])
                assert_server_copy(['irsync', '-V', '-N', '3', '-R', dest_resc,
                                    'i:' + source_obj, 'i:' + existing_sync_obj], existing_sync_obj)

                stdout, stderr, status = self.user0.run_icommand(
                    ['irsync', '-V', '-N', '3', '-R', dest_resc,
                     'i:' + source_obj, 'i:' + existing_sync_obj])
                self.assertEqual(0, status, stderr)
                self.assertIn('a match', stdout)
                self.assertNotIn('multi-1247 transfer', stdout)

                # Disabling threading keeps the original single-copy route.
                log_offset = lib.get_file_size_by_path(paths.server_log_path())
                stdout, stderr, status = self.user0.run_icommand(
                    ['icp', '-V', '-N', '0', '-R', dest_resc, source_obj, legacy_obj])
                self.assertEqual(0, status, stderr)
                self.assertNotIn('multi-1247 transfer', stdout)
                self.assertTrue(lib.replica_exists_on_resource(self.user0, legacy_obj, dest_resc))
                self.assertTrue(lib.log_message_occurrences_equals_count(
                    msg='multi-1247 server copy: ' + legacy_obj,
                    count=0, start_index=log_offset))
                self.user0.assert_icommand(['iget', '-N', '0', '-f', legacy_obj, downloaded])
                self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))

                log_offset = lib.get_file_size_by_path(paths.server_log_path())
                self.user0.assert_icommand(['icp', '-N', '3', '-R', source_resc,
                                            source_obj, same_host_obj])
                self.assertTrue(lib.replica_exists_on_resource(self.user0, same_host_obj, source_resc))
                self.assertTrue(lib.log_message_occurrences_equals_count(
                    msg='multi-1247 server copy: ' + same_host_obj,
                    count=0, start_index=log_offset))
                self.user0.assert_icommand(['iget', '-N', '0', '-f', same_host_obj, downloaded])
                self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))

                # Replication shares dataObjCopy with icp and irsync.
                log_offset = lib.get_file_size_by_path(paths.server_log_path())
                self.user0.assert_icommand(['irepl', '-N', '3', '-R', dest_resc, source_obj])
                lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(
                    msg='multi-1247 server copy: ' + source_obj,
                    count=0, start_index=log_offset))
                self.assertTrue(lib.replica_exists_on_resource(self.user0, source_obj, dest_resc))
                self.user0.assert_icommand(['ichksum', '-K', source_obj])

                # Physical moves use the same byte-copy function, but must
                # still remove the source replica only after finalization.
                self.user0.assert_icommand(['iput', '-N', '0', '-R', source_resc,
                                            source, move_obj])
                log_offset = lib.get_file_size_by_path(paths.server_log_path())
                self.user0.assert_icommand(['iphymv', '-S', source_resc, '-R', dest_resc, move_obj])
                lib.delayAssert(lambda: lib.log_message_occurrences_greater_than_count(
                    msg='multi-1247 server copy: ' + move_obj,
                    count=0, start_index=log_offset))
                self.assertFalse(lib.replica_exists_on_resource(self.user0, move_obj, source_resc))
                self.assertTrue(lib.replica_exists_on_resource(self.user0, move_obj, dest_resc))
                self.user0.assert_icommand(['iget', '-N', '0', '-f', move_obj, downloaded])
                self.assertTrue(filecmp.cmp(source, downloaded, shallow=False))
            finally:
                for obj in (source_obj, copy_obj, new_sync_obj, existing_sync_obj,
                            legacy_obj, same_host_obj, move_obj):
                    self.user0.run_icommand(['irm', '-f', obj])
                lib.remove_resource(self.admin, dest_resc)
        finally:
            lib.remove_resource(self.admin, source_resc)

    def iput_r_large_collection(self, user_session, base_name, file_count, file_size):
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = lib.make_large_local_tmp_dir(local_dir, file_count, file_size)
        user_session.assert_icommand(['iput', '-r', local_dir])
        rods_files = set(user_session.get_entries_in_collection(base_name))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing:\n" + str(set(local_files) - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - set(local_files)))
        if not test.settings.RUN_IN_TOPOLOGY:
            vault_files = set(os.listdir(os.path.join(user_session.get_vault_session_path(), base_name)))
            self.assertTrue(set(local_files) == vault_files,
                            msg="Files missing from vault:\n" + str(set(local_files) - vault_files) + "\n\n" +
                                "Extra files in vault:\n" + str(vault_files - set(local_files)))
        return (local_dir, local_files)

    def test_irsync(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('irsync -l ' + filepath + ' i:file')

    def test_irsync_checksum_behavior(self):
        # set user0's checksum scheme to MD5 to mismatch with server scheme
        user0_env_backup = copy.deepcopy(self.user0.environment_file_contents)

        self.user0.environment_file_contents['irods_default_hash_scheme'] = 'MD5'
        self.user0.environment_file_contents['irods_match_hash_policy'] = 'compatible'

        # test settings
        depth = 1
        files_per_level = 5
        file_size = 1024*1024*40

        # make local nested dirs
        base_name = 'test_irsync_checksum_behavior'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r -K {local_dir} i:{base_name}".format(**locals()))
        self.user0.assert_icommand("ils -L {base_name}".format(**locals()), "STDOUT_SINGLELINE", "ec8bb3b24d5b0f1b5bdf8c8f0f541ee6")

        out, err, ec = self.user0.run_icommand("ichksum -r -K {base_name}".format(**locals()))
        self.assertEqual(ec, 0)
        self.assertTrue('C- ' in out)
        self.assertTrue('WARNING: ' not in out and 'WARNING: ' not in err)
        self.assertTrue('ERROR: ' not in out and 'ERROR: ' not in err)
        self.user0.assert_icommand("irsync -v -r -K -l {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", "junk0001                       40.000 MB --- a match no sync required")

        self.user0.assert_icommand("irm -f {base_name}/junk0001".format(**locals()), "EMPTY")
        self.user0.assert_icommand_fail("ils -L {base_name}".format(**locals()), "STDOUT_SINGLELINE", "junk0001")

        self.user0.assert_icommand("irsync -v -r -K -l {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", "junk0001   41943040   N")
        self.user0.assert_icommand("irsync -v -r -K {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", "junk0001                       40.000 MB ")
        self.user0.assert_icommand("irsync -v -r -K -l {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", "junk0001                       40.000 MB --- a match no sync required")

        path = '{local_dir}/junk0001'.format(**locals())
        if os.path.exists(path):
            os.unlink(path)

        self.user0.assert_icommand("irsync -v -r -K -l i:{base_name} {local_dir}".format(**locals()), "STDOUT_SINGLELINE", "junk0001   41943040   N")
        self.user0.assert_icommand("irsync -v -r -K i:{base_name} {local_dir}".format(**locals()), "STDOUT_SINGLELINE", "junk0001                       40.000 MB ")
        self.user0.assert_icommand("irsync -v -r -K -l i:{base_name} {local_dir}".format(**locals()), "STDOUT_SINGLELINE", "junk0001                       40.000 MB --- a match no sync required")

        self.user0.environment_file_contents = user0_env_backup

    def test_irsync_r_dir_to_coll(self):
        base_name = "test_irsync_r_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_names = set(lib.make_large_local_tmp_dir(local_dir, file_count=1000, file_size=100))

        self.user0.assert_icommand("irsync -r " + local_dir + " i:" + base_name)
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name)
        rods_files = set(self.user0.get_entries_in_collection(base_name))
        self.assertTrue(file_names == rods_files,
                        msg="Files missing:\n" + str(file_names - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - file_names))

        if not test.settings.RUN_IN_TOPOLOGY:
            vault_files_post_irsync = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                  base_name)))

            self.assertTrue(file_names == vault_files_post_irsync,
                            msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync) + "\n\n" +
                                "Extra files in vault:\n" + str(vault_files_post_irsync - file_names))

    def test_irsync_r_coll_to_dir(self):
        base_name_source = "test_irsync_r_coll_to_dir_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])
        local_dir = os.path.join(self.testing_tmp_dir, "test_irsync_r_coll_to_dir_target")
        self.user0.assert_icommand("irsync -r i:" + base_name_source + " " + local_dir, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_source)
        rods_files_source = set(self.user0.get_entries_in_collection(base_name_source))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        local_files = set(os.listdir(local_dir))
        self.assertTrue(file_names == local_files,
                        msg="Files missing from local dir:\n" + str(file_names - local_files) + "\n\n" +
                            "Extra files in local dir:\n" + str(local_files - file_names))

        if not test.settings.RUN_IN_TOPOLOGY:
            vault_files_post_irsync_source = set(
                os.listdir(os.path.join(self.user0.get_vault_session_path(), base_name_source)))

            self.assertTrue(file_names == vault_files_post_irsync_source,
                            msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                                "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

    def test_irsync_r_coll_to_coll(self):
        base_name_source = "test_irsync_r_coll_to_coll_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])
        base_name_target = "test_irsync_r_coll_to_coll_target"
        self.user0.assert_icommand("irsync -r i:" + base_name_source + " i:" + base_name_target, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_source)
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_target)
        rods_files_source = set(self.user0.get_entries_in_collection(base_name_source))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(self.user0.get_entries_in_collection(base_name_target))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files :\n" + str(rods_files_target - file_names))

        if not test.settings.RUN_IN_TOPOLOGY:
            vault_files_post_irsync_source = set(
                os.listdir(os.path.join(self.user0.get_vault_session_path(), base_name_source)))

            self.assertTrue(file_names == vault_files_post_irsync_source,
                            msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                                "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

            vault_files_post_irsync_target = set(
                os.listdir(os.path.join(self.user0.get_vault_session_path(), base_name_target)))

            self.assertTrue(file_names == vault_files_post_irsync_target,
                            msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_target) + "\n\n" +
                                "Extra files in vault:\n" + str(vault_files_post_irsync_target - file_names))

    def test_irsync_r_nested_dir_to_coll(self):
        # test settings
        depth = 10
        files_per_level = 100
        file_size = 100

        # make local nested dirs
        base_name = "test_irsync_r_nested_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()))

        # compare files at each level
        for dir, files in local_dirs.items():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            if not test.settings.RUN_IN_TOPOLOGY:
                # compare local files with files in vault
                files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(), partial_path)))
                self.assertTrue(local_files == files_in_vault,
                            msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                                "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_irsync_r_nested_dir_to_coll_large_files(self):
        # test settings
        depth = 4
        files_per_level = 4
        file_size = 1024*1024*40

        # make local nested dirs
        base_name = "test_irsync_r_nested_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()))

        # compare files at each level
        for dir, files in local_dirs.items():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            if not test.settings.RUN_IN_TOPOLOGY:
                # compare local files with files in vault
                files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(), partial_path)))
                self.assertTrue(local_files == files_in_vault,
                                msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                                "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_irsync_r_nested_coll_to_coll(self):
        # test settings
        depth = 10
        files_per_level = 100
        file_size = 100

        # make local nested dirs
        source_base_name = "test_irsync_r_nested_coll_to_coll_source"
        dest_base_name = "test_irsync_r_nested_coll_to_coll_dest"
        local_dir = os.path.join(self.testing_tmp_dir, source_base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()))

        # sync collections
        self.user0.assert_icommand("irsync -r i:{source_base_name} i:{dest_base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.items():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)

            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(source_partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            if not test.settings.RUN_IN_TOPOLOGY:
                # compare local files with files in vault
                files_in_vault = set(
                    lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(), source_partial_path)))
                self.assertTrue(local_files == files_in_vault,
                                msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                                    "Extra files in vault:\n" + str(files_in_vault - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(dest_partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            if not test.settings.RUN_IN_TOPOLOGY:
                # compare local files with files in vault
                files_in_vault = set(
                    lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(), dest_partial_path)))
                self.assertTrue(local_files == files_in_vault,
                                msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                                "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_irsync_r_nested_coll_to_coll_large_files(self):
        # test settings
        depth = 4
        files_per_level = 4
        file_size = 1024*1024*40

        # make local nested dirs
        source_base_name = "test_irsync_r_nested_coll_to_coll_source"
        dest_base_name = "test_irsync_r_nested_coll_to_coll_dest"
        local_dir = os.path.join(self.testing_tmp_dir, source_base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()))

        # sync collections
        self.user0.assert_icommand("irsync -r i:{source_base_name} i:{dest_base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.items():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)

            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(source_partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(dest_partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

    def test_irsync_r_nested_coll_to_dir(self):
        # test settings
        depth = 10
        files_per_level = 100
        file_size = 100

        # make local nested dirs
        base_name = "test_irsync_r_nested_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()))

        # remove local coll
        shutil.rmtree(local_dir)

        # now sync back coll to dir
        self.user0.assert_icommand("irsync -r i:{base_name} {local_dir}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.items():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

    def test_irsync_r_nested_coll_to_dir_large_files(self):
        # test settings
        depth = 4
        files_per_level = 4
        file_size = 1024*1024*40

        # make local nested dirs
        base_name = "test_irsync_r_nested_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()))

        # remove local coll
        shutil.rmtree(local_dir)

        # now sync back coll to dir
        self.user0.assert_icommand("irsync -r i:{base_name} {local_dir}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.items():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = self.user0.get_entries_in_collection(partial_path)

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.get_object_names_from_entries(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

    def test_irsync_r_symlink(self):

        # make local dir

        base_name = "test_irsync_r_symlink"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        lib.make_dir_p(local_dir)

        # make file
        file_name = os.path.join(local_dir, 'the_file')
        lib.make_file(file_name, 10)

        # make symlink with relative path
        link_path_1 = os.path.join(local_dir, 'link1')
        lib.execute_command(['ln', '-s', 'the_file', link_path_1])

        # make symlink with fully qualified path
        link_path_2 = os.path.join(local_dir, 'link2')
        lib.execute_command(['ln', '-s', file_name, link_path_2])

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()))

    def test_irsync_link_option_ignores_symlinks__issue_5359(self):
        """Creates a directory to target for irsync -r --ignore-symlinks with good and bad symlinks which should all be ignored."""

        dirname = 'test_irsync_link_option_ignores_broken_symlinks__issue_5359'
        collection_path = os.path.join(self.user0.session_collection, dirname)

        # Put all the files/dirs and symlinks in the test dir so it all gets cleaned up "for free".
        sync_dir = os.path.join(self.user0.local_session_dir, dirname)

        # These are the broken symlink files and directories.
        deleted_file_path = os.path.join(sync_dir, 'this_file_will_be_deleted')
        deleted_subdir_path = os.path.join(sync_dir, 'this_dir_will_be_deleted')
        broken_symlink_file_path = os.path.join(sync_dir, 'broken_symlink_file')
        broken_symlink_dir_path = os.path.join(sync_dir, 'broken_symlink_dir')

        # These are the data objects and collections which shouldn't exist for the broken symlink files and directories.
        nonexistent_data_object_path = os.path.join(collection_path, os.path.basename(deleted_file_path))
        nonexistent_subcollection_path = os.path.join(collection_path, os.path.basename(deleted_subdir_path))
        broken_symlink_data_object_path = os.path.join(collection_path, os.path.basename(broken_symlink_file_path))
        broken_symlink_collection_path = os.path.join(collection_path, os.path.basename(broken_symlink_dir_path))

        # These are the good symlink files and directories. The symlink target file and subdirectory actually have to
        # exist outside of the target directory because the symlinks will be followed and it cannot make multiples of
        # the same data object or collection.
        good_symlink_target_file_path = os.path.join(os.path.dirname(sync_dir), 'this_symlink_target_file_will_exist')
        good_symlink_target_dir_path = os.path.join(os.path.dirname(sync_dir), 'this_symlink_target_dir_will_exist')
        good_symlink_file_path = os.path.join(sync_dir, 'not_actually_broken_symlink_file')
        good_symlink_dir_path = os.path.join(sync_dir, 'not_actually_broken_symlink_dir')

        # These are the data objects and collections which shouldn't exist for the good symlink files and directories.
        good_symlink_target_file_data_object_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_file_path))
        good_symlink_target_dir_subcollection_path = os.path.join(
            collection_path, os.path.basename(good_symlink_target_dir_path))
        good_symlink_data_object_path = os.path.join(collection_path, os.path.basename(good_symlink_file_path))
        good_symlink_collection_path = os.path.join(collection_path, os.path.basename(good_symlink_dir_path))

        # These are the normal files and directories. These will be added so that SOMETHING is in iRODS at the end.
        existent_file_path = os.path.join(sync_dir, 'this_file_will_exist')
        existent_subdir_path = os.path.join(sync_dir, 'this_dir_will_exist')

        # These are the data objects and collections which should exist for the normal files and directories.
        existent_data_object_path = os.path.join(collection_path, os.path.basename(existent_file_path))
        existent_subcollection_path = os.path.join(collection_path, os.path.basename(existent_subdir_path))

        file_size = 1024 # I just made this up... it has no particular significance

        try:
            # Create the directory which will contain all the broken links...
            os.mkdir(sync_dir)
            self.assertTrue(os.path.exists(sync_dir))

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

            # Run irsync between the local directory and the collection (which is basically an iput). Use the --ignore-symlinks
            # option to instruct that symlinks are to be ignored, broken or not.
            self.user0.assert_icommand(['irsync', '--ignore-symlinks', '-r', sync_dir, f'i:{collection_path}'])

            # Confirm that the main collection was created - no funny business there.
            self.assertTrue(lib.collection_exists(self.user0, collection_path))

            # Confirm that a subcollection was created for the existent subcollection, but that no subcollection was
            # created for the symlink or the collection to which it points because irsync was supposed to ignore
            # symlinks.
            self.assertTrue(lib.collection_exists(self.user0, existent_subcollection_path))
            self.assertFalse(lib.collection_exists(self.user0, good_symlink_collection_path))
            self.assertFalse(lib.collection_exists(self.user0, good_symlink_target_dir_subcollection_path))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_collection_path, 0))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_target_dir_subcollection_path, 0))

            # Confirm that a subcollection was not created for the nonexistent subdirectory, and that no subcollection
            # was created for the symlink because irsync was supposed to ignore symlinks. Also make sure a data object
            # was not created.
            self.assertFalse(lib.collection_exists(self.user0, nonexistent_subcollection_path))
            self.assertFalse(lib.collection_exists(self.user0, broken_symlink_collection_path))
            self.assertFalse(lib.replica_exists(self.user0, broken_symlink_collection_path, 0))

            # Confirm that a data object was not created for the nonexistent file, and that no data object was created
            # for the symlink because irsync was supposed to ignore symlinks.
            self.assertFalse(lib.replica_exists(self.user0, nonexistent_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user0, broken_symlink_data_object_path, 0))

            # Confirm that a data object was created for the existent file, but that no data object was created for the
            # symlink because irsync was supposed to ignore symlinks.
            self.assertTrue(lib.replica_exists(self.user0, existent_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_target_file_data_object_path, 0))

        finally:
            self.user0.assert_icommand(['ils', '-lr'], 'STDOUT', self.user0.session_collection) # debugging

    def test_irsync_does_not_ignore_symlinks_by_default__issue_5359(self):
        """Creates a directory to target for irsync -r with good symlinks, all of which should be sync'd."""

        dirname = 'test_irsync_does_not_ignore_symlinks_by_default__issue_5359'
        collection_path = os.path.join(self.user0.session_collection, dirname)

        # Put all the files/dirs and symlinks in the test dir so it all gets cleaned up "for free".
        sync_dir = os.path.join(self.user0.local_session_dir, dirname)

        # These are the good symlink files and directories. The symlink target file and subdirectory actually have to
        # exist outside of the target directory because the symlinks will be followed and it cannot make multiples of
        # the same data object or collection.
        good_symlink_target_file_path = os.path.join(os.path.dirname(sync_dir), 'this_symlink_target_file_will_exist')
        good_symlink_target_dir_path = os.path.join(os.path.dirname(sync_dir), 'this_symlink_target_dir_will_exist')
        good_symlink_file_path = os.path.join(sync_dir, 'not_actually_broken_symlink_file')
        good_symlink_dir_path = os.path.join(sync_dir, 'not_actually_broken_symlink_dir')

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
        existent_file_path = os.path.join(sync_dir, 'this_file_will_exist')
        existent_subdir_path = os.path.join(sync_dir, 'this_dir_will_exist')

        # These are the data objects and collections which should exist for the normal files and directories.
        existent_data_object_path = os.path.join(collection_path, os.path.basename(existent_file_path))
        existent_subcollection_path = os.path.join(collection_path, os.path.basename(existent_subdir_path))

        file_size = 1024 # I just made this up... it has no particular significance

        try:
            # Create the directory which will contain all the broken links...
            os.mkdir(sync_dir)
            self.assertTrue(os.path.exists(sync_dir))

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

            # Run irsync between the local directory and the collection (which is basically an iput).
            self.user0.assert_icommand(['irsync', '-r', sync_dir, f'i:{collection_path}'])

            # Confirm that the main collection was created - no funny business there.
            self.assertTrue(lib.collection_exists(self.user0, collection_path))

            # Confirm that a subcollection was created for the existent subdirectory and the symlinked subdirectory, but
            # that no subcollection was created for the symlink itself because it should have been followed.
            self.assertTrue(lib.collection_exists(self.user0, existent_subcollection_path))
            self.assertTrue(lib.collection_exists(self.user0, good_symlink_collection_path))
            self.assertFalse(lib.collection_exists(self.user0, good_symlink_target_dir_subcollection_path))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_target_dir_subcollection_path, 0))

            # Confirm that a data object was created for the existent file and the symlinked file, but that no data
            # object was created for the symlink itself because it should have been followed.
            self.assertTrue(lib.replica_exists(self.user0, existent_data_object_path, 0))
            self.assertTrue(lib.replica_exists(self.user0, good_symlink_data_object_path, 0))
            self.assertFalse(lib.replica_exists(self.user0, good_symlink_target_file_data_object_path, 0))

        finally:
            self.user0.assert_icommand(['ils', '-lr'], 'STDOUT', self.user0.session_collection) # debugging

    def test_irsync_ignore_symlinks_option_is_an_alias_of_the_link_option__issue_7537(self):
        # Create a file.
        test_file_basename = 'irsync_issue_7537.txt'
        test_file = f'{self.user0.local_session_dir}/{test_file_basename}'
        with open(test_file, 'w') as f:
            f.write('This file will be ignored by irsync due to the symlink.')

        # Create a directory for the symlink.
        # The symlink file must be placed in a separate directory due to the recursive flag for irsync.
        symlink_dir_basename = 'irsync_issue_7537_symlink_dir'
        symlink_dir = f'{self.user0.local_session_dir}/{symlink_dir_basename}'
        os.mkdir(symlink_dir)

        # Create a symlink to the file.
        symlink_file_basename = f'{test_file_basename}.symlink'
        symlink_file = f'{symlink_dir}/{symlink_file_basename}'
        os.symlink(test_file, symlink_file)
        self.assertTrue(os.path.exists(symlink_file))

        # Try to upload the symlink file into iRODS.
        self.user0.assert_icommand(['irsync', '--ignore-symlinks', symlink_file, f'i:{symlink_file_basename}'])
        self.user0.assert_icommand(['ils', '-lr'], 'STDOUT') # Debugging

        # Show no data object exists, proving the symlink file was ignored.
        logical_path = f'{self.user0.session_collection}/{symlink_file_basename}'
        self.assertFalse(lib.replica_exists(self.user0, logical_path, 0))

        # Try to upload the symlink file again using the recursive flag and the parent directory.
        self.user0.assert_icommand(['irsync', '--ignore-symlinks', '-r', symlink_dir, f'i:{symlink_dir_basename}'])
        self.user0.assert_icommand(['ils', '-lr'], 'STDOUT') # Debugging

        # Show no data object exists, proving the symlink file was ignored.
        logical_path = f'{self.user0.session_collection}/{symlink_dir_basename}/{symlink_file_basename}'
        self.assertFalse(lib.replica_exists(self.user0, logical_path, 0))

    def test_irsync_succeeds_with_checksum_and_stale_replica__issue_8384(self):
        # Create a file.
        test_file_basename = 'irsync_issue_8384.txt'
        test_file = os.path.join(self.admin.local_session_dir,test_file_basename)

        with open(test_file, 'w') as f:
            f.write('Random test data.')

        # Set up hierarchy similar to the issue
        replication_resc = 'repl_resc_8384'
        child_resc = 'ufs1_8384'
        other_child_resc = 'ufs2_8384'
        source_coll = 'thesource_8384'
        dest_coll = 'thetarget_8384'
        child_resc_dir = os.path.join(self.admin.local_session_dir, 'ufs1vault_8384')
        other_child_resc_dir = os.path.join(self.admin.local_session_dir, 'ufs2vault_8384')

        try:
            self.admin.assert_icommand(['imkdir', f'{self.admin.session_collection}/{source_coll}', f'{self.admin.session_collection}/{dest_coll}'])
            self.admin.assert_icommand(['iadmin', 'mkresc', replication_resc, 'replication' ], 'STDOUT_SINGLELINE', 'replication')
            self.admin.assert_icommand(['iadmin', 'mkresc', child_resc, 'unixfilesystem', f'{lib.get_hostname()}:{child_resc_dir}'], 'STDOUT_SINGLELINE', 'unixfilesystem')
            self.admin.assert_icommand(['iadmin', 'mkresc', other_child_resc, 'unixfilesystem', f'{lib.get_hostname()}:{other_child_resc_dir}'], 'STDOUT_SINGLELINE', 'unixfilesystem')
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', replication_resc, child_resc])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', replication_resc, other_child_resc])

            self.admin.assert_icommand(['iput', test_file, f'{self.admin.session_collection}/{source_coll}'])
            self.admin.assert_icommand(['iput', '-R', replication_resc, test_file, f'{self.admin.session_collection}/{dest_coll}'])
            self.admin.assert_icommand(['ichksum', '-n', '0', f'{self.admin.session_collection}/{dest_coll}/{test_file_basename}'], 'STDOUT_SINGLELINE', test_file_basename)
            self.admin.assert_icommand(['ichksum', '-n', '1', f'{self.admin.session_collection}/{dest_coll}/{test_file_basename}'], 'STDOUT_SINGLELINE', test_file_basename)

            # Set one replica to stale
            # rsDataObjChksum will return with CHECK_VERIFICATION_RESULTS if any replicas are skipped
            # Previously, this was treated as a hard error, so irsync terminated early
            # This should no longer happen
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', f'{self.admin.session_collection}/{dest_coll}/{test_file_basename}', 'replica_number', '1', 'DATA_REPL_STATUS', '0'])

            # Should no longer error out due to CHECK_VERIFICATION_RESULTS
            self.admin.assert_icommand(['irsync', '-K', '-v', '-r', '-R', replication_resc, f'i:{source_coll}', f'i:{dest_coll}' ], 'STDOUT_SINGLELINE', test_file_basename)

            # Copy should have happened and set the replica to good
            self.assertTrue(lib.get_replica_status(self.admin, test_file_basename, 1) == '1')

        finally:
            # Clean up
            self.admin.run_icommand(['irm', '-rf', f'{self.admin.session_collection}/{source_coll}'])
            self.admin.run_icommand(['irm', '-rf', f'{self.admin.session_collection}/{dest_coll}'])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', replication_resc, child_resc])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', replication_resc, other_child_resc])
            self.admin.run_icommand(['iadmin', 'rmresc', child_resc])
            self.admin.run_icommand(['iadmin', 'rmresc', other_child_resc])
            self.admin.run_icommand(['iadmin', 'rmresc', replication_resc])

    def test_irsync_does_not_create_collections_nor_directories_on_dry_run_issue_7774(self):
        source_coll = 'thesource_7774'
        dest_coll = 'thetarget_7774'
        source_coll_path = f'{self.user0.session_collection}/{source_coll}'
        dest_coll_path = f'{self.user0.session_collection}/{dest_coll}'

        dest_dir_should_not_exist = 'nonexistent_7774'
        nonexistent_dir_path = os.path.join(self.testing_tmp_dir, dest_dir_should_not_exist)

        self.user0.assert_icommand(['imkdir', source_coll_path])

        # Make some dummy data objects
        self.user0.assert_icommand(['itouch', f'{source_coll_path}/one'])
        self.user0.assert_icommand(['itouch', f'{source_coll_path}/two'])
        self.user0.assert_icommand(['itouch', f'{source_coll_path}/three'])

        # Perform the irsync
        self.user0.assert_icommand(['irsync', '-rl', f'i:{source_coll_path}', f'i:{dest_coll_path}'], 'STDOUT', ['one', 'two', 'three'])

        # Assure that the target collection does not exist
        self.user0.assert_icommand(['ils', '-L', dest_coll_path], 'STDERR_SINGLELINE', 'does not exist')

        # Other case-- ensure that irods-to-local irsync do not create the target dir
        self.user0.assert_icommand(['irsync', '-rl', f'i:{source_coll_path}', nonexistent_dir_path], 'STDOUT', ['one', 'two', 'three'])
        self.assertFalse(os.path.exists(nonexistent_dir_path))

    def test_irsync_with_all_keyword_does_not_fail_on_overwrite__issue_8295(self):
        data_object_name = 'test_dataobj_8295'
        test_data_object_path = f'{self.user0.session_collection}/{data_object_name}'

        test_file_path = os.path.join(self.testing_tmp_dir, data_object_name)
        empty_file_path = os.path.join(self.testing_tmp_dir, 'empty_file_8295')

        with open(test_file_path, 'w') as f:
            f.write('potato')

        with open(empty_file_path, 'w') as f:
            pass

        self.user0.assert_icommand(['iput', empty_file_path, test_data_object_path])

        self.user0.assert_icommand(['irsync', '-a', test_file_path, f'i:{test_data_object_path}'])

    def test_irsync_with_all_keyword__issue_8613(self):
        data_object_name = 'all_keyword_dataobj'
        another_resc  = 'resc_all_keyword'
        data_object_path = f'{self.user0.session_collection}/{data_object_name}'

        test_file_path = os.path.join(self.testing_tmp_dir, data_object_name)
        empty_file_path = os.path.join(self.testing_tmp_dir, 'empty_file_8613')

        with open(test_file_path, 'w') as f:
            f.write('potato')

        with open(empty_file_path, 'w') as f:
            pass

        self.user0.assert_icommand(['iput', empty_file_path, data_object_path])
        lib.create_ufs_resource(self.admin, another_resc)

        try:
            self.user0.assert_icommand(['irepl', '-R', another_resc, data_object_path])
            self.user0.assert_icommand(['iput', '-f', test_file_path, data_object_path])

            # Replica should be stale after the iput
            self.assertEqual(lib.get_replica_status(self.user0, data_object_name, 1), '0')

            # Edit the file, or irsync will not do the copy
            with open(test_file_path, 'w') as f:
                f.write('mashed potato')

            self.user0.assert_icommand(['irsync', '-a', test_file_path, f'i:{data_object_path}'])

            # Replica should be good after the irsync
            self.assertEqual(lib.get_replica_status(self.user0, data_object_name, 1), '1')

        finally:
            self.user0.run_icommand(['irm', '-f', data_object_path])
            self.admin.run_icommand(['iadmin', 'rmresc', another_resc])

    def test_irsync_overwrites_stale_replicas__issue_8590(self):
        source_coll = 'thesource_8590'
        dest_coll = 'thetarget_8590'
        data_object_name = 'stale_replica_file_8590'

        test_file_path = os.path.join(self.testing_tmp_dir, data_object_name)
        empty_file_path = os.path.join(self.testing_tmp_dir, 'empty_file_8590')

        source_coll_path = f'{self.user0.session_collection}/{source_coll}'
        dest_coll_path = f'{self.user0.session_collection}/{dest_coll}'

        dest_data_obj_path = f'{dest_coll_path}/{data_object_name}'

        with open(test_file_path, 'w') as f:
            f.write('potato')

        with open(empty_file_path, 'w') as f:
            pass

        self.user0.assert_icommand(['imkdir', source_coll_path])
        self.user0.assert_icommand(['imkdir', dest_coll_path])

        self.user0.assert_icommand(['iput', test_file_path, f'{source_coll_path}/{data_object_name}'])
        self.user0.assert_icommand(['iput', empty_file_path, dest_data_obj_path])

        # Stale the replica to check that irsync will not fail with SYS_NO_GOOD_REPLICA when the target is stale
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', dest_data_obj_path, 'replica_number', '0', 'DATA_REPL_STATUS', '0'])

        # Should not fail with SYS_NO_GOOD_REPLICA
        self.user0.assert_icommand(['irsync', '-K', '-v', '-r', f'i:{source_coll_path}', f'i:{dest_coll_path}' ], 'STDOUT_SINGLELINE', data_object_name)

        # Copy should have happened and set the replica to good
        self.assertEqual(lib.get_replica_status(self.admin, data_object_name, 0), '1')

        # Ensure destination has a checksum
        self.user0.assert_icommand(['ichksum', dest_data_obj_path], 'STDOUT', [data_object_name, 'sha2:6RwlStWIYKAseI37XBpl1qiEarHcZJYxx9sW/vSvLew='])
        # Stale the replica to check that irsync will still overwrite even when the checksums match (i.e. do not trust a stale checksum)
        self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', dest_data_obj_path, 'replica_number', '0', 'DATA_REPL_STATUS', '0'])

        # Should cause an overwrite
        self.user0.assert_icommand(['irsync', '-K', '-v', '-r', f'i:{source_coll_path}', f'i:{dest_coll_path}' ], 'STDOUT_SINGLELINE', data_object_name)

        # Copy should have happened and set the replica to good
        self.assertEqual(lib.get_replica_status(self.admin, data_object_name, 0), '1')
