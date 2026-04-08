import json
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..controller import IrodsController
from ..exceptions import IrodsError

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

        if value is None:
            del svr_cfg['negotiation_key']
        else:
            svr_cfg['negotiation_key'] = value

        new_server_config = json.dumps(svr_cfg,
                                       sort_keys=True,
                                       indent=4,
                                       separators=(',', ': '))

        with lib.file_backed_up(paths.server_config_path()):
            with open(paths.server_config_path(), 'w') as f:
                f.write(new_server_config)

            # Attempting to reload the configuration will result in an exception being
            # raised because the server report that the configuration failed validation.
            # This will result in the server continuing to use the existing configuration.
            with self.assertRaises(IrodsError):
                IrodsController().reload_configuration()

            # Show that the server is still responsive.
            self.admin.assert_icommand(['ils'], 'STDOUT', [self.admin.session_collection])


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


class test_server_authentication__issue_2295(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())

        self.provider_resc = "provider_resc"
        lib.create_ufs_resource(self.admin, self.provider_resc, hostname=test.settings.ICAT_HOSTNAME)
        self.consumer1_resc = "consumer1_resc"
        lib.create_ufs_resource(self.admin, self.consumer1_resc, hostname=test.settings.HOSTNAME_1)
        self.consumer2_resc = "consumer2_resc"
        lib.create_ufs_resource(self.admin, self.consumer2_resc, hostname=test.settings.HOSTNAME_2)

    @classmethod
    def tearDownClass(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand(['iadmin', 'rmresc', self.provider_resc])
            admin_session.run_icommand(['iadmin', 'rmresc', self.consumer1_resc])
            admin_session.run_icommand(['iadmin', 'rmresc', self.consumer2_resc])

            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def do_signed_zone_key_mismatch_test_on_consumer(self, configuration_key, configuration_value):
        try:
            with open(paths.server_config_path()) as f:
                svr_cfg = json.load(f)

            svr_cfg[configuration_key] = configuration_value

            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

            with lib.file_backed_up(paths.server_config_path()):
                with open(paths.server_config_path(), 'w') as f:
                    f.write(new_server_config)

                # Show that the server is still responsive.
                self.admin.assert_icommand(['ils'], 'STDOUT', [self.admin.session_collection])

                # Attempting to reload the configuration will succeed, but reaching the provider will fail.
                IrodsController().reload_configuration()

                # Show that the catalog provider can no longer be reached.
                self.admin.assert_icommand(['ils'], 'STDERR', '-147000 ZONE_KEY_SIGNATURE_MISMATCH')

        finally:
            # Reload configuration since configuration has been restored.
            IrodsController().reload_configuration()

            # Show that the server is still responsive.
            self.admin.assert_icommand(['ils'], 'STDOUT', [self.admin.session_collection])

    @unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER,
                         'expected behavior manifests only in server-to-server communications')
    def test_zone_keys_mismatch_on_consumer_causes_failure_to_connect_to_provider(self):
        self.do_signed_zone_key_mismatch_test_on_consumer("zone_key", "TEMPORARY______KEY")

    @unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER,
                         'expected behavior manifests only in server-to-server communications')
    def test_negotiation_keys_mismatch_on_consumer_causes_failure_to_connect_to_provider(self):
        self.do_signed_zone_key_mismatch_test_on_consumer("negotiation_key", "32_byte____________________key__")

    @unittest.skipUnless(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER,
                         'expected behavior manifests only in server-to-server communications')
    def test_hash_scheme_mismatch_on_consumer_causes_failure_to_connect_to_provider(self):
        self.do_signed_zone_key_mismatch_test_on_consumer("zone_key_signing_hash_scheme", "sha256")

    def do_signed_zone_key_mismatch_test_on_provider(self, configuration_key, configuration_value):
        logical_path = f"{self.admin.session_collection}/do_signed_zone_key_mismatch_test_on_provider"
        content = "content!"
        if test.settings.TOPOLOGY_FROM_RESOURCE_SERVER:
            expected_output = 'ZONE_KEY_SIGNATURE_MISMATCH'
        else:
            expected_output = 'Error: Cannot open data object.'
        try:
            with open(paths.server_config_path()) as f:
                svr_cfg = json.load(f)

            svr_cfg[configuration_key] = configuration_value

            new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

            with lib.file_backed_up(paths.server_config_path()):
                with open(paths.server_config_path(), 'w') as f:
                    f.write(new_server_config)

                # Attempting to reload the configuration will succeed, but reaching other servers will fail.
                IrodsController().reload_configuration()

                # Try to stream a file to a catalog consumer and fail because signed zone key should differ.
                self.admin.assert_icommand(
                    ['istream', "-R", self.consumer2_resc, "write", logical_path],
                    "STDERR", expected_output, input=content)

        finally:
            # Reload configuration since configuration has been restored.
            IrodsController().reload_configuration()

            # Remove the data object just in case it was created.
            self.admin.assert_icommand(['irm', "-f", logical_path])

    def test_zone_keys_mismatch_on_provider_causes_failure_to_connect_to_consumer(self):
        self.do_signed_zone_key_mismatch_test_on_provider("zone_key", "zone_key_mismatch")

    def test_negotiation_keys_mismatch_on_provider_causes_failure_to_connect_to_consumer(self):
        self.do_signed_zone_key_mismatch_test_on_provider("negotiation_key", "32_byte_negotiation_key_mismatch")

    def test_hash_scheme_mismatch_on_provider_causes_failure_to_connect_to_consumer(self):
        self.do_signed_zone_key_mismatch_test_on_provider("zone_key_signing_hash_scheme", "sha256")
