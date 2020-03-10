from __future__ import print_function

import os
import shutil
import subprocess
import sys
from threading import Timer
import ustrings

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from ..configuration import IrodsConfig
from .. import test
from .. import lib
from .resource_suite import ResourceBase
from . import session


class Test_Resource_Replication_Timing(ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

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
