from __future__ import print_function

import copy
import json
import logging
import os
import pprint
import tempfile
import textwrap
import time
import unittest

from . import resource_suite
from . import session
from . import settings
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file
from ..test.command import assert_command


class Test_Auth(resource_suite.ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        cfg = lib.open_and_load_json(os.path.join(IrodsConfig().irods_directory, 'test', 'test_framework_configuration.json'))
        auth_user = cfg['irods_authuser_name']
        auth_pass = cfg['irods_authuser_password']

        # Requires existence of OS account 'irodsauthuser' with password ';=iamnotasecret'
        try:
            import pwd
            pwd.getpwnam(auth_user)

        except KeyError:
            # This is a requirement in order to run these tests and running the tests is required for our test suite, so
            # we always error here when the prerequisites are not being met on the test-running host.
            raise EnvironmentError(
                'OS user "{}" with password "{}" must exist in order to run these tests.'.format(auth_user, auth_pass))

        super(Test_Auth, self).setUp()

        self.auth_session = session.mkuser_and_return_session('rodsuser', auth_user, auth_pass, lib.get_hostname())

    def tearDown(self):
        self.auth_session.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', self.auth_session.username])
        super(Test_Auth, self).tearDown()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or test.settings.USE_SSL, 'Topo from resource or SSL')
    def test_authentication_PAM_without_negotiation(self):
        numbits = 2048
        irods_config = IrodsConfig()
        server_key_path = os.path.join(irods_config.irods_directory, 'test', 'server.key')
        chain_pem_path = os.path.join(irods_config.irods_directory, 'test', 'chain.pem')
        dhparams_pem_path = os.path.join(irods_config.irods_directory, 'test', 'dhparams.pem')
        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, str(numbits)])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-x509', '-key', server_key_path, '-out', chain_pem_path, '-days', '365'])
        lib.execute_command(['openssl', 'dhparam', '-2', '-out', dhparams_pem_path, str(numbits)])

        IrodsController().stop()

        service_account_environment_file_path = os.path.join(os.path.expanduser('~'), '.irods', 'irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            server_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_ca_certificate_file': chain_pem_path,
                'irods_ssl_verify_server': 'none',
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_ca_certificate_file': chain_pem_path,
                'irods_ssl_verify_server': 'none',
                'irods_authentication_scheme': 'pam_password',
            }

            # now the actual test
            auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
            self.auth_session.environment_file_contents.update(client_update)

            # server reboot to pick up new irodsEnv settings
            IrodsController().start()

            # do the reauth
            self.auth_session.assert_icommand('iinit', 'STDOUT', 'Enter your current PAM password',
                                              input=f'{self.auth_session.password}\n')
            # connect and list some files
            self.auth_session.assert_icommand('icd')
            self.auth_session.assert_icommand('ils -L', 'STDOUT_SINGLELINE', 'home')

            # reset client environment to original
            self.auth_session.environment_file_contents = auth_session_env_backup

            # clean up
            for filename in [chain_pem_path, server_key_path, dhparams_pem_path]:
                os.unlink(filename)

        # server reboot to pick up new irodsEnv and server settings
        IrodsController().restart()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or test.settings.USE_SSL, 'Topo from resource or SSL')
    def test_authentication_PAM_with_server_params(self):
        irods_config = IrodsConfig()
        numbits = 2048
        server_key_path = os.path.join(irods_config.irods_directory, 'test', 'server.key')
        chain_pem_path = os.path.join(irods_config.irods_directory, 'test', 'chain.pem')
        dhparams_pem_path = os.path.join(irods_config.irods_directory, 'test', 'dhparams.pem')
        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, str(numbits)])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-x509', '-key', server_key_path, '-out', chain_pem_path, '-days', '365'])
        lib.execute_command(['openssl', 'dhparam', '-2', '-out', dhparams_pem_path, str(numbits)])

        IrodsController().stop()

        service_account_environment_file_path = os.path.join(os.path.expanduser('~'), '.irods', 'irods_environment.json')
        auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
        try:
            with lib.file_backed_up(service_account_environment_file_path):
                irods_config = IrodsConfig()
                server_update = {
                    'irods_client_server_policy': 'CS_NEG_REQUIRE',
                    'irods_ssl_certificate_chain_file': chain_pem_path,
                    'irods_ssl_certificate_key_file': server_key_path,
                    'irods_ssl_dh_params_file': dhparams_pem_path,
                    'irods_ssl_ca_certificate_file': chain_pem_path,
                    'irods_ssl_verify_server': 'none',
                }
                lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

                client_update = {
                    'irods_ssl_certificate_chain_file': chain_pem_path,
                    'irods_ssl_certificate_key_file': server_key_path,
                    'irods_ssl_dh_params_file': dhparams_pem_path,
                    'irods_ssl_ca_certificate_file': chain_pem_path,
                    'irods_ssl_verify_server': 'none',
                    'irods_authentication_scheme': 'pam_password',
                    'irods_client_server_policy': 'CS_NEG_REQUIRE',
                }

                self.auth_session.environment_file_contents.update(client_update)

                with lib.file_backed_up(irods_config.server_config_path):
                    server_config_update = {
                        'authentication' : {
                            'pam_password' : {
                                'password_length': 20,
                                'no_extend': False,
                                'password_min_time': 121,
                                'password_max_time': 1209600
                            }
                        }
                    }
                    lib.update_json_file_from_dict(irods_config.server_config_path, server_config_update)

                    pep_map = {
                        'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                            acPreConnect(*OUT) {
                                *OUT = 'CS_NEG_REQUIRE';
                            }
                        '''),
                        'irods_rule_engine_plugin-python': textwrap.dedent('''
                            def acPreConnect(rule_args, callback, rei):
                                rule_args[0] = 'CS_NEG_REQUIRE'
                        ''')
                    }

                    with temporary_core_file() as core:
                        core.add_rule(pep_map[self.plugin_name])

                        IrodsController().start()

                        # the test
                        print(f'running iinit for PAM user [{self.auth_session.username}] [{self.auth_session.password}]')
                        self.auth_session.assert_icommand('iinit', 'STDOUT', 'Enter your current PAM password',
                                                          input=f'{self.auth_session.password}\n')
                        self.auth_session.assert_icommand("icd")
                        self.auth_session.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "home")
        finally:
            self.auth_session.environment_file_contents = auth_session_env_backup
            irods_config = IrodsConfig()
            for filename in [chain_pem_path, server_key_path, dhparams_pem_path]:
                if os.path.exists(filename):
                    os.unlink(filename)

            IrodsController().restart()

    def test_iinit_repaving_2646(self):
        l = logging.getLogger(__name__)
        initial_contents = copy.deepcopy(self.admin.environment_file_contents)
        del self.admin.environment_file_contents['irods_zone_name']
        self.admin.run_icommand('iinit', input='{0}\n{1}\n'.format(initial_contents['irods_zone_name'], self.admin.password))
        final_contents = lib.open_and_load_json(os.path.join(self.admin.local_session_dir, 'irods_environment.json'))
        self.admin.environment_file_contents = initial_contents
        for k in initial_contents.keys():
            if k not in final_contents.keys() or initial_contents[k] != final_contents[k]:
                l.debug('Discrepancy on key "%s"\ninitial: %s\nfinal: %s', k,
                        pprint.pformat(initial_contents[k]) if k in initial_contents else "Not Present",
                        pprint.pformat(final_contents[k]) if k in final_contents else "Not Present")
        l.debug("initial contents:\n%s", initial_contents)
        l.debug("final contents:\n%s", final_contents)
        assert initial_contents == final_contents

    def test_denylist(self):
        IrodsController().stop()
        with lib.file_backed_up(paths.server_config_path()):
            server_config_update = {
                'controlled_user_connection_list' : {
                    'control_type' : 'denylist',
                    'users' : [
                        'user1#tempZone',
                        'user2#otherZone',
                        '#'.join([self.user_sessions[0].username, self.user_sessions[0].zone_name]),
                        'user4#otherZone'
                    ]
                }
            }
            lib.update_json_file_from_dict(paths.server_config_path(), server_config_update)

            IrodsController().start()
            self.user_sessions[0].assert_icommand( 'ils', 'STDERR_SINGLELINE', 'SYS_USER_NOT_ALLOWED_TO_CONN' )
            self.user_sessions[1].assert_icommand( 'ils', 'STDOUT_SINGLELINE', '/home' )
        IrodsController().restart()

    def test_allowlist(self):
        IrodsController().stop()
        with lib.file_backed_up(paths.server_config_path()):
            service_account = IrodsConfig().client_environment
            server_config_update = {
                'controlled_user_connection_list' : {
                    'control_type' : 'allowlist',
                    'users' : [
                        'user1#tempZone',
                        'user2#otherZone',
                        '#'.join([self.user_sessions[0].username, self.user_sessions[0].zone_name]),
                        '#'.join([service_account['irods_user_name'], service_account['irods_zone_name']]),
                        'user4#otherZone'
                    ]
                }
            }
            lib.update_json_file_from_dict(paths.server_config_path(), server_config_update)

            IrodsController().start()
            self.user_sessions[0].assert_icommand( 'ils', 'STDOUT_SINGLELINE', '/home' )
            self.user_sessions[1].assert_icommand( 'ils', 'STDERR_SINGLELINE', 'SYS_USER_NOT_ALLOWED_TO_CONN' )
        IrodsController().restart()


class test_iinit(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(test_iinit, self).setUp()

        self.admin = self.admin_sessions[0]

        # Set up the basic client environment dict.
        self.host = lib.get_hostname()
        self.port = '1247'
        self.user = 'test_iinit_basic_user'
        self.zone = self.admin.zone_name
        self.password = 'password123'

        self.base_client_environment = {
            "irods_host": self.host,
            "irods_port": int(self.port),
            "irods_user_name": self.user,
            "irods_zone_name": self.zone
        }

        # Set up the client SSL environment.
        # TODO: Can this be derived somehow?
        directory_for_ssl_files = '/etc/irods'
        self.server_key_path = os.path.join(directory_for_ssl_files, 'server.key')
        self.server_crt_path = os.path.join(directory_for_ssl_files, 'server.crt')
        self.chain_pem_path = os.path.join(directory_for_ssl_files, 'chain.pem')
        self.dhparams_pem_path = os.path.join(directory_for_ssl_files, 'dhparams.pem')

        self.client_server_negotiation = 'request_server_negotiation'
        self.ssl_verify_server = 'cert'
        self.client_server_policy = 'CS_NEG_REQUIRE'
        self.ssl_ca_certificate_file = self.server_crt_path
        self.ssl_certificate_chain_file = self.chain_pem_path
        self.ssl_certificate_key_file = self.server_key_path
        self.ssl_dh_params_file = self.dhparams_pem_path
        self.encryption_algorithm = 'AES-256-CBC'
        self.encryption_key_size = '32'
        self.encryption_salt_size = '8'
        self.encryption_num_hash_rounds = '16'

        self.ssl_client_environment = {
            'irods_client_server_negotiation': self.client_server_negotiation,
            "irods_ssl_verify_server": self.ssl_verify_server,
            'irods_client_server_policy': self.client_server_policy,
            'irods_ssl_ca_certificate_file': self.ssl_ca_certificate_file,
            'irods_ssl_certificate_chain_file': self.ssl_certificate_chain_file,
            'irods_ssl_certificate_key_file': self.ssl_certificate_key_file,
            'irods_ssl_dh_params_file': self.ssl_dh_params_file,
            'irods_encryption_algorithm': self.encryption_algorithm,
            'irods_encryption_key_size': int(self.encryption_key_size),
            'irods_encryption_salt_size': int(self.encryption_salt_size),
            'irods_encryption_num_hash_rounds': int(self.encryption_num_hash_rounds)
        }

        # Set up the native authentication user.
        self.admin.assert_icommand(['iadmin', 'mkuser', self.user, 'rodsuser'])
        self.admin.assert_icommand(['iadmin', 'moduser', self.user, 'password', self.password])

        # Create the irodsauthuser for any pam_password tests. No password needed.
        cfg = lib.open_and_load_json(os.path.join(IrodsConfig().irods_directory, 'test', 'test_framework_configuration.json'))
        self.pam_user = cfg['irods_authuser_name']
        self.pam_password = cfg['irods_authuser_password']
        self.admin.assert_icommand(['iadmin', 'mkuser', self.pam_user, 'rodsuser'])

        # Set up the alternative location for the environment file and the authentication file.
        self.session_dir = tempfile.mkdtemp()
        self.environment_file_path = os.path.join(self.session_dir, 'irods_environment.json')
        self.authentication_file_path = os.path.join(self.session_dir, '.irodsA')

        self.env = os.environ.copy()
        self.env['IRODS_ENVIRONMENT_FILE'] = self.environment_file_path
        self.env['IRODS_AUTHENTICATION_FILE'] = self.authentication_file_path


    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmuser', self.user])
        self.admin.assert_icommand(['iadmin', 'rmuser', self.pam_user])

        super(test_iinit, self).tearDown()


    # 'd | other' was not added until python 3.9 -- need to manually merge dictionaries for all platforms
    @staticmethod
    def merge_dicts(d, other, sort=False):
        to_return = d.copy()

        for k in other:
            to_return[k] = other[k]

        return {k: to_return[k] for k in sorted(to_return)} if sort else to_return


    def assert_basic_iinit_prompts_are_in_stdout(self, stdout):
        self.assertIn('Enter the host name (DNS) of the server to connect to', stdout)
        self.assertIn('Enter the port number', stdout)
        self.assertIn('Enter your iRODS user name', stdout)
        self.assertIn('Enter your iRODS zone', stdout)


    def assert_ssl_iinit_prompts_are_in_stdout(self, stdout):
        self.assertIn('Enter the server verification level', stdout)
        self.assertIn('Enter the full path to the CA certificate file', stdout)
        self.assertIn('Enter the full path to the certificate key file', stdout)
        self.assertIn('Enter the full path to the certificate chain file', stdout)
        self.assertIn('Enter the full path to the DH parameters file', stdout)
        self.assertIn('Enter the encryption algorithm', stdout)
        self.assertIn('Enter the encryption key size', stdout)
        self.assertIn('Enter the encryption salt size', stdout)
        self.assertIn('Enter the number of hash rounds', stdout)


    def assert_auth_scheme_iinit_prompts_are_in_stdout(self, stdout):
        self.assertIn('Enter your iRODS authentication scheme', stdout)


    def assert_environment_file_contents(self, expected_json_contents=None, environment_file_path=None):
        if expected_json_contents is None:
            expected_json_contents = self.base_client_environment

        if environment_file_path is None:
            environment_file_path = self.environment_file_path

        self.assertTrue(os.path.exists(environment_file_path))
        with open(environment_file_path, 'r') as f:
            environment_file_contents = json.load(f)

        self.assertEqual(environment_file_contents, expected_json_contents)


    @unittest.skipIf(test.settings.USE_SSL, "Basic iinit usage does not configure SSL.")
    def test_iinit_basic_success(self):
        # Execute iinit with the basic prompts only, with only correct answers.
        cmd = 'iinit'
        user_input = os.linesep.join(
            [self.host, self.port, self.user, self.zone, self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program terminated gracefully.
        self.assertEqual(rc, 0)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents()

        # Make sure that the authentication succeeded.
        assert_command('ils', 'STDOUT', f'/{self.zone}/home/{self.user}', env=self.env)


    @unittest.skipIf(test.settings.USE_SSL, "Basic iinit usage does not configure SSL.")
    def test_iinit_basic_authentication_failure(self):
        # Execute iinit with the basic prompts only, with an incorrect password.
        cmd = 'iinit'
        user_input = os.linesep.join(
            [self.host, self.port, self.user, self.zone, 'nopes']) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program did not terminate gracefully.
        self.assertNotEqual(rc, 0)

        # Check for expected error string.
        error_string = 'CAT_INVALID_AUTHENTICATION'
        self.assertIn(error_string, stderr)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)

        # Make sure that the environment file saved despite authentication failure.
        self.assert_environment_file_contents()

        # Make sure that the authentication failed.
        assert_command('ils', 'STDERR', error_string, env=self.env)


    @unittest.skipIf(test.settings.USE_SSL, "This test does not configure SSL for the authenticated user.")
    def test_iinit_authscheme_success_with_default_answers(self):
        # Execute iinit with the auth scheme prompt, with default answers where possible.
        cmd = 'iinit --prompt-auth-scheme'
        user_input = os.linesep.join([
            self.host,
            '', # port
            self.user,
            self.zone,
            '', # authentication scheme
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program terminated gracefully.
        self.assertEqual(rc, 0)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_auth_scheme_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(
                self.base_client_environment, {'irods_authentication_scheme': 'native'}))

        # Make sure that the authentication succeeded.
        assert_command('ils', 'STDOUT', f'/{self.zone}/home/{self.user}', env=self.env)


    @unittest.skipIf(test.settings.USE_SSL, "This test does not configure SSL for the authenticated user.")
    def test_iinit_authscheme_failure_with_invalid_scheme(self):
        # Execute iinit with the auth scheme prompt and provide an invalid scheme.
        auth_scheme = 'nonexistent_auth_scheme'
        cmd = 'iinit --prompt-auth-scheme'
        user_input = os.linesep.join([
            self.host,
            self.port,
            self.user,
            self.zone,
            auth_scheme,
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program did not terminate gracefully.
        self.assertNotEqual(rc, 0)

        # Check for expected error string.
        error_string = 'PLUGIN_ERROR_MISSING_SHARED_OBJECT'
        self.assertIn(error_string, stderr)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_auth_scheme_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved despite authentication failure.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(
                self.base_client_environment, {'irods_authentication_scheme': auth_scheme}))

        # Make sure that the authentication failed.
        assert_command('ils', 'STDERR', error_string, env=self.env)


    @unittest.skipUnless(test.settings.USE_SSL, "This tests configuring an SSL environment, so server must use SSL.")
    def test_iinit_ssl_success_with_no_default_answers(self):
        # Execute iinit with the basic and SSL prompts, with only correct answers.
        cmd = 'iinit --with-ssl'
        user_input = os.linesep.join([
            self.host,
            self.port,
            self.user,
            self.zone,
            self.ssl_verify_server,
            self.ssl_ca_certificate_file,
            self.ssl_certificate_key_file,
            self.ssl_certificate_chain_file,
            self.ssl_dh_params_file,
            self.encryption_algorithm,
            self.encryption_key_size,
            self.encryption_salt_size,
            self.encryption_num_hash_rounds,
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program terminated gracefully.
        self.assertEqual(rc, 0)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Make sure that the authentication succeeded.
        assert_command('ils', 'STDOUT', f'/{self.zone}/home/{self.user}', env=self.env)


    @unittest.skipUnless(test.settings.USE_SSL, "This tests configuring an SSL environment, so server must use SSL.")
    def test_iinit_ssl_success_with_all_default_answers(self):
        # Execute iinit with the basic and SSL prompts, with default answers where possible.
        cmd = 'iinit --with-ssl'
        user_input = os.linesep.join([
            self.host,
            '', # port
            self.user,
            self.zone,
            self.ssl_verify_server, # We cannot use the default ("hostname") here because the server uses "cert".
            self.ssl_ca_certificate_file,
            self.ssl_certificate_key_file,
            self.ssl_certificate_chain_file,
            self.ssl_dh_params_file,
            '', # encryption algorithm
            '', # key size
            '', # salt size
            '', # num hash rounds
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program terminated gracefully.
        self.assertEqual(rc, 0)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Make sure that the authentication succeeded.
        assert_command('ils', 'STDOUT', f'/{self.zone}/home/{self.user}', env=self.env)


    @unittest.skipIf(test.settings.USE_SSL, "This tests configuring an SSL environment with no SSL in the server.")
    def test_iinit_ssl_failure_with_no_default_answers(self):
        # Execute iinit with the basic and SSL prompts, with only correct answers. Authentication should fail anyway
        # because the server should not have SSL configured.
        cmd = 'iinit --with-ssl'
        user_input = os.linesep.join([
            self.host,
            self.port,
            self.user,
            self.zone,
            self.ssl_verify_server,
            self.ssl_ca_certificate_file,
            self.ssl_certificate_key_file,
            self.ssl_certificate_chain_file,
            self.ssl_dh_params_file,
            self.encryption_algorithm,
            self.encryption_key_size,
            self.encryption_salt_size,
            self.encryption_num_hash_rounds,
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program did not terminate gracefully.
        self.assertNotEqual(rc, 0)

        # Check for expected error string.
        error_string = 'CLIENT_NEGOTIATION_ERROR'
        self.assertIn(error_string, stderr)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Make sure that the authentication failed.
        assert_command('ils', 'STDERR', error_string, env=self.env)


    @unittest.skipIf(test.settings.USE_SSL, "This tests configuring an SSL environment with no SSL in the server.")
    def test_iinit_ssl_failure_with_all_default_answers(self):
        # Execute iinit with the basic and SSL prompts, with default answers where possible. Authentication should fail
        # anyway because the server should not have SSL configured.
        cmd = 'iinit --with-ssl'
        user_input = os.linesep.join([
            self.host,
            '', # port
            self.user,
            self.zone,
            self.ssl_verify_server, # We cannot use the default ("hostname") here because the server uses "cert".
            self.ssl_ca_certificate_file,
            self.ssl_certificate_key_file,
            self.ssl_certificate_chain_file,
            self.ssl_dh_params_file,
            '', # encryption algorithm
            '', # key size
            '', # salt size
            '', # num hash rounds
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program did not terminate gracefully.
        self.assertNotEqual(rc, 0)

        # Check for expected error string.
        error_string = 'CLIENT_NEGOTIATION_ERROR'
        self.assertIn(error_string, stderr)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Make sure that the authentication failed.
        assert_command('ils', 'STDERR', error_string, env=self.env)

        # Now, do the iinit again, but we should only get prompts for the SSL parameters. All prompts should have
        # default values now which are being pulled from the environment file which was saved.
        user_input = os.linesep.join([
            '', # We can use the default here because previously entered value should be saved
            '', # CA certificate file
            '', # certificate key file
            '', # certificate chain file
            '', # DH params file
            '', # encryption algorithm
            '', # key size
            '', # salt size
            '', # num hash rounds
            self.password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program did not terminate gracefully.
        self.assertNotEqual(rc, 0)

        # Check for expected error string.
        error_string = 'CLIENT_NEGOTIATION_ERROR'
        self.assertIn(error_string, stderr)

        # The basic iinit prompts should not be shown again here. Just check the first one.
        self.assertNotIn('Enter the host name (DNS) of the server to connect to', stdout)
        self.assertIn('Enter your current iRODS password', stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        self.assert_environment_file_contents(
            expected_json_contents = self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Make sure that the authentication failed.
        assert_command('ils', 'STDERR', error_string, env=self.env)


    @unittest.skipUnless(test.settings.USE_SSL, "This tests using pam_password which requires server-side SSL.")
    def test_iinit_with_pam_password_and_ssl(self):
        # Requires existence of OS account 'irodsauthuser' with password ';=iamnotasecret'
        try:
            import pwd
            pwd.getpwnam(self.pam_user)

        except KeyError:
            # This is a requirement in order to run these tests and running the tests is required for our test suite, so
            # we always fail here when the prerequisites are not being met on the test-running host.
            self.fail('OS user "[{}]" with password "[{}]" must exist in order to run these tests.'.format(
                self.pam_user, self.pam_password))

        auth_scheme = 'pam_password'
        cmd = 'iinit --with-ssl --prompt-auth-scheme'
        user_input = os.linesep.join([
            self.host,
            self.port,
            self.pam_user,
            self.zone,
            auth_scheme,
            self.ssl_verify_server,
            self.ssl_ca_certificate_file,
            self.ssl_certificate_key_file,
            self.ssl_certificate_chain_file,
            self.ssl_dh_params_file,
            self.encryption_algorithm,
            self.encryption_key_size,
            self.encryption_salt_size,
            self.encryption_num_hash_rounds,
            self.pam_password]) + os.linesep
        stdout, stderr, rc = lib.execute_command_permissive(cmd, input=user_input, env=self.env)
        lib.log_command_result(cmd, stdout, stderr, rc)

        # Ensure program terminated gracefully.
        self.assertEqual(rc, 0)

        self.assert_basic_iinit_prompts_are_in_stdout(stdout)
        self.assertIn('Enter your current PAM password', stdout)
        self.assert_auth_scheme_iinit_prompts_are_in_stdout(stdout)
        self.assert_ssl_iinit_prompts_are_in_stdout(stdout)

        # Make sure that the environment file saved with the correct contents.
        expected_environment = self.merge_dicts(
            {'irods_authentication_scheme': auth_scheme},
            self.merge_dicts(self.base_client_environment, self.ssl_client_environment))

        # Update username to expected value.
        expected_environment['irods_user_name'] = self.pam_user

        self.assert_environment_file_contents(expected_json_contents = expected_environment)

        # Make sure that the authentication succeeded.
        assert_command('ils', 'STDOUT', f'/{self.zone}/home/{self.pam_user}', env=self.env)

