import os
import sys
import shutil
import socket

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from ..configuration import IrodsConfig
from .resource_suite import ResourceBase
from .. import lib

def create_large_hierarchy(self, count, hostname, directory):
    self.admin.assert_icommand("iadmin mkresc repRescPrime replication", 'STDOUT_SINGLELINE', 'replication')
    for i in range(count):
        self.admin.assert_icommand("iadmin mkresc repResc{0} replication".format(i), 'STDOUT_SINGLELINE', 'replication')
        self.admin.assert_icommand("iadmin mkresc ptResc{0} passthru".format(i), 'STDOUT_SINGLELINE', 'passthru')
        self.admin.assert_icommand("iadmin mkresc ufs{0} unixfilesystem ".format(i) + hostname + ":" + directory + "/ufs{0}Vault".format(i), 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin addchildtoresc repResc{0} ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin addchildtoresc ptResc{0} ufs{0}".format(i))
        self.admin.assert_icommand("iadmin addchildtoresc repRescPrime repResc{0}".format(i))

def remove_large_hierarchy(self, count, directory):
    for i in range(count):
        self.admin.assert_icommand("iadmin rmchildfromresc repRescPrime repResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmchildfromresc ptResc{0} ufs{0}".format(i))
        self.admin.assert_icommand("iadmin rmchildfromresc repResc{0} ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc repResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc ufs{0}".format(i))
        shutil.rmtree(directory + "/ufs{0}Vault".format(i), ignore_errors=True)
    self.admin.assert_icommand("iadmin rmresc repRescPrime")

class Test_Iquest(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Iquest, self).setUp()

    def tearDown(self):
        super(Test_Iquest, self).tearDown()

    def test_iquest_MAX_SQL_ROWS_results__3262(self):
        MAX_SQL_ROWS = 256 # needs to be the same as constant in server code
        filename = 'test_iquest_MAX_SQL_ROWS_results__3262'
        lib.make_file(filename, 1)
        data_object_prefix = 'loiuaxnlaskdfpiewrnsliuserd'
        for i in range(MAX_SQL_ROWS):
            self.admin.assert_icommand(['iput', filename, '{0}_file_{1}'.format(data_object_prefix, i)])

        self.admin.assert_icommand(['iquest', "select count(DATA_ID) where DATA_NAME like '{0}%'".format(data_object_prefix)], 'STDOUT_SINGLELINE', 'DATA_ID = {0}'.format(MAX_SQL_ROWS))
        self.admin.assert_icommand_fail(['iquest', '--no-page', "select DATA_ID where DATA_NAME like '{0}%'".format(data_object_prefix)], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')

    def test_iquest_with_data_resc_hier__3705(self):
        filename = 'test_iquest_data_name_with_data_resc_hier__3705'
        lib.make_file(filename, 1)
        self.admin.assert_icommand(['iput', filename])

        # Make sure DATA_RESC_HIER appears
        self.admin.assert_icommand(['iquest', "select DATA_NAME, DATA_RESC_HIER where DATA_RESC_HIER = '{0}'".format(self.admin.default_resource)], 'STDOUT_SINGLELINE', 'DATA_RESC_HIER')

        # Make sure DATA_RESC_ID appears
        self.admin.assert_icommand(['iquest', "select DATA_RESC_ID where DATA_RESC_HIER = '{0}'".format(self.admin.default_resource)], 'STDOUT_SINGLELINE', 'DATA_RESC_ID')

        # Make sure HIER and ID appear in the correct order
        _,out,_ = self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER, DATA_RESC_ID where DATA_NAME = '{0}' and DATA_RESC_HIER = '{1}'".format(filename, self.admin.default_resource)], 'STDOUT_SINGLELINE')
        self.assertTrue("DATA_RESC_HIER" in out.split('\n')[0])
        self.assertTrue("DATA_RESC_ID" in out.split('\n')[1])

        os.remove(filename)

    def test_iquest_resc_hier_with_like__3714(self):
        # Create a hierarchy to test
        LEAF_COUNT = 10
        directory = IrodsConfig().irods_directory
        create_large_hierarchy(self, LEAF_COUNT, socket.gethostname(), directory)
        filename = 'test_iquest_resc_hier_with_like__3714'
        lib.make_file(filename, 1)
        self.admin.assert_icommand("iput -R repRescPrime " + filename)
        expected_hier = 'repRescPrime;repResc3;ptResc3;ufs3'

        # SUCCESS
        # Matches ufs3's hierarchy
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '%ufs3'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches all hierarchies starting with repRescPrime
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'repRescPrime;%'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches all hierarchies containing resource ptResc3
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '%;ptResc3;%'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches leaf resource from full hierarchy
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = '{0}'".format(expected_hier)], 'STDOUT_SINGLELINE', expected_hier)
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '{0}'".format(expected_hier)], 'STDOUT_SINGLELINE', expected_hier)

        # FAILURE
        # % needed at front - ufs is not a root
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'ufs%'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # % needed at end - repRescPrime has children
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'repRescPrime'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # = with % results in error
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = '%ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # Results in direct match - must match full hier
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = 'ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')

        # Clean up
        os.remove(filename)
        self.admin.assert_icommand("irm -f " + filename)
        remove_large_hierarchy(self, LEAF_COUNT, directory)
