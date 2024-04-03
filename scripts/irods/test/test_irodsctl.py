import unittest

from subprocess import PIPE
import json
import os
import shutil
import sys
import tempfile

from . import session
from . import settings
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController, capture_process_tree
from ..test.command import assert_command

class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_creation(self):
        if 'irods_rule_engine_plugin-irods_rule_language' in IrodsConfig().configured_rule_engine_plugins:
            assert lib.re_shm_exists()

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(IrodsConfig().irods_directory, 'irodsctl')
        assert_command([irodsctl_fullpath, 'stop', '-v'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
        assert not lib.re_shm_exists(), lib.re_shm_exists()
        IrodsController().start()

    def test_configuration_schema_validation_from_file(self):
        with lib.file_backed_up(IrodsConfig().server_config_path) as server_config_filename:
            server_config = lib.open_and_load_json(server_config_filename)
            server_config['schema_validation_base_uri'] = 'file://{0}/configuration_schemas'.format(paths.irods_directory())
            lib.update_json_file_from_dict(server_config_filename, server_config)
            irodsctl_fullpath = os.path.join(IrodsConfig().irods_directory, 'irodsctl')

            if lib.is_jsonschema_installed():
                expected_lines = ['Validating [{0}/.irods/irods_environment.json]... Success'.format(IrodsConfig().home_directory),
                                  'Validating [{0}/server_config.json]... Success'.format(IrodsConfig().config_directory),
                                  'Validating [{0}/version.json]... Success'.format(IrodsConfig().irods_directory)]
                assert_command([irodsctl_fullpath, 'restart', '-v'], 'STDOUT_MULTILINE', expected_lines)
            else:
                assert_command([irodsctl_fullpath, 'restart', '-v'], 'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0)

    def test_agents_kept_alive_by_connected_clients_are_cleaned_up_after_failed_graceful_shutdown__issue_7619(self):
        # Get the server process so that we can monitor the child processes.
        server_proc = IrodsController().get_server_proc()
        self.assertIsNotNone(server_proc)

        with session.make_session_for_existing_admin() as admin_session:
            user = session.mkuser_and_return_session('rodsuser', 'bilbo', 'bpass', lib.get_hostname())
            logical_path = f'{user.session_collection}/the_replica_that_never_should_have_been'

            try:
                # Create a session and kick off a client call that will never return. In this case, a call to
                # istream write with no input.
                istream_command = ['istream', 'write', logical_path]
                kwargs = dict()
                user._prepare_run_icommand(istream_command, kwargs)
                istream_proc = lib.execute_command_nonblocking(
                    istream_command, stdout=PIPE, stderr=PIPE, stdin=PIPE, **kwargs)
                self.assertIsNone(istream_proc.poll())
                # Wait until the data object is created in the catalog before proceeding. This is how we confirm that
                # an agent exists for the istream call.
                lib.delayAssert(lambda: lib.replica_exists(user, logical_path, 0))

                # Collect all the server processes. Should be 3-4 depending on whether the delay server is querying.
                # capture_process_tree does not count the grandpa process.
                server_descendants_before_shutdown = set()
                self.assertTrue(
                    capture_process_tree(
                        server_proc, server_descendants_before_shutdown, IrodsController().server_binaries))
                print(server_descendants_before_shutdown) # debugging
                self.assertGreaterEqual(len(server_descendants_before_shutdown), 3)

                # Shut down the server - this will take a while as it will need to time out first.
                assert_command(
                    [os.path.join(IrodsConfig().irods_directory, 'irodsctl'), 'stop', '-v'],
                    'STDERR', 'Killing forcefully...')

                # Ensure that the server shut down but the client process did not.
                self.assertFalse(server_proc.is_running())
                self.assertIsNone(istream_proc.poll())

                # Now for the most important part... Confirm that all the server processes from before are gone.
                # Pass None for server_proc because it should not be running anymore. get_binary_to_procs_dict()
                # defaults to searching for processes with the server binary names.
                self.assertEqual(0, len(IrodsController().get_binary_to_procs_dict(server_proc=None)))

            finally:
                # Restart the server.
                IrodsController().restart()

                # Set the replica to stale (it will be in the intermediate status - this is not a bug) and remove.
                admin_session.run_icommand(
                    ['iadmin', 'modrepl', 'logical_path', logical_path, 'replica_number', '0', 'DATA_REPL_STATUS', '0'])
                user.run_icommand(['irm', '-f', logical_path])

                # Exit the session created from before and remove the user.
                user.__exit__()
                admin_session.run_icommand(['iadmin', 'rmuser', user.username])

                # Kill the client process spawned above, if applicable.
                istream_proc.terminate()


def stop_irods_server():
    assert_command([os.path.join(IrodsConfig().irods_directory, 'irodsctl'), 'stop', '-v'],
                   'STDOUT_SINGLELINE', 'Success')

def start_irods_server(env=None):
    def is_jsonschema_available():
        try:
            import jsonschema
            return True
        except ImportError:
            return False

    if is_jsonschema_available():
        assert_command('{0} graceful_start -v'.format(os.path.join(IrodsConfig().irods_directory, 'irodsctl')),
                       'STDOUT_SINGLELINE', 'Success', env=env)
    else:
        assert_command('{0} graceful_start -v'.format(os.path.join(IrodsConfig().irods_directory, 'irodsctl')),
                       'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0, env=env)
    with session.make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand('ils', 'STDOUT_SINGLELINE', admin_session.zone_name)

def restart_irods_server(env=None):
    stop_irods_server()
    start_irods_server(env=env)
