import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import time
import shutil

import configuration
import lib
import resource_suite


@unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
class Test_ICommands_File_Operations(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_ICommands_File_Operations, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-icommands-recursive'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_ICommands_File_Operations, self).tearDown()

    def iput_r_large_collection(self, session, base_name, file_count, file_size):
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = lib.make_large_local_tmp_dir(local_dir, file_count, file_size)
        session.assert_icommand(['iput', '-r', local_dir])
        rods_files = set(lib.ils_output_to_entries(session.run_icommand(['ils', base_name])[1]))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing:\n" + str(set(local_files) - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - set(local_files)))
        vault_files = set(os.listdir(os.path.join(lib.get_vault_session_path(session), base_name)))
        self.assertTrue(set(local_files) == vault_files,
                        msg="Files missing from vault:\n" + str(set(local_files) - vault_files) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files - set(local_files)))
        return (local_dir, local_files)

    def test_iput_r(self):
        self.iput_r_large_collection(self.user0, "test_iput_r_dir", file_count=1000, file_size=100)

    def test_irm_r(self):
        base_name = "test_irm_r_dir"
        self.iput_r_large_collection(self.user0, base_name, file_count=1000, file_size=100)

        self.user0.assert_icommand("irm -r " + base_name, "EMPTY")
        self.user0.assert_icommand("ils " + base_name, 'STDERR', "does not exist")

        vault_files_post_irm = os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                       base_name))
        self.assertTrue(len(vault_files_post_irm) == 0,
                        msg="Files not removed from vault:\n" + str(vault_files_post_irm))

    def test_imv_r(self):
        base_name_source = "test_imv_r_dir_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_imv_r_dir_target"
        self.user0.assert_icommand("imv " + base_name_source + " " + base_name_target, "EMPTY")
        self.user0.assert_icommand("ils " + base_name_source, 'STDERR', "does not exist")
        self.user0.assert_icommand("ils", 'STDOUT', base_name_target)
        rods_files_post_imv = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_post_imv,
                        msg="Files missing:\n" + str(file_names - rods_files_post_imv) + "\n\n" +
                            "Extra files:\n" + str(rods_files_post_imv - file_names))

        vault_files_post_irm_source = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                                  base_name_source)))
        self.assertTrue(len(vault_files_post_irm_source) == 0)

        vault_files_post_irm_target = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
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
        self.user0.assert_icommand("ils", 'STDOUT', base_name_target)
        rods_files_source = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files:\n" + str(rods_files_target - file_names))

        vault_files_post_icp_source = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                                  base_name_source)))

        self.assertTrue(file_names == vault_files_post_icp_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_source - file_names))

        vault_files_post_icp_target = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                                  base_name_target)))
        self.assertTrue(file_names == vault_files_post_icp_target,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_target) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_target - file_names))

    def test_irsync_r_dir_to_coll(self):
        base_name = "test_irsync_r_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_names = set(lib.make_large_local_tmp_dir(local_dir, file_count=1000, file_size=100))

        self.user0.assert_icommand("irsync -r " + local_dir + " i:" + base_name, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT', base_name)
        rods_files = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name])[1]))
        self.assertTrue(file_names == rods_files,
                        msg="Files missing:\n" + str(file_names - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - file_names))

        vault_files_post_irsync = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
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
        self.user0.assert_icommand("ils", 'STDOUT', base_name_source)
        self.user0.assert_icommand("ils", 'STDOUT', base_name_target)
        rods_files_source = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files :\n" + str(rods_files_target - file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

        vault_files_post_irsync_target = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
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
        self.user0.assert_icommand("ils", 'STDOUT', base_name_source)
        rods_files_source = set(lib.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        local_files = set(os.listdir(local_dir))
        self.assertTrue(file_names == local_files,
                        msg="Files missing from local dir:\n" + str(file_names - local_files) + "\n\n" +
                            "Extra files in local dir:\n" + str(local_files - file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(lib.get_vault_session_path(self.user0),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

    def test_cancel_large_iput(self):
        base_name = 'test_cancel_large_put'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_size = pow(2, 30)
        file_name = lib.make_large_local_tmp_dir(local_dir, file_count=1, file_size=file_size)[0]
        file_local_full_path = os.path.join(local_dir, file_name)
        iput_cmd = "iput '" + file_local_full_path + "'"
        file_vault_full_path = os.path.join(lib.get_vault_session_path(self.user0), file_name)
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
        self.user0.assert_icommand('ils -l', 'STDOUT', [file_name, str(new_size)])
