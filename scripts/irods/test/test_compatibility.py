import sys
import string
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
from .resource_suite import ResourceBase
from .. import lib


class Test_Compatibility(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Compatibility, self).setUp()

    def tearDown(self):
        super(Test_Compatibility, self).tearDown()

    def test_imeta_set(self):
        self.admin.assert_icommand('iadmin lu', 'STDOUT_SINGLELINE', self.admin.username)
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'None')
        self.admin.assert_icommand('imeta add -u ' + self.user0.username + ' att val')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'attribute: att')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att newval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'value: newval')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att newval someunit')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'units: someunit')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att verynewval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'value: verynewval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT_SINGLELINE', 'units: someunit')
