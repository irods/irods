import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .resource_suite import ResourceBase
from .. import lib


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
        assert "DATA_RESC_HIER" in out.split('\n')[0]
        assert "DATA_RESC_ID" in out.split('\n')[1]

        os.remove(filename)
