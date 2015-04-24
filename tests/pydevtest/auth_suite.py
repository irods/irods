import copy
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import time
import json

import configuration
import lib
import resource_suite


# Requires OS account 'irods' to have password 'temporarypasswordforci'
class Test_OSAuth_Only(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_OSAuth_Only, self).setUp()
        self.auth_session = lib.mkuser_and_return_session('rodsuser', 'irods', 'temporarypasswordforci',
                                                                         lib.get_hostname())

    def tearDown(self):
        super(Test_OSAuth_Only, self).tearDown()
        self.auth_session.__exit__()
        lib.rmuser(self.auth_session.username)

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_authentication_OSAuth(self):
        self.auth_session.environment_file_contents['irods_authentication_scheme'] = 'OSAuth'

        # setup the irods.key file necessary for OSAuth
        keyfile_path = os.path.join(lib.get_irods_config_dir(), 'irods.key')
        with open(keyfile_path, 'w') as f:
            f.write('gibberish\n')

        # do the reauth
        self.auth_session.assert_icommand('iexit full')
        self.auth_session.assert_icommand(['iinit', self.auth_session.password])
        # connect and list some files
        self.auth_session.assert_icommand('icd')
        self.auth_session.assert_icommand('ils -L', 'STDOUT_SINGLELINE', 'home')

        # reset client environment to original
        del self.auth_session.environment_file_contents['irods_authentication_scheme']

        # clean up keyfile
        os.unlink(keyfile_path)

# Requires existence of OS account 'irodsauthuser' with password 'iamnotasecret'
class Test_Auth_Suite(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Auth_Suite, self).setUp()
        self.auth_session = lib.mkuser_and_return_session('rodsuser', 'irodsauthuser', 'iamnotasecret',
                                                                         lib.get_hostname())

    def tearDown(self):
        super(Test_Auth_Suite, self).tearDown()
        self.auth_session.__exit__()
        lib.rmuser(self.auth_session.username)

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server')
    def test_authentication_PAM_without_negotiation(self):
        lib.run_command('openssl genrsa -out server.key')
        lib.run_command('openssl req -batch -new -key server.key -out server.csr')
        lib.run_command('openssl req -batch -new -x509 -key server.key -out chain.pem -days 365')
        lib.run_command('openssl dhparam -2 -out dhparams.pem 100') # normally 2048, but smaller size here for speed

        service_account_environment_file_path = os.path.expanduser('~/.irods/irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            server_update = {
                'irods_ssl_certificate_chain_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/chain.pem'),
                'irods_ssl_certificate_key_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/server.key'),
                'irods_ssl_dh_params_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/dhparams.pem'),
                'irods_ssl_verify_server': 'none',
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_ssl_certificate_chain_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/chain.pem'),
                'irods_ssl_certificate_key_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/server.key'),
                'irods_ssl_dh_params_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/dhparams.pem'),
                'irods_ssl_verify_server': 'none',
                'irods_authentication_scheme': 'PaM',
            }

            # now the actual test
            auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
            self.auth_session.environment_file_contents.update(client_update)

            # server reboot to pick up new irodsEnv settings
            lib.restart_irods_server()

            # do the reauth
            self.auth_session.assert_icommand(['iinit', self.auth_session.password])
            # connect and list some files
            self.auth_session.assert_icommand('icd')
            self.auth_session.assert_icommand('ils -L', 'STDOUT_SINGLELINE', 'home')

            # reset client environment to original
            self.auth_session.environment_file_contents = auth_session_env_backup

            # clean up
            for file in ['tests/pydevtest/server.key', 'tests/pydevtest/chain.pem', 'tests/pydevtest/dhparams.pem']:
                os.unlink(os.path.join(lib.get_irods_top_level_dir(),file))

        # server reboot to pick up new irodsEnv and server settings
        lib.restart_irods_server()

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server')
    def test_authentication_PAM_with_server_params(self):
        lib.run_command('openssl genrsa -out server.key')
        lib.run_command('openssl req -batch -new -key server.key -out server.csr')
        lib.run_command('openssl req -batch -new -x509 -key server.key -out chain.pem -days 365')
        lib.run_command('openssl dhparam -2 -out dhparams.pem 100') # normally 2048, but smaller size here for speed

        service_account_environment_file_path = os.path.expanduser('~/.irods/irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            server_update = {
                'irods_ssl_certificate_chain_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/chain.pem'),
                'irods_ssl_certificate_key_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/server.key'),
                'irods_ssl_dh_params_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/dhparams.pem'),
                'irods_ssl_verify_server': 'none',
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_ssl_certificate_chain_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/chain.pem'),
                'irods_ssl_certificate_key_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/server.key'),
                'irods_ssl_dh_params_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/dhparams.pem'),
                'irods_ssl_verify_server': 'none',
                'irods_authentication_scheme': 'PaM',
                'irods_client_server_policy': 'CS_NEG_REQUIRE',
            }

            #session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
            #self.admin.environment_file_contents.update(client_update)

            # now the actual test
            auth_session_env_backup = copy.deepcopy(self.auth_session.environment_file_contents)
            self.auth_session.environment_file_contents.update(client_update)

            # add server_config.json settings
            serverConfigFile = lib.get_irods_config_dir() + "/server_config.json"
            with open(serverConfigFile) as f:
                contents = json.load(f)
            os.system("cp %s %sOrig" % (serverConfigFile, serverConfigFile))
            contents['pam_password_length'] = 20
            contents['pam_no_extend'] = False
            contents['pam_password_min_time'] = 121
            contents['pam_password_max_time'] = 1209600
            with open(serverConfigFile, 'w') as f:
                json.dump(contents, f)

            # server reboot to pick up new irodsEnv and server settings
            lib.restart_irods_server()

            # do the reauth
            self.auth_session.assert_icommand(['iinit', self.auth_session.password])
            # connect and list some files
            self.auth_session.assert_icommand("icd")
            self.auth_session.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "home")

            # reset server_config.json to original
            os.system('mv %sOrig %s' % (serverConfigFile, serverConfigFile))

            # reset client environment to original
            self.auth_session.environment_file_contents = auth_session_env_backup

            # clean up
            for file in ['tests/pydevtest/server.key', 'tests/pydevtest/chain.pem', 'tests/pydevtest/dhparams.pem']:
                os.unlink(os.path.join(lib.get_irods_top_level_dir(),file))

        # server reboot to pick up new irodsEnv and server settings
        lib.restart_irods_server()

    def test_iinit_repaving_2646(self):
        initial_contents = copy.deepcopy(self.admin.environment_file_contents)
        del self.admin.environment_file_contents['irods_zone_name']
        self.admin.run_icommand('iinit', stdin_string='{0}\n{1}\n'.format(initial_contents['irods_zone_name'], self.admin.password))
        final_contents = lib.open_and_load_json_ascii(os.path.join(self.admin.local_session_dir, 'irods_environment.json'))
        self.admin.environment_file_contents = initial_contents
        print initial_contents
        print final_contents
        assert initial_contents == final_contents
