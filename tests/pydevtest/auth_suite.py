import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, get_irods_config_dir, get_irods_top_level_dir
import pydevtest_sessions as s
import commands
import os
import time

class Test_OSAuth_Only(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    #@unittest.skip("TODO: Seeing inconsistent behavior in CI")
    def test_authentication_OSAuth(self):
        # add auth test user
        authTestUser = "irods"
        authTestPass = "temporarypasswordforci"
        assertiCmd(s.adminsession,"iadmin mkuser %s rodsuser" % authTestUser)

        # add client irodsEnv settings
        clientEnvFile = s.adminsession.sessionDir+"/.irodsEnv"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))
        os.system("echo \"irodsAuthScheme 'OSAuth'\" >> %s" % clientEnvFile)
        os.system("echo \"irodsUserName '%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsHome '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsCwd '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))

        # setup the irods.key file necessary for OSAuth
        keyfile = get_irods_config_dir() + "/irods.key"
        os.system("echo \"gibberish\" > %s" % keyfile)

        # do the reauth
        assertiCmd(s.adminsession,"iexit full" )             # exit out entirely
        assertiCmd(s.adminsession,"iinit %s" % authTestPass) # reinitialize
        # connect and list some files
        assertiCmd(s.adminsession,"icd") # home directory
        assertiCmd(s.adminsession,"ils -L","LIST","home") # should be listed

        # reset client environment to original
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))
        # reconnect as admin
        assertiCmd(s.adminsession,"iexit full" )                     # exit out entirely
        assertiCmd(s.adminsession,"iinit %s" % s.users[0]['passwd']) # reinitialize

        # remove auth test user
        assertiCmd(s.adminsession,"iadmin rmuser %s" % authTestUser)

        # clean up keyfile
        os.system("rm %s" % keyfile)

class Test_Auth_Suite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    #@unittest.skip("TODO: Seeing inconsistent behavior in CI")
    def test_authentication_PAM_without_negotiation(self):
        # add auth test user
        authTestUser = "irodsauthuser"
        authTestPass = "iamnotasecret"
        assertiCmd(s.adminsession,"iadmin mkuser %s rodsuser" % authTestUser)

        # set up client and server side for ssl handshake

        # server side certificate setup
        os.system("openssl genrsa -out server.key")
        #os.system("openssl req -batch -new -key server.key -out server.csr")    # if use external CA
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")    # self-signed certificate
        os.system("mv server.crt chain.pem")
        os.system("openssl dhparam -2 -out dhparams.pem 100") # normally 2048, but smaller size here for speed

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = get_irods_top_level_dir() + "/tests/pydevtest/chain.pem"
        os.environ['irodsSSLCertificateKeyFile'] = get_irods_top_level_dir() + "/tests/pydevtest/server.key"
        os.environ['irodsSSLDHParamsFile'] = get_irods_top_level_dir() + "/tests/pydevtest/dhparams.pem"

        # client side environment variables
        os.environ['irodsSSLVerifyServer'] = "none"

        # add client irodsEnv settings
        clientEnvFile = s.adminsession.sessionDir+"/.irodsEnv"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))
#        os.system("echo \"irodsClientServerPolicy 'CS_NEG_REQUIRE'\" >> %s" % clientEnvFile) # commented out to test legacy SSL path
        os.system("echo \"irodsAuthScheme 'PaM'\" >> %s" % clientEnvFile) # check for auth to_lower
        os.system("echo \"irodsUserName '%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsHome '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsCwd '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))

        # server reboot to pick up new irodsEnv settings
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl restart")

        # do the reauth
        assertiCmd(s.adminsession,"iinit %s" % authTestPass) # reinitialize
        # connect and list some files
        assertiCmd(s.adminsession,"icd") # home directory
        assertiCmd(s.adminsession,"ils -L","LIST","home") # should be listed

        # reset client environment to original
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))
        # reconnect as admin
        assertiCmd(s.adminsession,"iinit %s" % s.users[0]['passwd']) # reinitialize

        # remove auth test user
        assertiCmd(s.adminsession,"iadmin rmuser %s" % authTestUser)

        # clean up
        os.system("rm server.key chain.pem dhparams.pem")

    #@unittest.skip("TODO: Seeing inconsistent behavior in CI")
    def test_authentication_PAM_with_server_params(self):
        # add auth test user
        authTestUser = "irodsauthuser"
        authTestPass = "iamnotasecret"
        assertiCmd(s.adminsession,"iadmin mkuser %s rodsuser" % authTestUser)

        # set up client and server side for ssl handshake

        # server side certificate setup
        os.system("openssl genrsa -out server.key")
        # os.system("openssl req -batch -new -key server.key -out server.csr")    # if use external CA
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")    # self-signed certificate
        os.system("mv server.crt chain.pem")
        os.system("openssl dhparam -2 -out dhparams.pem 100") # normally 2048, but smaller size here for speed

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = get_irods_top_level_dir() + "/tests/pydevtest/chain.pem"
        os.environ['irodsSSLCertificateKeyFile'] = get_irods_top_level_dir() + "/tests/pydevtest/server.key"
        os.environ['irodsSSLDHParamsFile'] = get_irods_top_level_dir() + "/tests/pydevtest/dhparams.pem"

        # client side environment variables
        os.environ['irodsSSLVerifyServer'] = "none"

        # add client irodsEnv settings
        clientEnvFile = s.adminsession.sessionDir+"/.irodsEnv"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))
        os.system("echo \"irodsClientServerPolicy 'CS_NEG_REQUIRE'\" >> %s" % clientEnvFile)
        os.system("echo \"irodsAuthScheme 'PaM'\" >> %s" % clientEnvFile) # check for auth to_lower
        os.system("echo \"irodsUserName '%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsHome '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))
        os.system("echo \"irodsCwd '/tempZone/home/%s'\" >> %s" % (authTestUser, clientEnvFile))
        
        # add server.config settings
        serverConfigFile = get_irods_config_dir() + "/server.config"
        os.system("cp %s %sOrig" % (serverConfigFile, serverConfigFile))
        os.system("echo \"pam_password_length 20\" >> %s" % serverConfigFile)
        os.system("echo \"pam_no_extend false\" >> %s" % serverConfigFile)
        os.system("echo \"pam_password_min_time 121\" >> %s" % serverConfigFile)
        os.system("echo \"pam_password_max_time 1209600\" >> %s" % serverConfigFile)


        # server reboot to pick up new irodsEnv and server settings
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl restart")

        # do the reauth
        assertiCmd(s.adminsession,"iinit %s" % authTestPass) # reinitialize
        # connect and list some files
        assertiCmd(s.adminsession,"icd") # home directory
        assertiCmd(s.adminsession,"ils -L","LIST","home") # should be listed

        # reset client environment to original
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))
        # reconnect as admin
        assertiCmd(s.adminsession,"iinit %s" % s.users[0]['passwd']) # reinitialize

        # remove auth test user
        assertiCmd(s.adminsession,"iadmin rmuser %s" % authTestUser)

        # clean up
        os.system("rm server.key chain.pem dhparams.pem")
        
        # reset server.config to original
        os.system("mv %sOrig %s" % (serverConfigFile, serverConfigFile))
        
        # server reboot to revert to previous server configuration
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(get_irods_top_level_dir() + "/tests/zombiereaper.sh")
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl start")
        
        

