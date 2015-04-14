import sys
import string
import os
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
import lib


class Test_CompatibilitySuite(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_CompatibilitySuite, self).setUp()

    def tearDown(self):
        super(Test_CompatibilitySuite, self).tearDown()

    def test_imeta_set(self):
        self.admin.assert_icommand('iadmin lu', 'STDOUT', self.admin.username)
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'None')
        self.admin.assert_icommand('imeta add -u ' + self.user0.username + ' att val')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'attribute: att')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att newval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'value: newval')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att newval someunit')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'units: someunit')
        self.admin.assert_icommand('imeta set -u ' + self.user0.username + ' att verynewval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'value: verynewval')
        self.admin.assert_icommand('imeta ls -u ' + self.user0.username + ' att', 'STDOUT', 'units: someunit')

    def test_iphybun_n(self):
        self.admin.assert_icommand('imkdir testColl')
        self.admin.assert_icommand('icd testColl')
        for i in range(8):
            f = 'empty{0}.txt'.format(i)
            lib.cat(f, str(i))
            self.admin.assert_icommand(['iput', f])
            os.unlink(f)
        self.admin.assert_icommand('icd ..')
        self.admin.assert_icommand('iphybun -N3 -SdemoResc -RdemoResc testColl')
        coll_dir = self.admin.run_icommand('ils /'+self.admin.zone_name+'/bundle/home/'+self.admin.username)[1].split('\n')[-2].lstrip(string.printable.translate(None, '/'))
        after = self.admin.run_icommand(['ils', coll_dir])[1].split('\n')
        assert len(after) == 2 + 3
        self.admin.assert_icommand('irm -rf testColl')
