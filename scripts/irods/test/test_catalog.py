import datetime
import os
import platform
import sys
import time
import json

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .resource_suite import ResourceBase
from .. import lib
from .. import paths
from ..test.command import assert_command
from ..configuration import IrodsConfig


class Test_Catalog(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Catalog, self).setUp()

    def tearDown(self):
        super(Test_Catalog, self).tearDown()

    def test_case_sensitivity(self):
        self.user0.assert_icommand(['iput', self.testfile, 'a'])
        self.user0.assert_icommand(['iput', self.testfile, 'A'])
        self.user0.assert_icommand(['irm', 'a'])
        self.user0.assert_icommand(['irm', 'A'])

    def test_no_distinct(self):
        self.admin.assert_icommand(['iquest', 'no-distinct', 'select RESC_ID'], 'STDOUT_SINGLELINE', 'RESC_ID = ');

class Test_CatalogPermissions(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_CatalogPermissions, self).setUp()

    def tearDown(self):
        super(Test_CatalogPermissions, self).tearDown()

    def test_isysmeta_no_permission(self):
        self.user0.assert_icommand('icd /' + self.user0.zone_name + '/home/public')  # get into public/
        self.user0.assert_icommand('ils -L ', 'STDOUT_SINGLELINE', 'testfile.txt')
        self.user0.assert_icommand('isysmeta ls testfile.txt', 'STDOUT_SINGLELINE',
                                   'data_expiry_ts (expire time): 00000000000: None')  # initialized with zeros
        self.user0.assert_icommand('isysmeta mod testfile.txt 1', 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')  # cannot set expiry
