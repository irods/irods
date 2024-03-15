import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import copy
import json
import os
import time

from .. import test
from . import settings
from .resource_suite import ResourceBase
from ..controller import IrodsController
from ..configuration import IrodsConfig
from .. import lib
from .. import paths
from . import session
from ..test.command import assert_command

try:
    import zmq
except ImportError:
    zmq = None

SessionsMixin = session.make_sessions_mixin([('otherrods','pass')], [])

class TestControlPlane(SessionsMixin, unittest.TestCase):

    def test_pause_and_resume(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            admin_session.assert_icommand('irods-grid pause --all', 'STDOUT_SINGLELINE', 'pausing')

            # need a time-out assert icommand for ils here

            admin_session.assert_icommand('irods-grid resume --all', 'STDOUT_SINGLELINE', 'resuming')

            # Make sure server is actually responding
            admin_session.assert_icommand('ils', 'STDOUT_SINGLELINE', IrodsConfig().client_environment['irods_zone_name'])
            admin_session.assert_icommand('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    def test_status(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            admin_session.assert_icommand('irods-grid status --all', 'STDOUT_SINGLELINE', 'hosts')

    def test_hosts_separator(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            for s in [',', ', ']:
                hosts_string = s.join([lib.get_hostname()]*2)
                admin_session.assert_icommand(['irods-grid', 'status', '--hosts', hosts_string], 'STDOUT_SINGLELINE', lib.get_hostname())

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            if 'irods_rule_engine_plugin-irods_rule_language' in IrodsConfig().configured_rule_engine_plugins:
                self.assertTrue(lib.re_shm_exists())

            try:
                assert_command('irods-grid shutdown --all', 'STDOUT_SINGLELINE', 'shutting down')
                #time.sleep(2)
                assert_command('ils', 'STDERR_SINGLELINE', 'connectToRhost error')

                self.assertFalse(lib.re_shm_exists())
            finally:
                IrodsController().start()

    def test_shutdown_local_server(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_parent_log_path)
            try:
                admin_session.assert_icommand(['irods-grid', 'shutdown', '--hosts', lib.get_hostname()], 'STDOUT_SINGLELINE', 'shutting down')
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='iRODS Server is done',
                        server_log_path=IrodsConfig().server_parent_log_path,
                        start_index=initial_size_of_server_log))
            finally:
                IrodsController().start()

    def test_shutdown_local_server_using_hostname_alias_defined_in_server_config__issue_5700(self):
        try:
            with lib.file_backed_up(paths.server_config_path()):
                hostname_alias = 'hostname-alias.issue-5700'

                with open(paths.server_config_path(), 'r+') as f:
                    server_config = json.load(f)
                    server_config['host_resolution']['host_entries'] = [{
                        'address_type': 'local',
                        'addresses': [
                            lib.get_hostname(),
                            hostname_alias
                        ]
                    }]

                    f.seek(0)
                    f.truncate(0)
                    f.write(json.dumps(server_config, indent=4))

                # Restart the server so that the original process (parent of the agent factory)
                # sees the updates to the server_config.json file.
                IrodsController().restart(test_mode=True)

                with session.make_session_for_existing_admin() as admin_session:
                    admin_session.environment_file_contents = IrodsConfig().client_environment
                    initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_parent_log_path)

                    expected_output = [json.dumps({'hosts': [{'shutting down': lib.get_hostname()}]}, indent=4)]
                    admin_session.assert_icommand(['irods-grid', 'shutdown', '--hosts', hostname_alias], 'STDOUT', expected_output)
                    lib.delayAssert(
                        lambda: lib.log_message_occurrences_equals_count(
                            msg='iRODS Server is done',
                            server_log_path=IrodsConfig().server_parent_log_path,
                            start_index=initial_size_of_server_log))

        finally:
            # Restart the server so that the original process (parent of the agent factory)
            # sees the restored server_config.json file.
            IrodsController().restart(test_mode=True)

    def test_control_plane_uses_whole_string_matching_for_hostname_validation__issue_5700(self):
        try:
            with lib.file_backed_up(paths.server_config_path()):
                hostname_alias = 'hostname-alias.issue-5700.' + lib.get_hostname()

                with open(paths.server_config_path(), 'r+') as f:
                    server_config = json.load(f)
                    server_config['host_resolution']['host_entries'] = [{
                        'address_type': 'local',
                        'addresses': [
                            lib.get_hostname(),
                            hostname_alias
                        ]
                    }]

                    f.seek(0)
                    f.truncate(0)
                    f.write(json.dumps(server_config, indent=4))

                # Restart the server so that the original process (parent of the agent factory)
                # sees the updates to the server_config.json file.
                IrodsController().restart(test_mode=True)

                with session.make_session_for_existing_admin() as admin_session:
                    admin_session.environment_file_contents = IrodsConfig().client_environment

                    # Show that hostnames passed to the --hosts option must match a name
                    # in hosts_config.json exactly. Hostnames containing dots and ending in
                    # ".<server_hostname>" results in an error. See the following comment for
                    # more information:
                    # 
                    #     https://github.com/irods/irods/issues/5700#issuecomment-963390725
                    #
                    invalid_hostname_alias = 'invalid.' + lib.get_hostname()
                    expected_output = ['server responded with an error']
                    admin_session.assert_icommand(['irods-grid', 'shutdown', '--hosts', invalid_hostname_alias], 'STDERR', expected_output)

                    # Show that the server is still running.
                    admin_session.assert_icommand(['irods-grid', 'status', '--hosts', hostname_alias], 'STDOUT_SINGLELINE', ['server_state_running'])

        finally:
            # Restart the server so that the original process (parent of the agent factory)
            # sees the restored server_config.json file.
            IrodsController().restart(test_mode=True)

    def test_invalid_client_environment(self):
        env_backup = copy.deepcopy(self.admin_sessions[0].environment_file_contents)

        if 'irods_server_control_plane_port' in self.admin_sessions[0].environment_file_contents:
            del self.admin_sessions[0].environment_file_contents['irods_server_control_plane_port']
        if 'irods_server_control_plane_key' in self.admin_sessions[0].environment_file_contents:
            del self.admin_sessions[0].environment_file_contents['irods_server_control_plane_key']
        if 'irods_server_control_plane_encryption_num_hash_rounds' in self.admin_sessions[0].environment_file_contents:
            del self.admin_sessions[0].environment_file_contents['irods_server_control_plane_encryption_num_hash_rounds']
        if 'irods_server_control_plane_port' in self.admin_sessions[0].environment_file_contents:
            del self.admin_sessions[0].environment_file_contents['irods_server_control_plane_encryption_algorithm']

        self.admin_sessions[0].assert_icommand('irods-grid status --all', 'STDERR_SINGLELINE', 'USER_INVALID_CLIENT_ENVIRONMENT')

        self.admin_sessions[0].environment_file_contents = env_backup

    def test_status_with_invalid_host(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            try:
                admin_session.assert_icommand('iadmin mkresc invalid_resc unixfilesystem invalid_host:/tmp/irods/invalid', 'STDOUT_SINGLELINE', 'not a valid DNS entry')
                _, stdout, _ = admin_session.assert_icommand(['irods-grid', 'status', '--all'], 'STDOUT_SINGLELINE')
                host_responses = json.loads(stdout)['hosts']
                self.assertTrue(({'failed_to_connect': 'tcp://invalid_host:1248'} in host_responses) or ({'response_message_is_empty_from': 'tcp://invalid_host:1248'} in host_responses))
            finally:
                admin_session.assert_icommand('iadmin rmresc invalid_resc')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Skip for Topology Testing: No way to restart grid')
    def test_shutdown_with_invalid_host(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            try:
                admin_session.assert_icommand('iadmin mkresc invalid_resc unixfilesystem invalid_host:/tmp/irods/invalid', 'STDOUT_SINGLELINE', 'not a valid DNS entry')
                try:
                    _, stdout, _ = admin_session.assert_icommand(['irods-grid', 'shutdown', '--wait-forever', '--all'], 'STDOUT_SINGLELINE', 'shutting down')
                    host_responses = json.loads(stdout)['hosts']
                    self.assertTrue(({'failed_to_connect': 'tcp://invalid_host:1248'} in host_responses) or ({'response_message_is_empty_from': 'tcp://invalid_host:1248'} in host_responses))
                    self.admin_sessions[0].assert_icommand('ils','STDERR_SINGLELINE','connectToRhost error')
                finally:
                    # TODO: Restart here in case the main server isn't finished tearing down yet.
                    # We should only need to perform a start() here, though.
                    #IrodsController().start()
                    IrodsController().restart(test_mode=True)
            finally:
                admin_session.assert_icommand('iadmin rmresc invalid_resc')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Skip for Topology Testing')
    def test_provider_shutdown_with_no_consumer(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            try:
                admin_session.assert_icommand('iadmin rmresc demoResc')
                host_name = lib.get_hostname()
                admin_session.assert_icommand(['irods-grid', 'status', '--hosts', host_name], 'STDOUT_SINGLELINE', host_name)
            finally:
                host_name = lib.get_hostname()
                admin_session.assert_icommand('iadmin mkresc demoResc unixfilesystem '+host_name+':/var/lib/irods/Vault', 'STDOUT_SINGLELINE', 'demoResc')

    @unittest.skipIf(zmq is None, 'Skip if pyzmq is unavailable')
    def test_empty_message(self):
        config = IrodsConfig()
        context = zmq.Context()
        socket = context.socket(zmq.REQ)
        control_plane_endpoint = "tcp://%s:%s" % (
            config.client_environment['irods_host'],
            config.server_config['server_control_plane_port'])
        socket.connect(control_plane_endpoint)
        socket.send("")

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = config.client_environment
            _, out, _ = admin_session.assert_icommand('irods-grid status --hosts=%s' % config.client_environment['irods_host'], 'STDOUT_SINGLELINE', "")
        status_dict = json.loads(out)
        assert status_dict['hosts'][0]['status'] == 'server_state_running'

