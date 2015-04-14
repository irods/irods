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
        self.auth_session.assert_icommand('ils -L', 'STDOUT', 'home')

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

    @unittest.skipIf(True, '''Renable after #2614 fixed. configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server"''')
    def test_authentication_PAM_without_negotiation(self):
        ## set up client and server side for ssl handshake
        # server side certificate setup
        os.system("openssl genrsa -out server.key")
        # os.system("openssl req -batch -new -key server.key -out server.csr")    # if use external CA
        # self-signed certificate
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")
        os.system("mv server.crt chain.pem")
        os.system("openssl dhparam -2 -out dhparams.pem 100")  # normally 2048, but smaller size here for speed

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/chain.pem'
        os.environ['irodsSSLCertificateKeyFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/server.key'
        os.environ['irodsSSLDHParamsFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/dhparams.pem'

        # client side environment variables
        self.auth_session.environment_file_contents['irods_ssl_verify_server'] = 'none'
        self.auth_session.environment_file_contents['irods_authentication_scheme'] = 'PaM'

        # server reboot to pick up new irodsEnv settings
        lib.restart_irods_server()

        # do the reauth
        self.auth_session.assert_icommand(['iinit', self.auth_session.password])
        # connect and list some files
        self.auth_session.assert_icommand('icd')
        self.auth_session.assert_icommand('ils -L', 'STDOUT', 'home')

        # reset client environment to original
        del self.auth_session.environment_file_contents['irods_authentication_scheme']

        # clean up
        for file in ['server.key', 'chain.pem', 'dhparams.pem']:
            os.unlink(file)

    @unittest.skipIf(True, '''Renable after #2614 fixed. configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server"''')
    def test_authentication_PAM_with_server_params(self):
        ## set up client and server side for ssl handshake
        # server side certificate setup
        os.system('openssl genrsa -out server.key')
        os.system('openssl req -batch -new -x509 -key server.key -out server.crt -days 365')
        os.system('mv server.crt chain.pem')
        os.system('openssl dhparam -2 -out dhparams.pem 100')  # normally 2048, but smaller size here for speed

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/chain.pem'
        os.environ['irodsSSLCertificateKeyFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/server.key'
        os.environ['irodsSSLDHParamsFile'] = lib.get_irods_top_level_dir() + '/tests/pydevtest/dhparams.pem'

        # client side environment variables
        backup_env_contents = copy.deepcopy(self.auth_session.environment_file_contents)
        self.auth_session.environment_file_contents['irods_ssl_verify_server'] = 'none'
        self.auth_session.environment_file_contents['irods_client_server_policy'] = 'CS_NEG_REQUIRE'
        self.auth_session.environment_file_contents['irods_authentication_scheme'] = 'PaM'

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
        self.auth_session.assert_icommand("ils -L", 'STDOUT', "home")

        # reset client environment to original
        self.auth_session.environment_file_contents = backup_env_contents


        # clean up
        for file in ['server.key', 'chain.pem', 'dhparams.pem']:
            os.unlink(file)

        # reset server_config.json to original
        os.system('mv %sOrig %s' % (serverConfigFile, serverConfigFile))

        # server reboot to revert to previous server configuration
        os.system(lib.get_irods_top_level_dir() + '/iRODS/irodsctl stop')
        os.system(lib.get_irods_top_level_dir() + '/tests/zombiereaper.sh')
        os.system(lib.get_irods_top_level_dir() + '/iRODS/irodsctl start')
