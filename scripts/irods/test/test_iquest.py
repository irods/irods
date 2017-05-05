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
