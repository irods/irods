import sys
import os
import subprocess
import json
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
import pydevtest_sessions as s
from pydevtest_common import get_irods_top_level_dir, get_irods_config_dir, mod_json_file
import pydevtest_common


class Test_ixmsg(unittest.TestCase):
    serverConfigFile = get_irods_config_dir() + "/server_config.json"
    xmsgHost = 'localhost'
    xmsgPort = 1279

    def setUp(self):
        # Set up admin session
        s.admin_up()

        # add Xmsg settings to server_config.json
        with open(self.serverConfigFile) as f:
            contents = json.load(f)
        os.system("cp %s %s_orig" % (self.serverConfigFile, self.serverConfigFile))
        contents["xmsg_host"] = self.xmsgHost
        contents["xmsg_port"] = self.xmsgPort
        with open(self.serverConfigFile, 'w') as f:
            json.dump(contents, f)

        # apparently needed by the server too...
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)

        # restart server with Xmsg
        args = [get_irods_top_level_dir() + '/iRODS/irodsctl', 'restart']
        subprocess.Popen(args, env=my_env).communicate()

    def tearDown(self):
        # revert to original server_config.json
        os.system("mv -f %s_orig %s" % (self.serverConfigFile, self.serverConfigFile))

        # restart server
        my_env = os.environ.copy()
        args = [get_irods_top_level_dir() + '/iRODS/irodsctl', 'restart']
        subprocess.Popen(args, env=my_env).communicate()

        # Close admin session
        s.admin_down()

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_AS_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_send_and_receive_one_xmsg(self):
        message = 'Hello World!'

        # set up Xmsg in client environment
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)

        # send msg
        args = ['/usr/bin/ixmsg', 's', '-M "{0}"'.format(message)]
        # couldn't get ixmsg to work non-interactively in assertiCmd()...
        subprocess.Popen(args, env=my_env).communicate()

        # receive msg
        args = ['/usr/bin/ixmsg', 'r', '-n 1']
        res = subprocess.Popen(args, env=my_env, stdout=subprocess.PIPE).communicate()

        # assertion
        print 'looking for "{0}" in "{1}"'.format(message, res[0].rstrip())
        assert (res[0].find(message) >= 0)
