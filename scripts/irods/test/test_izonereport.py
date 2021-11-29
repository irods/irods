from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import json
from .. import lib
from . import session
from ..test.command import assert_command
from ..configuration import IrodsConfig

class Test_Izonereport(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        cls.admin.assert_icommand(['iadmin', 'mkresc', 'fs_resc_1', 'unixfilesystem', lib.get_hostname() + '/tmp/irods/fs_resc_1'], 'STDOUT_SINGLELINE', 'unixfilesystem')
        cls.admin.assert_icommand(['iadmin', 'mkresc', 'fs_resc_2', 'unixfilesystem', lib.get_hostname() + '/tmp/irods/fs_resc_2'], 'STDOUT_SINGLELINE', 'unixfilesystem')
        cls.admin.assert_icommand(['iadmin', 'mkresc', 'rand_resc', 'random'], 'STDOUT_SINGLELINE', 'rand_resc')
        cls.admin.assert_icommand(['iadmin', 'mkresc', 'repl_resc', 'replication'], 'STDOUT_SINGLELINE', 'repl_resc')
        cls.admin.assert_icommand(['iadmin', 'mkresc', 'comp_resc', 'compound'], 'STDOUT_SINGLELINE', 'comp_resc')
        cls.admin.assert_icommand(['iadmin', 'modresc', 'comp_resc', 'context', 'comp_resc_context'])
        cls.admin.assert_icommand(['iadmin', 'modresc', 'repl_resc', 'context', 'repl_resc_context'])

    @classmethod
    def tearDownClass(cls):
        cls.admin.assert_icommand(['iadmin', 'rmresc', 'fs_resc_1'])
        cls.admin.assert_icommand(['iadmin', 'rmresc', 'fs_resc_2'])
        cls.admin.assert_icommand(['iadmin', 'rmresc', 'rand_resc'])
        cls.admin.assert_icommand(['iadmin', 'rmresc', 'repl_resc'])
        cls.admin.assert_icommand(['iadmin', 'rmresc', 'comp_resc'])
        with session.make_session_for_existing_admin() as admin_session:
            cls.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

    def test_izonereport_key_sanitization(self):
        self.admin.assert_icommand("izonereport | grep key | grep -v XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
                                   'STDOUT_SINGLELINE', '"irods_encryption_key_size": 32,', use_unsafe_shell=True)

    def test_izonereport_with_coordinating_resources__ticket_3303(self):
        _, stdout, _ = self.admin.assert_icommand('izonereport', 'STDOUT_SINGLELINE', 'comp_resc')

        expected_names = [
            'rand_resc',
            'repl_resc',
            'comp_resc'
        ]

        icat_server_object = json.loads(stdout)['zones'][0]['icat_server']
        self.assertIn('coordinating_resources', icat_server_object.keys())
        coord_array = icat_server_object['coordinating_resources']
        coord_names = map(lambda r : r['name'], coord_array)

        for n in expected_names:
            self.assertIn(n, coord_names)

    @unittest.skip('FIXME: Remove this line once we figure out why the test fails in ci')
    def test_izonereport_and_validate(self):
        jsonschema_installed = True
        if lib.get_os_distribution() == 'Ubuntu' and lib.get_os_distribution_version_major() == '12':
            jsonschema_installed = False

        validate_json_path = os.path.join(IrodsConfig().scripts_directory, 'validate_json.py')
        zone_report = os.path.join(self.admin.local_session_dir, 'out.txt')
        # bad URL
        self.admin.assert_icommand("izonereport > %s" % (zone_report), use_unsafe_shell=True)
        if jsonschema_installed:
            assert_command('python2 %s %s https://irods.org/badurl' % (validate_json_path, zone_report), 'STDERR_MULTILINE',
                               ['WARNING: Validation Failed'], desired_rc=2)
        else:
            assert_command('python2 %s %s https://irods.org/badurl' % (validate_json_path, zone_report),
                               'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=2)

        # good URL
        self.admin.assert_icommand("izonereport > out.txt", use_unsafe_shell=True)
        irods_config = IrodsConfig()
        command = [sys.executable, validate_json_path, zone_report, '{0}/{1}/zone_bundle.json'.format(irods_config.server_config['schema_validation_base_uri'], irods_config.server_config['schema_version'])]
        if jsonschema_installed:
            assert_command(command, 'STDOUT_MULTILINE', ['Validating', '... Success'], desired_rc=0)
        else:
            assert_command(command, 'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=2)

    # see issue #5170
    def test_resource_json_has_id(self):
        with session.make_session_for_existing_admin() as admin:
            _, stdout, _ = admin.assert_icommand(['izonereport'], 'STDOUT')
            icat_server_object = json.loads(stdout)['zones'][0]['icat_server']

            self.assertIn('resources', icat_server_object.keys())
            self.assertGreaterEqual(len(icat_server_object['resources']), 1)
            for resource in icat_server_object['resources']:
                self.assertIn('id', resource.keys())

            self.assertIn('coordinating_resources', icat_server_object.keys())
            self.assertGreaterEqual(len(icat_server_object['coordinating_resources']), 1)
            for resource in icat_server_object['coordinating_resources']:
                self.assertIn('id', resource.keys())
