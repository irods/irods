import signal
import time
import unittest

from . import session
from .. import lib
from .. import test

admins = []
users  = [('alice', 'apass')]

class Test_Ips(session.make_sessions_mixin(admins, users), unittest.TestCase):

    def setUp(self):
        super(Test_Ips, self).setUp()
        self.user = self.user_sessions[0]


    def tearDown(self):
        super(Test_Ips, self).tearDown()


    def test_server_replaces_spaces_with_hyphens_in_client_names_reported_by_ips__issue_8733(self):
        # Launch a client that opens a connection to the server. This client will
        # not terminate until it receives the SIGINT signal. Notice the spaces in the name.
        proc = lib.execute_command_nonblocking(
            ['irods_test_issue_8733', 'Cyberduck/9.3.0.44005 (Windows 10/10.0.19045.0) (amd64)'])

        try:
            # Give the computer a little time for the process to fully launch.
            time.sleep(2)

            # Show that server was able to replace spaces in the name reported to the
            # server by the "irods_test_issue_8733" client. The "-a" flag is required
            # when running this test from a consumer in a topology environment.
            expected_output = ['  Cyberduck/9.3.0.44005-(Windows-10/10.0.19045.0)-(amd64)  ']
            self.user.assert_icommand(['ips', '-a'], 'STDOUT', expected_output)

            # Confirm the output is what we expect when "-a" isn't passed. This applies
            # to runs where the test is launched from a provider.
            if not test.settings.TOPOLOGY_FROM_RESOURCE_SERVER:
                self.user.assert_icommand(['ips'], 'STDOUT', expected_output)

            # Instruct the client to exit cleanly.
            proc.send_signal(signal.SIGINT)
            proc.wait(timeout=10)

        finally:
            proc.kill();
            proc.wait();
