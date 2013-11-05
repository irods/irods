import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import pydevtest_sessions as s
import commands
import os

class Test_Auth_Suite(unittest.TestCase, ResourceBase):

    my_test_resource = {"setup":[],"teardown":[]}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

    def tearDown(self):
        self.run_resource_teardown()
        s.twousers_down()

    def test_authentication_PAM(self):
        # add auth test user
        authTestUser = "irodsauthuser"
        authTestPass = "iamnotasecret"
        assertiCmd(s.adminsession,"iadmin mkuser %s rodsuser" % authTestUser)

        # set up client and server side for ssl handshake

        # server side certificate setup
        os.system("openssl genrsa -out server.key")
        os.system("openssl req -batch -new -key server.key -out server.csr")
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")
        os.system("mv server.crt chain.pem")
        os.system("openssl dhparam -2 -out dhparams.pem 100") # normally 2048, but smaller size here for speed

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = "/var/lib/eirods/tests/pydevtest/chain.pem"
        os.environ['irodsSSLCertificateKeyFile'] = "/var/lib/eirods/tests/pydevtest/server.key"
        os.environ['irodsSSLDHParamsFile'] = "/var/lib/eirods/tests/pydevtest/dhparams.pem"

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

        # server reboot to pick up new irodsEnv settings
        os.system("/var/lib/eirods/iRODS/irodsctl stop")
        os.system("/var/lib/eirods/tests/zombiereaper.sh")
        os.system("/var/lib/eirods/iRODS/irodsctl start")

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
        os.system("rm server.key server.csr chain.pem dhparams.pem")

