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
from .. import paths
from ..controller import IrodsController
from ..configuration import IrodsConfig


class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_creation(self):
        if 'irods_rule_engine_plugin-irods_rule_language' in IrodsConfig().configured_rule_engine_plugins:
            assert lib.re_shm_exists()

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(IrodsConfig().irods_directory, 'irodsctl')
        assert_command([irodsctl_fullpath, 'stop'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
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
                                    'Validating [{0}/VERSION.json]... Success'.format(IrodsConfig().irods_directory),
                                    'Validating [{0}/hosts_config.json]... Success'.format(IrodsConfig().config_directory),
                                    'Validating [{0}/host_access_control_config.json]... Success'.format(IrodsConfig().config_directory)]
                assert_command([irodsctl_fullpath, 'restart'], 'STDOUT_MULTILINE', expected_lines)
            else:
                assert_command([irodsctl_fullpath, 'restart'], 'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0)

def stop_irods_server():
    assert_command([os.path.join(IrodsConfig().irods_directory, 'irodsctl'), 'stop'],
                   'STDOUT_SINGLELINE', 'Success')

def start_irods_server(env=None):
    def is_jsonschema_available():
        try:
            import jsonschema
            return True
        except ImportError:
            return False

    if is_jsonschema_available():
        assert_command('{0} graceful_start'.format(os.path.join(IrodsConfig().irods_directory, 'irodsctl')),
                       'STDOUT_SINGLELINE', 'Success', env=env)
    else:
        assert_command('{0} graceful_start'.format(os.path.join(IrodsConfig().irods_directory, 'irodsctl')),
                       'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0, env=env)
    with make_session_for_existing_admin() as admin_session:
        admin_session.assert_icommand('ils', 'STDOUT_SINGLELINE', admin_session.zone_name)

def restart_irods_server(env=None):
    stop_irods_server()
    start_irods_server(env=env)
