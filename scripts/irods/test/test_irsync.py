import copy
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

    def iput_r_large_collection(self, user_session, base_name, file_count, file_size):
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = lib.make_large_local_tmp_dir(local_dir, file_count, file_size)
        user_session.assert_icommand(['iput', '-r', local_dir])
        rods_files = set(user_session.get_entries_in_collection(base_name))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing:\n" + str(set(local_files) - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - set(local_files)))
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

        vault_files_post_irsync = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                              base_name)))

        if not test.settings.RUN_IN_TOPOLOGY:
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
        """Creates a directory to target for irsync -r --link with good and bad symlinks which should all be ignored."""

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

            # Run irsync between the local directory and the collection (which is basically an iput). Use the --link
            # option to instruct that symlinks are to be ignored, broken or not.
            self.user0.assert_icommand(['irsync', '--link', '-r', sync_dir, f'i:{collection_path}'])

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
