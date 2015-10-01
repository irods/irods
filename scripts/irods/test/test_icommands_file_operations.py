import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import contextlib
import errno
import os
import tempfile
import time
import shutil

import configuration
import session
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
        local_files = session.make_large_local_tmp_dir(local_dir, file_count, file_size)
        session.assert_icommand(['iput', '-r', local_dir])
        rods_files = set(session.ils_output_to_entries(session.run_icommand(['ils', base_name])[1]))
        self.assertTrue(set(local_files) == rods_files,
                        msg="Files missing:\n" + str(set(local_files) - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - set(local_files)))
        vault_files = set(os.listdir(os.path.join(session.get_vault_session_path(session), base_name)))
        self.assertTrue(set(local_files) == vault_files,
                        msg="Files missing from vault:\n" + str(set(local_files) - vault_files) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files - set(local_files)))
        return (local_dir, local_files)

    def iput_to_root_collection(self):
        filename = 'original.txt'
        filepath = session.create_local_testfile(filename)
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
        filepath = session.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iput -f " + filename)  # put file
        self.admin.assert_icommand("iput -fR " + self.testresc + " " + filename, 'STDERR_SINGLELINE', 'HIERARCHY_ERROR')  # fail

    def test_get_null_perms__2833(self):
        base_name = 'test_dir_for_perms'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        local_files = session.make_large_local_tmp_dir(local_dir, 30, 10)
        self.admin.assert_icommand(['iput', '-r', local_dir])
        rods_files = session.ils_output_to_entries(self.admin.run_icommand(['ils', base_name])[1])

        test_dir = 'test_get_null_perms__2833_dir'
        self.admin.assert_icommand(['ichmod','-r','null',self.admin.username,base_name])
        self.admin.assert_icommand(['ichmod','read',self.admin.username,base_name+'/'+rods_files[-1]])
        self.admin.assert_icommand(['iget','-r',base_name,test_dir],'STDERR_SINGLELINE','CAT_NO_ACCESS_PERMISSION')

        assert os.path.isfile(os.path.join(test_dir,'junk0029'))

        self.admin.assert_icommand(['ichmod','-r','own',self.admin.username,base_name])
        session.run_command(['rm','-rf',test_dir])



    def test_iput_r(self):
        self.iput_r_large_collection(self.user0, "test_iput_r_dir", file_count=1000, file_size=100)

    def test_irm_r(self):
        base_name = "test_irm_r_dir"
        self.iput_r_large_collection(self.user0, base_name, file_count=1000, file_size=100)

        self.user0.assert_icommand("irm -r " + base_name, "EMPTY")
        self.user0.assert_icommand("ils " + base_name, 'STDERR_SINGLELINE', "does not exist")

        vault_files_post_irm = os.listdir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

        # force remove collection
        self.user0.assert_icommand("irm -rf {coll_name}".format(**locals()), "EMPTY")
        self.user0.assert_icommand("ils {coll_name}".format(**locals()), 'STDERR_SINGLELINE', "does not exist")

        # make sure no files are left in the vault
        user_vault_dir = os.path.join(session.get_vault_session_path(self.user0), coll_name)
        cmd_out = session.run_command('find {user_vault_dir} -type f'.format(**locals()))
        self.assertEqual(cmd_out[1], '')

    def test_imv_r(self):
        base_name_source = "test_imv_r_dir_source"
        file_names = set(self.iput_r_large_collection(
            self.user0, base_name_source, file_count=1000, file_size=100)[1])

        base_name_target = "test_imv_r_dir_target"
        self.user0.assert_icommand("imv " + base_name_source + " " + base_name_target, "EMPTY")
        self.user0.assert_icommand("ils " + base_name_source, 'STDERR_SINGLELINE', "does not exist")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name_target)
        rods_files_post_imv = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_post_imv,
                        msg="Files missing:\n" + str(file_names - rods_files_post_imv) + "\n\n" +
                            "Extra files:\n" + str(rods_files_post_imv - file_names))

        vault_files_post_irm_source = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
                                                                  base_name_source)))
        self.assertTrue(len(vault_files_post_irm_source) == 0)

        vault_files_post_irm_target = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
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
        rods_files_source = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files:\n" + str(rods_files_target - file_names))

        vault_files_post_icp_source = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
                                                                  base_name_source)))

        self.assertTrue(file_names == vault_files_post_icp_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_source - file_names))

        vault_files_post_icp_target = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
                                                                  base_name_target)))
        self.assertTrue(file_names == vault_files_post_icp_target,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_icp_target) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_icp_target - file_names))

    def test_irsync_r_dir_to_coll(self):
        base_name = "test_irsync_r_dir_to_coll"
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_names = set(session.make_large_local_tmp_dir(local_dir, file_count=1000, file_size=100))

        self.user0.assert_icommand("irsync -r " + local_dir + " i:" + base_name, "EMPTY")
        self.user0.assert_icommand("ils", 'STDOUT_SINGLELINE', base_name)
        rods_files = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name])[1]))
        self.assertTrue(file_names == rods_files,
                        msg="Files missing:\n" + str(file_names - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - file_names))

        vault_files_post_irsync = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
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
        rods_files_source = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        rods_files_target = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_target])[1]))
        self.assertTrue(file_names == rods_files_target,
                        msg="Files missing:\n" + str(file_names - rods_files_target) + "\n\n" +
                            "Extra files :\n" + str(rods_files_target - file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
                                                                     base_name_source)))

        self.assertTrue(file_names == vault_files_post_irsync_source,
                        msg="Files missing from vault:\n" + str(file_names - vault_files_post_irsync_source) + "\n\n" +
                            "Extra files in vault:\n" + str(vault_files_post_irsync_source - file_names))

        vault_files_post_irsync_target = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
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
        rods_files_source = set(session.ils_output_to_entries(self.user0.run_icommand(['ils', base_name_source])[1]))
        self.assertTrue(file_names == rods_files_source,
                        msg="Files missing:\n" + str(file_names - rods_files_source) + "\n\n" +
                            "Extra files:\n" + str(rods_files_source - file_names))

        local_files = set(os.listdir(local_dir))
        self.assertTrue(file_names == local_files,
                        msg="Files missing from local dir:\n" + str(file_names - local_files) + "\n\n" +
                            "Extra files in local dir:\n" + str(local_files - file_names))

        vault_files_post_irsync_source = set(os.listdir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

        # sync collections
        self.user0.assert_icommand("irsync -r i:{source_base_name} i:{dest_base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)

            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', source_partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
                                                              source_partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', dest_partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # iput dir
        self.user0.assert_icommand("iput -r {local_dir}".format(**locals()), "EMPTY")

        # sync collections
        self.user0.assert_icommand("irsync -r i:{source_base_name} i:{dest_base_name}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)

            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', source_partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
                                                              source_partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', dest_partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

        # remove local coll
        shutil.rmtree(local_dir)

        # now sync back coll to dir
        self.user0.assert_icommand("irsync -r i:{base_name} {local_dir}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
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
        local_dirs = session.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # sync dir to coll
        self.user0.assert_icommand("irsync -r {local_dir} i:{base_name}".format(**locals()), "EMPTY")

        # remove local coll
        shutil.rmtree(local_dir)

        # now sync back coll to dir
        self.user0.assert_icommand("irsync -r i:{base_name} {local_dir}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = session.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])

            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(session.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # compare local files with files in vault
            files_in_vault = set(session.files_in_dir(os.path.join(session.get_vault_session_path(self.user0),
                                                              partial_path)))
            self.assertTrue(local_files == files_in_vault,
                        msg="Files missing from vault:\n" + str(local_files - files_in_vault) + "\n\n" +
                            "Extra files in vault:\n" + str(files_in_vault - local_files))

    def test_cancel_large_iput(self):
        base_name = 'test_cancel_large_put'
        local_dir = os.path.join(self.testing_tmp_dir, base_name)
        file_size = pow(2, 30)
        file_name = session.make_large_local_tmp_dir(local_dir, file_count=1, file_size=file_size)[0]
        file_local_full_path = os.path.join(local_dir, file_name)
        iput_cmd = "iput '" + file_local_full_path + "'"
        file_vault_full_path = os.path.join(session.get_vault_session_path(self.user0), file_name)
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
        self.admin.assert_icommand('iadmin mkresc test1 unixfilesystem ' + session.get_hostname() + ':' + session.get_irods_top_level_dir() + '/test1',
                'STDOUT_SINGLELINE', '')
        self.admin.assert_icommand('iadmin mkresc test2 unixfilesystem ' + session.get_hostname() + ':' + session.get_irods_top_level_dir() + '/test2',
                'STDOUT_SINGLELINE', '')
        self.admin.assert_icommand('iphymv -S test1 -R test2 -r /', 'STDERR_SINGLELINE',
                'ERROR: phymvUtil: \'/\' does not specify a zone; physical move only makes sense within a zone.')
        self.admin.assert_icommand('iadmin rmresc test1')
        self.admin.assert_icommand('iadmin rmresc test2')

    def test_iput_bulk_check_acpostprocforput__2841(self):
        # prepare test directory
        number_of_files = 5
        dirname = self.admin.local_session_dir + '/files'
        corefile = session.get_core_re_dir() + "/core.re"
        # files less than 4200000 were failing to trigger the writeLine
        for filesize in range(5000, 6000000, 500000):
            files = session.make_large_local_tmp_dir(dirname, number_of_files, filesize)
            # manipulate core.re and check the server log
            with session.file_backed_up(corefile):
                initial_size_of_server_log = session.get_log_size('server')
                rules_to_prepend = '''
acBulkPutPostProcPolicy { msiSetBulkPutPostProcPolicy("on"); }
acPostProcForPut { writeLine("serverLog", "acPostProcForPut called for $objPath"); }
            '''
                time.sleep(1)  # remove once file hash fix is committed #2279
                session.prepend_string_to_file(rules_to_prepend, corefile)
                time.sleep(1)  # remove once file hash fix is committed #2279
                self.admin.assert_icommand(['iput', '-frb', dirname])
                assert number_of_files == session.count_occurrences_of_string_in_log(
                    'server', 'writeLine: inString = acPostProcForPut called for', start_index=initial_size_of_server_log)
                shutil.rmtree(dirname)

    def test_large_irods_maximum_size_for_single_buffer_in_megabytes_2880(self):
        self.admin.environment_file_contents['irods_maximum_size_for_single_buffer_in_megabytes'] = 2000
        with tempfile.NamedTemporaryFile(prefix='test_large_irods_maximum_size_for_single_buffer_in_megabytes_2880') as f:
            session.make_file(f.name, 800*1000*1000, contents='arbitrary')
            self.admin.assert_icommand(['iput', f.name, '-v'], 'STDOUT_SINGLELINE', '0 thr')

    def test_ils_connection_refused_2948(self):
        @contextlib.contextmanager
        def irods_server_stopped():
            session.stop_irods_server()
            try:
                yield
            finally:
                session.start_irods_server()
        with irods_server_stopped():
            self.admin.assert_icommand(['ils'], 'STDERR_SINGLELINE', 'Connection refused')
            _, out, err = self.admin.assert_icommand(['ils', '-V'], 'STDERR_SINGLELINE', 'Connection refused')
            assert 'errno = {0}'.format(errno.ECONNREFUSED) in out, 'missing ECONNREFUSED errno in\n' + out
            assert 'errno = {0}'.format(errno.ECONNABORTED) not in out, 'found ECONNABORTED errno in\n' + out
            assert 'errno = {0}'.format(errno.EINVAL) not in out, 'found EINVAL errno in\n' + out
