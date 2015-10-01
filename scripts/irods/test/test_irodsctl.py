import json
import os
import shutil
import sys
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import test
from . import settings
from ..test.command import assert_command
from .. import lib
from ..controller import IrodsController
from ..configuration import IrodsConfig


class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_creation(self):
        assert lib.re_shm_exists()

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(IrodsConfig().top_level_directory, 'iRODS', 'irodsctl')
        assert_command([irodsctl_fullpath, 'stop'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
        assert not lib.re_shm_exists(), lib.re_shm_exists()
        IrodsController().start()

    def test_configuration_schema_validation_from_file(self):
        schemas_git_dir = tempfile.mkdtemp(prefix='irods-test_configuration_schema_validation_from_file-git')
        with lib.directory_deleter(schemas_git_dir):
            schemas_repo = 'https://github.com/irods/irods_schema_configuration'
            lib.execute_command(['git', 'clone', schemas_repo, schemas_git_dir])
            schemas_branch = 'v3'
            lib.execute_command(['git', 'checkout', schemas_branch], cwd=schemas_git_dir)
            schemas_deploy_dir = tempfile.mkdtemp(prefix='irods-test_configuration_schema_validation_from_file-schemas')
            with lib.directory_deleter(schemas_deploy_dir):
                assert_command(['python', os.path.join(schemas_git_dir, 'deploy_schemas_locally.py'), '--output_directory_base', schemas_deploy_dir])
                with lib.file_backed_up(IrodsConfig().server_config_path) as server_config_filename:
                    server_config = lib.open_and_load_json(server_config_filename)
                    server_config['schema_validation_base_uri'] = 'file://' + schemas_deploy_dir
                    lib.update_json_file_from_dict(server_config_filename, server_config)
                    irodsctl_fullpath = os.path.join(IrodsConfig().top_level_directory, 'iRODS', 'irodsctl')

                    if lib.is_jsonschema_installed():
                        expected_lines = ['Validating [/var/lib/irods/.irods/irods_environment.json]... Success',
                                          'Validating [/etc/irods/server_config.json]... Success',
                                          'Validating [/var/lib/irods/VERSION.json]... Success',
                                          'Validating [/etc/irods/hosts_config.json]... Success',
                                          'Validating [/etc/irods/host_access_control_config.json]... Success']
                        if not test.settings.TOPOLOGY_FROM_RESOURCE_SERVER:
                            expected_lines.append('Validating [/etc/irods/database_config.json]... Success')
                        assert_command([irodsctl_fullpath, 'restart'], 'STDOUT_MULTILINE', expected_lines)
                    else:
                        assert_command([irodsctl_fullpath, 'restart'], 'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0)

def stop_irods_server():
    assert_command([os.path.join(IrodsConfig().top_level_directory, 'iRODS', 'irodsctl'), 'stop'],
                   'STDOUT_SINGLELINE', 'Success')

def start_irods_server(env=None):
    def is_jsonschema_available():
        try:
            import jsonschema
            return True
        except ImportError:
            return False

    if is_jsonschema_available():
        assert_command('{0} graceful_start'.format(os.path.join(IrodsConfig().top_level_directory, 'iRODS', 'irodsctl')),
                       'STDOUT_SINGLELINE', 'Success', env=env)
    else:
        assert_command('{0} graceful_start'.format(os.path.join(IrodsConfig().top_level_directory, 'iRODS', 'irodsctl')),
                       'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0, env=env)
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand('ils', 'STDOUT_SINGLELINE', admin_session.zone_name)

def restart_irods_server(env=None):
    stop_irods_server()
    start_irods_server(env=env)
