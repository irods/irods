import time
import sys
import shutil
import os
import socket
import datetime
import imp
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import test
from . import settings
from . import session
from .. import lib
from ..configuration import IrodsConfig
from . import resource_suite

class Test_DeferredToDeferred(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            context_prefix = lib.get_hostname() + ':' + IrodsConfig().irods_directory
            admin_session.assert_icommand('iadmin modresc demoResc name origResc', 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand('iadmin mkresc demoResc deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc1 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc2 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc3 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc defResc4 deferred', 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand('iadmin mkresc rescA "unixfilesystem" ' + context_prefix + '/rescAVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc rescB "unixfilesystem" ' + context_prefix + '/rescBVault', 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin addchildtoresc defResc3 rescA')
            admin_session.assert_icommand('iadmin addchildtoresc defResc4 rescB')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc defResc1')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc defResc2')
            admin_session.assert_icommand('iadmin addchildtoresc defResc1 defResc3')
            admin_session.assert_icommand('iadmin addchildtoresc defResc2 defResc4')
        super(Test_DeferredToDeferred, self).setUp()

    def tearDown(self):
        super(Test_DeferredToDeferred, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc defResc3 rescA")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc4 rescB")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc1 defResc3")
            admin_session.assert_icommand("iadmin rmchildfromresc defResc2 defResc4")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc defResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc defResc2")
            admin_session.assert_icommand("iadmin rmresc rescA")
            admin_session.assert_icommand("iadmin rmresc rescB")
            admin_session.assert_icommand("iadmin rmresc defResc1")
            admin_session.assert_icommand("iadmin rmresc defResc2")
            admin_session.assert_icommand("iadmin rmresc defResc3")
            admin_session.assert_icommand("iadmin rmresc defResc4")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/rescAVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/rescBVault", ignore_errors=True)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Skip for topology testing from resource server")
    def test_iput_irm(self):
            # =-=-=-=-=-=-=-
            # build a logical path for putting a file
            test_file = self.admin.session_collection + "/test_file.txt"

            # =-=-=-=-=-=-=-
            # put a test_file.txt - should be on rescA given load table values
            self.admin.assert_icommand("iput -f %s %s" % (self.testfile, test_file))
            self.admin.assert_icommand("irm -f " + test_file)
