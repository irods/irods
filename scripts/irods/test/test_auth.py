from __future__ import print_function
import copy
import logging
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import pprint
import time
import json

from .. import test
from . import settings
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .. import paths
from . import resource_suite
from . import session


# Requires OS account 'irods' to have password 'temporarypasswordforci'
class Test_OSAuth_Only(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_OSAuth_Only, self).setUp()
        self.auth_session = session.mkuser_and_return_session('rodsuser', 'irods', 'temporarypasswordforci',
                                                          lib.get_hostname())

    def tearDown(self):
        self.auth_session.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', self.auth_session.username])
        super(Test_OSAuth_Only, self).tearDown()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_authentication_OSAuth(self):
        self.auth_session.environment_file_contents['irods_authentication_scheme'] = 'OSAuth'

        # setup the irods.key file necessary for OSAuth
        keyfile_path = os.path.join(IrodsConfig().config_directory, 'irods.key')
        with open(keyfile_path, 'wt') as f:
            f.write('gibberish\n')

        # do the reauth
        self.auth_session.assert_icommand('iexit full')
        self.auth_session.assert_icommand(['iinit', self.auth_session.password])
        # connect and list some files
        self.auth_session.assert_icommand('icd')
        self.auth_session.assert_icommand('ils -L', 'STDOUT_SINGLELINE', 'home')

        # reset client environment to original
        del self.auth_session.environment_file_contents['irods_authentication_scheme']
        # do the reauth
        self.auth_session.assert_icommand('iexit full')
        self.auth_session.assert_icommand(['iinit', self.auth_session.password])

        # clean up keyfile
        os.unlink(keyfile_path)

# Requires existence of OS account 'irodsauthuser' with password 'iamnotasecret'


class Test_Auth(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Auth, self).setUp()
        cfg = lib.open_and_load_json(os.path.join(IrodsConfig().irods_directory, 'test', 'test_framework_configuration.json'))
        auth_user = cfg['irods_authuser_name']
        auth_pass = cfg['irods_authuser_password']
        self.auth_session = session.mkuser_and_return_session('rodsuser', auth_user, auth_pass, lib.get_hostname())

    def tearDown(self):
        self.auth_session.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', self.auth_session.username])
        super(Test_Auth, self).tearDown()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or test.settings.USE_SSL, 'Topo from resource or SSL')
    def test_authentication_PAM_without_negotiation(self):
        irods_config = IrodsConfig()
        server_key_path = os.path.join(irods_config.irods_directory, 'test', 'server.key')
        server_csr_path = os.path.join(irods_config.irods_directory, 'test', 'server.csr')
        chain_pem_path = os.path.join(irods_config.irods_directory, 'test', 'chain.pem')
        dhparams_pem_path = os.path.join(irods_config.irods_directory, 'test', 'dhparams.pem')
        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, '1024'])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-key', server_key_path, '-out', server_csr_path])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-x509', '-key', server_key_path, '-out', chain_pem_path, '-days', '365'])
        lib.execute_command(['openssl', 'dhparam', '-2', '-out', dhparams_pem_path, '1024'])  # normally 2048, but smaller size here for speed

        service_account_environment_file_path = os.path.join(os.path.expanduser('~'), '.irods', 'irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            server_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_verify_server': 'none',
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_verify_server': 'none',
                'irods_authentication_scheme': 'PaM',
            }

            # now the actual test
            auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
            self.auth_session.environment_file_contents.update(client_update)

            # server reboot to pick up new irodsEnv settings
            IrodsController().restart()

            # do the reauth
            self.auth_session.assert_icommand(['iinit', self.auth_session.password])
            # connect and list some files
            self.auth_session.assert_icommand('icd')
            self.auth_session.assert_icommand('ils -L', 'STDOUT_SINGLELINE', 'home')

            # reset client environment to original
            self.auth_session.environment_file_contents = auth_session_env_backup

            # clean up
            for filename in [chain_pem_path, server_key_path, dhparams_pem_path, server_csr_path]:
                os.unlink(filename)

        # server reboot to pick up new irodsEnv and server settings
        IrodsController().restart()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or test.settings.USE_SSL, 'Topo from resource or SSL')
    def test_authentication_PAM_with_server_params(self):
        irods_config = IrodsConfig()
        server_key_path = os.path.join(irods_config.irods_directory, 'test', 'server.key')
        server_csr_path = os.path.join(irods_config.irods_directory, 'test', 'server.csr')
        chain_pem_path = os.path.join(irods_config.irods_directory, 'test', 'chain.pem')
        dhparams_pem_path = os.path.join(irods_config.irods_directory, 'test', 'dhparams.pem')
        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, '1024'])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-key', server_key_path, '-out', server_csr_path])
        lib.execute_command(['openssl', 'req', '-batch', '-new', '-x509', '-key', server_key_path, '-out', chain_pem_path, '-days', '365'])
        lib.execute_command(['openssl', 'dhparam', '-2', '-out', dhparams_pem_path, '1024'])  # normally 2048, but smaller size here for speed

        service_account_environment_file_path = os.path.join(os.path.expanduser('~'), '.irods', 'irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            irods_config = IrodsConfig()
            server_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_verify_server': 'none',
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_ssl_certificate_chain_file': chain_pem_path,
                'irods_ssl_certificate_key_file': server_key_path,
                'irods_ssl_dh_params_file': dhparams_pem_path,
                'irods_ssl_verify_server': 'none',
                'irods_authentication_scheme': 'PaM',
                'irods_client_server_policy': 'CS_NEG_REQUIRE',
            }

            auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
            self.auth_session.environment_file_contents.update(client_update)

            with lib.file_backed_up(irods_config.server_config_path):
                server_config_update = {
                    'authentication' : {
                        'pam' : {
                            'password_length': 20,
                            'no_extend': False,
                            'password_min_time': 121,
                            'password_max_time': 1209600,
                            }
                        }
                    }
                lib.update_json_file_from_dict(irods_config.server_config_path, server_config_update)

                IrodsController().restart()

                # the test
                self.auth_session.assert_icommand(['iinit', self.auth_session.password])
                self.auth_session.assert_icommand("icd")
                self.auth_session.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "home")

        self.auth_session.environment_file_contents = auth_session_env_backup
        irods_config = IrodsConfig()
        for filename in [chain_pem_path, server_key_path, dhparams_pem_path, server_csr_path]:
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

    def test_blacklist(self):
        with lib.file_backed_up(paths.server_config_path()):
            server_config_update = {
                'controlled_user_connection_list' : {
                    'control_type' : 'blacklist',
                    'users' : [
                        'user1#tempZone',
                        'user2#otherZone',
                        '#'.join([self.user_sessions[0].username, self.user_sessions[0].zone_name]),
                        'user4#otherZone'
                    ]
                }
            }
            lib.update_json_file_from_dict(paths.server_config_path(), server_config_update)

            IrodsController().restart()
            self.user_sessions[0].assert_icommand( 'ils', 'STDERR_SINGLELINE', 'SYS_USER_NOT_ALLOWED_TO_CONN' )
            self.user_sessions[1].assert_icommand( 'ils', 'STDOUT_SINGLELINE', '/home' )
        IrodsController().restart()

    def test_whitelist(self):
        with lib.file_backed_up(paths.server_config_path()):
            service_account = IrodsConfig().client_environment
            server_config_update = {
                'controlled_user_connection_list' : {
                    'control_type' : 'whitelist',
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

            IrodsController().restart()
            self.user_sessions[0].assert_icommand( 'ils', 'STDOUT_SINGLELINE', '/home' )
            self.user_sessions[1].assert_icommand( 'ils', 'STDERR_SINGLELINE', 'SYS_USER_NOT_ALLOWED_TO_CONN' )
        IrodsController().restart()

