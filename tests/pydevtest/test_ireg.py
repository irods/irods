import sys
import shutil
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import datetime
import socket

import configuration
import lib
import resource_suite


RODSHOME = lib.get_irods_top_level_dir() + "/iRODS/"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"

@unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Registers files on remote resources')
class Test_Ireg(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Ireg, self).setUp()
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg.py', ABSPATHTESTDIR + '/file0')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg.py', ABSPATHTESTDIR + '/file1')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg.py', ABSPATHTESTDIR + '/file2')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg.py', ABSPATHTESTDIR + '/file3')

        self.admin.assert_icommand('iadmin mkresc r_resc passthru', 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand('iadmin mkresc m_resc passthru', 'STDOUT_SINGLELINE', "Creating")
        hostname = socket.gethostname()
        self.admin.assert_icommand('iadmin mkresc l_resc unixfilesystem ' + hostname + ':/tmp/l_resc', 'STDOUT_SINGLELINE', "Creating")

        self.admin.assert_icommand("iadmin addchildtoresc r_resc m_resc")
        self.admin.assert_icommand("iadmin addchildtoresc m_resc l_resc")

        # make local test dir
        self.testing_tmp_dir = '/tmp/irods-test-ireg'
        shutil.rmtree(self.testing_tmp_dir, ignore_errors=True)
        os.mkdir(self.testing_tmp_dir)

    def tearDown(self):
        # remove local test dir
        shutil.rmtree(self.testing_tmp_dir)

        self.admin.run_icommand('irm -f ' + self.admin.home_collection + '/file0')
        self.admin.run_icommand('irm -f ' + self.admin.home_collection + '/file1')
        self.admin.run_icommand('irm -f ' + self.admin.home_collection + '/file3')

        self.admin.assert_icommand("irmtrash -M")
        self.admin.assert_icommand("iadmin rmchildfromresc m_resc l_resc")
        self.admin.assert_icommand("iadmin rmchildfromresc r_resc m_resc")

        self.admin.assert_icommand("iadmin rmresc r_resc")
        self.admin.assert_icommand("iadmin rmresc m_resc")
        self.admin.assert_icommand("iadmin rmresc l_resc")

        os.remove(ABSPATHTESTDIR + '/file2')

        super(Test_Ireg, self).tearDown()

    def test_ireg_files(self):
        self.admin.assert_icommand("ireg -R l_resc " + ABSPATHTESTDIR + '/file0 ' + self.admin.home_collection + '/file0')
        self.admin.assert_icommand('ils -l ' + self.admin.home_collection + '/file0', 'STDOUT_SINGLELINE', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R l_resc " + ABSPATHTESTDIR + '/file1 ' + self.admin.home_collection + '/file1')
        self.admin.assert_icommand('ils -l ' + self.admin.home_collection + '/file1', 'STDOUT_SINGLELINE', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R m_resc " + ABSPATHTESTDIR +
                                   '/file2 ' + self.admin.home_collection + '/file2', 'STDERR_SINGLELINE', 'ERROR: regUtil: reg error for')

        self.admin.assert_icommand("ireg -R demoResc " + ABSPATHTESTDIR + '/file3 ' + self.admin.home_collection + '/file3')

    def test_ireg_new_replica__2847(self):
        filename = 'regfile.txt'
        filename2 = filename+'2'
        os.system('rm -f {0} {1}'.format(filename, filename2))
        lib.make_file(filename, 234)
        os.system('cp {0} {1}'.format(filename, filename2))
        self.admin.assert_icommand('ireg -Kk -R {0} {1} {2}'.format(self.testresc, os.path.abspath(filename), self.admin.session_collection+'/'+filename))
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', [' 0 '+self.testresc, '& '+filename])
        self.admin.assert_icommand('ireg -Kk --repl -R {0} {1} {2}'.format(self.anotherresc, os.path.abspath(filename2), self.admin.session_collection+'/'+filename))
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', [' 1 '+self.anotherresc, '& '+filename])
        os.system('rm -f {0} {1}'.format(filename, filename2))

    def test_ireg_dir_exclude_from(self):
        # test settings
        depth = 10
        files_per_level = 10
        file_size = 100

        # make a local test dir under /tmp/
        test_dir_name = 'test_ireg_dir_exclude_from'
        local_dir = os.path.join(self.testing_tmp_dir, test_dir_name)
        local_dirs = lib.make_deep_local_tmp_dir(local_dir, depth, files_per_level, file_size)

        # arbitrary list of files to exclude
        excluded = set(['junk0001', 'junk0002', 'junk0003'])

        # make exclusion file
        exclude_file_path = os.path.join(self.testing_tmp_dir, 'exclude.txt')
        with open(exclude_file_path, 'w') as exclude_file:
            for filename in excluded:
                print >>exclude_file, filename

        # register local dir
        target_collection = self.admin.home_collection + '/' + test_dir_name
        self.admin.assert_icommand("ireg --exclude-from {exclude_file_path} -C {local_dir} {target_collection}".format(**locals()), "EMPTY")

        # compare files at each level
        for dir, files in local_dirs.iteritems():
            subcollection = dir.replace(local_dir, target_collection, 1)

            # run ils on subcollection
            self.admin.assert_icommand(['ils', subcollection], 'STDOUT_SINGLELINE')
            ils_out = lib.ils_output_to_entries(self.admin.run_icommand(['ils', subcollection])[1])

            # compare irods objects with local files, minus the excluded ones
            local_files = set(files)
            rods_files = set(lib.files_in_ils_output(ils_out))
            self.assertSetEqual(rods_files, local_files - excluded,
                            msg="Files missing:\n" + str(local_files - rods_files) + "\n\n" +
                            "Extra files:\n" + str(rods_files - local_files))

        # unregister collection
        self.admin.assert_icommand("irm -rfU {target_collection}".format(**locals()))

        # remove local test dir and exclusion file
        os.remove(exclude_file_path)
        shutil.rmtree(local_dir)

    def test_ireg_irepl_coordinating_resource__issue_3517(self): 
        filename = 'test_ireg_irepl_coordinating_resource__issue_3517'
        filename_2 = filename + '_2'
        lib.make_file(filename, 1024, 'arbitrary')
        lib.make_file(filename_2, 1024, 'arbitrary')
        self.admin.assert_icommand(['ireg', '-Kk', '-R', 'demoResc', os.path.abspath(filename), self.admin.session_collection + '/' + filename]) 
        # ireg --repl should fail if it targets a coordinating (i.e. non-leaf) resource  
        self.admin.assert_icommand_fail(['ireg', '-Kk', '--repl', '-R', 'r_resc', os.path.abspath(filename_2), self.admin.session_collection + '/' + filename], 'STDOUT_SINGLELINE', 'coordinating resource')
        # ireg --repl should fail if it targets a coordinating (i.e. non-leaf) resource  
        self.admin.assert_icommand(['ireg', '-Kk', '--repl', '-R', 'r_resc', os.path.abspath(filename_2), self.admin.session_collection + '/' + filename], 'STDERR_SINGLELINE', 'USER_INVALID_RESC_INPUT')
        # ireg --repl should succeed targeting a leaf resource 
        self.admin.assert_icommand(['ireg', '-Kk', '--repl', '-R', 'l_resc', os.path.abspath(filename_2), self.admin.session_collection + '/' + filename]) 
        # ils is just for debug information
        self.admin.assert_icommand(['ils', '-L', self.admin.session_collection], 'STDOUT_SINGLELINE')
        # trim from l_resc so that resource can be deleted later
        self.admin.assert_icommand(['itrim', '-n1', '-N1', self.admin.session_collection + '/' + filename], 'STDOUT_SINGLELINE', 'trimmed')
        # ils is just for debug information
        self.admin.assert_icommand(['ils', '-L', self.admin.session_collection], 'STDOUT_SINGLELINE')
        os.system('rm -f ' + filename)
        os.system('rm -f ' + filename_2)
