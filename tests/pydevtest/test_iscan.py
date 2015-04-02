import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from resource_suite import ResourceBase
import pydevtest_common
from pydevtest_common import assertiCmd, assertiCmdFail, getiCmdOutput

import pydevtest_sessions as s


class Test_iScan(unittest.TestCase, ResourceBase):

    my_test_resource = {'setup': [], 'teardown': []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    @unittest.skipIf(pydevtest_common.irods_test_constants.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_local_file(self):
        assertiCmd( s.sessions[1], 'iscan non_existent_file', 'STDERR', 'ERROR: scanObj: non_existent_file does not exist' )
        existent_file = os.path.join( s.sessions[1]._session_dir, 'existent_file' )
        pydevtest_common.touch( existent_file )
        assertiCmd( s.sessions[1], 'iscan ' + existent_file, 'STDOUT', existent_file + ' is not registered in iRODS' )
        assertiCmd( s.sessions[1], 'iput ' + existent_file );
        output = getiCmdOutput( s.sessions[1], '''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )[0]
        data_path = output.strip().strip('-').strip()[12:]
        assertiCmd( s.sessions[1], 'iscan ' + data_path );
        assertiCmd( s.sessions[1], 'irm -f existent_file' );

    @unittest.skipIf(pydevtest_common.irods_test_constants.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: Reads Vault')
    def test_iscan_data_object(self):
        #test that rodsusers can't use iscan -d
        assertiCmd( s.sessions[1], 'iscan -d non_existent_file', 'STDOUT', 'Could not find the requested data object or collection in iRODS.' )
        existent_file = os.path.join( s.sessions[1]._session_dir, 'existent_file' )
        pydevtest_common.make_file( existent_file, 1 )
        assertiCmd( s.sessions[1], 'iput ' + existent_file );
        output = getiCmdOutput( s.adminsession, 'iquest "SELECT DATA_PATH WHERE DATA_NAME = \'existent_file\'"' )[0]
        data_path = output.strip().strip('-').strip()[12:]
        assertiCmd( s.sessions[1], 'iscan -d existent_file', 'STDOUT', 'User must be a rodsadmin to scan iRODS data objects.' );
        os.remove( data_path )
        assertiCmd( s.sessions[1], 'iscan -d existent_file', 'STDOUT', 'User must be a rodsadmin to scan iRODS data objects.' );
        pydevtest_common.make_file( data_path, 1 )
        assertiCmd( s.sessions[1], 'irm -f existent_file' );
        zero_file = os.path.join( s.sessions[1]._session_dir, 'zero_file' )
        pydevtest_common.touch( zero_file )
        assertiCmd( s.sessions[1], 'iput ' + zero_file );
        assertiCmd( s.sessions[1], 'iscan -d zero_file', 'STDOUT', 'User must be a rodsadmin to scan iRODS data objects.' );
        assertiCmd( s.sessions[1], 'irm -f zero_file' );

        #test that rodsadmins can use iscan -d
        assertiCmd( s.adminsession, 'iscan -d non_existent_file', 'STDOUT', 'Could not find the requested data object or collection in iRODS.' )
        existent_file = os.path.join( s.adminsession._session_dir, 'existent_file' )
        pydevtest_common.make_file( existent_file, 1 )
        assertiCmd( s.adminsession, 'iput ' + existent_file );
        output = getiCmdOutput( s.adminsession, '''iquest "SELECT DATA_PATH WHERE DATA_NAME = 'existent_file'"''' )[0]
        data_path = output.strip().strip('-').strip()[12:]
        assertiCmd( s.adminsession, 'iscan -d existent_file' );
        os.remove( data_path )
        assertiCmd( s.adminsession, 'iscan -d existent_file', 'STDOUT', 'is missing, corresponding to iRODS object' );
        pydevtest_common.make_file( data_path, 1 )
        assertiCmd( s.adminsession, 'irm -f existent_file' );
        zero_file = os.path.join( s.adminsession._session_dir, 'zero_file' )
        pydevtest_common.touch( zero_file )
        assertiCmd( s.adminsession, 'iput ' + zero_file );
        assertiCmd( s.adminsession, 'iscan -d zero_file' );
        assertiCmd( s.adminsession, 'irm -f zero_file' );
