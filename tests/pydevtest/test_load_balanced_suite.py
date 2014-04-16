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
import shlex
import datetime

pydevtestdir = os.path.realpath(__file__)
topdir = os.path.dirname(os.path.dirname(os.path.dirname(pydevtestdir)))
packagingdir = os.path.join(topdir,"packaging")
sys.path.append(packagingdir)
from server_config import Server_Config

class Test_LoadBalanced_Resource(unittest.TestCase, ResourceBase):
  
    hostname = socket.gethostname()
    my_test_resource = {
        "setup"    : [
            "iadmin modresc demoResc name origResc",
            "iadmin mkresc demoResc load_balanced",
            "iadmin mkresc rescA 'unix file system' "+hostname+":/var/lib/irods/rescAVault",
            "iadmin mkresc rescB 'unix file system' "+hostname+":/var/lib/irods/rescBVault",
            "iadmin mkresc rescC 'unix file system' "+hostname+":/var/lib/irods/rescCVault",
            "iadmin addchildtoresc demoResc rescA",
            "iadmin addchildtoresc demoResc rescB",
            "iadmin addchildtoresc demoResc rescC",
        ],
        "teardown" : [
            "iadmin rmchildfromresc demoResc rescA",
            "iadmin rmchildfromresc demoResc rescB",
            "iadmin rmchildfromresc demoResc rescC",
            "iadmin rmresc rescA",
            "iadmin rmresc rescB",
            "iadmin rmresc rescC",
            "iadmin rmresc demoResc",
            "iadmin modresc origResc name demoResc",
            "rm -rf /var/lib/irods/rescAVault",
            "rm -rf /var/lib/irods/rescBVault",
            "rm -rf /var/lib/irods/rescCVault",
        ],
    }

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()
    
    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_load_balanced(self):
       # =-=-=-=-=-=-=-
       # read server.config and .odbc.ini
       cfg = Server_Config() 
    
       if cfg.values[ 'catalog_database_type' ] == "postgres" :
           # =-=-=-=-=-=-=-
           # seed load table with fake values - rescA should win
           secs = int( time.time() )
           cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescA', 50, %s)" % secs )
           cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescB', 75, %s)" % secs )
           cfg.exec_sql_cmd( "insert into r_server_load_digest values ('rescC', 95, %s)" % secs )

           # =-=-=-=-=-=-=-
           # build a logical path for putting a file
           test_file_path = "/"+s.adminsession.getZoneName()+"/home/"+s.adminsession.getUserName()+"/"+s.adminsession.sessionId
           test_file = test_file_path + "/test_file.txt"

           # =-=-=-=-=-=-=-
           # put a test_file.txt - should be on rescA given load table values
           assertiCmd( s.adminsession ,"iput -f ./test_load_balanced_suite.py "+test_file )
           assertiCmd( s.adminsession, "ils -L "+test_file, "LIST", "rescA" )
           assertiCmd( s.adminsession ,"irm -f "+test_file )
        
           # =-=-=-=-=-=-=-
           # drop rescC to a load of 15 - this should now win
           cfg.exec_sql_cmd( "update r_server_load_digest set load_factor=15 where resc_name='rescC'")
           
           # =-=-=-=-=-=-=-
           # put a test_file.txt - should be on rescC given load table values
           assertiCmd( s.adminsession ,"iput -f ./test_load_balanced_suite.py "+test_file )
           assertiCmd( s.adminsession, "ils -L "+test_file, "LIST", "rescC" )
           assertiCmd( s.adminsession ,"irm -f "+test_file )
        
           # =-=-=-=-=-=-=-
           # clean up our alterations to the load table
           cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescA'")
           cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescB'")
           cfg.exec_sql_cmd( "delete from r_server_load_digest where resc_name='rescC'")
       else:
           print 'skipping test_load_balanced due to unsupported database for this test.'
