from __future__ import print_function
import sys
import os

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController

class Test_LogLevels(unittest.TestCase):

    def setUp(self):
        super(Test_LogLevels, self).setUp()

    def tearDown(self):
        super(Test_LogLevels, self).tearDown()

    def test_log_sql__issue_3865(self):

        # PART 1:
        # We take two measurements/counts.  The first for when no
        # LOG_SQL lines should be showing up in the server log:
        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)

        # Restart with spLogSql not set in the environment
        IrodsController().restart()

        # There should be no LOG_SQL lines in the log at this point.  None.
        self.assertEqual( lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, ' LOG_SQL: ', initial_log_size), 0 )

        # PART 2:
        # The second measurement is for when more than 20 lines with
        # LOG_SQL should be in the logfile.
        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)

        env = os.environ.copy()
        env['spLogSql'] = '1'

        # Restart with spLogSql set in the environment
        IrodsController(IrodsConfig(injected_environment=env)).restart()

        # Restart with spLogSql not set in the environment
        IrodsController().restart()

        # There should be more than 20 LOG_SQL lines in the log at this point.
        self.assertGreater( lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, ' LOG_SQL: ', initial_log_size), 20 )

