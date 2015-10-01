import json
import os
import shutil
import sys
import tempfile

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
import session


class Test_Irodsctl(unittest.TestCase):
    def test_re_shm_creation(self):
        assert session.re_shm_exists()

    def test_re_shm_cleanup(self):
        irodsctl_fullpath = os.path.join(session.get_irods_top_level_dir(), 'iRODS', 'irodsctl')
        session.assert_command([irodsctl_fullpath, 'stop'], 'STDOUT_SINGLELINE', 'Stopping iRODS server')
        assert not session.re_shm_exists(), session.re_shm_exists()
        session.start_irods_server()

    def test_configuration_schema_validation_from_file(self):
        schemas_git_dir = tempfile.mkdtemp(prefix='irods-test_configuration_schema_validation_from_file-git')
        with session.directory_deleter(schemas_git_dir):
            schemas_repo = 'https://github.com/irods/irods_schema_configuration'
            session.run_command(['git', 'clone', schemas_repo, schemas_git_dir])
            schemas_branch = 'v3'
            session.run_command(['git', 'checkout', schemas_branch], cwd=schemas_git_dir)
            schemas_deploy_dir = tempfile.mkdtemp(prefix='irods-test_configuration_schema_validation_from_file-schemas')
            with session.directory_deleter(schemas_deploy_dir):
                session.assert_command(['python', os.path.join(schemas_git_dir, 'deploy_schemas_locally.py'), '--output_directory_base', schemas_deploy_dir])
                with session.file_backed_up(os.path.join(session.get_irods_config_dir(), 'server_config.json')) as server_config_filename:
                    with open(server_config_filename) as f:
                        server_config = json.load(f)
                    server_config['schema_validation_base_uri'] = 'file://' + schemas_deploy_dir
                    session.update_json_file_from_dict(server_config_filename, server_config)
                    irodsctl_fullpath = os.path.join(session.get_irods_top_level_dir(), 'iRODS', 'irodsctl')

                    if session.is_jsonschema_installed():
                        expected_lines = ['Validating [{0}]... Success'.format(os.path.expanduser('~/.irods/irods_environment.json')),
                                          'Validating [{0}/server_config.json]... Success'.format(session.get_irods_config_dir()),
                                          'Validating [{0}/VERSION.json]... Success'.format(session.get_irods_top_level_dir()),
                                          'Validating [{0}/hosts_config.json]... Success'.format(session.get_irods_config_dir()),
                                          'Validating [{0}/host_access_control_config.json]... Success'.format(session.get_irods_config_dir())]
                        if not configuration.TOPOLOGY_FROM_RESOURCE_SERVER:
                            expected_lines.append('Validating [{0}/database_config.json]... Success'.format(session.get_irods_config_dir()))
                        session.assert_command([irodsctl_fullpath, 'restart'], 'STDOUT_MULTILINE', expected_lines)
                    else:
                        session.assert_command([irodsctl_fullpath, 'restart'], 'STDERR_SINGLELINE', 'jsonschema not installed', desired_rc=0)
