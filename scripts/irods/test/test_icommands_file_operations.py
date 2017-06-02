import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import contextlib
import errno
import inspect
import logging
import os
import pprint
import tempfile
import time
import shutil

from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from .. import paths
from .. import test
from . import settings
from .. import lib
from . import resource_suite
from .rule_texts_for_tests import rule_texts


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
class Test_ICommands_File_Operations(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_ICommands_File_Operations'

    def setUp(self):
        super(Test_ICommands_File_Operations, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-icommands-recursive'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_ICommands_File_Operations, self).tearDown()

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

    def iput_to_root_collection(self):
        filename = 'original.txt'
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand('iput ' + filename + ' /nopes', 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM')

    def imkdir_to_root_collection(self):
        self.admin.assert_icommand('imkdir /nopes', 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM')

    def test_icommand_help_and_error_codes(self):
        icmds = ['iput', 'iget']
        for i in icmds:
            self.admin.assert_icommand(i + ' -h', 'STDOUT_SINGLELINE', 'Usage', desired_rc=0)
            self.admin.assert_icommand(i + ' -thisisanerror', 'STDERR_SINGLELINE', 'Usage', desired_rc=1)

    def test_iget_with_verify_to_stdout(self):
        self.admin.assert_icommand("iget -K nopes -", 'STDERR_SINGLELINE', 'Cannot verify checksum if data is piped to stdout')

    def test_force_iput_to_diff_resc(self):
        filename = "original.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iput -f " + filename)  # put file
        self.admin.assert_icommand("iput -fR " + self.testresc + " " + filename, 'STDERR_SINGLELINE', 'HIERARCHY_ERROR')  # fail

    def test_get_null_perms__2833(self):
        l = logging.getLogger(__name__)
        base_name = 'test_dir_for_perms'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = lib.make_large_local_tmp_dir(local_dir, 30, 10)
        self.admin.assert_icommand(['iput', '-r', local_dir])
        ils_out, _, _ = self.admin.run_icommand(['ils', base_name])
        rods_files = [f for f in lib.get_object_names_from_entries(ils_out)]

        self.admin.assert_icommand(['ichmod','-r','null',self.admin.username,base_name])
        self.admin.assert_icommand(['ichmod','read',self.admin.username,base_name+'/'+rods_files[-1]])
        self.admin.assert_icommand(['iget','-r',base_name,self.admin.local_session_dir],'STDERR_SINGLELINE','CAT_NO_ACCESS_PERMISSION')

        assert os.path.isfile(os.path.join(self.admin.local_session_dir,base_name,rods_files[-1]))

        self.admin.assert_icommand(['ichmod','-r','own',self.admin.username,base_name])

    def test_iput_r(self):
        self.iput_r_large_collection(self.user0, "test_iput_r_dir", file_count=1000, file_size=100)

    def test_irm_r(self):
        base_name = "test_irm_r_dir"
        self.iput_r_large_collection(self.user0, base_name, file_count=1000, file_size=100)

        self.user0.assert_icommand("irm -r " + base_name, "EMPTY")
        self.user0.assert_icommand("ils " + base_name, 'STDERR_SINGLELINE', "does not exist")

        vault_files_post_irm = os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                       base_name))
        self.assertTrue(len(vault_files_post_irm) == 0,
                        msg="Files not removed from vault:\n" + str(vault_files_post_irm))

    def test_irm_rf_nested_coll(self):
        # test settings
        depth = 50
        files_per_level = 5
        file_size = 5

        # make local nested dirs
        coll_name = "test_irm_r_nested_coll"
        local_dir = os.path.join(self.testing_tmp_dir, coll_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

        # force remove collection
        self.user0.assert_icommand("irm -rf {coll_name}".format(**locals()), "EMPTY")
        self.user0.assert_icommand("ils {coll_name}".format(**locals()), 'STDERR_SINGLELINE', "does not exist")

        # make sure no files are left in the vault
        user_vault_dir = os.path.join(self.user0.get_vault_session_path(), coll_name)
        out, _ = lib.execute_command('find {user_vault_dir} -type f'.format(**locals()))
        self.assertEqual(out, '')

    def test_iput_r_with_kw_and_obj_count(self):
        # test settings
        depth = 50
        files_per_level = 5
        file_size = 5

        # make local nested dirs
        coll_name = "test_iput_r_with_kw_and_obj_count"
        local_dir = os.path.join(self.testing_tmp_dir, coll_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # restart server with LOG_DEBUG
        env = os.environ.copy()
        env['spLogLevel'] = '7'
        IrodsController(IrodsConfig(injected_environment=env)).restart()

        # get log offset
        initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_log_path)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

        # look for occurences of debug sequences in the log
        obj_count_string = 'DEBUG: unix_file_resolve_hierarchy: object_count = [{0}]'.format(files_per_level * depth)
        rec_op_kw_string = 'DEBUG: unix_file_resolve_hierarchy: recursiveOpr = [1]'
        obj_count_string_count = lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, obj_count_string, start_index=initial_size_of_server_log)
        rec_op_kw_string_count = lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, rec_op_kw_string, start_index=initial_size_of_server_log)

        # assertions
        self.assertEqual(obj_count_string_count, files_per_level * depth)
        self.assertEqual(rec_op_kw_string_count, files_per_level * depth)

        # restart server with original environment
        del(env['spLogLevel'])
        IrodsController(IrodsConfig(injected_environment=env)).restart()

    def test_imv_r(self):
        base_name_source = "test_imv_r_dir_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_imv_r_dir_target"
        self.user0.assert_icommand("imv " + base_name_source + " " + base_name_target, "EMPTY")
        self.user0.assert_icommand("ils " + base_name_source, 'STDERR_SINGLELINE', "does not exist")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_target)
        rods_files_post_imv = set(self.user0.get_entries_in_collection(base_name_target))
        self.assertTrue(file_names == rods_files_post_imv,
                        msg="Files missing:\n" + str(file_names - rods_files_post_imv) + "\n\n" +
                            "Extra files:\n" + str(rods_files_post_imv - file_names))

        vault_files_post_irm_source = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                  base_name_source)))
        self.assertTrue(len(vault_files_post_irm_source) == 0)

        vault_files_post_irm_target = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                  base_name_target)))
        self.assertTrue(file_names == vault_files_post_irm_target,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irm_target) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irm_target - file_names))

    def test_icp_r(self):
        base_name_source = "test_icp_r_dir_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_icp_r_dir_target"
        self.user0.assert_icommand("icp -r " + base_name_source + " " + base_name_target, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_target)
        rods_files_source = set(self.user0.get_entries_in_collection(base_name_source))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(self.user0.get_entries_in_collection(base_name_target))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files:\n" + str(rods_files_target - file_names))

        vault_files_post_icp_source = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                  base_name_source)))

        self.assertTrue(file_names == vault_files_post_icp_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_source - file_names))

        vault_files_post_icp_target = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                  base_name_target)))
        self.assertTrue(file_names == vault_files_post_icp_target,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_target) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_target - file_names))

    def test_icp_collection_into_itself(self):
        base_name_source = "test_icp_collection_into_itself"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1, file_size=100)[1])

        self.user0.assert_icommand("icp -r " + base_name_source + " " + base_name_source + "/", 'STDERR_SINGLELINE', 'SAME_SRC_DEST_PATHS_ERR')

    def test_irsync_r_dir_to_coll(self):
        base_name = "test_irsync_r_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_names = set(lib.make_large_local_tmp_dir(local_dir, file_count=1000, file_size=100))

        self.user0.assert_icommand("irsync -r " + local_dir + " i:" + base_name, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name)
        rods_files = set(self.user0.get_entries_in_collection(base_name))
        self.assertTrue(file_names == rods_files,
                        msg="Files missing:\n" + str(file_names - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - file_names))

        vault_files_post_irsync = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                              base_name)))

        self.assertTrue(file_names == vault_files_post_irsync,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync - file_names))

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

        vault_files_post_irsync_source = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

        vault_files_post_irsync_target = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                     base_name_target)))

        self.assertTrue(file_names == vault_files_post_irsync_target,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_target) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_target - file_names))

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

        vault_files_post_irsync_source = set(os.listdir(os.path.join(self.user0.get_vault_session_path(),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              partial_path)))
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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              partial_path)))
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
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              source_partial_path)))
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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              dest_partial_path)))
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
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              source_partial_path)))
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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              dest_partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

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
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_cancel_large_iput(self):
        base_name = 'test_cancel_large_put'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_size = pow(2, 30)
        file_name = lib.make_large_local_tmp_dir(local_dir, file_count=1, file_size=file_size)[0]
        file_local_full_path = os.path.join(local_dir, file_name)
        iput_cmd = "iput '" + file_local_full_path + "'"
        file_vault_full_path = os.path.join(self.user0.get_vault_session_path(), file_name)
        self.user0.interrupt_icommand(iput_cmd, file_vault_full_path, 10)

        # multiple threads could still be writing on the server side, so we need to wait for
        # the size in the vault to converge - then were done.
        old_size = None
        new_size = os.path.getsize(file_vault_full_path)
        while old_size != new_size:
            time.sleep(1)
            old_size = new_size
            new_size = os.path.getsize(file_vault_full_path)

        # wait for select() call to timeout, set to "SELECT_TIMEOUT_FOR_CONN", which is 60 seconds
        time.sleep(63)
        self.user0.assert_icommand('ils -l', 'STDOUT_SINGLELINE', [file_name, str(new_size)])

    def test_iphymv_root(self):
        self.admin.assert_icommand('iadmin mkresc test1 unixfilesystem ' + lib.get_hostname() + ':' + paths.irods_directory() + '/test1',
                'STDOUT_SINGLELINE', '')
        self.admin.assert_icommand('iadmin mkresc test2 unixfilesystem ' + lib.get_hostname() + ':' + paths.irods_directory() + '/test2',
                'STDOUT_SINGLELINE', '')
        self.admin.assert_icommand('iphymv -S test1 -R test2 -r /', 'STDERR_SINGLELINE',
                'ERROR: phymvUtil: \'/\' does not specify a zone; physical move only makes sense within a zone.')
        self.admin.assert_icommand('iadmin rmresc test1')
        self.admin.assert_icommand('iadmin rmresc test2')

    def test_delay_in_dynamic_pep__3342(self):
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            with tempfile.NamedTemporaryFile(prefix='test_delay_in_dynamic_pep__3342') as f:
                lib.make_file(f.name, 80, contents='arbitrary')
                self.admin.assert_icommand(['iput', '-f', f.name])
            time.sleep(35)
            assert 1 == lib.count_occurrences_of_string_in_log(paths.server_log_path(),
                'writeLine: inString = dynamic pep in delay', start_index=initial_size_of_server_log)

    def test_iput_bulk_check_acpostprocforput__2841(self):
        # prepare test directory
        number_of_files = 5
        dirname = self.admin.local_session_dir + '/files'
        # files less than 4200000 were failing to trigger the writeLine
        for filesize in range(5000, 6000000, 500000):
            files = lib.make_large_local_tmp_dir(dirname, number_of_files, filesize)
            # manipulate core.re and check the server log
            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
                time.sleep(1)  # remove once file hash fix is committed #2279

                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['iput', '-frb', dirname])
                assert number_of_files == lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'writeLine: inString = acPostProcForPut called for', start_index=initial_size_of_server_log)
                shutil.rmtree(dirname)

    def test_large_irods_maximum_size_for_single_buffer_in_megabytes_2880(self):
        self.admin.environment_file_contents['irods_maximum_size_for_single_buffer_in_megabytes'] = 2000
        with tempfile.NamedTemporaryFile(prefix='test_large_irods_maximum_size_for_single_buffer_in_megabytes_2880') as f:
            lib.make_file(f.name, 800*1000*1000, contents='arbitrary')
            self.admin.assert_icommand(['iput', f.name, '-v'], 'STDOUT_SINGLELINE', '0 thr')

    def test_ils_connection_refused_2948(self):
        @contextlib.contextmanager
        def irods_server_stopped():
            control = IrodsController()
            control.stop()
            try:
                yield
            finally:
                control.start()
        with irods_server_stopped():
            self.admin.assert_icommand(['ils'], 'STDERR_SINGLELINE', 'Connection refused')
            _, out, err = self.admin.assert_icommand(['ils', '-V'], 'STDERR_SINGLELINE', 'Connection refused')
            assert 'errno = {0}'.format(errno.ECONNREFUSED) in out, 'missing ECONNREFUSED errno in\n' + out
            assert 'errno = {0}'.format(errno.ECONNABORTED) not in out, 'found ECONNABORTED errno in\n' + out
            assert 'errno = {0}'.format(errno.EINVAL) not in out, 'found EINVAL errno in\n' + out

    def test_iput_resc_scheme_forced(self):
        filename = 'test_iput_resc_scheme_forced_test_file.txt'
        filepath = lib.create_local_testfile(filename)

        # manipulate core.re and check the server log
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # test as rodsuser
            self.user0.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.user0.assert_icommand(['iput', '-f', filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.user0.assert_icommand(['irm', '-f', filename])

            self.user0.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.admin.assert_icommand(['irm', '-f', filename])

            os.unlink(filepath)

    def test_iput_resc_scheme_preferred(self):
        filename = 'test_iput_resc_scheme_preferred_test_file.txt'
        filepath = lib.create_local_testfile(filename)

        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # test as rodsuser
            self.user0.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.user0.assert_icommand(['iput', '-f', filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.user0.assert_icommand(['irm', '-f', filename])

            self.user0.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.admin.assert_icommand(['irm', '-f', filename])

            os.unlink(filepath)

    def test_iput_resc_scheme_null(self):
        filename = 'test_iput_resc_scheme_null_test_file.txt'
        filepath = lib.create_local_testfile(filename)

        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # test as rodsuser
            self.user0.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.user0.assert_icommand(['iput', '-f', filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.user0.assert_icommand(['irm', '-f', filename])

            self.user0.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR '+self.testresc, filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.admin.assert_icommand(['irm', '-f', filename])

            os.unlink(filepath)

    def test_igetwild_with_semicolon_in_filename(self):

        localfile = 'thelocalfile.txt'
        localpath = lib.create_local_testfile(localfile)
        badfiles = ['; touch oops', '\;\ touch\ oops']
        counter = 0
        for badname in badfiles:
            counter = counter + 1
            print("====================[{0}of{1}]=[{2}]===================".format(counter, len(badfiles), badname))
            badpath = lib.create_local_testfile(badname)
            os.unlink(badpath)
            self.user0.assert_icommand(['imkdir', 'subdir'])
            self.user0.assert_icommand(['ils', '-rL', 'subdir/'+badname], 'STDERR_SINGLELINE', 'does not exist')
            self.user0.assert_icommand(['iput', localfile, 'subdir/'+badname])
            self.user0.assert_icommand(['ils', '-rL'], 'STDOUT_SINGLELINE', 'subdir/'+badname)
            self.user0.assert_icommand(['ils', '-L', 'oops'], 'STDERR_SINGLELINE', 'does not exist')
            self.user0.assert_icommand(['igetwild', self.user0.session_collection+'/subdir', 'oops', 'e'], 'STDOUT_SINGLELINE', badname)
            assert os.path.isfile(badpath)
            assert not os.path.isfile(os.path.join(self.user0.session_collection, 'oops'))
            self.user0.assert_icommand(['irm', '-rf', 'subdir'])
            os.unlink(badpath)
        os.unlink(localpath)

    def test_ichksum_replica_reporting__3499(self):
        initial_file_contents = 'a'
        filename = 'test_ichksum_replica_reporting__3499'
        with open(filename, 'wb') as f:
            f.write(initial_file_contents)
        self.admin.assert_icommand(['iput', '-K', filename])
        vault_session_path = self.admin.get_vault_session_path()
        final_file_contents = 'b'
        with open(os.path.join(vault_session_path, filename), 'wb') as f:
            f.write(final_file_contents)
        out, err, rc = self.admin.run_icommand(['ichksum', '-KarR', 'demoResc', self.admin.session_collection])
        self.assertNotEqual(rc, 0)
        self.assertTrue(filename in out, out)
        self.assertTrue('replNum [0]' in out, out)
        self.assertTrue('USER_CHKSUM_MISMATCH' in err, err)
        os.unlink(filename)

    def test_irm_colloprstat__3572(self):
        collection_to_delete = 'collection_to_delete'
        self.admin.assert_icommand(['imkdir', collection_to_delete])
        filename = 'test_irm_colloprstat__3572'
        lib.make_file(filename, 50)
        for i in range(10):
            self.admin.assert_icommand(['iput', filename, '{0}/file_{1}'.format(collection_to_delete, str(i))])

        initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
        self.admin.assert_icommand(['irm', '-rf', collection_to_delete])
        self.assertEqual(0, lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'ERROR', start_index=initial_size_of_server_log))
        os.unlink(filename)
