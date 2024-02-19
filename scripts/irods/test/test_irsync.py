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
from . import ustrings

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
        self.user0.assert_icommand("irsync -r -K {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
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

        self.user0.assert_icommand("irsync -r " + local_dir + " i:" + base_name, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
