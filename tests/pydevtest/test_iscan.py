import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
from resource_suite import ResourceBase
import lib


class Test_iScan(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iScan, self).setUp()

    def tearDown(self):
        super(Test_iScan, self).tearDown()

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_local_file(self):
        self.user0.assert_icommand('iscan non_existent_file', 'STDERR_SINGLELINE', 'ERROR: scanObj: non_existent_file does not exist' )
        existent_file = os.path.join( self.user0.local_session_dir, 'existent_file' )
        lib.touch( existent_file )
        self.user0.assert_icommand('iscan ' + existent_file, 'STDOUT_SINGLELINE', existent_file + ' is not registered in iRODS' )
        self.user0.assert_icommand('iput ' + existent_file );
        output = self.user0.run_icommand('''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )[1]
        data_path = output.strip().strip('-').strip()[12:]
        self.user0.assert_icommand('iscan ' + data_path );
        self.user0.assert_icommand('irm -f existent_file' );

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_data_object(self):
        #test that rodsusers can't use iscan -d
        self.user0.assert_icommand('iscan -d non_existent_file', 'STDOUT_SINGLELINE', 'Could not find the requested data object or collection in iRODS.' )
        existent_file = os.path.join( self.user0.local_session_dir, 'existent_file' )
        lib.make_file( existent_file, 1 )
        self.user0.assert_icommand('iput ' + existent_file );
        output = self.admin.run_icommand('iquest "SELECT DATA_PATH WHERE DATA_NAME = \'existent_file\'"' )[1]
        data_path = output.strip().strip('-').strip()[12:]
        self.user0.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.' );
        os.remove( data_path )
        self.user0.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.' );
        lib.make_file( data_path, 1 )
        self.user0.assert_icommand('irm -f existent_file' );
        zero_file = os.path.join( self.user0.local_session_dir, 'zero_file' )
        lib.touch( zero_file )
        self.user0.assert_icommand('iput ' + zero_file );
        self.user0.assert_icommand('iscan -d zero_file', 'STDOUT_SINGLELINE', 'User must be a rodsadmin to scan iRODS data objects.' );
        self.user0.assert_icommand('irm -f zero_file' );

        #test that rodsadmins can use iscan -d
        self.admin.assert_icommand('iscan -d non_existent_file', 'STDOUT_SINGLELINE', 'Could not find the requested data object or collection in iRODS.' )
        existent_file = os.path.join( self.admin.local_session_dir, 'existent_file' )
        lib.make_file( existent_file, 1 )
        self.admin.assert_icommand('iput ' + existent_file );
        output = self.admin.run_icommand('''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )[1]
        data_path = output.strip().strip('-').strip()[12:]
        self.admin.assert_icommand('iscan -d existent_file' );
        os.remove( data_path )
        self.admin.assert_icommand('iscan -d existent_file', 'STDOUT_SINGLELINE', 'is missing, corresponding to iRODS object' );
        lib.make_file( data_path, 1 )
        self.admin.assert_icommand('irm -f existent_file' );
        zero_file = os.path.join( self.admin.local_session_dir, 'zero_file' )
        lib.touch( zero_file )
        self.admin.assert_icommand('iput ' + zero_file );
        self.admin.assert_icommand('iscan -d zero_file' );
        self.admin.assert_icommand('irm -f zero_file' );
