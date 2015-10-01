import os
import re
import sys
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import resource_suite
from .. import lib


class Test_Quotas(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Quotas, self).setUp()

    def tearDown(self):
        super(Test_Quotas, self).tearDown()

    def test_iquota__3044(self):
        myfile = 'quotafile'
        corefile = lib.get_core_re_dir() + "/core.re"
        with lib.file_backed_up(corefile):
            rules_to_prepend = 'acRescQuotaPolicy {msiSetRescQuotaPolicy("on"); }\n'
            time.sleep(2)  # remove once file hash fix is commited #2279
            lib.prepend_string_to_file(rules_to_prepend, corefile)
            time.sleep(2)  # remove once file hash fix is commited #2279
            for quotatype in [['suq',self.admin.username], ['sgq','public']]: # user and group
                for quotaresc in [self.testresc, 'total']: # resc and total
                    cmd = 'iadmin {0} {1} {2} 8000'.format(quotatype[0], quotatype[1], quotaresc) # set high quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'irepl -R {0} {1}'.format(self.testresc, self.testfile)
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iadmin cu' # calculate, update db
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iquota'
                    self.admin.assert_icommand(cmd.split(), 'STDOUT_SINGLELINE', 'Nearing quota') # not over yet
                    cmd = 'iadmin {0} {1} {2} 40'.format(quotatype[0], quotatype[1], quotaresc) # set low quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iquota'
                    self.admin.assert_icommand(cmd.split(), 'STDOUT_SINGLELINE', 'OVER QUOTA') # confirm it's over
                    lib.make_file(myfile, 30, contents='arbitrary')
                    cmd = 'iput -R {0} {1}'.format(self.testresc, myfile) # should fail
                    self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'SYS_RESC_QUOTA_EXCEEDED')
                    cmd = 'iadmin {0} {1} {2} 0'.format(quotatype[0], quotatype[1], quotaresc) # remove quota
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iadmin cu' # update db
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'iput -R {0} {1}'.format(self.testresc, myfile) # should succeed again
                    self.admin.assert_icommand(cmd.split())
                    cmd = 'irm -rf {0}'.format(myfile) # clean up
                    self.admin.assert_icommand(cmd.split())
            time.sleep(2)  # remove once file hash fix is commited #2279

    def test_iquota_empty__3048(self):
        cmd = 'iadmin suq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing username parameter') # usage information
        cmd = 'iadmin sgq' # no arguments
        self.admin.assert_icommand(cmd.split(), 'STDERR_SINGLELINE', 'ERROR: missing group name parameter') # usage information

