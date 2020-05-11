from __future__ import print_function
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib

class Test_istream(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_istream, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_istream, self).tearDown()

    def test_istream_empty_file(self):
        filename = 'test_istream_empty_file.txt'
        contents = '''don't look at what I'm doing'''
        lib.cat(filename, contents)
        try:
            logical_path = os.path.join(self.user.session_collection, filename)
            self.user.assert_icommand(['iput', filename])
            self.user.assert_icommand(['istream', 'read', filename], 'STDOUT', contents)
            self.admin.assert_icommand(['iadmin', 'modrepl', 'logical_path', logical_path, 'replica_number', '0', 'DATA_SIZE', '0'])
            # The catalog says the file is empty, so nothing should come back from a read
            self.user.assert_icommand(['istream', 'read', filename])
            self.user.assert_icommand(['irm', '-f', filename])
        finally:
            if os.path.exists(filename):
                os.unlink(filename)
