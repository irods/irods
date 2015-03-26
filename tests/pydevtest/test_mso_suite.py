import time
import sys
import shutil
import os
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput, get_irods_top_level_dir, get_irods_config_dir
import pydevtest_sessions as s
import socket
import commands
import os
import shlex
import datetime
import time

RODSHOME = "/home/irodstest/irodsfromsvn/iRODS"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname(sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"


class Test_MSOSuite(unittest.TestCase, ResourceBase):

    hostname = socket.gethostname()
    my_test_resource = {
        "setup": [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc compound",
            "iadmin mkresc cacheResc 'unix file system' " + hostname +
            ":" + get_irods_top_level_dir() + "/cacheRescVault",
            "iadmin mkresc archiveResc mso " + hostname + ":/fake/vault/",
            "iadmin addchildtoresc demoResc cacheResc cache",
            "iadmin addchildtoresc demoResc archiveResc archive",
        ],
        "teardown": [
            "iadmin rmchildfromresc demoResc archiveResc",
            "iadmin rmchildfromresc demoResc cacheResc",
            "iadmin rmresc archiveResc",
            "iadmin rmresc cacheResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf " + get_irods_top_level_dir() + "/archiveRescVault",
            "rm -rf " + get_irods_top_level_dir() + "/cacheRescVault",
        ],
    }

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_mso_http(self):
        test_file_path = "/" + s.adminsession.get_zone_name() + "/home/" + s.adminsession.get_username() + \
            "/" + s.adminsession._session_id
        assertiCmd(s.adminsession, 'ireg -D mso -R archiveResc "//http://people.renci.org/~jasonc/irods/http_mso_test_file.txt" ' +
                   test_file_path + '/test_file.txt')
        assertiCmd(s.adminsession, 'iget -f ' + test_file_path + '/test_file.txt')
        assertiCmdFail(s.adminsession, 'ils -L ' + test_file_path + '/test_file.txt', 'STDOUT', ' -99 ')
        os.remove('test_file.txt')
        # unregister the object
        assertiCmd(s.adminsession, 'irm -U ' + test_file_path + '/test_file.txt')
        assertiCmd(s.adminsession, 'ils -L', 'STDOUT', 'tempZone')

    def test_mso_slink(self):
        test_file_path = "/" + s.adminsession.get_zone_name() + "/home/" + s.adminsession.get_username() + \
            "/" + s.adminsession._session_id
        assertiCmd(s.adminsession, 'iput -fR origResc ../zombiereaper.sh src_file.txt')
        assertiCmd(s.adminsession, 'ireg -D mso -R archiveResc "//slink:' +
                   test_file_path + '/src_file.txt" ' + test_file_path + '/test_file.txt')
        assertiCmd(s.adminsession, 'iget -f ' + test_file_path + '/test_file.txt')

        result = os.system("diff %s %s" % ('./test_file.txt', '../zombiereaper.sh'))
        assert result == 0

        assertiCmd(s.adminsession, 'iput -f ../zombiereaper.sh ' + test_file_path + '/test_file.txt')

        # unregister the object
        assertiCmd(s.adminsession, 'irm -U ' + test_file_path + '/test_file.txt')

    def test_mso_irods(self):
        hostname = socket.gethostname()
        test_file_path = "/" + s.adminsession.get_zone_name() + "/home/" + s.adminsession.get_username() + \
            "/" + s.adminsession._session_id
        assertiCmd(s.adminsession, 'iput -fR pydevtest_AnotherResc ../zombiereaper.sh src_file.txt')
        assertiCmd(s.adminsession, 'ireg -D mso -R archiveResc "//irods:' + hostname +
                   ':1247:rods@tempZone' + test_file_path + '/src_file.txt" ' + test_file_path + '/test_file.txt')
        assertiCmd(s.adminsession, 'iget -f ' + test_file_path + '/test_file.txt')

        result = os.system("diff %s %s" % ('./test_file.txt', '../zombiereaper.sh'))
        assert result == 0

        assertiCmd(s.adminsession, 'iput -f ../zombiereaper.sh ' + test_file_path + '/test_file.txt')

        # unregister the object
        assertiCmd(s.adminsession, 'irm -U ' + test_file_path + '/test_file.txt')
        assertiCmd(s.adminsession, 'irm -f src_file.txt')
