import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest
import shutil

import configuration
from resource_suite import ResourceBase
import lib


@unittest.skip("TURN ON WHEN READY")
class Test_iRsync(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_iRsync, self).setUp()
        self.testing_tmp_dir = '/tmp/irods-test-irsync'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        shutil.rmtree(self.testing_tmp_dir)
        super(Test_iRsync, self).tearDown()

    def test_irsync(self):
        filepath = os.path.join(self.admin.local_session_dir, 'file')
        lib.make_file(filepath, 1)
        self.admin.assert_icommand('iput ' + filepath)
        self.admin.assert_icommand('irsync -l ' + filepath + ' i:file')

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
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])
            
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

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
        for dir, files in local_dirs.iteritems():
            partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)

            # run ils on subcollection
            self.user0.assert_icommand(['ils', partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])
            
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

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
        for dir, files in local_dirs.iteritems():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)
 
            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', source_partial_path])[1])
             
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', dest_partial_path])[1])
             
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

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
        for dir, files in local_dirs.iteritems():
            source_partial_path = dir.replace(self.testing_tmp_dir+'/', '', 1)
            dest_partial_path = dir.replace(self.testing_tmp_dir+'/'+source_base_name, dest_base_name, 1)
 
            # run ils on source subcollection
            self.user0.assert_icommand(['ils', source_partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', source_partial_path])[1])
             
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

            # now the same thing with dest subcollection
            self.user0.assert_icommand(['ils', dest_partial_path], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', dest_partial_path])[1])
             
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
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
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])
            
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
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
            ils_out = lib.ils_output_to_entries(self.user0.run_icommand(['ils', partial_path])[1])
            
            # compare local files with irods objects
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertTrue(local_files == rods_files,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))
