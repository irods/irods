import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import json

from . import session
from .. import lib
from .. import paths
from .. import test

@unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER,
                     'expected behavior manifests only in server-to-server communications')
class test_invalid_negotiation_keys(unittest.TestCase):
    """Test the negotiation_key setting in the iRODS server configuration."""
    @classmethod
    def setUpClass(self):
        """Set up the test class."""
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())


    @classmethod
    def tearDownClass(self):
        """Tear down the test class."""
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])


    def do_negotiation_key_test(self, value):
        """Perform the actions for the negotiation_key test using the specified value.

        Arguments:
        self -- this test class
        value -- the str value for negotiation_key (if None is provided, the key is deleted)
        """
        with open(paths.server_config_path()) as f:
            svr_cfg = json.load(f)

        if value is not None:
            svr_cfg['negotiation_key'] = ''
        else:
            del svr_cfg['negotiation_key']

        new_server_config = json.dumps(svr_cfg,
                                       sort_keys=True,
                                       indent=4,
                                       separators=(',', ': '))

        with lib.file_backed_up(paths.server_config_path()):
            with open(paths.server_config_path(), 'w') as f:
                f.write(new_server_config)

            # should result in a connection error, so look for "is probably down"
            # specific error codes may differ depending on the nature of the test
            self.admin.assert_icommand(['ils'], 'STDERR', 'is probably down')


    def test_negotiation_key_missing(self):
        """Test client-server negotiation when negotiation_key is missing."""
        self.do_negotiation_key_test(None)


    def test_negotiation_key_empty_string(self):
        """Test client-server negotiation when negotiation_key is an empty string."""
        self.do_negotiation_key_test('')


    def test_negotiation_key_too_short(self):
        """Test client-server negotiation when negotiation_key is a 31 character string."""
        self.do_negotiation_key_test('31_byte_server_negotiation_key_')


    def test_negotiation_key_too_long(self):
        """Test client-server negotiation when negotiation_key is a 33 character string."""
        self.do_negotiation_key_test('33_byte_server_negotiation_key___')


    def test_negotiation_key_with_invalid_characters(self):
        """Test client-server negotiation when negotiation_key contains invalid characters."""
        self.do_negotiation_key_test('32_byte_server_negotiation_key!!')
