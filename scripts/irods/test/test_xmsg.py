from __future__ import print_function
import shutil
import sys
import os
import subprocess
import json
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import test
from . import settings
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController


class Test_Xmsg(unittest.TestCase):
    serverConfigFile = IrodsConfig().server_config_path
    serverConfigFileBackup = serverConfigFile + '_orig'
    xmsgHost = 'localhost'
    xmsgPort = 1279

    def setUp(self):
        # add Xmsg settings to server_config.json
        shutil.copyfile(self.serverConfigFile, self.serverConfigFileBackup)
        contents = IrodsConfig().server_config
        update = {
            'xmsg_host': self.xmsgHost,
            'xmsg_port': self.xmsgPort,
        }
        lib.update_json_file_from_dict(self.serverConfigFile, update)

        # apparently needed by the server too...
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)

        IrodsController(IrodsConfig(injected_environment=my_env)).restart()

    def tearDown(self):
        shutil.move(self.serverConfigFileBackup, self.serverConfigFile)
        IrodsController().restart()

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER or test.settings.USE_SSL, "Skip for topology testing from resource server or SSL")
    def test_send_and_receive_one_xmsg(self):
        message = 'Hello World!'

        # set up Xmsg in client environment
        my_env = os.environ.copy()
        my_env['XMSG_HOST'] = self.xmsgHost
        my_env['XMSG_PORT'] = str(self.xmsgPort)

        # send msg
        args = ['ixmsg', 's', '-M "{0}"'.format(message)]
        subprocess.Popen(args, env=my_env).communicate()

        # receive msg
        args = ['ixmsg', 'r', '-n 1']
        res = subprocess.Popen(args, env=my_env, stdout=subprocess.PIPE).communicate()

        # assertion
        print('looking for "{0}" in "{1}"'.format(message, res[0].rstrip()))
        assert res[0].decode().find(message) >= 0
