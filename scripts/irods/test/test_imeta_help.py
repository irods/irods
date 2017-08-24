import os
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .resource_suite import ResourceBase
from ..test.command import assert_command


class Test_ImetaHelp(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_ImetaHelp, self).setUp()

    def tearDown(self):
        super(Test_ImetaHelp, self).tearDown()

    def test_imeta_help(self):
        env_without_valid_environment_file = os.environ.copy()
        env_without_valid_environment_file['IRODS_ENVIRONMENT_FILE'] = '/invalid'
        env_without_valid_environment_file['IRODS_AUTHENTICATION_FILE'] = '/invalid'
        assert_command('imeta help', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help add', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help adda', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help addw', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help rm', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help rmw', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help rmi', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help mod', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help set', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help ls', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help lsw', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help qu', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help cp', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
        assert_command('imeta help upper', 'STDOUT_SINGLELINE', env=env_without_valid_environment_file)
