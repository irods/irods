import json
import os
import shutil
import subprocess
import tempfile
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..exceptions import IrodsError


def configure_password_storage_mode(value):
	# Update server config and reload configuration.
	config = IrodsConfig()
	config.server_config["user_password_storage_mode"] = value
	lib.update_json_file_from_dict(config.server_config_path, config.server_config)
	IrodsController(config).reload_configuration()


def authenticate_with_scheme(user_session, scheme_name, password, should_fail=False):
	# Get client environment file contents so that we can modify it.
	client_env = user_session.environment_file_contents

	# Modify client environment to use specified authentication scheme.
	client_env["irods_authentication_scheme"] = scheme_name
	user_session.environment_file_contents = client_env

	# Try to authenticate and make assertions as appropriate to the should_fail option.
	if should_fail:
		# Note: CAT_INVALID_USER might be returned for native here instead of AUTHENTICATION_ERROR...
		# This has been deemed an acceptable state of affairs.
		error_msg = "Error occurred while authenticating user"
		user_session.assert_icommand("iinit", "STDERR", error_msg, input=f"{password}\n")
	else:
		user_session.assert_icommand("iinit", "STDOUT", "iRODS password", input=f"{password}\n")
		# Expecting to see the home collection because the consequence of not setting a user's password when the
		# session is created is that the session will not create a session collection to manage for that user.
		user_session.assert_icommand("ils", "STDOUT", user_session.home_collection)

	# Run iexit to remove all the credentials files. We accept output on stdout here because the test is likely
	# running as the service account Linux user and this causes iexit to print a warning which does not apply here.
	user_session.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_modifying_user_password(unittest.TestCase):
	@classmethod
	def setUpClass(self):
		# Create an admin user / session for all the tests so we don't have to keep making it for each test.
		self.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		# Make a backup of the server_config and configure insecure mode for irods authentication scheme if the tests
		# are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			self.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
			shutil.copyfile(paths.server_config_path(), self.server_config_backup)
			with open(paths.server_config_path()) as f:
				server_config = json.load(f)
			server_config['plugin_configuration']['authentication']['irods'] = {
				'insecure_mode': True
			}
			lib.update_json_file_from_dict(paths.server_config_path(), server_config)
			IrodsController().reload_configuration()

	@classmethod
	def tearDownClass(self):
		# Restore the original server configuration if necessary.
		if False == test.settings.USE_SSL:
			shutil.copyfile(self.server_config_backup, paths.server_config_path())
			IrodsController().reload_configuration()

		# Exit the test admin session and remove the user.
		self.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(["iadmin", "rmuser", self.admin.username])

	def setUp(self):
		# Make a test user without a password. The method being called here will not authenticate the user because
		# there will not be a password set. This ensures that the user has no password set in any table.
		self.test_user = session.mkuser_and_return_session("rodsuser", "bilbo", None, lib.get_hostname())

	def tearDown(self):
		# Exit the test user session and remove the user.
		self.test_user.__exit__()
		self.admin.assert_icommand(["iadmin", "rmuser", self.test_user.username])

	def test_config_legacy_sets_password_for_native_scheme_but_not_irods_scheme(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "legacy"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_PASSWORD
			# but not R_USER_CREDENTIALS.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", scrambled_password])

				# Authenticate with native scheme - should succeed.
				authenticate_with_scheme(self.test_user, "native", should_fail=False, password=scrambled_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=scrambled_password)

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", non_scrambled_password, "no-scramble"])

				# Authenticate with native scheme - should succeed. Old password should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "native", should_fail=False, password=non_scrambled_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=non_scrambled_password)

		IrodsController().reload_configuration()

	def test_config_hashed_sets_password_for_irods_scheme_but_not_native_scheme(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "hashed"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_CREDENTIALS
			# but not R_USER_PASSWORD.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", scrambled_password])

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=scrambled_password)

				# Authenticate with irods scheme - should succeed.
				authenticate_with_scheme(self.test_user, "irods", should_fail=False, password=scrambled_password)

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", non_scrambled_password, "no-scramble"])

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=non_scrambled_password)

				# Authenticate with irods scheme - should succeed. Old password should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "irods", should_fail=False, password=non_scrambled_password)

		IrodsController().reload_configuration()

	def test_config_sets_password_for_both_native_and_irods_schemes(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "both"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_PASSWORD
			# and also R_USER_CREDENTIALS.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", scrambled_password])

				# Authenticate with native scheme - should succeed.
				authenticate_with_scheme(self.test_user, "native", should_fail=False, password=scrambled_password)

				# Authenticate with irods scheme - should succeed.
				authenticate_with_scheme(self.test_user, "irods", should_fail=False, password=scrambled_password)

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", non_scrambled_password, "no-scramble"])

				# Authenticate with native scheme - should succeed. Old password should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "native", should_fail=False, password=non_scrambled_password)

				# Authenticate with irods scheme - should succeed. Old password should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(self.test_user, "irods", should_fail=False, password=non_scrambled_password)

		IrodsController().reload_configuration()

	def test_config_both_with_long_passwords(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "both"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password to a very long string (257 characters) and try to send it in scrambled form.
			long_password = "1234567_" * 32 + "z"
			with self.subTest("send very long password in scrambled form"):
				# The password should be too long to scramble and neither password should be set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password],
					"STDERR", "-903000 PASSWORD_EXCEEDS_MAX_SIZE")

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=long_password)

			# Now do it again, but do not scramble it. This should still fail because the native password is scrambled.
			with self.subTest("send very long password in non-scrambled form"):
				# The password should be too long and neither password should be set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password, "no-scramble"],
					"STDERR", "-903000 PASSWORD_EXCEEDS_MAX_SIZE")

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=long_password)

		IrodsController().reload_configuration()

	def test_config_legacy_with_long_passwords(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "legacy"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password to a very long string (257 characters) and try to send it in scrambled form.
			long_password = "1234567_" * 32 + "z"
			with self.subTest("send very long password in scrambled form"):
				# The password should be too long to scramble and neither password should be set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password],
					"STDERR", "-903000 PASSWORD_EXCEEDS_MAX_SIZE")

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=long_password)

			# Now do it again, but do not scramble it. This should still fail because the native password is scrambled.
			with self.subTest("send very long password in non-scrambled form"):
				# The password should be too long and neither password should be set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password, "no-scramble"],
					"STDERR", "-903000 PASSWORD_EXCEEDS_MAX_SIZE")

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=long_password)

		IrodsController().reload_configuration()

	@unittest.skip("TODO(#8730): This test fails because we cannot handle passwords over 42 characters.")
	def test_config_hashed_with_long_passwords(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "hashed"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password to a very long string (257 characters) and try to send it in scrambled form.
			long_password = "1234567_" * 32 + "z"
			with self.subTest("send very long password in scrambled form"):
				# The password should be too long to scramble and neither password should be set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password],
					"STDERR", "-903000 PASSWORD_EXCEEDS_MAX_SIZE")

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(self.test_user, "irods", should_fail=True, password=long_password)

			# Now do it again, but do not scramble it. This should succeed because hashed passwords can be very long.
			with self.subTest("send very long password in non-scrambled form"):
				# The hashed password should be successfully set.
				self.admin.assert_icommand(
					["iadmin", "moduser", self.test_user.username, "password", long_password, "no-scramble"])

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(self.test_user, "native", should_fail=True, password=long_password)

				# Authenticate with irods scheme - should succeed.
				authenticate_with_scheme(self.test_user, "irods", should_fail=False, password=long_password)

		IrodsController().reload_configuration()


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_igroupadmin_mkuser(unittest.TestCase):
	@classmethod
	def setUpClass(self):
		# Create a groupadmin user / session for all the tests so we don't have to keep making it for each test.
		self.groupadmin = session.mkuser_and_return_session("groupadmin", "grouper", "gpass", lib.get_hostname())

		# Make a backup of the server_config and configure insecure mode for irods authentication scheme if the tests
		# are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			self.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
			shutil.copyfile(paths.server_config_path(), self.server_config_backup)
			with open(paths.server_config_path()) as f:
				server_config = json.load(f)
			server_config['plugin_configuration']['authentication']['irods'] = {
				'insecure_mode': True
			}
			lib.update_json_file_from_dict(paths.server_config_path(), server_config)
			IrodsController().reload_configuration()

		self.test_user_name = "bilbo"
		self.test_user_zone = self.groupadmin.zone_name

	@classmethod
	def tearDownClass(self):
		# Restore the original server configuration if necessary.
		if False == test.settings.USE_SSL:
			shutil.copyfile(self.server_config_backup, paths.server_config_path())
			IrodsController().reload_configuration()

		# Exit the test admin session and remove the user.
		self.groupadmin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.run_icommand(["iadmin", "rmuser", self.groupadmin.username])

	def tearDown(self):
		# Exit the test user session and remove the user, if the user was created.
		with session.make_session_for_existing_admin() as admin_session:
			if lib.user_exists(admin_session, self.test_user_name, self.test_user_zone):
				admin_session.run_icommand(["iadmin", "rmuser", self.test_user_name])

	def test_config_legacy_sets_password_for_native_scheme_but_not_irods_scheme(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "legacy"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_PASSWORD
			# but not R_USER_CREDENTIALS.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, scrambled_password, self.test_user_zone])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should succeed.
				authenticate_with_scheme(test_user, "native", should_fail=False, password=scrambled_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(test_user, "irods", should_fail=True, password=scrambled_password)

			# Remove the user so that we can create it again.
			with session.make_session_for_existing_admin() as admin_session:
				admin_session.assert_icommand(["iadmin", "rmuser", self.test_user_name])

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, non_scrambled_password, self.test_user_zone, "no-scramble"])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should succeed. Old password should fail.
				authenticate_with_scheme(test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "native", should_fail=False, password=non_scrambled_password)

				# Authenticate with irods scheme - should fail.
				authenticate_with_scheme(test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "irods", should_fail=True, password=non_scrambled_password)

		IrodsController().reload_configuration()

	def test_config_hashed_sets_password_for_irods_scheme_but_not_native_scheme(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "hashed"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_CREDENTIALS
			# but not R_USER_PASSWORD.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, scrambled_password, self.test_user_zone])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(test_user, "native", should_fail=True, password=scrambled_password)

				# Authenticate with irods scheme - should succeed.
				authenticate_with_scheme(test_user, "irods", should_fail=False, password=scrambled_password)

			# Remove the user so that we can create it again.
			with session.make_session_for_existing_admin() as admin_session:
				admin_session.assert_icommand(["iadmin", "rmuser", self.test_user_name])

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, non_scrambled_password, self.test_user_zone, "no-scramble"])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should fail.
				authenticate_with_scheme(test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "native", should_fail=True, password=non_scrambled_password)

				# Authenticate with irods scheme - should succeed. Old password should fail.
				authenticate_with_scheme(test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "irods", should_fail=False, password=non_scrambled_password)

		IrodsController().reload_configuration()

	def test_config_sets_password_for_both_native_and_irods_schemes(self):
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration.
			config.server_config["user_password_storage_mode"] = "both"
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			IrodsController(config).reload_configuration()

			# Set user password and send in scrambled form. This should set the user's password in R_USER_PASSWORD
			# and also R_USER_CREDENTIALS.
			scrambled_password = "scrambled_password"
			with self.subTest("send password in scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, scrambled_password, self.test_user_zone])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should succeed.
				authenticate_with_scheme(test_user, "native", should_fail=False, password=scrambled_password)

				# Authenticate with irods scheme - should succeed.
				authenticate_with_scheme(test_user, "irods", should_fail=False, password=scrambled_password)

			# Remove the user so that we can create it again.
			with session.make_session_for_existing_admin() as admin_session:
				admin_session.assert_icommand(["iadmin", "rmuser", self.test_user_name])

			# Now do it again, but do not scramble it. Also, make it something different to ensure the old password no
			# longer works.
			non_scrambled_password = "something_different"
			with self.subTest("send password in non-scrambled form"):
				self.groupadmin.assert_icommand(
					["igroupadmin", "mkuser", self.test_user_name, non_scrambled_password, self.test_user_zone, "no-scramble"])

				# Make a session for the test user so we can try to authenticate.
				test_user = session.make_session_for_existing_user(
					self.test_user_name, None, lib.get_hostname(), self.test_user_zone)

				# Authenticate with native scheme - should succeed. Old password should fail.
				authenticate_with_scheme(test_user, "native", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "native", should_fail=False, password=non_scrambled_password)

				# Authenticate with irods scheme - should succeed. Old password should fail.
				authenticate_with_scheme(test_user, "irods", should_fail=True, password=scrambled_password)
				authenticate_with_scheme(test_user, "irods", should_fail=False, password=non_scrambled_password)

		IrodsController().reload_configuration()


class test_invalid_configurations_and_options(unittest.TestCase):
	@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for this test.")
	def test_invalid_password_storage_mode_value_causes_configuration_reload_to_fail(self):
		# A test to see what the server does when the configuration is an invalid value cannot be created because the
		# server validates the configuration before starting. The invalid configuration causes the validation to fail
		# and so the configuration does not reload. If we could test this, we would expect setting user passwords to
		# fail for all cases and return an error because the configuration cannot direct what should happen.
		config = IrodsConfig()
		with lib.file_backed_up(config.server_config_path):
			# Update server config and reload configuration. This should cause an error.
			invalid_config = "banana"
			config.server_config["user_password_storage_mode"] = invalid_config
			lib.update_json_file_from_dict(config.server_config_path, config.server_config)
			res = subprocess.run(['irodsServer', '-c'], capture_output=True, text=True)
			self.assertNotEqual(res.returncode, 0)
			self.assertIn('"valid": false', res.stderr)
			self.assertIn('"instanceLocation": "/user_password_storage_mode"', res.stderr)
			self.assertIn(f'"error": "\'{invalid_config}\' is not a valid enum value."', res.stderr)

		IrodsController().reload_configuration()

	def test_igroupadmin_mkuser_no_scramble_option_requires_optional_zone_to_be_specified(self):
		user_name = "nobody"
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(
				["igroupadmin", "mkuser", user_name, "whatever", "no-scramble"],
				"STDERR", "-828000 CAT_INVALID_ZONE")

			self.assertFalse(lib.user_exists(admin_session, user_name))


class ipasswd_test_base(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		# Create an admin user / session for all the tests so we don't have to keep making it for each test.
		cls.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		cls.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
		shutil.copyfile(paths.server_config_path(), cls.server_config_backup)
		with open(paths.server_config_path()) as f:
			server_config = json.load(f)
		server_config["user_password_storage_mode"] = cls.password_storage_mode
		# Configure insecure mode for irods authentication scheme if the tests are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			server_config['plugin_configuration']['authentication']['irods'] = {'insecure_mode': True}
		# Update the server configuration and reload the configuration.
		lib.update_json_file_from_dict(paths.server_config_path(), server_config)
		IrodsController().reload_configuration()

		cls.old_password = "old_password"
		cls.new_password = "new_password"
		cls.ipasswd_input = "{}\n{}\n{}\n".format(cls.old_password, cls.new_password, cls.new_password)

	@classmethod
	def tearDownClass(cls):
		# Restore the original server configuration.
		shutil.copyfile(cls.server_config_backup, paths.server_config_path())
		IrodsController().reload_configuration()

		# Exit the test admin session and remove the user.
		cls.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(["iadmin", "rmuser", cls.admin.username])

	def setUp(self):
		# Make a test user and set the password to a known value with the password storage mode for this test class.
		# Do not set the password when creating the user because we do not want the user to authenticate right away.
		# Authenticating the user (or failing to authenticate) after setting the password is part of the tests.
		self.test_user = session.mkuser_and_return_session("rodsuser", "bilbo", None, lib.get_hostname())
		self.admin.assert_icommand(["iadmin", "moduser", self.test_user.username, "password", self.old_password])

	def tearDown(self):
		# Exit the test user session and remove the user.
		self.test_user.__exit__()
		self.admin.assert_icommand(["iadmin", "rmuser", self.test_user.username])
		IrodsController().reload_configuration()

	def configure_user_session_auth_scheme(self, auth_scheme):
		# Get client environment file contents so that we can modify it.
		client_env = self.test_user.environment_file_contents

		# Modify client environment to use specified authentication scheme.
		client_env["irods_authentication_scheme"] = auth_scheme
		self.test_user.environment_file_contents = client_env


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_ipasswd_with_both_passwords_set(ipasswd_test_base):

	password_storage_mode = "both"

	def test_password_storage_mode_legacy_and_auth_scheme_native(self):
		# Case 1
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only native password updated. Old irods password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_legacy_and_auth_scheme_irods(self):
		# Case 2
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only native password updated. Old irods password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_hashed_and_auth_scheme_native(self):
		# Case 3
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only irods password updated. Old native password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_and_auth_scheme_irods(self):
		# Case 4
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only irods password updated. Old native password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_and_auth_scheme_native(self):
		# Case 5
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that both passwords updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_both_and_auth_scheme_irods(self):
		# Case 6
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that both passwords updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_ipasswd_with_only_native_password_set(ipasswd_test_base):

	password_storage_mode = "legacy"

	def test_password_storage_mode_legacy_and_auth_scheme_native(self):
		# Case 7
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only native password updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_legacy_and_auth_scheme_irods(self):
		# Case 8
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with native scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_and_auth_scheme_native(self):
		# Case 9
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only irods password updated. Old native password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_and_auth_scheme_irods(self):
		# Case 10
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with native scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_and_auth_scheme_native(self):
		# Case 11
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("native")

			# Change password using native credentials to authenticate.
			self.test_user.assert_icommand(["ipasswd"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that both passwords updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_both_and_auth_scheme_irods(self):
		# Case 12
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with native scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_ipasswd_with_only_irods_password_set(ipasswd_test_base):

	password_storage_mode = "hashed"

	def test_password_storage_mode_legacy_and_auth_scheme_native(self):
		# Case 13
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with irods scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_legacy_and_auth_scheme_irods(self):
		# Case 14
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only native password updated. Old irods password should still work.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_password_storage_mode_hashed_and_auth_scheme_native(self):
		# Case 15
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with irods scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_and_auth_scheme_irods(self):
		# Case 16
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that only irods password updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_and_auth_scheme_native(self):
		# Case 17
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Can only authenticate with irods scheme using old password.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_and_auth_scheme_irods(self):
		# Case 18
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("irods")

			# Change password using irods credentials to authenticate. Note: --no-scramble option is required in
			# order to use irods auth scheme. Other tests cover using ipasswd with irods auth scheme and no
			# --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", input=self.ipasswd_input, desired_rc=0)

			# Confirm that both passwords updated.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=False)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=False)

	def test_ipasswd_with_session_token_forcibly_prompts_for_current_user_password(self):
		# This test ensures that ipasswd prompts the user for the current password when using the irods authentication
		# scheme rather than authenticating with a session token.
		irods_auth_password_prompt = "Enter your iRODS password:"
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("irods")

			# Authenticate the user so that we have a session token.
			self.test_user.assert_icommand(
				"iinit", "STDOUT", irods_auth_password_prompt, input=f"{self.old_password}\n")
			# Expecting to see the home collection because the consequence of not setting a user's password when the
			# session is created is that the session will not create a session collection to manage for that user.
			self.test_user.assert_icommand("ils", "STDOUT", self.test_user.home_collection)

			# Change password using irods credentials to authenticate. Ensure that the prompt for the user's current
			# password is shown and that it does not try to use the session token to authenticate. There are so many
			# other tests that confirm authenticating with the new password, so we just make sure no error occurs here.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"], "STDOUT", irods_auth_password_prompt, input=self.ipasswd_input, desired_rc=0)


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_ipasswd_with_no_password_set(unittest.TestCase):
	@classmethod
	def setUpClass(self):
		# Create an admin user / session for all the tests so we don't have to keep making it for each test.
		self.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		# Configure insecure mode for irods authentication scheme if the tests are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			self.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
			shutil.copyfile(paths.server_config_path(), self.server_config_backup)
			with open(paths.server_config_path()) as f:
				server_config = json.load(f)
			server_config['plugin_configuration']['authentication']['irods'] = {'insecure_mode': True}
			# Update the server configuration and reload the configuration.
			lib.update_json_file_from_dict(paths.server_config_path(), server_config)
			IrodsController().reload_configuration()

		self.old_password = "old_password"
		self.new_password = "new_password"
		self.ipasswd_input = "{}\n{}\n{}\n".format(self.old_password, self.new_password, self.new_password)

	@classmethod
	def tearDownClass(self):
		# Restore the original server configuration, if necessary.
		if False == test.settings.USE_SSL:
			shutil.copyfile(self.server_config_backup, paths.server_config_path())
			IrodsController().reload_configuration()

		# Exit the test admin session and remove the user.
		self.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(["iadmin", "rmuser", self.admin.username])

	def setUp(self):
		# Make a test user and do NOT set a password.
		self.test_user = session.mkuser_and_return_session("rodsuser", "bilbo", None, lib.get_hostname())

	def tearDown(self):
		# Exit the test user session and remove the user.
		self.test_user.__exit__()
		self.admin.assert_icommand(["iadmin", "rmuser", self.test_user.username])
		IrodsController().reload_configuration()

	def configure_user_session_auth_scheme(self, auth_scheme):
		# Get client environment file contents so that we can modify it.
		client_env = self.test_user.environment_file_contents

		# Modify client environment to use specified authentication scheme.
		client_env["irods_authentication_scheme"] = auth_scheme
		self.test_user.environment_file_contents = client_env

	def test_password_storage_mode_legacy_auth_scheme_native(self):
		# Case 19
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_legacy_auth_scheme_irods(self):
		# Case 20
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("legacy")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_auth_scheme_native(self):
		# Case 21
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_hashed_auth_scheme_irods(self):
		# Case 22
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("hashed")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_auth_scheme_native(self):
		# Case 23
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("native")

			# Attempt changing password using native credentials and fail to authenticate. No native password is set.
			self.test_user.assert_icommand(
				["ipasswd"], "STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)

	def test_password_storage_mode_both_auth_scheme_irods(self):
		# Case 24
		with lib.file_backed_up(paths.server_config_path()):
			configure_password_storage_mode("both")
			self.configure_user_session_auth_scheme("irods")

			# Attempt changing password using irods credentials and fail to authenticate. No irods password is set.
			# Note: --no-scramble option is required in order to use irods auth scheme. Other tests cover using ipasswd
			# with irods auth scheme and no --no-scramble option.
			self.test_user.assert_icommand(
				["ipasswd", "--no-scramble"],
				"STDERR", "Error occurred while authenticating user", input=self.ipasswd_input)

			# Confirm that nothing changed. Cannot authenticate as this user because no password is set.
			authenticate_with_scheme(self.test_user, "irods", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "irods", self.new_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.old_password, should_fail=True)
			authenticate_with_scheme(self.test_user, "native", self.new_password, should_fail=True)
