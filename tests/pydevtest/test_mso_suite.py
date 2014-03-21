import time
import sys
import shutil
import os
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, getiCmdOutput
import pydevtest_sessions as s
import socket
import commands
import os
import shlex
import datetime
import time

RODSHOME = "/home/irodstest/irodsfromsvn/iRODS"
ABSPATHTESTDIR = os.path.abspath(os.path.dirname (sys.argv[0]))
RODSHOME = ABSPATHTESTDIR + "/../../iRODS"

class Test_MSOSuite(unittest.TestCase, ResourceBase):
 
    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc compound",
            "iadmin mkresc cacheResc 'unix file system' "+hostname+":/var/lib/irods/cacheRescVault",
            "iadmin mkresc archiveResc mso "+hostname+":/fake/vault/",
            "iadmin addchildtoresc demoResc cacheResc cache",
            "iadmin addchildtoresc demoResc archiveResc archive",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc archiveResc",
            "iadmin rmchildfromresc demoResc cacheResc",
            "iadmin rmresc archiveResc",
            "iadmin rmresc cacheResc",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/irods/archiveRescVault",
            "rm -rf /var/lib/irods/cacheRescVault",
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
        test_file_path = "/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmd( s.adminsession, 'ireg -D mso -R archiveResc "//http://people.renci.org/~jasonc/irods/http_mso_test_file.txt" '+test_file_path+'/test_file.txt')
        assertiCmd( s.adminsession, 'iget -f '+test_file_path+'/test_file.txt')
        
        shutil.copy( 'test_file.txt', 'backup_test_file.txt' )
        myfile = open( 'test_file.txt', "a")
        myfile.write('THIS IS A NEW LINE\n')
        myfile.close()
        
        assertiCmd( s.adminsession, 'iput -f test_file.txt '+test_file_path+'/test_file.txt')
        os.remove( 'test_file.txt' )
        
	assertiCmd( s.adminsession, 'iget -f '+test_file_path+'/test_file.txt')


	result = os.system("diff %s %s" % ( 'test_file.txt', 'backup_test_file.txt' ))
        assert result != 0 

        # repave remote file with old copy of the file
        assertiCmd( s.adminsession, 'iput -f backup_test_file.txt '+test_file_path+'/test_file.txt')
        
        # unregister the object
        assertiCmd( s.adminsession, 'irm -U '+test_file_path+'/test_file.txt')
        
        os.remove( 'test_file.txt' )
        os.remove( 'backup_test_file.txt' )

    def test_mso_slink(self):
        test_file_path = "/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmd( s.adminsession, 'iput -fR origResc ../zombiereaper.sh src_file.txt' )
        assertiCmd( s.adminsession, 'ireg -D mso -R archiveResc "//slink:'+test_file_path+'/src_file.txt" '+test_file_path+'/test_file.txt')
        assertiCmd( s.adminsession, 'iget -f '+test_file_path+'/test_file.txt')
	
        result = os.system("diff %s %s" % ( './test_file.txt', '../zombiereaper.sh' ))
        assert result == 0 
        
        assertiCmd( s.adminsession, 'iput -f ../zombiereaper.sh '+test_file_path+'/test_file.txt')
        
        # unregister the object
        assertiCmd( s.adminsession, 'irm -U '+test_file_path+'/test_file.txt')

    def test_mso_irods(self):
        hostname = socket.gethostname()
        test_file_path = "/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
        assertiCmd( s.adminsession, 'iput -fR origResc ../zombiereaper.sh src_file.txt' )
        assertiCmd( s.adminsession, 'ireg -D mso -R archiveResc "//irods:'+hostname+':1247:rods@tempZone'+test_file_path+'/src_file.txt" '+test_file_path+'/test_file.txt')
        assertiCmd( s.adminsession, 'iget -f '+test_file_path+'/test_file.txt')
	
        result = os.system("diff %s %s" % ( './test_file.txt', '../zombiereaper.sh' ))
        assert result == 0 
        
        assertiCmd( s.adminsession, 'iput -f ../zombiereaper.sh '+test_file_path+'/test_file.txt')
        
        # unregister the object
        assertiCmd( s.adminsession, 'irm -U '+test_file_path+'/test_file.txt')

