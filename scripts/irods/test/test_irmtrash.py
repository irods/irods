from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib

admins = [('otherrods', 'rods')]
users  = []

class Test_Irmtrash(session.make_sessions_mixin(admins, users), unittest.TestCase):

    def setUp(self):
        super(Test_Irmtrash, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Irmtrash, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irmtrash_does_not_delete_registered_data_objects__issue_4403(self):
        # Register a file into iRODS.
        filename = 'foo'
        filepath = os.path.join(self.admin.local_session_dir, filename)
        lib.make_file(filepath, 1024, 'arbitrary')
        data_object = os.path.join(self.admin.session_collection, filename)
        self.admin.assert_icommand(['ireg', filepath, data_object])

        # Move it to the trash.
        self.admin.assert_icommand(['irm', data_object])

        # Show that irmtrash unregisters data objects instead of deleting them
        # if the data object is not in a vault.
        self.admin.assert_icommand(['irmtrash'])
        self.assertTrue(os.path.exists(filepath))

