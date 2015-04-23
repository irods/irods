import time
import sys
import shutil
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import socket
import commands
import os
import datetime
import time

import resource_suite
import lib


RODSHOME = "/home/irodstest/irodsfromsvn/iRODS"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"

class Test_MSOSuite(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_MSOSuite, self).setUp()
        hostname = lib.get_hostname()
        self.admin.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        self.admin.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
        self.admin.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + hostname + ":" + lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin mkresc archiveResc mso " + hostname + ":/fake/vault/", 'STDOUT_SINGLELINE', 'mso')
        self.admin.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
        self.admin.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")

    def tearDown(self):
        super(Test_MSOSuite, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheRescVault")

    def test_mso_http(self):
        test_file_path = self.admin.session_collection
        self.admin.assert_icommand('ireg -D mso -R archiveResc "//http://people.renci.org/~jasonc/irods/http_mso_test_file.txt" ' +
                   test_file_path + '/test_file.txt')
        self.admin.assert_icommand('iget -f ' + test_file_path + '/test_file.txt')
        self.admin.assert_icommand_fail('ils -L ' + test_file_path + '/test_file.txt', 'STDOUT_SINGLELINE', ' -99 ')
        os.remove('test_file.txt')
        # unregister the object
        self.admin.assert_icommand('irm -U ' + test_file_path + '/test_file.txt')
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', self.admin.zone_name)

    def test_mso_slink(self):
        test_file_path = self.admin.session_collection
        self.admin.assert_icommand('iput -fR origResc ../zombiereaper.sh src_file.txt')
        self.admin.assert_icommand('ireg -D mso -R archiveResc "//slink:' +
                   test_file_path + '/src_file.txt" ' + test_file_path + '/test_file.txt')
        self.admin.assert_icommand('iget -f ' + test_file_path + '/test_file.txt')

        result = os.system("diff %s %s" % ('./test_file.txt', '../zombiereaper.sh'))
        assert result == 0

        self.admin.assert_icommand('iput -f ../zombiereaper.sh ' + test_file_path + '/test_file.txt')

        # unregister the object
        self.admin.assert_icommand('irm -U ' + test_file_path + '/test_file.txt')

    def test_mso_irods(self):
        hostname = socket.gethostname()
        test_file_path = self.admin.session_collection
        self.admin.assert_icommand('iput -fR pydevtest_AnotherResc ../zombiereaper.sh src_file.txt')
        self.admin.assert_icommand('ireg -D mso -R archiveResc "//irods:' + hostname +
                   ':1247:' + self.admin.username + '@' + self.admin.zone_name + test_file_path + '/src_file.txt" ' + test_file_path + '/test_file.txt')
        self.admin.assert_icommand('iget -f ' + test_file_path + '/test_file.txt')

        result = os.system("diff %s %s" % ('./test_file.txt', '../zombiereaper.sh'))
        assert result == 0

        self.admin.assert_icommand('iput -f ../zombiereaper.sh ' + test_file_path + '/test_file.txt')

        # unregister the object
        self.admin.assert_icommand('irm -U ' + test_file_path + '/test_file.txt')
        self.admin.assert_icommand('irm -f src_file.txt')
