import shutil
import sys
import os
import subprocess
import json
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

import configuration
import lib


class Test_ixmsg(unittest.TestCase):
    serverConfigFile = lib.get_irods_config_dir() + '/server_config.json'
    serverConfigFileBackup = serverConfigFile + '_orig'
    xmsgHost = 'localhost'
    xmsgPort = 1279

    def setUp(self):
        # add Xmsg settings to server_config.json
        shutil.copyfile(self.serverConfigFile, self.serverConfigFileBackup)
        contents = lib.open_and_load_json_ascii(self.serverConfigFile)
        update = {
            'xmsg_host': self.xmsgHost,
            'xmsg_port': self.xmsgPort,
        }
        lib.update_json_file_from_dict(self.serverConfigFile, update)

        # apparently needed by the server too...
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)
        lib.restart_irods_server(env=my_env)

    def tearDown(self):
        os.rename(self.serverConfigFileBackup, self.serverConfigFile)
        lib.restart_irods_server()

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_send_and_receive_one_xmsg(self):
        message = 'Hello World!'

        # set up Xmsg in client environment
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)

        # send msg
        args = ['/usr/bin/ixmsg', 's', '-M "{0}"'.format(message)]
        subprocess.Popen(args, env=my_env).communicate()

        # receive msg
        args = ['/usr/bin/ixmsg', 'r', '-n 1']
        res = subprocess.Popen(args, env=my_env, stdout=subprocess.PIPE).communicate()

        # assertion
        print 'looking for "{0}" in "{1}"'.format(message, res[0].rstrip())
        assert res[0].find(message) >= 0
