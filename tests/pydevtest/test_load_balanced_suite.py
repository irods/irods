import time
import sys
import shutil
import os
import socket
import commands
import datetime
import imp
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

import configuration
import lib
import resource_suite

pydevtestdir = os.path.dirname(os.path.realpath(__file__))
topdir = os.path.dirname(os.path.dirname(pydevtestdir))
packagingdir = os.path.join(topdir, 'packaging')
module_tuple = imp.find_module('server_config', [packagingdir])
imp.load_module('server_config', *module_tuple)
from server_config import ServerConfig


class Test_LoadBalanced_Resource(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            context_prefix = lib.get_hostname() + ':' + lib.get_irods_top_level_dir()
            admin_session.assert_icommand('iadmin modresc demoResc name origResc', 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand('iadmin mkresc demoResc load_balanced', 'STDOUT_SINGLELINE', 'load_balanced')
            admin_session.assert_icommand('iadmin mkresc rescA "unixfilesystem" ' + context_prefix + '/rescAVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc rescB "unixfilesystem" ' + context_prefix + '/rescBVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc rescC "unixfilesystem" ' + context_prefix + '/rescCVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc rescA')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc rescB')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc rescC')
        super(Test_LoadBalanced_Resource, self).setUp()

    def tearDown(self):
        super(Test_LoadBalanced_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescB")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescC")
            admin_session.assert_icommand("iadmin rmresc rescA")
            admin_session.assert_icommand("iadmin rmresc rescB")
            admin_session.assert_icommand("iadmin rmresc rescC")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/rescAVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/rescBVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/rescCVault", ignore_errors=True)

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_load_balanced(self):
        # =-=-=-=-=-=-=-
        # read server_config.json and .odbc.ini
        cfg = ServerConfig()

        if cfg.get('catalog_database_type') == "postgres":
            # =-=-=-=-=-=-=-
            # seed load table with fake values - rescA should win
            secs = int(time.time())
            cfg.exec_sql_cmd("insert into r_server_load_digest values ('rescA', 50, %s)" % secs)
            cfg.exec_sql_cmd("insert into r_server_load_digest values ('rescB', 75, %s)" % secs)
            cfg.exec_sql_cmd("insert into r_server_load_digest values ('rescC', 95, %s)" % secs)

            # =-=-=-=-=-=-=-
            # build a logical path for putting a file
            test_file = self.admin.session_collection + "/test_file.txt"

            # =-=-=-=-=-=-=-
            # put a test_file.txt - should be on rescA given load table values
            self.admin.assert_icommand("iput -f ./test_load_balanced_suite.py " + test_file)
            self.admin.assert_icommand("ils -L " + test_file, 'STDOUT_SINGLELINE', "rescA")
            self.admin.assert_icommand("irm -f " + test_file)

            # =-=-=-=-=-=-=-
            # drop rescC to a load of 15 - this should now win
            cfg.exec_sql_cmd("update r_server_load_digest set load_factor=15 where resc_name='rescC'")

            # =-=-=-=-=-=-=-
            # put a test_file.txt - should be on rescC given load table values
            self.admin.assert_icommand("iput -f ./test_load_balanced_suite.py " + test_file)
            self.admin.assert_icommand("ils -L " + test_file, 'STDOUT_SINGLELINE', "rescC")
            self.admin.assert_icommand("irm -f " + test_file)

            # =-=-=-=-=-=-=-
            # clean up our alterations to the load table
            cfg.exec_sql_cmd("delete from r_server_load_digest where resc_name='rescA'")
            cfg.exec_sql_cmd("delete from r_server_load_digest where resc_name='rescB'")
            cfg.exec_sql_cmd("delete from r_server_load_digest where resc_name='rescC'")
        else:
            print 'skipping test_load_balanced due to unsupported database for this test.'
