from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from . import settings
from . import resource_suite

class Test_Ipasswd(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ipasswd, self).setUp()

    def tearDown(self):
        super(Test_Ipasswd, self).tearDown()

    def test_ipasswd(self):
        self.user_sessions[0].assert_icommand(['ipasswd', 'new_password'])

