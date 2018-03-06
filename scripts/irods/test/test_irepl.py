from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os

from . import session
from . import settings
from . import resource_suite

class Test_IRepl(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_IRepl, self).setUp()

    def tearDown(self):
        super(Test_IRepl, self).tearDown()

    def test_irepl_with_U_and_R_options__issue_3659(self):
        self.admin.assert_icommand(['irepl', 'dummyFile', '-U', '-R', 'demoResc'],
                                    'STDERR_SINGLELINE',
                                    'ERROR: irepl: -R and -U cannot be used together')
