from __future__ import print_function
import copy
import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import paths
from .. import lib
from ..configuration import IrodsConfig

class Test_SSL(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_SSL, self).setUp()

    def tearDown(self):
        super(Test_SSL, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_icommands_segfault_when_ssl_cert_hostname_not_matching__issue_3609(self):
        self.init_properties()
        self.create_ssl_files()

        with lib.file_backed_up(self.config.server_config_path):
            with lib.file_backed_up(self.config.client_environment_path):
                session_env_backup = copy.deepcopy(self.admin.environment_file_contents)

                self.enable_server_ssl()
                self.enable_client_ssl_verify_cert()
                self.admin.assert_icommand(['ils'], 'STDOUT', use_regex=True, expected_results='.*tempZone.*')

                self.enable_client_ssl_verify_hostname()
                self.admin.assert_icommand(['ils'], 'STDERR', use_regex=True, expected_results='.*-2105000 SSL_CERT_ERROR.*')

                self.enable_client_ssl_verify_cert()
                self.admin.assert_icommand(['ils'], 'STDOUT', use_regex=True, expected_results='.*tempZone.*')

                self.admin.environment_file_contents = session_env_backup

                self.remove_files()

    # NOTE: The methods below assume use of the native rule language rule engine plugin
    # Please skip any additional tests using these methods unless the native rule language REP is in use by default
    def init_properties(self):
        self.admin = self.admin_sessions[0]

        self.config = IrodsConfig()

        self.rule_filename = 'rule_issue_3609.re'
        self.rule_filename_path = os.path.join(paths.core_re_directory(), self.rule_filename)

        self.ssl_key_path = os.path.join(self.admin.local_session_dir, 'server.key')
        self.ssl_crt_path = os.path.join(self.admin.local_session_dir, 'server.crt')
        self.dhparams_pem_path = os.path.join(self.admin.local_session_dir, 'dhparams.pem')

    def create_ssl_files(self):
        lib.execute_command(['openssl', 'genrsa', '-out', self.ssl_key_path, '2048'])
        lib.execute_command('openssl req -batch -new -x509 -key %s -out %s -days 365' % (self.ssl_key_path, self.ssl_crt_path))
        lib.execute_command('openssl dhparam -2 -out %s 2048' % (self.dhparams_pem_path))

    def remove_files(self):
        os.remove(self.ssl_key_path)
        os.remove(self.ssl_crt_path)
        os.remove(self.dhparams_pem_path)
        os.remove(self.rule_filename_path)

    def add_ssl_plugin_rule(self):
        # Create new rule.
        with open(self.rule_filename_path, 'w') as f:
            f.write("acPreConnect(*OUT) { *OUT = 'CS_NEG_REQUIRE'; }\n")

        # Update rule engine.
        rule_engine = self.config.server_config['plugin_configuration']['rule_engines'][0]
        rule_engine['plugin_specific_configuration']['re_rulebase_set'].insert(0, self.rule_filename[:-3])
        lib.update_json_file_from_dict(self.config.server_config_path, self.config.server_config)

    def enable_server_ssl(self):
        self.add_ssl_plugin_rule()

        env_path = self.config.client_environment_path

        lib.update_json_file_from_dict(env_path, {'irods_client_server_policy': 'CS_NEG_REQUIRE',
                                                  'irods_ssl_certificate_chain_file': self.ssl_crt_path,
                                                  'irods_ssl_certificate_key_file': self.ssl_key_path,
                                                  'irods_ssl_dh_params_file': self.dhparams_pem_path,
                                                  'irods_ssl_ca_certificate_file': self.ssl_crt_path,
                                                  'irods_ssl_verify_server': 'cert'})

    def enable_client_ssl_verify_cert(self):
        self.admin.environment_file_contents.update({'irods_client_server_policy': 'CS_NEG_REQUIRE',
                                                     'irods_ssl_certificate_chain_file': self.ssl_crt_path,
                                                     'irods_ssl_certificate_key_file': self.ssl_key_path,
                                                     'irods_ssl_dh_params_file': self.dhparams_pem_path,
                                                     'irods_ssl_ca_certificate_file': self.ssl_crt_path,
                                                     'irods_ssl_verify_server': 'cert'})

    def enable_client_ssl_verify_hostname(self):
        self.admin.environment_file_contents.update({'irods_client_server_policy': 'CS_NEG_REQUIRE',
                                                     'irods_ssl_certificate_chain_file': self.ssl_crt_path,
                                                     'irods_ssl_certificate_key_file': self.ssl_key_path,
                                                     'irods_ssl_dh_params_file': self.dhparams_pem_path,
                                                     'irods_ssl_ca_certificate_file': self.ssl_crt_path,
                                                     'irods_ssl_verify_server': 'hostname'})

