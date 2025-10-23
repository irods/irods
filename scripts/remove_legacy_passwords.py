import argparse
import contextlib
import logging
import sys
import textwrap

from irods import database_connect
from irods.configuration import IrodsConfig


# In iRODS 5.1.0, a new form of password storage in a new table called R_USER_CREDENTIALS has been added, and is meant
# to replace the legacy iRODS passwords stored in R_USER_PASSWORD. This script was created to aid in removing these
# legacy passwords from the catalog. Please note that removing the passwords from R_USER_PASSWORD means that they can
# no longer be used to authenticate users. If an administrator wishes to remove all passwords from R_USER_PASSWORD, it
# should be pretty easy to do that without this script: "DELETE FROM r_user_password".


def configure_logs(verbosity=1):
	"""Configure the standard Python logger.

	Arguments:
	verbosity -- Used to calculate log level. Higher value means more log messages.
	"""
	# CRITICAL messages will always be printed, but anything after that is a function of the number of -v
	level = logging.CRITICAL - 10 * verbosity

	handlers = [logging.StreamHandler(sys.stdout)]

	logging.basicConfig(
		level = level if level > logging.NOTSET else logging.DEBUG,
		format = "%(asctime)-15s - %(message)s",
		handlers = handlers
	)


class user:
	"""
	Holds user name and zone name for an iRODS user.
	"""
	def __init__(self, username_input):
		self.user_name, self.zone_name = user.parse_user_name(username_input)

	@staticmethod
	def parse_user_name(username_input, use_local_zone_by_default=True):
		"""Parse username_input into its username and zone name parts as a fully-qualified iRODS username."""
		logging.info("username_input input:[%s]" % username_input)
		parts = username_input.split("#")

		# If only 1 part was made, assume it is the username and no zone name was specified. If no zone name was
		# specified, use the local zone from the local server config.
		if 1 == len(parts) and use_local_zone_by_default:
			parts.append(IrodsConfig().server_config["zone_name"])

		# If there is not both a user name and a zone name, bail.
		if 2 != len(parts):
			raise ValueError(f"Username must be properly formatted. input: [{username_input}]")

		return parts[0], parts[1]


class delete_passwords_sql_builder():
	_user = None
	_forever_passwords_only = False

	def __init__(self):
		self.reset()

	def reset(self):
		"""Reset the SQL builder."""
		self._user = None
		self._forever_passwords_only = False

	def set_user(self, username_input):
		"""Set the user information for the query."""
		self._user = user(username_input)
		return self

	def set_forever_passwords_only(self, forever_passwords_only):
		"""Set the 'forever passwords only' flag."""
		self._forever_passwords_only = forever_passwords_only
		return self

	def get(self):
		"""Construct and return the SQL statement string."""
		logging.info("user_name:[%s], zone_name:[%s]" % (self._user.user_name, self._user.zone_name))
		sql = (
			"DELETE FROM R_USER_PASSWORD "
			"WHERE user_id=(SELECT user_id FROM R_USER_MAIN WHERE user_name='{}' AND zone_name='{}')"
			.format(self._user.user_name, self._user.zone_name)
		)
		if self._forever_passwords_only:
			sql += " AND pass_expiry_ts LIKE '9999-%'"
		return sql


def execute_sql(sql, dry_run=False):
	"""Execute the provided sql, optionally skipping the commit if dry_run is specified."""
	try:
		with contextlib.closing(database_connect.get_database_connection(IrodsConfig())) as connection:
			connection.autocommit = False
			with contextlib.closing(connection.cursor()) as cursor:
				logging.info("Executing SQL: %s" % sql)
				sql_rows_affected = database_connect.execute_sql_statement(cursor, sql).rowcount
				if dry_run:
					logging.info("SQL executed, but results will not be committed.")
				else:
					logging.info("SQL executed. Committing results...")
					connection.commit()
				return sql_rows_affected
	except Exception:
		logging.critical("Exception occurred while attempting to execute SQL: %s" % sql)
		raise


if __name__ == "__main__":
	# Construct parser with command line options.
	parser = argparse.ArgumentParser(
		description=textwrap.dedent("""\
			Remove passwords from R_USER_PASSWORD for a specified user.
			This script is meant to be run on an iRODS catalog service provider with database credentials available.
			This script is meant to be run as the iRODS service account user."""))
	parser.add_argument(
		"user_name",
		help=textwrap.dedent("""\
			iRODS username in this format: user_name[#zone_name]. The zone name is optional. If not specified, the
			local zone will be detected and used.""")
	)
	parser.add_argument(
		"--dry-run", dest="dry_run", action="store_true",
		help=textwrap.dedent("""\
			Executes the SQL as specified by the provided options so that the results will be shown, but does not
			commit the transaction. If --sql-only is specified, the SQL will not be executed and this option does
			nothing.""")
	)
	parser.add_argument(
		"--sql-only", dest="sql_only", action="store_true",
		help=textwrap.dedent("""\
			Displays the SQL which would be executed with the given options. If specified, the SQL will not be
			executed, even if --dry-run is specified.""")
	)
	parser.add_argument(
		"--verbose", "-v", dest="verbosity", action="count", default=1,
		help=textwrap.dedent("""\
			Increase the level of output to stdout. CRITICAL and ERROR messages will always be printed. Add more to see
			more log messages (e.g. -vvv displays DEBUG).""")
	)
	parser.add_argument(
		"--forever-passwords-only", dest="forever_passwords_only", action="store_true", default=False,
		help=textwrap.dedent("""\
			If specified, only passwords used in native authentication which do not expire will be removed. This means
			that passwords with a Time-To-Live (TTL) such as those used by pam_password auth scheme will not be
			removed from the catalog.""")
	)
	args = parser.parse_args()

	# Configure standard Python logger.
	configure_logs(args.verbosity)

	config = IrodsConfig()
	if not config.is_provider:
		logging.critical(
			"This script should be run on an iRODS catalog service provider as the iRODS service account. Exiting.")
		sys.exit(1)

	# Construct the SQL statement based on the arguments.
	delete_sql = delete_passwords_sql_builder().set_user(args.user_name).set_forever_passwords_only(args.forever_passwords_only)

	# --sql-only just displays the SQL to execute without executing it.
	if args.sql_only:
		print(delete_sql.get())
		sys.exit(0)

	# Print dry-run message for reassurance.
	if args.dry_run:
		print("NOTE: This is a dry-run and no SQL transactions will be committed!")

	# Execute the SQL statement generated above.
	sql_rows_affected = execute_sql(delete_sql.get(), dry_run=args.dry_run)

	# Print the results for the user.
	print(f"Rows deleted: {sql_rows_affected}")
