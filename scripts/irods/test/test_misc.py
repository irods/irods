import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

class Test_Misc(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Misc, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Misc, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_invalid_client_irodsProt_handled_cleanly__issue_4130(self):
        env = os.environ.copy()
        env['irodsProt'] = '2'
        out, err, ec = lib.execute_command_permissive(['ils'], env=env)
        self.assertTrue('Invalid protocol value.' in err)

