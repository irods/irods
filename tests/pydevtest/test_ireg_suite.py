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

import lib
import resource_suite


RODSHOME = lib.get_irods_top_level_dir() + "/iRODS/"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"

class Test_ireg_Suite(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_ireg_Suite, self).setUp()
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file0')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file1')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file2')
        shutil.copy2(ABSPATHTESTDIR + '/test_ireg_suite.py', ABSPATHTESTDIR + '/file3')

        self.admin.assert_icommand('iadmin mkresc r_resc passthru', 'STDOUT', "Creating")
        self.admin.assert_icommand('iadmin mkresc m_resc passthru', 'STDOUT', "Creating")
        hostname = socket.gethostname()
        self.admin.assert_icommand('iadmin mkresc l_resc unixfilesystem ' + hostname + ':/tmp/l_resc', 'STDOUT', "Creating")

        self.admin.assert_icommand("iadmin addchildtoresc r_resc m_resc")
        self.admin.assert_icommand("iadmin addchildtoresc m_resc l_resc")

    def tearDown(self):
        self.admin.assert_icommand('irm -f ' + self.admin.home_collection + '/file0')
        self.admin.assert_icommand('irm -f ' + self.admin.home_collection + '/file1')
        self.admin.assert_icommand('irm -f ' + self.admin.home_collection + '/file3')

        self.admin.assert_icommand("irmtrash -M")
        self.admin.assert_icommand("iadmin rmchildfromresc m_resc l_resc")
        self.admin.assert_icommand("iadmin rmchildfromresc r_resc m_resc")

        self.admin.assert_icommand("iadmin rmresc r_resc")
        self.admin.assert_icommand("iadmin rmresc m_resc")
        self.admin.assert_icommand("iadmin rmresc l_resc")

        os.remove(ABSPATHTESTDIR + '/file2')

        super(Test_ireg_Suite, self).tearDown()

    def test_ireg_files(self):
        self.admin.assert_icommand("ireg -R l_resc " + ABSPATHTESTDIR + '/file0 ' + self.admin.home_collection + 'file0')
        self.admin.assert_icommand('ils -l ' + self.admin.home_collection + '/file0', 'STDOUT', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R l_resc " + ABSPATHTESTDIR + '/file1 ' + self.admin.home_collection + 'file1')
        self.admin.assert_icommand('ils -l ' + self.admin.home_collection + '/file1', 'STDOUT', "r_resc;m_resc;l_resc")

        self.admin.assert_icommand("ireg -R m_resc " + ABSPATHTESTDIR +
                                   '/file2 ' + self.admin.home_collection + '/file2', 'STDERR', 'ERROR: regUtil: reg error for')

        self.admin.assert_icommand("ireg -R demoResc " + ABSPATHTESTDIR + '/file3 ' + self.admin.home_collection + '/file3')
