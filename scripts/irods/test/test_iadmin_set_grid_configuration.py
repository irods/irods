from __future__ import print_function

import unittest

from . import session
from .. import lib


class test_get_grid_configuration(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())

    @classmethod
    def tearDownClass(self):
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def test_no_namespace(self):
        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration'],
            'STDERR', 'Error: namespace must be between 1 and 2699 characters.')

    def test_really_long_namespace(self):
        # The input buffer to set_grid_configuration_value API is only 2700 characters long. If a value of 2700
        # characters or more is fed to the input struct for the set_pam_password_config API, packstruct gives an error.
        # iadmin will catch this case and show a slightly more presentable error, which is checked in this test.
        really_long_namespace = 'this_is_27_characters_long_' * 100
        option_name = 'password_max_time'

        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', really_long_namespace, option_name],
            'STDERR', 'Error: namespace must be between 1 and 2699 characters.')

    def test_nonexistent_namespace(self):
        bad_namespace = 'nopes'
        option_name = 'password_max_time'

        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', bad_namespace, option_name], 'STDERR',
            f'Failed to get grid configuration for namespace [{bad_namespace}] and option [{option_name}] [ec=-808000]')

    def test_no_option_name(self):
        namespace = 'authentication::native'
        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace],
            'STDERR', 'Error: option name must be between 1 and 2699 characters.')

    def test_really_long_option_name(self):
        # The input buffer to set_grid_configuration_value API is only 2700 characters long. If a value of 2700
        # characters or more is fed to the input struct for the set_pam_password_config API, packstruct gives an error.
        # iadmin will catch this case and show a slightly more presentable error, which is checked in this test.
        namespace = 'authentication::native'
        really_long_option_name = 'this_is_27_characters_long_' * 100

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, really_long_option_name],
            'STDERR', 'Error: option name must be between 1 and 2699 characters.')

    def test_nonexistent_option_name(self):
        namespace = 'authentication::native'
        bad_option_name = 'nopes'

        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, bad_option_name], 'STDERR',
            f'Failed to get grid configuration for namespace [{namespace}] and option [{bad_option_name}] [ec=-808000]')

    def test_get_grid_configuration_valid(self):
        namespace = 'authentication::native'
        option_name = 'password_max_time'

        # Assert that a value is returned and that there are no errors.
        self.admin.assert_icommand(['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')


class test_set_grid_configuration(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())

    @classmethod
    def tearDownClass(self):
        with session.make_session_for_existing_admin() as admin_session:
            self.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', self.admin.username])

    def test_no_namespace(self):
        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration'],
            'STDERR', 'Error: namespace must be between 1 and 2699 characters.')

    def test_really_long_namespace(self):
        # The input buffer to set_grid_configuration_value API is only 2700 characters long. If a value of 2700
        # characters or more is fed to the input struct for the set_pam_password_config API, packstruct gives an error.
        # iadmin will catch this case and show a slightly more presentable error, which is checked in this test.
        really_long_namespace = 'this_is_27_characters_long_' * 100
        option_name = 'password_max_time'
        option_value = '1000'

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', really_long_namespace, option_name, option_value],
            'STDERR', 'Error: namespace must be between 1 and 2699 characters.')

    def test_nonexistent_namespace(self):
        bad_namespace = 'nopes'
        option_name = 'password_max_time'
        option_value = '1000'

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', bad_namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{bad_namespace}] and option [{option_name}] [ec=-808000]')

    def test_no_option_name(self):
        namespace = 'authentication::native'
        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace],
            'STDERR', 'Error: option name must be between 1 and 2699 characters.')

    def test_really_long_option_name(self):
        # The input buffer to set_grid_configuration_value API is only 2700 characters long. If a value of 2700
        # characters or more is fed to the input struct for the set_pam_password_config API, packstruct gives an error.
        # iadmin will catch this case and show a slightly more presentable error, which is checked in this test.
        namespace = 'authentication::native'
        really_long_option_name = 'this_is_27_characters_long_' * 100
        option_value = '1000'

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, really_long_option_name, option_value],
            'STDERR', 'Error: option name must be between 1 and 2699 characters.')

    def test_nonexistent_option_name(self):
        namespace = 'authentication::native'
        bad_option_name = 'nopes'
        option_value = '1000'

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, bad_option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{bad_option_name}] [ec=-808000]')

    def test_no_option_value(self):
        namespace = 'authentication::native'
        option_name = 'password_max_time'
        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name],
            'STDERR', 'Error: option value must be between 1 and 2699 characters.')

    def test_really_long_option_value(self):
        namespace = 'authentication::native'
        option_name = 'password_max_time'

        # The input buffer to set_grid_configuration_value API is only 2700 characters long. If a value of 2700
        # characters or more is fed to the input struct for the set_pam_password_config API, packstruct gives an error.
        # iadmin will catch this case and show a slightly more presentable error, which is checked in this test.
        really_long_option_value = 'this_is_27_characters_long_' * 100

        original_value = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1]

        self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, really_long_option_value],
            'STDERR', 'Error: option value must be between 1 and 2699 characters.')

        # Assert that nothing changed due to the error.
        self.assertEqual(
            original_value,
            self.admin.assert_icommand(['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1])

    def test_set_grid_configuration_valid(self):
        namespace = 'authentication::native'
        option_name = 'password_max_time'

        original_value = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip()

        option_value = str(int(original_value) + 100)

        try:
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', namespace, option_name, option_value])

            # Assert that the value was updated appropriately.
            self.assertEqual(
                option_value,
                self.admin.assert_icommand(
                    ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip())

        finally:
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', namespace, option_name, original_value])

    def test_set_invalid_grid_configuration_with_option_name_that_is_protected_in_another_namespace(self):
        namespace = 'authentication::native'
        option_name = 'schema_version'

        # Make sure this namespace doesn't have the option_name used in the test...
        self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDERR',
            f'Failed to get grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-808000]')

        other_namespace = 'database'
        original_value = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', other_namespace, option_name], 'STDOUT')[1].strip()

        option_value = str(int(original_value) + 100)

        try:
            # The failure to set the configuration is due to the option name not existing in the namespace, not due to
            # its being protected in another namespace.
            self.admin.assert_icommand(
                ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
                f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-808000]')

            # Assert that the value was appropriately not updated.
            self.assertEqual(
                original_value,
                self.admin.assert_icommand(
                    ['iadmin', 'get_grid_configuration', other_namespace, option_name], 'STDOUT')[1].strip())

        finally:
            # Just in case, make super sure to put this back.
            self.admin.run_icommand(['iadmin', 'set_grid_configuration', namespace, option_name, original_value])

    def test_set_delay_server_leader_is_protected(self):
        namespace = 'delay_server'
        option_name = 'leader'
        option_value = 'followme'

        original_leader = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip()

        self.assertNotEqual(option_value, original_leader)

        _, out, _ = self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-169000]')
        self.assertIn('Specified grid configuration is not allowed to be modified.', out)

        # Assert that the value was appropriately not updated.
        self.assertEqual(
            original_leader,
            self.admin.assert_icommand(
                ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip())

    def test_set_delay_server_successor_is_protected(self):
        namespace = 'delay_server'
        option_name = 'successor'
        option_value = 'followmenext'

        original_successor = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip()

        self.assertNotEqual(option_value, original_successor)

        _, out, _ = self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-169000]')
        self.assertIn('Specified grid configuration is not allowed to be modified.', out)

        # Assert that the value was appropriately not updated.
        self.assertEqual(
            original_successor,
            self.admin.assert_icommand(
                ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip())

    def test_set_delay_server_namespace_is_protected_even_with_invalid_option_name(self):
        namespace = 'delay_server'
        option_name = 'follower'
        option_value = 'iwillfollow'

        # The error message will remain the same because the namespace is protected.
        _, out, _ = self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-169000]')
        self.assertIn('Specified grid configuration is not allowed to be modified.', out)

    def test_set_delay_server_namespace_is_protected_even_with_option_name_from_unprotected_namespaces(self):
        namespace = 'delay_server'
        other_namespace = 'authentication::native'
        option_name = 'password_max_time'
        option_value = 'shenanigans!'

        original_other_namespace_value = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', other_namespace, option_name], 'STDOUT')[1].strip()

        _, out, _ = self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-169000]')
        self.assertIn('Specified grid configuration is not allowed to be modified.', out)

        # Assert that the value was appropriately not updated in the other namespace.
        self.assertEqual(
            original_other_namespace_value,
            self.admin.assert_icommand(
                ['iadmin', 'get_grid_configuration', other_namespace, option_name], 'STDOUT')[1].strip())

    def test_set_database_schema_version_is_disallowed(self):
        namespace = 'database'
        option_name = 'schema_version'
        option_value = '9001'

        original_value = self.admin.assert_icommand(
            ['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip()

        # Make sure the value is not set to our ridiculous test value.
        self.assertNotEqual(option_value, original_value)

        _, out, _ = self.admin.assert_icommand(
            ['iadmin', 'set_grid_configuration', namespace, option_name, option_value], 'STDERR',
            f'Failed to set grid configuration for namespace [{namespace}] and option [{option_name}] [ec=-169000]')
        self.assertIn('Specified grid configuration is not allowed to be modified.', out)

        # Assert that the value was appropriately not updated.
        self.assertEqual(
            original_value,
            self.admin.assert_icommand(['iadmin', 'get_grid_configuration', namespace, option_name], 'STDOUT')[1].strip())

    @unittest.skip('This situation cannot happen with the current protected configs. Write this test if/when it can.')
    def test_set_mutable_grid_configuration_with_option_name_that_is_protected_in_another_namespace(self):
        # This is the situation:
        #   R_GRID_CONFIGRUATION
        #                   namespace           |       option_name        | option_value
        #         ------------------------------+--------------------------+--------------
        #          namespace1                   | option1                  | will_not_change
        #          namespace1                   | option2                  | can_change
        #          namespace2                   | option1                  | can_change
        #          namespace2                   | option2                  | will_not_change
        #
        #   Here is the deny list:
        #       | namespace  | option_name |
        #       | ---------- | ----------- |
        #       | namespace1 | option1     |
        #       | namespace2 | option2     |
        #
        #   And then this is run:
        #   $ iadmin set_grid_configuration namespace2 option1 what_happens_now
        #
        #   Only option2 is protected for namespace namespace2. Need to make sure that option1 is allowed to be modified
        #   even though it is in the deny list for the namespace1 namespace.
        pass

