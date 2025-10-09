import json
import os
import shutil
import tempfile
import time
import unittest

from . import session
from .. import lib
from .. import paths
from .. import test
from ..controller import IrodsController


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_session_token_lifetime_configuration(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		cls.authentication_scheme = "irods"
		cls.configuration_namespace = "authentication"
		cls.configuration_option_name = "token_lifetime_in_seconds"

		# Configure password storage mode to set both native and irods auth passwords.
		cls.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
		shutil.copyfile(paths.server_config_path(), cls.server_config_backup)
		with open(paths.server_config_path()) as f:
			server_config = json.load(f)
		# Configure insecure mode for irods authentication scheme if the tests are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			server_config['plugin_configuration']['authentication']['irods'] = {
				'insecure_mode': True
			}
		server_config["user_password_storage_mode"] = "both"
		lib.update_json_file_from_dict(paths.server_config_path(), server_config)
		IrodsController().reload_configuration()

		# Create an admin user / session for all the tests so we don't have to keep making sessions for each test.
		cls.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		# Create a test user and set the user's password. This should allow for authenticating both with native and
		# irods auth schemes due to the user_password_storage_mode configuration being set to "both" above.
		cls.test_user = session.mkuser_and_return_session("rodsuser", "bilbo", "baggins", lib.get_hostname())

		# Change the user's authentication scheme to irods authentication because that's what all of these tests use.
		client_update = {"irods_authentication_scheme": cls.authentication_scheme}
		cls.test_user.environment_file_contents.update(client_update)

	@classmethod
	def tearDownClass(cls):
		# Restore the original server configuration if necessary.
		shutil.copyfile(cls.server_config_backup, paths.server_config_path())
		IrodsController().reload_configuration()

		# Exit the test admin and test user session and remove the user. Have to authenticate using native
		# authentication because all the sessions tooling in tests make assumptions.
		client_update = {"irods_authentication_scheme": "native"}
		cls.test_user.environment_file_contents.update(client_update)
		cls.test_user.assert_icommand(
			["iinit"], "STDOUT", "iRODS password", input=f"{cls.test_user.password}\n")
		cls.test_user.__exit__()
		cls.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(["iadmin", "rmuser", cls.test_user.username])
			admin_session.assert_icommand(["iadmin", "rmuser", cls.admin.username])

	def test_invalid_session_token_lifetime_configurations_result_in_using_default_value(self):
		# Stash away the original configuration for later...
		original_config = self.admin.assert_icommand(
				["iadmin", "get_grid_configuration", self.configuration_namespace, self.configuration_option_name], "STDOUT")[1].strip()

		try:
			for option_value in [" ", "banana", str(-1), str(0), str(2147483648), str(-2147483648)]:
				with self.subTest(f"test with value [{option_value}]"):
					# Set the grid configuration to the desired value and reload configuration.
					self.admin.assert_icommand(
						["iadmin", "set_grid_configuration", "--", self.configuration_namespace, self.configuration_option_name, option_value])
					IrodsController().reload_configuration()

					# These invalid configurations will not cause any errors, but default values will be used.
					self.test_user.assert_icommand(
						["iinit"], "STDOUT", "iRODS password", input=f"{self.test_user.password}\n")
					self.test_user.assert_icommand(["ils"], "STDOUT", self.test_user.session_collection)

					# We cannot use GenQuery to check session token expirations and we are not waiting around 2 weeks
					# for it to expire. Just be content that the session token works for now.

					# Run iexit to remove all the credentials files. We accept output on stdout here because the test is
					# likely running as the service account Linux user and this causes iexit to print a warning which
					# does not apply here.
					self.test_user.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")

		finally:
			# Restore the original grid configuration.
			self.admin.assert_icommand(
				["iadmin", "set_grid_configuration", self.configuration_namespace, self.configuration_option_name, original_config])
			IrodsController().reload_configuration()


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_password_authentication_returning_session_tokens(unittest.TestCase):
	@classmethod
	def setUpClass(self):
		self.authentication_scheme = "irods"
		self.configuration_namespace = "authentication"
		self.configuration_option_name = "token_lifetime_in_seconds"
		# Set this to a short value so we don't have to wait very long for the session tokens to expire.
		self.token_lifetime_value = 3

		# Create an admin user / session for all the tests so we don't have to keep making sessions for each test.
		self.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		# Configure password storage mode to set both native and irods auth passwords.
		self.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
		shutil.copyfile(paths.server_config_path(), self.server_config_backup)
		with open(paths.server_config_path()) as f:
			server_config = json.load(f)
		# Configure insecure mode for irods authentication scheme if the tests are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			server_config['plugin_configuration']['authentication']['irods'] = {
				'insecure_mode': True
			}
		server_config["user_password_storage_mode"] = "both"
		lib.update_json_file_from_dict(paths.server_config_path(), server_config)

		# Set the grid configuration to the desired value and reload configuration.
		self.original_token_lifetime = self.admin.assert_icommand(
			["iadmin", "get_grid_configuration", self.configuration_namespace, self.configuration_option_name], "STDOUT")[1].strip()
		self.admin.assert_icommand(
			["iadmin", "set_grid_configuration", self.configuration_namespace, self.configuration_option_name, str(self.token_lifetime_value)])
		IrodsController().reload_configuration()

		# Create a test user and set the user's password. This should allow for authenticating both with native and
		# irods auth schemes due to the user_password_storage_mode configuration being set to "both" above.
		self.test_rodsadmin = session.mkuser_and_return_session(
			"rodsadmin", "test_rodsadmin", "rodsadminpass", lib.get_hostname())
		self.test_rodsuser = session.mkuser_and_return_session(
			"rodsuser", "test_rodsuser", "rodsuserpass", lib.get_hostname())
		self.test_groupadmin = session.mkuser_and_return_session(
			"groupadmin", "test_groupadmin", "groupadminpass", lib.get_hostname())

		# Change the user's authentication scheme to irods authentication because that's what all of these tests use.
		client_update = {"irods_authentication_scheme": self.authentication_scheme}
		self.test_rodsadmin.environment_file_contents.update(client_update)
		self.test_rodsuser.environment_file_contents.update(client_update)
		self.test_groupadmin.environment_file_contents.update(client_update)

	@classmethod
	def tearDownClass(self):
		# Restore the original grid configuration and server configuration.
		shutil.copyfile(self.server_config_backup, paths.server_config_path())
		self.admin.assert_icommand(
			["iadmin", "set_grid_configuration", self.configuration_namespace, self.configuration_option_name, self.original_token_lifetime])
		IrodsController().reload_configuration()

		# Exit test user sessions and remove the users. Have to re-authenticate because the users may not have
		# valid sessions after the tests complete and this is required to exit the sessions. Have to use native
		# authentication because all the sessions tooling in tests make assumptions.
		client_update = {"irods_authentication_scheme": "native"}
		self.test_rodsadmin.environment_file_contents.update(client_update)
		self.test_rodsadmin.assert_icommand(
			["iinit"], "STDOUT", "iRODS password", input=f"{self.test_rodsadmin.password}\n")
		self.test_rodsuser.environment_file_contents.update(client_update)
		self.test_rodsuser.assert_icommand(
			["iinit"], "STDOUT", "iRODS password", input=f"{self.test_rodsuser.password}\n")
		self.test_groupadmin.environment_file_contents.update(client_update)
		self.test_groupadmin.assert_icommand(
			["iinit"], "STDOUT", "iRODS password", input=f"{self.test_groupadmin.password}\n")
		self.test_rodsadmin.__exit__()
		self.test_rodsuser.__exit__()
		self.test_groupadmin.__exit__()
		self.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			admin_session.assert_icommand(["iadmin", "rmuser", self.test_rodsadmin.username])
			admin_session.assert_icommand(["iadmin", "rmuser", self.test_rodsuser.username])
			admin_session.assert_icommand(["iadmin", "rmuser", self.test_groupadmin.username])
			admin_session.assert_icommand(["iadmin", "rmuser", self.admin.username])

	def tearDown(self):
		# Run iexit to remove all the credentials files. We accept output on stdout here because the test is
		# likely running as the service account Linux user and this causes iexit to print a warning which
		# does not apply here.
		self.test_rodsadmin.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")
		self.test_rodsuser.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")
		self.test_groupadmin.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")

	def do_test_session_tokens_are_not_returned_when_password_is_incorrect(self, user):
		bad_password = "that_aint_right"

		# Attempt to authenticate using an incorrect password and fail.
		out, err, rc = user.run_icommand("iinit", input=f"{bad_password}\n")
		self.assertNotEqual(0, rc)
		self.assertIn("iRODS password", out)
		self.assertIn("AUTHENTICATION_ERROR", err)
		self.assertIn("ec=-178000", err)

		# Run another iCommand to ensure that no session token was returned. The fact that it asks for a password
		# indicates that no session token was recorded by iinit, so that is all we need to assert here.
		out, err, rc = user.run_icommand("ils", input=f"{bad_password}\n")
		self.assertNotEqual(0, rc)
		self.assertIn("iRODS password", out)
		self.assertIn("AUTHENTICATION_ERROR", err)
		self.assertIn("ec=-178000", err)

	def test_session_tokens_are_not_returned_when_password_is_incorrect_for_rodsadmins(self):
		self.do_test_session_tokens_are_not_returned_when_password_is_incorrect(self.test_rodsadmin)

	def test_session_tokens_are_not_returned_when_password_is_incorrect_for_rodsuser(self):
		self.do_test_session_tokens_are_not_returned_when_password_is_incorrect(self.test_rodsuser)

	def test_session_tokens_are_not_returned_when_password_is_incorrect_for_groupadmin(self):
		self.do_test_session_tokens_are_not_returned_when_password_is_incorrect(self.test_groupadmin)

	def do_test_expiring_session_tokens_returned_when_password_is_correct(self, user):
		# Successfully authenticate and run a command using the returned session token.
		user.assert_icommand(["iinit"], "STDOUT", "iRODS password", input=f"{user.password}\n")
		user.assert_icommand(["ils"], "STDOUT", user.session_collection)

		# Now wait until the token is expired and try to use it again - this should fail.
		time.sleep(self.token_lifetime_value + 1)
		user.assert_icommand(["ils"], "STDERR_MULTILINE", ["AUTHENTICATION_ERROR", "ec=-178000"])

	def test_expiring_session_tokens_returned_when_password_is_correct_for_rodsadmin(self):
		self.do_test_expiring_session_tokens_returned_when_password_is_correct(self.test_rodsadmin)

	def test_expiring_session_tokens_returned_when_password_is_correct_for_rodsuser(self):
		self.do_test_expiring_session_tokens_returned_when_password_is_correct(self.test_rodsuser)

	def test_expiring_session_tokens_returned_when_password_is_correct_for_groupadmin(self):
		self.do_test_expiring_session_tokens_returned_when_password_is_correct(self.test_groupadmin)

	def test_rodsadmins_can_get_nonexpiring_session_tokens(self):
		# Successfully authenticate and run a command using the returned session token.
		self.test_rodsadmin.assert_icommand(
			["iinit", "--no-token-expiration"], "STDOUT", "iRODS password", input=f"{self.test_rodsadmin.password}\n")
		self.test_rodsadmin.assert_icommand(["ils"], "STDOUT", self.test_rodsadmin.session_collection)

		# Now wait until the token would have expired if it did expire, and try to use it again. This should succeed.
		time.sleep(self.token_lifetime_value + 1)
		self.test_rodsadmin.assert_icommand(["ils"], "STDOUT", self.test_rodsadmin.session_collection)

	def do_test_non_rodsadmin_try_to_get_nonexpiring_session_token_returns_no_token(self, user):
		# Try to authenticate and request a session token which does not expire. This should reject the authentication
		# request even if the password is correct.
		out, err, rc = user.run_icommand(["iinit", "--no-token-expiration"], input=f"{user.password}\n")
		self.assertNotEqual(0, rc)
		self.assertIn("iRODS password", out)
		self.assertIn("SYS_NOT_ALLOWED", err)
		self.assertIn("ec=-169000", err)

		# Run another iCommand to ensure that no session token was returned. The fact that it asks for a password
		# indicates that no session token was recorded by iinit, so that is all we need to assert here.
		out, err, rc = user.run_icommand("ils", input="does_not_matter")
		self.assertNotEqual(0, rc)
		self.assertIn("iRODS password", out)
		self.assertIn("AUTHENTICATION_ERROR", err)
		self.assertIn("ec=-178000", err)

	def test_rodsusers_fail_to_authenticate_when_trying_to_get_nonexpiring_session_tokens(self):
		self.do_test_non_rodsadmin_try_to_get_nonexpiring_session_token_returns_no_token(self.test_rodsuser)

	def test_groupadmins_fail_to_authenticate_when_trying_to_get_nonexpiring_session_tokens(self):
		self.do_test_non_rodsadmin_try_to_get_nonexpiring_session_token_returns_no_token(self.test_groupadmin)


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "Must configure catalog provider for these tests.")
class test_remove_session_tokens(unittest.TestCase):
	@classmethod
	def setUpClass(self):
		# Create an admin user / session for all the tests so we don't have to keep making it for each test.
		self.admin = session.mkuser_and_return_session("rodsadmin", "otherrods", "rods", lib.get_hostname())

		# Configure password storage mode to set both native and irods auth passwords.
		self.server_config_backup = tempfile.NamedTemporaryFile(prefix=os.path.basename(paths.server_config_path())).name
		shutil.copyfile(paths.server_config_path(), self.server_config_backup)
		with open(paths.server_config_path()) as f:
			server_config = json.load(f)
		server_config["user_password_storage_mode"] = "both"
		# Configure insecure mode for irods authentication scheme if the tests are not being run with TLS enabled.
		if False == test.settings.USE_SSL:
			server_config['plugin_configuration']['authentication']['irods'] = {'insecure_mode': True}
		# Update the server configuration and reload the configuration.
		lib.update_json_file_from_dict(paths.server_config_path(), server_config)

		# Configure the session token lifetime to something short so we can test in a reasonable amount of time.
		self.token_lifetime_value = 5
		self.admin.assert_icommand(
			["iadmin", "set_grid_configuration", "authentication", "token_lifetime_in_seconds", str(self.token_lifetime_value)])

		IrodsController().reload_configuration()

		# Create test user sessions.
		self.test_users = []
		for username in ["bilbo", "smeagol"]:
			self.test_users.append(session.mkuser_and_return_session("rodsuser", username, f"{username}_pass", lib.get_hostname()))

		# Change the user's authentication scheme to irods authentication because that's what all of these tests use.
		self.authentication_scheme = "irods"
		client_update = {"irods_authentication_scheme": self.authentication_scheme}
		for user in self.test_users:
			user.environment_file_contents.update(client_update)

		# Add a specific query so that we can see what session tokens exist for this user.
		query = "select session_expiry_ts from R_USER_SESSION_KEY " \
				"where user_id = (" \
				"select user_id from R_USER_MAIN where user_name = '{}' and zone_name = '{}'" \
				")"
		for user in self.test_users:
			self.admin.assert_icommand(
				["iadmin", "asq", query.format(user.username, user.zone_name), f"get_session_tokens_for_{user.username}"])

	@classmethod
	def tearDownClass(self):
		# Restore the original server configuration.
		shutil.copyfile(self.server_config_backup, paths.server_config_path())
		IrodsController().reload_configuration()

		# Exit the test admin and test user session and remove the user. Have to authenticate using native
		# authentication because all the sessions tooling in tests make assumptions.
		client_update = {"irods_authentication_scheme": "native"}
		for user in self.test_users:
			self.admin.run_icommand(["iadmin", "rsq", f"get_session_tokens_for_{user.username}"])
			user.environment_file_contents.update(client_update)
			user.assert_icommand(["iinit"], "STDOUT", "iRODS password", input=f"{user.password}\n")
			user.__exit__()
		self.admin.__exit__()
		with session.make_session_for_existing_admin() as admin_session:
			for user in self.test_users:
				admin_session.assert_icommand(["iadmin", "rmuser", user.username])
			admin_session.assert_icommand(["iadmin", "rmuser", self.admin.username])

	def setUp(self):
		# Authenticate as the test users at the beginning of each test so that we have some session tokens.
		for user in self.test_users:
			user.assert_icommand(["iinit"], "STDOUT", "iRODS password", input=f"{user.password}\n")

	def tearDown(self):
		# Run iexit to remove all the credentials files. We accept output on stdout here because the test is likely
		# running as the service account Linux user and this causes iexit to print a warning which does not apply here.
		for user in self.test_users:
			user.assert_icommand(["iexit", "--remove-session-token-file", "-f"], "STDOUT")
			self.admin.assert_icommand(["iadmin", "remove_session_tokens", "all", user.username])

	def get_session_tokens_for_user(self, user):
		session_tokens = self.admin.assert_icommand(
			["iquest", "--sql", f"get_session_tokens_for_{user.username}"], "STDOUT")[1].strip()
		if "CAT_NO_ROWS_FOUND" in session_tokens:
			return []
		return session_tokens.split("\n----\n")

	def test_error_occurs_when_no_type_is_specified(self):
		self.admin.assert_icommand(["iadmin", "remove_session_tokens"], "STDERR", "-130000 SYS_INVALID_INPUT_PARAM")

	def test_error_occurs_when_invalid_type_is_specified(self):
		self.admin.assert_icommand(
			["iadmin", "remove_session_tokens", "nope"], "STDERR", "-130000 SYS_INVALID_INPUT_PARAM")

	def test_error_occurs_when_specified_user_name_is_incorrectly_formatted(self):
		self.admin.assert_icommand(
			["iadmin", "remove_session_tokens", "all", "nope##"], "STDERR", "Invalid username format.")

	def test_nothing_happens_when_specified_user_name_does_not_exist(self):
		# Create a map of test usernames to lists of session tokens.
		session_tokens = {user.username: self.get_session_tokens_for_user(user) for user in self.test_users}
		# Ensure that each test user has one session token.
		for _, tokens in session_tokens.items():
			self.assertEqual(1, len(tokens))

		# The user specified does not exist, so no session tokens will be deleted.
		self.admin.assert_icommand(["iadmin", "remove_session_tokens", "all", "nope"])

		# No session tokens should have been deleted.
		for user in self.test_users:
			self.assertEqual(session_tokens[user.username], self.get_session_tokens_for_user(user))

	def test_user_name_with_no_zone_name_succeeds(self):
		# Ensure that the user has session tokens before attempting to delete.
		self.assertEqual(1, len(self.get_session_tokens_for_user(self.test_users[0])))

		# The zone name being empty just means that the local zone will be used. The specified user exists in the
		# local zone, so all of that user's session tokens will be deleted.
		self.admin.assert_icommand(["iadmin", "remove_session_tokens", "all", self.test_users[0].username])

		# Ensure that the user's session tokens are all deleted.
		self.assertEqual(0, len(self.get_session_tokens_for_user(self.test_users[0])))

	def test_user_name_with_empty_zone_name_succeeds(self):
		# Create a map of test usernames to lists of session tokens.
		session_tokens = {user.username: self.get_session_tokens_for_user(user) for user in self.test_users}
		# Ensure that each test user has one session token.
		for _, tokens in session_tokens.items():
			self.assertEqual(1, len(tokens))

		# The zone name being empty just means that the local zone will be used. The specified user exists in the
		# local zone, so all of that user's session tokens will be deleted.
		self.admin.assert_icommand(["iadmin", "remove_session_tokens", "all", f"{self.test_users[0].username}#"])

		# Ensure that the user's session tokens are all deleted.
		self.assertEqual(0, len(self.get_session_tokens_for_user(self.test_users[0])))
		# Ensure that other test user's session tokens remain unchanged.
		for user in self.test_users[1:]:
			self.assertEqual(session_tokens[user.username], self.get_session_tokens_for_user(user))

	def test_user_name_with_zone_name_succeeds(self):
		# Create a map of test usernames to lists of session tokens.
		session_tokens = {user.username: self.get_session_tokens_for_user(user) for user in self.test_users}
		# Ensure that each test user has one session token.
		for _, tokens in session_tokens.items():
			self.assertEqual(1, len(tokens))

		# The specified user exists in the specified zone, so all of that user's session tokens will be deleted.
		self.admin.assert_icommand(
			["iadmin", "remove_session_tokens", "all", f"{self.test_users[0].username}#{self.test_users[0].zone_name}"])

		# Ensure that the user's session tokens are all deleted.
		self.assertEqual(0, len(self.get_session_tokens_for_user(self.test_users[0])))
		# Ensure that other test user's session tokens remain unchanged.
		for user in self.test_users[1:]:
			self.assertEqual(session_tokens[user.username], self.get_session_tokens_for_user(user))

	def test_remove_all_session_tokens_for_all_users(self):
		# Create a map of test usernames to lists of session tokens.
		session_tokens = {user.username: self.get_session_tokens_for_user(user) for user in self.test_users}
		# Ensure that each test user has one session token.
		for user in self.test_users:
			self.assertEqual(1, len(session_tokens[user.username]))

		# Delete all session tokens for all users.
		self.admin.assert_icommand(["iadmin", "remove_session_tokens", "all"])

		# Ensure that all of the session tokens are gone.
		for user in self.test_users:
			self.assertEqual(0, len(self.get_session_tokens_for_user(user)))

	def test_remove_expired_session_tokens_for_all_users(self):
		time_between_expiration_and_making_new_tokens = 2

		# Let the existing session tokens almost expire by waiting until just before they expire.
		time.sleep(self.token_lifetime_value - time_between_expiration_and_making_new_tokens)

		# Before the existing session tokens expire, make some fresh session tokens. This should not delete the other
		# session tokens because they have not expired yet, hopefully.
		for user in self.test_users:
			user.assert_icommand(["iinit"], "STDOUT", "iRODS password", input=f"{user.password}\n")

		# Create a map of test usernames to lists of session tokens.
		session_tokens = {user.username: self.get_session_tokens_for_user(user) for user in self.test_users}
		# Ensure that each test user has two session tokens.
		for user in self.test_users:
			self.assertEqual(2, len(session_tokens[user.username]))

		# Let the first session tokens expire.
		time.sleep(time_between_expiration_and_making_new_tokens)

		# Delete expired session tokens for all users.
		self.admin.assert_icommand(["iadmin", "remove_session_tokens", "expired"])

		# Ensure that only expired session tokens are gone.
		for user in self.test_users:
			self.assertEqual(1, len(self.get_session_tokens_for_user(user)))
