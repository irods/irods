from __future__ import print_function

import os
import shutil
import subprocess
import sys
from threading import Timer

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from ..test.command import assert_command
from ..configuration import IrodsConfig
from .. import test
from .. import lib
from .resource_suite import ResourceBase
from . import session
from . import ustrings


class Test_Resource_Replication_Timing(ResourceBase, unittest.TestCase):
    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + test.settings.HOSTNAME_3 + ":" +
                                          irods_config.irods_directory + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix3Resc")
            self.child_replication_count = 3
        super(Test_Resource_Replication_Timing, self).setUp()

    def tearDown(self):
        super(Test_Resource_Replication_Timing, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix3Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix3Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix2RescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix3RescVault", ignore_errors=True)

    def test_rebalance_invocation_timestamp__3665(self):
        # prepare out of balance tree with enough objects to trigger rebalance paging (>500)
        localdir = '3665_tmpdir'
        shutil.rmtree(localdir, ignore_errors=True)
        lib.make_large_local_tmp_dir(dir_name=localdir, file_count=600, file_size=5)
        self.admin.assert_icommand(['iput', '-r', localdir], "STDOUT_SINGLELINE", ustrings.recurse_ok_string())
        self.admin.assert_icommand(['iadmin', 'mkresc', 'newchild', 'unixfilesystem', test.settings.HOSTNAME_1+':/tmp/newchildVault'], 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand(['iadmin','addchildtoresc','demoResc','newchild'])
        # run rebalance with concurrent, interleaved put/trim of new file
        self.admin.assert_icommand(['ichmod','-r','own','rods',self.admin.session_collection])
        self.admin.assert_icommand(['ichmod','-r','inherit',self.admin.session_collection])
        laterfilesize = 300
        laterfile = '3665_laterfile'
        lib.make_file(laterfile, laterfilesize)
        put_thread = Timer(2, subprocess.check_call, [('iput', '-R', 'demoResc', laterfile, self.admin.session_collection)])
        trim_thread = Timer(3, subprocess.check_call, [('itrim', '-n3', self.admin.session_collection + '/' + laterfile)])
        put_thread.start()
        trim_thread.start()
        self.admin.assert_icommand(['iadmin','modresc','demoResc','rebalance'])
        put_thread.join()
        trim_thread.join()
        # new file should not be balanced (rebalance should have skipped it due to it being newer)
        self.admin.assert_icommand(['ils', '-l', laterfile], 'STDOUT_SINGLELINE', [str(laterfilesize), ' 0 ', laterfile])
        self.admin.assert_icommand(['ils', '-l', laterfile], 'STDOUT_SINGLELINE', [str(laterfilesize), ' 1 ', laterfile])
        self.admin.assert_icommand(['ils', '-l', laterfile], 'STDOUT_SINGLELINE', [str(laterfilesize), ' 2 ', laterfile])
        self.admin.assert_icommand_fail(['ils', '-l', laterfile], 'STDOUT_SINGLELINE', [str(laterfilesize), ' 3 ', laterfile])
        # cleanup
        os.unlink(laterfile)
        shutil.rmtree(localdir, ignore_errors=True)
        self.admin.assert_icommand(['iadmin','rmchildfromresc','demoResc','newchild'])
        self.admin.assert_icommand(['itrim', '-Snewchild', '-r', '/tempZone'], 'STDOUT_SINGLELINE', 'Total size trimmed')
        self.admin.assert_icommand(['iadmin','rmresc','newchild'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Reads server log')
    def test_rebalance_logging_replica_update__3463(self):
        filename = 'test_rebalance_logging_replica_update__3463'
        file_size = 400
        lib.make_file(filename, file_size)
        self.admin.assert_icommand(['iput', filename])
        self.update_specific_replica_for_data_objs_in_repl_hier([(filename, filename)])
        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        data_id = session.get_data_id(self.admin, self.admin.session_collection, filename)
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg='updating out-of-date replica for data id [{0}]'.format(str(data_id)),
                count=2,
                server_log_path=IrodsConfig().server_log_path,
                start_index=initial_log_size))
        os.unlink(filename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Reads server log')
    def test_rebalance_logging_replica_creation__3463(self):
        filename = 'test_rebalance_logging_replica_creation__3463'
        file_size = 400
        lib.make_file(filename, file_size)
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['itrim', '-S', 'demoResc', '-N1', filename], 'STDOUT_SINGLELINE', 'Number of files trimmed = 1.')
        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        data_id = session.get_data_id(self.admin, self.admin.session_collection, filename)
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg='creating new replica for data id [{0}]'.format(str(data_id)),
                count=2,
                server_log_path=IrodsConfig().server_log_path,
                start_index=initial_log_size))
        os.unlink(filename)

    def update_specific_replica_for_data_objs_in_repl_hier(self, name_pair_list, repl_num=0):
        # determine which resource has replica 0
        _,out,_ = self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_NAME = '{0}' and DATA_REPL_NUM = '{1}'".format(name_pair_list[0][0], repl_num)], 'STDOUT_SINGLELINE', 'DATA_RESC_HIER')
        replica_0_resc = out.splitlines()[0].split()[-1].split(';')[-1]
        # remove from replication hierarchy
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', 'demoResc', replica_0_resc])
        # update all the replica 0's
        for (data_obj_name, filename) in name_pair_list:
            self.admin.assert_icommand(['iput', '-R', replica_0_resc, '-f', '-n', str(repl_num), filename, data_obj_name])
        # restore to replication hierarchy
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', 'demoResc', replica_0_resc])

@unittest.skipIf(False == test.settings.USE_MUNGEFS, "These tests require mungefs")
class Test_Resource_Replication_With_Retry(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    def setUp(self):
        irods_config = IrodsConfig()

        with session.make_session_for_existing_admin() as admin_session:
            self.munge_mount = os.path.abspath('munge_mount')
            self.munge_target = os.path.abspath('munge_target')
            lib.make_dir_p(self.munge_mount)
            lib.make_dir_p(self.munge_target)
            assert_command('mungefs ' + self.munge_mount + ' -omodules=subdir,subdir=' + self.munge_target)

            self.munge_resc = 'ufs1munge'
            self.munge_passthru = 'pt1'
            self.normal_vault = irods_config.irods_directory + '/vault2'

            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand('iadmin mkresc ' + self.munge_passthru + ' passthru', 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand('iadmin mkresc pt2 passthru', 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand('iadmin mkresc ' + self.munge_resc + ' unixfilesystem ' + test.settings.HOSTNAME_1 + ':' +
                                          self.munge_mount, 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc ufs2 unixfilesystem ' + test.settings.HOSTNAME_2 + ':' +
                                          self.normal_vault, 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin addchildtoresc ' + self.munge_passthru + ' ' + self.munge_resc)
            admin_session.assert_icommand('iadmin addchildtoresc pt2 ufs2')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc ' + self.munge_passthru)
            admin_session.assert_icommand('iadmin addchildtoresc demoResc pt2')

            # Increase write on pt2 because could vote evenly with the 0.5 of pt1 due to locality of reference
            admin_session.assert_icommand('iadmin modresc ' + self.munge_passthru + ' context "write=0.5"')
            admin_session.assert_icommand('iadmin modresc pt2 context "write=3.0"')

        # Get count of existing failures in the log
        self.failure_message = 'Failed to replicate data object'
        self.log_message_starting_location = lib.get_file_size_by_path(irods_config.server_log_path)

        self.valid_scenarios = [
            self.retry_scenario(1, 1, 1, self.make_context()),
            self.retry_scenario(3, 1, 1, self.make_context('3')),
            self.retry_scenario(1, 3, 1, self.make_context(delay='3')),
            self.retry_scenario(1, 3, 1, self.make_context(delay='3')),
            self.retry_scenario(3, 2, 2, self.make_context('3', '2', '2')),
            self.retry_scenario(3, 2, 1.5, self.make_context('3', '2', '1.5'))
            ]

        self.invalid_scenarios = [
            self.retry_scenario(1, 1, 1, self.make_context('-2')),
            self.retry_scenario(1, 1, 1, self.make_context('2.0')),
            self.retry_scenario(1, 1, 1, self.make_context('one')),
            self.retry_scenario(1, 1, 1, self.make_context(delay='0')),
            self.retry_scenario(1, 1, 1, self.make_context(delay='-2')),
            self.retry_scenario(1, 1, 1, self.make_context(delay='2.0')),
            self.retry_scenario(1, 1, 1, self.make_context(delay='one')),
            self.retry_scenario(3, 2, 1, self.make_context('3', '2', '0')),
            self.retry_scenario(3, 2, 1, self.make_context('3', '2', '0.5')),
            self.retry_scenario(3, 2, 1, self.make_context('3', '2', '-2')),
            self.retry_scenario(3, 2, 1, self.make_context('3', '2', 'one'))
            ]
        super(Test_Resource_Replication_With_Retry, self).setUp()

    def tearDown(self):
        super(Test_Resource_Replication_With_Retry, self).tearDown()

        # unmount mungefs and destroy directories
        assert_command(['fusermount', '-u', self.munge_mount])
        shutil.rmtree(self.munge_mount, ignore_errors=True)
        shutil.rmtree(self.munge_target, ignore_errors=True)
        shutil.rmtree(self.normal_vault, ignore_errors=True)

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc " + self.munge_passthru)
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc pt2")
            admin_session.assert_icommand("iadmin rmchildfromresc " + self.munge_passthru + ' ' + self.munge_resc)
            admin_session.assert_icommand("iadmin rmchildfromresc pt2 ufs2")
            admin_session.assert_icommand("iadmin rmresc " + self.munge_resc)
            admin_session.assert_icommand("iadmin rmresc ufs2")
            admin_session.assert_icommand("iadmin rmresc " + self.munge_passthru)
            admin_session.assert_icommand("iadmin rmresc pt2")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')

    # Nested class for containing test case information
    class retry_scenario(object):
        """Stores context information about a test scenario.

        See https://docs.irods.org/4.2.4/plugins/composable_resources
        """

        def __init__(self, retries, delay, multiplier, context_string=None):
            """Initializes retry_scenario instance

            Arguments:
            retries -- integer representing the number of times the replication resource will retry a replication
            delay -- integer representing the number of seconds to wait before attempting the first retry of the replication
            multiplier --  floating point number >=1.0 which multiplies the value of delay after each retry
            context_string -- the above values in the form of a context string used by the replication resource
            """
            self.retries = retries
            self.delay = delay
            self.multiplier = multiplier
            self.context_string = context_string

        def munge_delay(self):
            # No retries means no delay
            if 0 == self.retries:
                return 0

            # Get total time over all retries
            if self.multiplier == 1.0 or self.retries == 1:
                # Since there is no multiplier, delay increases linearly
                time_to_failure = self.delay * self.retries
            else:
                # Delay represented by geometric sequence
                # Example for 3 retries:
                #   First retry -> delay * multiplier^0
                #   Second retry -> delay * multiplier^1
                #   Third retry -> delay * multiplier^2
                time_to_failure = 0
                for i in range(0, self.retries): # range is non-inclusive at the top end because python
                    time_to_failure += int(self.delay * pow(self.multiplier, i))

            # Shave some time to ensure mungefsctl can restore filesystem for successful replication
            return time_to_failure - 0.25

    # Ensure that expected number of new failure messages appear in the log
    def verify_new_failure_messages_in_log(self, expected_count, expected_message=None):
        if not expected_message:
            expected_message = self.failure_message

        irods_config = IrodsConfig()
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg=expected_message,
                count=expected_count,
                server_log_path=IrodsConfig().server_log_path,
                start_index=self.log_message_starting_location))
        self.log_message_starting_location = lib.get_file_size_by_path(irods_config.server_log_path)

    # Generate a context string for replication resource
    @staticmethod
    def make_context(retries=None, delay=None, multiplier=None):
        context_string = ""
        # Only generate for populated parameters
        if retries:
            context_string += 'retry_attempts={0};'.format(retries)
        if delay:
            context_string += 'first_retry_delay_in_seconds={0};'.format(delay)
        if multiplier:
            context_string += 'backoff_multiplier={0}'.format(multiplier)

        # Empty string causes an error, so put none if using all defaults
        if not context_string:
            context_string = 'none=none'

        return context_string

    @staticmethod
    def run_mungefsctl(operation, modifier=None):
        mungefsctl_call = 'mungefsctl --operations ' + str(operation)
        if modifier:
            mungefsctl_call += ' ' + str(modifier)
        assert_command(mungefsctl_call)

    def run_rebalance_test(self, scenario, filename):
        # Setup test and context string
        filepath = lib.create_local_testfile(filename)
        with session.make_session_for_existing_admin() as admin_session:
            if scenario.context_string:
                admin_session.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

            # Get wait time for mungefs to reset
            wait_time = scenario.munge_delay()
            self.assertTrue( wait_time > 0, msg='wait time for mungefs [{0}] <= 0'.format(wait_time))

            # For each test, iput will fail to replicate; then we can run a rebalance
            # Perform corrupt read (checksum) test
            self.run_mungefsctl('read', '--corrupt_data')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message )
            self.verify_new_failure_messages_in_log(scenario.retries + 1)
            munge_thread = Timer(wait_time, self.run_mungefsctl, ['read'])
            munge_thread.start()
            admin_session.assert_icommand('iadmin modresc demoResc rebalance')
            munge_thread.join()
            admin_session.assert_icommand(['irm', '-f', filename])
            self.verify_new_failure_messages_in_log(scenario.retries)

            # Perform corrupt size test
            self.run_mungefsctl('getattr', '--corrupt_size')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(scenario.retries + 1)
            munge_thread = Timer(wait_time, self.run_mungefsctl, ['getattr'])
            munge_thread.start()
            admin_session.assert_icommand('iadmin modresc demoResc rebalance')
            munge_thread.join()
            admin_session.assert_icommand(['irm', '-f', filename])
            self.verify_new_failure_messages_in_log(scenario.retries)

        # Cleanup
        os.unlink(filepath)

    def run_iput_test(self, scenario, filename):
        filepath = lib.create_local_testfile(filename)
        with session.make_session_for_existing_admin() as admin_session:
            # Setup test and context string
            if scenario.context_string:
                admin_session.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

            # Get wait time for mungefs to reset
            wait_time = scenario.munge_delay()
            self.assertTrue( wait_time > 0, msg='wait time for mungefs [{0}] <= 0'.format(wait_time))

            # Perform corrupt read (checksum) test
            self.run_mungefsctl('read', '--corrupt_data')
            munge_thread = Timer(wait_time, self.run_mungefsctl, ['read'])
            munge_thread.start()
            admin_session.assert_icommand(['iput', '-K', filepath])
            munge_thread.join()
            admin_session.assert_icommand(['irm', '-f', filename])
            self.verify_new_failure_messages_in_log(scenario.retries)

            # Perform corrupt size test
            self.run_mungefsctl('getattr', '--corrupt_size')
            munge_thread = Timer(wait_time, self.run_mungefsctl, ['getattr'])
            munge_thread.start()
            admin_session.assert_icommand(['iput', '-K', filepath])
            munge_thread.join()
            admin_session.assert_icommand(['irm', '-f', filename])
            self.verify_new_failure_messages_in_log(scenario.retries)

        # Cleanup
        os.unlink(filepath)

    # Helper function to recreate replication node to ensure empty context string
    def reset_repl_resource(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc " + self.munge_passthru)
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc pt2")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand('iadmin addchildtoresc demoResc ' + self.munge_passthru)
            admin_session.assert_icommand('iadmin addchildtoresc demoResc pt2')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_valid(self):
        for scenario in self.valid_scenarios:
            self.run_iput_test(scenario, 'test_repl_retry_iput_valid')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_invalid(self):
        for scenario in self.invalid_scenarios:
            self.run_iput_test(scenario, 'test_repl_retry_iput_invalid')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_valid(self):
        for scenario in self.valid_scenarios:
            self.run_rebalance_test(scenario, 'test_repl_retry_rebalance_valid')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_invalid(self):
        for scenario in self.invalid_scenarios:
            self.run_rebalance_test(scenario, 'test_repl_retry_rebalance_invalid')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_no_context(self):
        self.reset_repl_resource()
        self.run_iput_test(self.retry_scenario(1, 1, 1), 'test_repl_retry_iput_no_context')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_no_context(self):
        self.reset_repl_resource()
        self.run_rebalance_test(self.retry_scenario(1, 1, 1), 'test_repl_retry_rebalance_no_context')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_no_retries(self):
        filename = "test_repl_retry_iput_no_retries"
        filepath = lib.create_local_testfile(filename)
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand('iadmin modresc demoResc context "{0}"'.format(self.make_context('0')))

            # Perform corrupt read (checksum) test
            self.run_mungefsctl('read', '--corrupt_data')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            self.run_mungefsctl('read')
            admin_session.assert_icommand(['irm', '-f', filename])

            # Perform corrupt size test
            self.run_mungefsctl('getattr', '--corrupt_size')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            self.run_mungefsctl('getattr')

            admin_session.assert_icommand(['irm', '-f', filename])

        os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_no_retries(self):
        filename = "test_repl_retry_rebalance_no_retries"
        filepath = lib.create_local_testfile(filename)
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand('iadmin modresc demoResc context "{0}"'.format(self.make_context('0')))

            # Perform corrupt read (checksum) test
            self.run_mungefsctl('read', '--corrupt_data')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            admin_session.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            self.run_mungefsctl('read')
            admin_session.assert_icommand(['irm', '-f', filename])

            # Perform corrupt size test
            self.run_mungefsctl('getattr', '--corrupt_size')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            admin_session.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1)
            self.run_mungefsctl('getattr')

            admin_session.assert_icommand(['irm', '-f', filename])

        os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_multiplier_overflow(self):
        filename = "test_repl_retry_iput_large_multiplier"
        filepath = lib.create_local_testfile(filename)
        with session.make_session_for_existing_admin() as admin_session:
            large_number = pow(2, 32)
            scenario = self.retry_scenario(2, 1, large_number, self.make_context('2', '1', str(large_number)))
            failure_message = 'bad numeric conversion'
            admin_session.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

            self.run_mungefsctl('read', '--corrupt_data')
            admin_session.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1, failure_message)
            admin_session.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
            self.verify_new_failure_messages_in_log(1, failure_message)
            self.run_mungefsctl('read')

            admin_session.assert_icommand(['irm', '-f', filename])

        os.unlink(filepath)

