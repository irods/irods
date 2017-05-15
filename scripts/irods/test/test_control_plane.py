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
from . import session
from ..test.command import assert_command

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
            initial_size_of_server_log = lib.get_file_size_by_path(IrodsConfig().server_log_path)
            try:
                admin_session.assert_icommand(['irods-grid', 'shutdown', '--hosts', lib.get_hostname()], 'STDOUT_SINGLELINE', 'shutting down')
                time.sleep(10) # server gives control plane the all-clear before printing done message
                self.assertEqual(1, lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, 'iRODS Server is done', initial_size_of_server_log))
            finally:
                IrodsController().start()

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
                    IrodsController().start()
            finally:
                admin_session.assert_icommand('iadmin rmresc invalid_resc')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Skip for Topology Testing')
    def test_icat_shutdown_with_no_resource(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.environment_file_contents = IrodsConfig().client_environment
            try:
                admin_session.assert_icommand('iadmin rmresc demoResc')
                host_name = lib.get_hostname()
                admin_session.assert_icommand(['irods-grid', 'status', '--hosts', host_name], 'STDOUT_SINGLELINE', host_name)
            finally:
                host_name = lib.get_hostname()
                admin_session.assert_icommand('iadmin mkresc demoResc unixfilesystem '+host_name+':/var/lib/irods/Vault', 'STDOUT_SINGLELINE', 'demoResc')
