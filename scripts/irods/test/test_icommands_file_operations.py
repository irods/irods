import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import contextlib
import copy
import errno
import inspect
import json
import logging
import os
import tempfile
import time
import shutil
import re

from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from .. import paths
from .. import test
from .. import lib
from . import resource_suite
from . import ustrings
from .rule_texts_for_tests import rule_texts


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
class Test_ICommands_File_Operations(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_ICommands_File_Operations'

    def setUp(self):
        # TODO: Remove this sleep when #4359 is resolved
        time.sleep(30)
        super(Test_ICommands_File_Operations, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-icommands-recursive'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_ICommands_File_Operations, self).tearDown()

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'only applicable for python REP')
    def test_re_serialization__prep_13(self):
        try:
            IrodsController().stop()
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            with temporary_core_file() as core:
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
                IrodsController().start()
                with tempfile.NamedTemporaryFile(prefix='test_re_serialization__prep_13') as f:
                    lib.make_file(f.name, 80, contents='arbitrary')
                    self.admin.assert_icommand(['iput', f.name])
            time.sleep(5)
            occur = lib.count_occurrences_of_regexp_in_log( paths.server_log_path(),
                                                            (r'writeLine: inString =\s*(\S+)=(\S*)',re.M),
                                                            start_index=initial_size_of_server_log)
            self.assertTrue(3 == len(occur))
            self.assertTrue(occur[0].group(1) == b'physical_path'   and occur[0].group(2).decode('utf-8').startswith(os.path.sep))
            self.assertTrue(occur[1].group(1) == b'logical_path'    and occur[1].group(2).startswith(b'/'))
            self.assertTrue(occur[2].group(1) == b'proxy_user_name' and occur[2].group(2).decode('utf-8') == self.admin.username)
        finally:
            IrodsController().reload_configuration()

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'only applicable for python REP')
    def test_re_serialization__prep_55(self):
        try:
            IrodsController().stop()
            initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
            with temporary_core_file() as core:
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
                IrodsController().start()
                with tempfile.NamedTemporaryFile(prefix='test_re_serialization__prep_55') as f:
                    lib.make_file(f.name, 80, contents='arbitrary')
                    self.admin.assert_icommand(['iput', f.name])
            time.sleep(5)
            occur = lib.count_occurrences_of_regexp_in_log( paths.server_log_path(),
                                                            (r'writeLine: inString =\s*(\S+)=(\S*)',re.M),
                                                            start_index=initial_size_of_server_log)
            self.assertTrue(1 == len(occur))
            self.assertTrue(occur[0].group(1) == b'user_rods_zone' and occur[0].group(2).decode('utf-8') == self.admin.zone_name)
        finally:
            IrodsController().reload_configuration()

    def iput_r_large_collection(self, user_session, base_name, file_count, file_size):
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = lib.make_large_local_tmp_dir(local_dir, file_count, file_size)
        user_session.assert_icommand(['iput', '-r', local_dir], "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        rods_files = set(user_session.get_entries_in_collection(base_name))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing:\n" + str(set(local_files) - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - set(local_files)))
        vault_files = set(os.listdir(os.path.join(user_session.get_vault_session_path(), base_name)))
        self.assertTrue(set(local_files) == vault_files,
                        msg="Files missing from vault:\n" + str(set(local_files) - vault_files) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files - set(local_files)))
        return (local_dir, local_files)

    def ichksum_with_multiple_bad_replicas(self):
        filename = 'checksum_test.txt'
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput -K -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("irepl -R " + self.anotherresc + " " + filename)
        with open(os.path.join(self.anothervault, "home", self.admin.username, self.admin._session_id, filename), "w") as f:
            f.write("SHAS FOR THE SHA256 GOD MD5 FOR THE MD5 THRONE")
        with open(os.path.join(self.testvault, "home", self.admin.username, self.admin._session_id, filename), "w") as f:
            f.write("SHAS FOR THE SHA256 GOD MD5 FOR THE MD5 THRONE")
        out, _, _ = self.admin.run_icommand("ichksum -aK " + filename)
        search_string = 'hierarchy [' + self.testresc + ']'
        self.assertTrue(search_string in out, 'String missing from ichksum -aK output:\n\t' + search_string)
        search_string = 'hierarchy [' + self.anotherresc + ']'
        self.assertTrue(search_string in out, 'String missing from ichksum -aK output:\n\t' + search_string)

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
        self.admin.assert_icommand(['iput', '-r', local_dir], "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        ils_out, _, _ = self.admin.run_icommand(['ils', base_name])
        rods_files = [f for f in lib.get_object_names_from_entries(ils_out)]

        self.admin.assert_icommand(['ichmod','-r','null',self.admin.username,base_name])
        self.admin.assert_icommand(['ichmod','-M','read',self.admin.username,base_name+'/'+rods_files[-1]])
        self.admin.assert_icommand(['iget','-r',base_name,self.admin.local_session_dir],'STDERR_SINGLELINE','CAT_NO_ACCESS_PERMISSION')

        assert os.path.isfile(os.path.join(self.admin.local_session_dir,base_name,rods_files[-1]))

        self.admin.assert_icommand(['ichmod','-M','-r','own',self.admin.username,base_name])

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
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # force remove collection
        self.user0.assert_icommand("irm -rf {coll_name}".format(**locals()), "EMPTY")
        self.user0.assert_icommand("ils {coll_name}".format(**locals()), 'STDERR_SINGLELINE', "does not exist")

        # make sure no files are left in the vault
        user_vault_dir = os.path.join(self.user0.get_vault_session_path(), coll_name)
        out, _ = lib.execute_command('find {user_vault_dir} -type f'.format(**locals()))
        self.assertEqual(out, '')

    def test_iput_r_with_kw(self):
        # test settings
        depth = 50
        files_per_level = 5
        file_size = 5

        # make local nested dirs
        coll_name = "test_iput_r_with_kw"
        local_dir = os.path.join(self.testing_tmp_dir, coll_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        try:
            # load server_config.json to inject new settings
            server_config_filename = paths.server_config_path()
            with open(server_config_filename) as f:
                svr_cfg = json.load(f)
            svr_cfg['log_level']['resource'] = 'debug'

            # dump to a string to repave the existing server_config.json
            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))
            with lib.file_backed_up(server_config_filename):
                # repave the existing server_config.json
                with open(server_config_filename, 'w') as f:
                    f.write(new_server_config)

                IrodsController().reload_configuration()

                # get log offset
                initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_log_path)

                # iput dir
                self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
                self.user0.assert_icommand('iquest "SELECT COUNT(DATA_ID) WHERE COLL_NAME LIKE \'%/{coll_name}%\'"'.format(**locals()), 'STDOUT', str(files_per_level * depth))

                # look for occurrences of debug sequences in the log
                rec_op_kw_string = 'recursiveOpr found in cond_input for file_obj'
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg=rec_op_kw_string,
                        count=files_per_level * depth,
                        server_log_path=IrodsConfig().server_log_path,
                        start_index=initial_size_of_server_log))

        finally:
            IrodsController().restart()

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

    def test_icp_collection_into_itself_3962(self):
        base_name_source = "test_icp_r_3962"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=10, file_size=100)[1])

        base_name_target = base_name_source + "_1"
        self.user0.assert_icommand("icp -r " + base_name_source + " " + base_name_target, "EMPTY")

        # The above was the test case that failed before the bug fix.
        # There is no need to test the ordinary icp -r onto itself, because
        # of the previous test - test_icp_collection_into_itself.

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

            # compare local files with files in vault
            files_in_vault = set(lib.files_in_dir(os.path.join(self.user0.get_vault_session_path(),
                                                              partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_cancel_large_iput_for_create(self):
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

    def test_cancel_large_iput_for_overwrite__issue_3091(self):
        filename = 'test_cancel_large_iput_for_overwrite__issue_3091'
        small_local_file = os.path.join(self.user0.local_session_dir, 'smol')
        large_local_file = os.path.join(self.user0.local_session_dir, filename)
        logical_path = os.path.join(self.user0.session_collection, filename)
        file_size = pow(2, 30)

        try:
            if not os.path.exists(small_local_file):
                lib.make_file(small_local_file, 1024)

            self.user0.assert_icommand(['iput', '-K', small_local_file, logical_path])
            out, err, ec = self.user0.run_icommand(
                ['iquest', '%s %s',
                 "select DATA_CHECKSUM, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"
                 .format(os.path.basename(logical_path), os.path.dirname(logical_path))])

            self.assertEqual(0, ec)
            self.assertEqual(0, len(err))
            self.assertEqual(1, len(out.splitlines()))

            result = out.split()
            self.assertEqual(1, int(result[1]))
            old_checksum = result[0]

            if not os.path.exists(large_local_file):
                lib.make_file(large_local_file, file_size)

            iput_cmd = 'iput -f -K {0} {1}'.format(large_local_file, logical_path)
            file_vault_full_path = os.path.join(self.user0.get_vault_session_path(), filename)
            self.user0.interrupt_icommand(iput_cmd, file_vault_full_path, 2048)
            self.user0.assert_icommand('ils -L', 'STDOUT_SINGLELINE', filename)

            # multiple threads could still be writing on the server side, so we need to wait for
            # the size in the vault to converge - then we're done.
            old_size = None
            new_size = os.path.getsize(file_vault_full_path)
            while old_size != new_size:
                time.sleep(1)
                old_size = new_size
                new_size = os.path.getsize(file_vault_full_path)

            # wait for select() call to timeout, set to slightly above "SELECT_TIMEOUT_FOR_CONN", which is 60 seconds
            time.sleep(63)
            self.user0.assert_icommand('ils -L', 'STDOUT_SINGLELINE', [filename, str(new_size)])

            out, err, ec = self.user0.run_icommand(
                ['iquest', '%s...%s...%s',
                 "select DATA_SIZE, DATA_CHECKSUM, DATA_REPL_STATUS where DATA_NAME = '{0}' and COLL_NAME = '{1}'"
                 .format(os.path.basename(logical_path), os.path.dirname(logical_path))])

            self.assertEqual(0, ec)
            self.assertEqual(0, len(err))

            self.assertEqual(1, len(out.splitlines()))
            result = out.split('...')

            self.assertEqual(new_size, int(result[0]))
            self.assertNotEqual(old_checksum, result[1])
            self.assertEqual(0, int(result[2]))

        finally:
            self.user0.assert_icommand(['irm', '-f', logical_path])

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
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg='writeLine: inString = dynamic pep in delay',
                    start_index=initial_size_of_server_log))

    def test_iput_bulk_check_acpostprocforput__2841(self):
        # prepare test directory
        number_of_files = 5
        dirname = self.admin.local_session_dir + '/files'
        # files less than 4200000 were failing to trigger the writeLine
        for filesize in range(5000, 6000000, 500000):
            lib.make_large_local_tmp_dir(dirname, number_of_files, filesize)
            # manipulate core.re and check the server log
            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
                time.sleep(1)  # remove once file hash fix is committed #2279

                initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
                self.admin.assert_icommand(['iput', '-frb', dirname], "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='writeLine: inString = acPostProcForPut called for',
                        count=number_of_files,
                        start_index=initial_size_of_server_log))
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

            # Crank up debugging level for this bit to verify failure behavior
            env_backup = copy.deepcopy(self.admin.environment_file_contents)
            self.admin.environment_file_contents.update({ 'irods_log_level' : 7 })

            _, out, err = self.admin.assert_icommand(['ils', '-V'], 'STDERR_SINGLELINE', 'Connection refused')
            self.assertTrue('errno = {0}'.format(errno.ECONNREFUSED) in out, 'missing ECONNREFUSED errno in\n' + out)
            self.assertTrue('errno = {0}'.format(errno.ECONNABORTED) not in out, 'found ECONNABORTED errno in\n' + out)
            self.assertTrue('errno = {0}'.format(errno.EINVAL) not in out, 'found EINVAL errno in\n' + out)

            self.admin.environment_file_contents = env_backup

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

            self.user0.assert_icommand(['iput', '-fR', self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR', self.testresc, filepath])
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

            self.user0.assert_icommand(['iput', '-fR', self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR', self.testresc, filepath])
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

            self.user0.assert_icommand(['iput', '-fR', self.testresc, filepath])
            self.user0.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', self.testresc)
            self.user0.assert_icommand(['irm', '-f', filename])

            # test as rodsadmin
            self.admin.assert_icommand(['ils', '-l', filename], 'STDERR_SINGLELINE', 'does not exist')

            self.admin.assert_icommand(['iput', '-f', filepath])
            self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', 'demoResc')
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand(['iput', '-fR', self.testresc, filepath])
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

    def test_irm_colloprstat__3572(self):
        collection_to_delete = 'collection_to_delete'
        self.admin.assert_icommand(['imkdir', collection_to_delete])
        filename = 'test_irm_colloprstat__3572'
        lib.make_file(filename, 50)
        for i in range(10):
            self.admin.assert_icommand(['iput', filename, '{0}/file_{1}'.format(collection_to_delete, str(i))])

        initial_size_of_server_log = lib.get_file_size_by_path(paths.server_log_path())
        self.admin.assert_icommand(['irm', '-rf', collection_to_delete])
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg='ERROR',
                count=0,
                start_index=initial_size_of_server_log))
        os.unlink(filename)

    def test_ichksum_admin_flag__3265(self):
        # Get files set up
        file_size = 50
        filename1 = 'test_ichksum_admin_flag__3265_1'
        filename2 = 'test_ichksum_admin_flag__3265_2'
        lib.make_file(filename1, file_size)
        lib.make_file(filename2, file_size)
        self.user0.assert_icommand(['iput', filename1])
        self.user0.assert_icommand(['iput', filename2])

        # Get paths to data objects
        data_path,_,_ = self.user0.run_icommand("ipwd")
        data_path1 = data_path.rstrip() + '/' + filename1
        data_path2 = data_path.rstrip() + '/' + filename2

        # Generate checksum as admin (no permissions; fail)
        self.admin.assert_icommand(['ichksum', '-K', data_path1], 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')
        # Generate checksum as rodsuser with admin flag (fail)
        self.user0.assert_icommand(['ichksum', '-K', '-M', data_path1], 'STDERR_SINGLELINE', 'CAT_INSUFFICIENT_PRIVILEGE_LEVEL')
        # Generate checksum as rodsuser (owner; pass)
        self.user0.assert_icommand(['ichksum', data_path1], 'STDOUT_SINGLELINE', '    sha2:')
        # Verify checksum as admin (no permissions; fail)
        self.admin.assert_icommand(['ichksum', '-K', data_path1], 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')
        # Verify checksum as admin with admin flag (pass)
        self.admin.assert_icommand(['ichksum', '-K', '-M', data_path1])

        # Generate checksum as admin with admin flag (pass)
        self.admin.assert_icommand(['ichksum', '-M', data_path2], 'STDOUT_SINGLELINE', '    sha2:')
        # Verify checksum as rodsuser (pass)
        self.user0.assert_icommand(['ichksum', '-K', data_path2])

        os.unlink(filename1)
        os.unlink(filename2)

    ##################################
    # Issue - 3997:
    # This tests the functionality of irsync and iput, with a single source directory
    # and a target collection which does not pre-exist.
    ########
    def test_target_not_exist_singlesource_3997(self):

        ##################################
        # All cases listed below create identical results: target 'dir1' is created.
        ########
        test_cases = [
                        'iput -r {dir1path}',
                        'iput -r {dir1path} {target1}',
                        'irsync -r {dir1path} i:{target1}'
        ]

        base_name = 'target_not_exist_singlesource_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = dir1
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:
                cmd = cmdstring.format(**locals())

                ##################################
                # Target collection exists or not based on runimkdir
                # Single source directory command
                # This means that the contents of dir1 will be
                # placed directly under target_collection (recursively).
                ########

                self.user0.assert_icommand(cmd, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

                self.user0.assert_icommand( 'ils {target1}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '/{target1}/subdir1'.format(**locals()) ])

                self.user0.assert_icommand( 'ils {target1}/subdir1'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                        [ '  0', '  1' ])

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                # Just to be paranoid, make sure it's really gone
                self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)


    ##################################
    # Issue - 3997:
    # This tests the functionality of iput, with a single source directory
    # and a target collection which does pre-exist.
    ########
    def test_iput_target_does_exist_singlesource_3997(self):

        ##################################
        # All cases listed below create identical results
        # (leaving this in list form, in case additional cases show up).
        ########
        test_cases = [
                        'iput -r {dir1path} {target1}',
        ]

        base_name = 'iput_target_does_exist_singlesource_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:

                # Create the pre-existing collection
                self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                cmd = cmdstring.format(**locals())

                ##################################
                # Target collection exists
                # Single source directory command
                # This means that the contents of dir1 will be
                # placed directly under target_collection (recursively).
                ########

                self.user0.assert_icommand(cmd, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

                # Command creates source dir under existing collection
                self.user0.assert_icommand( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', dir1 )

                self.user0.assert_icommand( 'ils {target1}/{dir1}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '/{target1}/{dir1}/subdir1'.format(**locals()) ])

                self.user0.assert_icommand( 'ils {target1}/{dir1}/subdir1'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                        [ '  0', '  1' ])

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                # Just to be paranoid, make sure it's really gone
                self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)


    ##################################
    # Issue - 3997:
    # This tests the functionality of irsync with a single source directory
    # and a target collection which does pre-exist.
    ########
    def test_irsync_target_does_exist_singlesource_3997(self):

        ##################################
        # All cases listed below create identical results
        # (leaving this in list form, in case additional cases show up).
        ########
        test_cases = [
                        'irsync -r {dir1path} i:{target1}'
        ]

        base_name = 'irsync_target_does_exist_singlesource_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:

                # Create the pre-existing collection
                self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                cmd = cmdstring.format(**locals())

                ##################################
                # Target collection exists
                # Single source directory command
                # This means that the contents of dir1 will be
                # placed directly under target_collection (recursively).
                ########

                self.user0.assert_icommand(cmd, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

                self.user0.assert_icommand( 'ils {target1}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '/{target1}/subdir1'.format(**locals()) ])

                self.user0.assert_icommand( 'ils {target1}/subdir1'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                        [ '  0', '  1' ])

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                # Just to be paranoid, make sure it's really gone
                self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)

    ##################################
    # Issue - 3997:
    # This tests the functionality of irsync and iput with multiple source directories
    # and a target collection which first does not exist, but then does (the testcases are run twice).
    ########
    def test_multiple_source_3997(self):

        ##################################
        # All cases listed below create identical results.
        # The test cases are run twice each - with and without an imkdir first.
        ########
        test_cases = [
                        'iput -r {dir1path} {dir2path} {target1}',
                        'irsync -r {dir1path} {dir2path} i:{target1}',
        ]

        base_name = 'multiple_source_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir2 = 'dir2'
            dir1path = os.path.join(local_dir, dir1)
            dir2path = os.path.join(local_dir, dir2)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = dir1
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(dir2path,4)     # Four files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:
                cmd = cmdstring.format(**locals())

                for runimkdir in [ 'no', 'yes' ]:

                    if runimkdir == 'yes':
                        self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                    self.user0.assert_icommand(cmd, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

                    # Command creates source dir under existing collection
                    self.user0.assert_icommand( 'ils {target1}'.format(**locals()), 'STDOUT_MULTILINE', [ dir1, dir2 ] )

                    self.user0.assert_icommand( 'ils {target1}/{dir1}'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                                [ '  0', '  1', '/{target1}/{dir1}/subdir1'.format(**locals()) ])

                    self.user0.assert_icommand( 'ils {target1}/{dir1}/subdir1'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                                [ '  0', '  1' ])

                    self.user0.assert_icommand( 'ils {target1}/{dir2}'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                                [ '  0', '  1', ' 2', ' 3' ] )

                    self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                    # Just to be paranoid, make sure it's really gone
                    self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dir2path), ignore_errors=True)


    ##################################
    # Issue - 3997:
    # This tests the functionality of icp, with a single collection
    # and a target collection which does not pre-exist.
    ########
    def test_icp_target_not_exist_singlesource_3997(self):

        ##################################
        # All cases listed below create identical results
        ########
        test_cases = [
                        'icp -r {dir1} {target1}'
        ]

        base_name = 'icp_target_not_exist_singlesource_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:

                # This will create everything under collection dir1
                self.user0.run_icommand('iput -r {dir1path}'.format(**locals()))

                cmd = cmdstring.format(**locals())

                ##################################
                # Target collection exists or not based on runimkdir
                # Single source directory command
                # This means that the contents of dir1 will be
                # placed directly under target_collection (recursively).
                ########

                # Successful icp is silent
                self.user0.assert_icommand(cmd, "EMPTY")

                self.user0.assert_icommand( 'ils {target1}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '/{target1}/subdir1'.format(**locals()) ])

                self.user0.assert_icommand( 'ils {target1}/subdir1'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                        [ '  0', '  1' ])

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                # Just to be paranoid, make sure it's really gone
                self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)

    ##################################
    # Issue - 3997:
    # This tests the functionality of icp, with a single collection
    # and a target collection which does not pre-exist.
    ########
    def test_icp_target_does_exist_singlesource_3997(self):

        ##################################
        # All cases listed below create identical results
        ########
        test_cases = [
                        'icp -r {dir1} {target1}'
        ]

        base_name = 'icp_target_does_exist_singlesource_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:

                # This will create the target collection 'target1'
                self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                # This will create everything under collection dir1
                self.user0.run_icommand('iput -r {dir1path}'.format(**locals()))

                cmd = cmdstring.format(**locals())

                ##################################
                # Target collection exists or not based on runimkdir
                # Single source directory command
                # This means that the contents of dir1 will be
                # placed directly under target_collection (recursively).
                ########

                # Successful icp is silent
                self.user0.assert_icommand(cmd, "EMPTY")

                # Command creates source dir under existing collection
                self.user0.assert_icommand( 'ils {target1}'.format(**locals()), 'STDOUT_MULTILINE', dir1 )

                self.user0.assert_icommand( 'ils {target1}/{dir1}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '/{target1}/{dir1}/subdir1'.format(**locals()) ])

                self.user0.assert_icommand( 'ils {target1}/{dir1}/subdir1'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                        [ '  0', '  1' ])

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                # Just to be paranoid, make sure it's really gone
                self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)


    ##################################
    # Issue - 3997:
    # This tests the functionality of icp, with a single collection
    # and a target collection. Each case is run twice - once without preexisting
    # target collection, and once after using imkdir to create the collection.
    ########
    def test_icp_multiple_src_3997(self):

        ##################################
        # All cases listed below create identical results
        ########
        test_cases = [
                        'icp -r {dir1} {dir2} {target1}'
        ]

        base_name = 'icp_multiple_src_3997'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            dir1 = 'dir1'
            dir2 = 'dir2'
            dir1path = os.path.join(local_dir, dir1)
            dir2path = os.path.join(local_dir, dir2)
            subdir1 = os.path.join(dir1path, 'subdir1')
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(dir2path,4)     # Four files in this one
            lib.create_directory_of_small_files(subdir1path,2)      # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            for cmdstring in test_cases:
                for runimkdir in [ 'no', 'yes' ]:

                    if runimkdir == 'yes':
                        self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                    # This will create all data objects under collections dir1 and dir2
                    self.user0.run_icommand('iput -r {dir1path}'.format(**locals()))
                    self.user0.run_icommand('iput -r {dir2path}'.format(**locals()))

                    cmd = cmdstring.format(**locals())

                    ##################################
                    # Target collection exists or not based on runimkdir
                    # Single source directory command
                    # This means that the contents of dir1 will be
                    # placed directly under target_collection (recursively).
                    ########

                    # Successful icp is silent
                    self.user0.assert_icommand(cmd, "EMPTY")

                    # Command creates source dir under existing collection
                    self.user0.assert_icommand( 'ils {target1}'.format(**locals()), 'STDOUT_MULTILINE', [ dir1, dir2 ] )

                    self.user0.assert_icommand( 'ils {target1}/{dir1}'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                                [ '  0', '  1', '/{target1}/{dir1}/subdir1'.format(**locals()) ])

                    self.user0.assert_icommand( 'ils {target1}/{dir2}'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                                [ '  0', '  1', '  2', '  3' ] )

                    self.user0.assert_icommand( 'ils {target1}/{dir1}/subdir1'.format(**locals()),
                                                'STDOUT_MULTILINE',
                                            [ '  0', '  1' ])

                    self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

                    # Just to be paranoid, make sure it's really gone
                    self.user0.assert_icommand_fail( 'ils {target1}'.format(**locals()), 'STDOUT_SINGLELINE', target1 )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)


    #################################################################
    # Issue 4006 - can no longer have regular files on the command line
    # when the -r flag is specified
    #############################
    def test_irsync_iput_file_dir_mix_with_recursive_4006(self):

        base_name = 'irsync_iput_file_dir_mix_with_recursive_4006'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            ##################################
            # Setup
            ########
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = 'subdir1'
            subdir1path = os.path.join(dir1path, subdir1)
            dir2 = 'dir2'
            dir2path = os.path.join(local_dir, dir2)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,4)  # Four files in this one
            lib.create_directory_of_small_files(dir2path,2)     # Two files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            ##################################
            # Grouped tests (should) produce the same behavior and results:
            ########
            test_cases = [
                            'iput -r {dir1path}/0 {subdir1path} {target1}',
                            'irsync -r {dir1path}/0 {subdir1path} i:{target1}',
                            'iput -r {dir2path} {dir1path}/0 {subdir1path} {target1}',
                            'irsync -r {dir2path} {dir1path}/0 {subdir1path} i:{target1}',
                         ]

            ##################################
            # Mix directories and files should fail with nothing created in irods
            ########
            for cmdstring in test_cases:

                cmd = cmdstring.format(**locals())
                self.user0.assert_icommand(cmd, "STDERR_SINGLELINE", 'ERROR: disallow_file_dir_mix_on_command_line: Cannot include regular file')
                self.user0.assert_icommand_fail( 'ils {target1path}'.format(**locals()), 'STDOUT_SINGLELINE', '{target1}'.format(**locals()) )

                # Create the pre-existing collection
                # self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

            ##################################
            # Grouped tests (should) produce the same behavior and results:
            ########
            test_cases = [
                            'iput {dir1path}/0 {dir1path}/1 {target1}',
                            'irsync {dir1path}/0 {dir1path}/1 i:{target1}',
                         ]

            ##################################
            # Transfer of multiple regular files to a target requires pre-existing target
            ########
            for cmdstring in test_cases:

                cmd = cmdstring.format(**locals())
                _,stderr,_ = self.user0.run_icommand(cmd)

                estr = 'ERROR: resolveRodsTarget: target {target1path} does not exist status = -310000 USER_FILE_DOES_NOT_EXIST'.format(**locals())
                self.assertIn(estr, stderr, '{cmd}: Expected stderr: "...{estr}...", got: "{stderr}"'.format(**locals()))

                self.user0.assert_icommand_fail( 'ils {target1path}'.format(**locals()), 'STDOUT_SINGLELINE', '{target1}'.format(**locals()) )

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)
            shutil.rmtree(os.path.abspath(dir2path), ignore_errors=True)

    #################################################################
    # Issue 4048 - irsync was not transferring regular files when specified
    # on the command line. When the regular files were inside directories, it
    # was ok.  We are checking both irsync and iput for consistency.
    #############################
    def test_irsync_iput_regular_files_only_4048(self):

        base_name = 'irsync_iput_regular_files_only_4048'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            ##################################
            # Setup
            ########
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = 'subdir1'
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = 'target1'
            target1path='{self.user0.session_collection}/{target1}'.format(**locals())

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,4)  # Four files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            ##################################
            # Grouped tests (should) produce the same behavior and results:
            ########
            test_cases = [
                            'irsync {dir1path}/0 {dir1path}/1 {subdir1path}/2 i:{target1}',
                            'iput {dir1path}/0 {dir1path}/1 {subdir1path}/2 {target1}',
                         ]

            ##################################
            # Transfer of multiple regular files to a target requires pre-existing target
            # This first try should fail for all test cases
            ########
            for cmdstring in test_cases:

                cmd = cmdstring.format(**locals())
                _,stderr,_ = self.user0.run_icommand(cmd)

                estr = 'ERROR: resolveRodsTarget: target {target1path} does not exist status = -310000 USER_FILE_DOES_NOT_EXIST'.format(**locals())
                self.assertIn(estr, stderr, '{cmd}: Expected stderr: "...{estr}...", got: "{stderr}"'.format(**locals()))

                self.user0.assert_icommand_fail( 'ils {target1path}'.format(**locals()), 'STDOUT_SINGLELINE', '{target1}'.format(**locals()) )

            ##################################
            # Transfer of multiple regular files to a target requires pre-existing target
            # This time we'll create the target collection first
            ########
            for cmdstring in test_cases:

                self.user0.run_icommand('imkdir -p {target1path}'.format(**locals()))

                cmd = cmdstring.format(**locals())
                self.user0.assert_icommand(cmd, "EMPTY")

                self.user0.assert_icommand( 'ils {target1path}'.format(**locals()),
                                            'STDOUT_MULTILINE',
                                            [ '  0', '  1', '  2' ] )

                self.user0.run_icommand('irm -rf {target1path}'.format(**locals()))

        finally:
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)



    #################################################################
    # Issue 4030 - failure to write to collection path "/" was providing
    # insufficient detail in the error message to the user.
    #############################
    def test_writing_collection_under_slash_4030(self):

        base_name = 'writing_collection_under_slash_4030'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)

        try:
            ##################################
            # Setup
            ########
            dir1 = 'dir1'
            dir1path = os.path.join(local_dir, dir1)
            subdir1 = 'subdir1'
            subdir1path = os.path.join(dir1path, subdir1)

            target1 = '/'

            lib.make_dir_p(local_dir)
            lib.create_directory_of_small_files(dir1path,2)     # Two files in this one
            lib.create_directory_of_small_files(subdir1path,4)  # Four files in this one

            self.user0.run_icommand('icd {self.user0.session_collection}'.format(**locals()))

            # We put a collection into irods so that we can run icp.
            self.user0.assert_icommand('iput -r {dir1path}'.format(**locals()), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

            ##################################
            # Grouped tests (should) produce the same behavior and results:
            ########
            test_cases = [
                            'iput -r {dir1path} {target1}',
                            'irsync -r {dir1path} i:{target1}',
                            'icp -r {dir1} {target1}'
                         ]

            for cmdstring in test_cases:

                cmd = cmdstring.format(**locals())
                stdout,_,_ = self.user0.run_icommand(cmd)

                estr = 'SYS_INVALID_INPUT_PARAM]  errno [] -- message [a valid zone name does not appear at the root of the object path'
                self.assertIn(estr, stdout, '{cmd}: Expected stdout: "...{estr}...", got: "{stdout}"'.format(**locals()))

        finally:
            self.user0.run_icommand('irm -rf {dir1}'.format(**locals()))
            shutil.rmtree(os.path.abspath(dir1path), ignore_errors=True)

    # These tests create a resource with a vault for which iRODS has no write permission and tries to put a file there
    def test_iput_small_file_to_resource_with_restricted_vault_permission(self):
        self.iput_to_resource_with_restricted_vault_permission_test(1)

    def test_iput_large_file_to_resource_with_restricted_vault_permission(self):
        self.iput_to_resource_with_restricted_vault_permission_test(40000001)

    def iput_to_resource_with_restricted_vault_permission_test(self, size):
        resc_name = 'cantwritetovaultresc'
        vault_path = os.path.join('/', 'var')
        self.admin.assert_icommand(['iadmin', 'mkresc', resc_name, 'unixfilesystem', lib.get_hostname() + ':' + vault_path], 'STDOUT_SINGLELINE', resc_name)
        file_name = 'test_iput_to_resource_with_restricted_vault_permission'
        file_path = os.path.join(self.testing_tmp_dir, file_name)
        lib.make_file(file_path, size)
        logical_path = os.path.join(self.admin.session_collection, file_name) # another user's home collection
        try:
            self.admin.assert_icommand(['iput', '-R', resc_name, file_path], 'STDERR', 'UNIX_FILE_MKDIR_ERR')
            self.admin.assert_icommand(['ils', '-l', file_name], 'STDERR', 'does not exist')
            session_vault_path = self.admin.get_vault_session_path()
            self.assertFalse(os.path.exists(os.path.join(session_vault_path, file_name)))
        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            os.unlink(file_path)
            self.admin.assert_icommand(['iadmin', 'rmresc', resc_name])

    # These tests attempt to put a file to a logical path to which the authenticated user has no access permission
    def test_iput_small_file_to_restricted_logical_path(self):
        self.iput_to_restricted_logical_path_test(1)

    def test_iput_large_file_to_restricted_logical_path(self):
        self.iput_to_restricted_logical_path_test(40000001)

    def iput_to_restricted_logical_path_test(self, size):
        file_name = 'iput_to_restricted_logical_path_test'
        file_path = os.path.join(self.testing_tmp_dir, file_name)
        lib.make_file(file_path, size)
        logical_path = os.path.join(self.user1.session_collection, file_name) # another user's home collection
        try:
            # attempt to put file where there is no permission
            self.user0.assert_icommand(['iput', file_path, logical_path], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDERR', 'does not exist')
            session_vault_path = self.user1.get_vault_session_path()
            self.assertFalse(os.path.exists(os.path.join(session_vault_path, file_name)))

            # attempt an overwrite
            self.user1.assert_icommand(['iput', file_path, logical_path])
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', file_name)
            self.user0.assert_icommand(['iput', file_path, logical_path], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', file_name)
            session_vault_path = self.user1.get_vault_session_path()
            self.assertTrue(os.path.exists(os.path.join(session_vault_path, file_name)))

            # attempt a forced overwrite
            self.user0.assert_icommand(['iput', '-f', file_path, logical_path], 'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT', file_name)
            session_vault_path = self.user1.get_vault_session_path()
            self.assertTrue(os.path.exists(os.path.join(session_vault_path, file_name)))
        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            os.unlink(file_path)

