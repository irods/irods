from __future__ import print_function

import contextlib
import time
import sys
import shutil
import os
import socket
import datetime
import imp
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import test
from . import settings
from . import resource_suite
from . import session
from .. import lib
from ..configuration import IrodsConfig


class Test_LoadBalanced_Resource(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            context_prefix = lib.get_hostname() + ':' + IrodsConfig().top_level_directory
            admin_session.assert_icommand('iadmin modresc demoResc name origResc', 'STDOUT_SINGLELINE', 'rename', input='yes\n')
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
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescB")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rescC")
            admin_session.assert_icommand("iadmin rmresc rescA")
            admin_session.assert_icommand("iadmin rmresc rescB")
            admin_session.assert_icommand("iadmin rmresc rescC")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.top_level_directory + "/rescAVault", ignore_errors=True)
        shutil.rmtree(irods_config.top_level_directory + "/rescBVault", ignore_errors=True)
        shutil.rmtree(irods_config.top_level_directory + "/rescCVault", ignore_errors=True)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_load_balanced(self):
        # =-=-=-=-=-=-=-
        # read server_config.json and .odbc.ini
        cfg = IrodsConfig()

        if cfg.database_config['catalog_database_type'] == "postgres":
            # =-=-=-=-=-=-=-
            # seed load table with fake values - rescA should win
            with contextlib.closing(cfg.get_database_connection()) as connection:
                with contextlib.closing(connection.cursor()) as cursor:
                    secs = int(time.time())
                    cursor.execute("insert into r_server_load_digest values ('rescA', 50, %s)" % secs)
                    cursor.execute("insert into r_server_load_digest values ('rescB', 75, %s)" % secs)
                    cursor.execute("insert into r_server_load_digest values ('rescC', 95, %s)" % secs)
                    cursor.commit()

                    # Make a local file to put
                    local_filepath = os.path.join(self.admin.local_session_dir, 'things.txt')
                    lib.make_file(local_filepath, 500, 'arbitrary')

                    # =-=-=-=-=-=-=-
                    # build a logical path for putting a file
                    test_file = self.admin.session_collection + "/test_file.txt"

                    # =-=-=-=-=-=-=-
                    # put a test_file.txt - should be on rescA given load table values
                    self.admin.assert_icommand("iput -f %s %s" % (local_filepath, test_file))
                    self.admin.assert_icommand("ils -L " + test_file, 'STDOUT_SINGLELINE', "rescA")
                    self.admin.assert_icommand("irm -f " + test_file)

                    # =-=-=-=-=-=-=-
                    # drop rescC to a load of 15 - this should now win
                    cursor.execute("update r_server_load_digest set load_factor=15 where resc_name='rescC'")
                    cursor.commit()

                    # =-=-=-=-=-=-=-
                    # put a test_file.txt - should be on rescC given load table values
                    self.admin.assert_icommand("iput -f %s %s" % (local_filepath, test_file))
                    self.admin.assert_icommand("ils -L " + test_file, 'STDOUT_SINGLELINE', "rescC")
                    self.admin.assert_icommand("irm -f " + test_file)

                    # =-=-=-=-=-=-=-
                    # clean up our alterations to the load table
                    cursor.execute("delete from r_server_load_digest where resc_name='rescA'")
                    cursor.execute("delete from r_server_load_digest where resc_name='rescB'")
                    cursor.execute("delete from r_server_load_digest where resc_name='rescC'")
                    cursor.commit()
        else:
            print('skipping test_load_balanced due to unsupported database for this test.')
