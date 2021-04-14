from __future__ import print_function
import getpass
import hashlib
import inspect
import os
import psutil
import re
import shutil
import subprocess
import sys
import tempfile
from threading import Timer
import time
import ustrings

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from ..test.command import assert_command
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file, CoreFile
from .. import test
from .. import paths
from . import settings
from .. import lib
from .resource_suite import ResourceSuite, ResourceBase
from .test_chunkydevtest import ChunkyDevTest
from . import session
from .rule_texts_for_tests import rule_texts

def assert_number_of_replicas(admin_session, logical_path, data_obj_name, replica_count):
    for i in range(0, replica_count):
        admin_session.assert_icommand(['ils', '-l', logical_path], 'STDOUT_SINGLELINE', [' {} '.format(str(i)), ' & ', data_obj_name])
    admin_session.assert_icommand_fail(['ils', '-l', logical_path], 'STDOUT_SINGLELINE', [' {} '.format(str(replica_count + 1)), data_obj_name])

class Test_Resource_RandomWithinReplication(ResourceSuite, ChunkyDevTest, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc random", 'STDOUT_SINGLELINE', 'random')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + test.settings.HOSTNAME_3 + ":" +
                                          irods_config.irods_directory + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
            self.child_replication_count = 2
        super(Test_Resource_RandomWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_RandomWithinReplication, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rrResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc rrResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixAVault", ignore_errors=True)

    def test_ichksum_no_file_modified_under_replication__4099(self):
        filename = 'test_ichksum_no_file_modified_under_replication__4099'
        filepath = lib.create_local_testfile(filename)

        logical_path = os.path.join(self.user0.session_collection, filename)

        # put file into iRODS and clean up local
        self.user0.assert_icommand(['iput', '-K', filepath, logical_path])
        os.unlink(filepath)

        # ensure that number of replicas is correct, trim, and check again
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count))
        self.user0.assert_icommand(['itrim', '-n1', '-N1', logical_path], 'STDOUT_SINGLELINE', 'Number of files trimmed = 1.')
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count - 1))

        # forcibly re-calculate corrupted checksum and ensure that no new replicas were generated
        self.user0.assert_icommand(['ichksum', '-f', '-n0', logical_path], 'STDOUT', 'sha2:')
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count - 1))

        #cleanup
        self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_ibun_creation_to_replication(self):
        collection_name = 'bundle_me'
        tar_name = 'bundle.tar'
        filename = 'test_ibun_creation_to_replication'
        copy_filename = filename + '_copy'
        test_data_obj_path = os.path.join(collection_name, filename)
        test_data_obj_copy_path = os.path.join(collection_name, copy_filename)

        # put file into iRODS and make a copy on a non-replicating resource
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand(['imkdir', collection_name])
        self.admin.assert_icommand(['iput', filepath, test_data_obj_path])
        os.unlink(filepath)
        self.admin.assert_icommand(['icp', '-R', 'TestResc', test_data_obj_path, test_data_obj_copy_path])

        # bundle the collection and ensure that it and its contents replicated properly
        self.admin.assert_icommand(['ibun', '-c', tar_name, collection_name])
        assert_number_of_replicas(self.admin, tar_name, tar_name, self.child_replication_count)
        assert_number_of_replicas(self.admin, test_data_obj_path, filename, self.child_replication_count)
        self.admin.assert_icommand(['ils', '-l', test_data_obj_copy_path], 'STDOUT_SINGLELINE', [' 0 ', 'TestResc', ' & ', copy_filename])
        assert_number_of_replicas(self.admin, test_data_obj_copy_path, copy_filename, self.child_replication_count + 1)

        # cleanup
        self.admin.assert_icommand(['irm', '-r', '-f', collection_name])
        self.admin.assert_icommand(['irm', '-f', tar_name])

    def test_redirect_map_regeneration__3904(self):
        # Setup
        filecount = 50
        dirname = 'test_redirect_map_regeneration__3904'
        lib.create_directory_of_small_files(dirname, filecount)

        self.admin.assert_icommand(['iput', '-r', dirname], "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # Count the number of recipients of replicas
        hier_ctr = {}
        stdout,_,_ = self.admin.run_icommand(['ils', '-l', dirname])
        lines = stdout.splitlines()
        # Check all lines but the first one
        for line in lines[1:]:
            res = line.split()
            hier_ctr[res[2]] = 'found_it'

        # Ensure that a different set of replication targets resulted at least once
        self.assertTrue(len(hier_ctr) == 3, msg="only %d resources received replicas, expected 3" % len(hier_ctr))

        # Cleanup
        self.admin.assert_icommand(['irm', '-rf', dirname])
        shutil.rmtree(dirname, ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("no support for non-compound resources")
    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource repl 0 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource repl 0 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource repl 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource 1 should have double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])

        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                                    #
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


@unittest.skip('Round Robin is deprecated and non-deterministic under load - #3778')
class Test_Resource_RoundRobinWithinReplication(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + test.settings.HOSTNAME_3 + ":" +
                                          irods_config.irods_directory + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
        super(Test_Resource_RoundRobinWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_RoundRobinWithinReplication, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rrResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc rrResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixAVault", ignore_errors=True)

    def test_next_child_iteration__2884(self):
        filename = 'foobar'
        lib.make_file(filename, 100)

        def get_resource_property(session, resource_name, property_):
            _, out, _ = session.assert_icommand(['ilsresc', '-l', resource_name], 'STDOUT_SINGLELINE', 'id')
            for line in out.split('\n'):
                if property_ in line:
                    return line.partition(property_ + ':')[2].strip()

        # extract the next resource in the rr from the context string
        next_resc = get_resource_property(self.admin, 'rrResc', 'context')

        # determine the 'other' resource
        resc_set = set(['unixB1', 'unixB2'])
        remaining_set = resc_set - set([next_resc])
        resc_remaining = remaining_set.pop()

        # resources listed should be 'next_resc'
        self.admin.assert_icommand(['iput', filename, 'file0'])  # put file
        self.admin.assert_icommand(['ils', '-L', 'file0'], 'STDOUT_SINGLELINE', next_resc)  # put file

        # resources listed should be 'resc_remaining'
        self.admin.assert_icommand(['iput', filename, 'file1'])  # put file
        self.admin.assert_icommand(['ils', '-L', 'file1'], 'STDOUT_SINGLELINE', resc_remaining)  # put file

        # resources listed should be 'next_resc' once again
        self.admin.assert_icommand(['iput', filename, 'file2'])  # put file
        self.admin.assert_icommand(['ils', '-L', 'file2'], 'STDOUT_SINGLELINE', next_resc)  # put file

        os.remove(filename)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        pass

    @unittest.skip('does not deterministically purge repl 0')
    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("no support for non-compound resources")
    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource repl 0 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource repl 0 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource repl 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource 1 should have double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])

        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                                    #
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


class Test_Resource_Unixfilesystem(ResourceSuite, ChunkyDevTest, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Resource_Unixfilesystem'

    def setUp(self):
        hostname = lib.get_hostname()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc 'unixfilesystem' " + hostname + ":" +
                                          IrodsConfig().irods_directory + "/demoRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        super(Test_Resource_Unixfilesystem, self).setUp()

    def tearDown(self):
        super(Test_Resource_Unixfilesystem, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        shutil.rmtree(IrodsConfig().irods_directory + "/demoRescVault", ignore_errors=True)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_unix_filesystem_free_space_on_root__3928(self):
        free_space = '100'
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'path', '/demoRescVault'], 'STDOUT_SINGLELINE', 'Previous resource path')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'free_space', free_space])

        rule_file_path = 'test_free_space_on_root.r'
        rule_str = '''
test_free_space_on_root {{
    msi_update_unixfilesystem_resource_free_space(*leaf_resource);
}}

INPUT *leaf_resource="demoResc"
OUTPUT ruleExecOut
        '''
        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        config = IrodsConfig()

        with lib.file_backed_up(config.client_environment_path):
            lib.update_json_file_from_dict(config.client_environment_path, {'irods_log_level': 7})

            log_offset = lib.get_file_size_by_path(paths.server_log_path())
            rc, _, stderr = self.admin.assert_icommand_fail(['irule', '-r', 'irods_rule_engine_plugin-irods_rule_language-instance', '-F', rule_file_path])

            self.admin.assert_icommand(['ilsresc', '-l', 'demoResc'], 'STDOUT_SINGLELINE', ['free space', free_space])
            self.assertNotEqual(0, rc)
            self.assertTrue('status = -32000 SYS_INVALID_RESC_INPUT' in stderr)

            msg = 'could not find existing non-root path from vault path'
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg=msg,
                    start_index=log_offset))

        os.unlink(rule_file_path)

    def test_unix_filesystem_free_space__3306(self):
        filename = 'test_unix_filesystem_free_space__3306.txt'
        filesize = 3000000
        lib.make_file(filename, filesize)

        free_space = 10000000
        self.admin.assert_icommand('iadmin modresc demoResc freespace {0}'.format(free_space))

        # free_space already below threshold - should NOT accept new file
        minimum = free_space + 10
        self.admin.assert_icommand('iadmin modresc demoResc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file1', 'STDERR_SINGLELINE', 'USER_FILE_TOO_LARGE')

        # free_space will be below threshold if replica is created - should NOT accept new file
        minimum = free_space - filesize/2
        self.admin.assert_icommand('iadmin modresc demoResc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file2', 'STDERR_SINGLELINE', 'USER_FILE_TOO_LARGE')

        # after replica creation, free_space will still be greater than minimum - should accept new file
        minimum = free_space - filesize*2
        self.admin.assert_icommand('iadmin modresc demoResc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file3')

    def test_msi_update_unixfilesystem_resource_free_space_and_acPostProcForParallelTransferReceived(self):
        try:
            filename = 'test_msi_update_unixfilesystem_resource_free_space_and_acPostProcForParallelTransferReceived'
            filepath = lib.make_file(filename, 50000000)

            # make sure the physical path exists
            lib.make_dir_p(self.admin.get_vault_path('demoResc'))

            with temporary_core_file() as core:
                time.sleep(1)  # remove once file hash fix is committed #2279
                core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
                time.sleep(1)  # remove once file hash fix is committed #2279

                self.user0.assert_icommand(['iput', filename])
                free_space = psutil.disk_usage(self.admin.get_vault_path('demoResc')).free

            ilsresc_output = self.admin.run_icommand(['ilsresc', '-l', 'demoResc'])[0]
            for l in ilsresc_output.splitlines():
                if l.startswith('free space:'):
                    ilsresc_freespace = int(l.rpartition(':')[2])
                    break
            else:
                assert False, '"free space:" not found in ilsresc output:\n' + ilsresc_output
            assert abs(free_space - ilsresc_freespace) < 4096*10, 'free_space {0}, ilsresc free space {1}'.format(free_space, ilsresc_freespace)
            os.unlink(filename)
        except:
            print('Skipping for plugin name ['+self.plugin_name+']')

    def test_key_value_passthru(self):
        env = os.environ.copy()
        env['spLogLevel'] = '11'
        IrodsController(IrodsConfig(injected_environment=env)).restart()

        lib.make_file('file.txt', 15)
        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)
        self.user0.assert_icommand('iput --kv_pass="put_key=val1" file.txt')
        assert lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, 'key [put_key] - value [val1]', start_index=initial_log_size) in [1, 2]  # double print if collection missing

        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)
        self.user0.assert_icommand('iget -f --kv_pass="get_key=val3" file.txt other.txt')
        # double print if collection missing
        lib.delayAssert(
            lambda: lib.log_message_occurrences_is_one_of_list_of_counts(
                msg='key [get_key] - value [val3]',
                expected_value_list=[1,2],
                server_log_path=IrodsConfig().server_log_path,
                start_index=initial_log_size))
        IrodsController().restart()
        if os.path.exists('file.txt'):
            os.unlink('file.txt')
        if os.path.exists('other.txt'):
            os.unlink('other.txt')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_ifsck__2650(self):
        # local setup
        filename = 'fsckfile.txt'
        filepath = lib.create_local_testfile(filename)
        orig_digest = lib.file_digest(filepath, 'sha256', encoding='base64')
        long_collection_name = '255_byte_directory_name_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12_abcdefghijklmnopqrstuvwxyz12'
        self.admin.assert_icommand("imkdir " + self.admin.session_collection + "/" + long_collection_name)
        full_logical_path = self.admin.session_collection + "/" + long_collection_name + "/" + filename
        # assertions
        self.admin.assert_icommand('ils -L ' + full_logical_path, 'STDERR_SINGLELINE', 'does not exist')  # should not be listed
        self.admin.assert_icommand('iput -K ' + filepath + ' ' + full_logical_path)  # iput
        self.admin.assert_icommand('ils -L ' + full_logical_path, 'STDOUT_SINGLELINE', filename)  # should be listed
        file_vault_full_path = os.path.join(self.admin.get_vault_session_path(), long_collection_name, filename)
        # method 1
        out, err, ec = self.admin.run_icommand('ichksum -K ' + full_logical_path)
        self.assertEqual(ec, 0)
        self.assertEqual(len(out), 0)
        self.assertEqual(len(err), 0)
        # method 2
        self.admin.assert_icommand("iquest \"select DATA_CHECKSUM where DATA_NAME = '%s'\"" % filename,
                                   'STDOUT_SINGLELINE', ['DATA_CHECKSUM = sha2:' + orig_digest])  # iquest
        # method 3
        self.admin.assert_icommand('ils -L ' + long_collection_name, 'STDOUT_SINGLELINE', filename)  # ils
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # change content in vault
        with open(file_vault_full_path, 'r+t') as f:
            f.seek(0)
            print("x", file=f, end='')
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION', 'checksum not consistent with iRODS object'])  # ifsck
        # change size in vault
        lib.cat(file_vault_full_path, 'extra letters')
        new_digest = lib.file_digest(file_vault_full_path, 'sha256', encoding='base64')
        self.admin.assert_icommand('ifsck ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION', 'not consistent with iRODS object'])  # ifsck
        # unregister, reregister (to update filesize in iCAT), recalculate checksum, and confirm
        self.admin.assert_icommand('iunreg ' + full_logical_path)
        self.admin.assert_icommand('ireg ' + file_vault_full_path + ' ' + full_logical_path)
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['WARNING: checksum not available'])  # ifsck
        self.admin.assert_icommand('ichksum -f ' + full_logical_path, 'STDOUT', ['sha2:' + new_digest])
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # local cleanup
        os.remove(filepath)


class Test_Resource_Passthru(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          IrodsConfig().irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Resource_Passthru, self).setUp()

    def tearDown(self):
        super(Test_Resource_Passthru, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        shutil.rmtree(IrodsConfig().irods_directory + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass


class Test_Resource_WeightedPassthru(ResourceBase, unittest.TestCase):

    def setUp(self):
        hostname = lib.get_hostname()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + hostname + ":" +
                                          irods_config.irods_directory + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB 'unixfilesystem' " + hostname + ":" +
                                          irods_config.irods_directory + "/unixBVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc w_pt passthru '' 'write=1.0;read=1.0'", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc w_pt")
            admin_session.assert_icommand("iadmin addchildtoresc w_pt unixB")
        super(Test_Resource_WeightedPassthru, self).setUp()

    def tearDown(self):
        super(Test_Resource_WeightedPassthru, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc w_pt unixB")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc w_pt")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmresc unixB")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin rmresc w_pt")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unixBVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixAVault", ignore_errors=True)

    def test_weighted_passthru(self):
        filename = "some_local_file.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("iput " + filepath)
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "local")

        # repave a copy in the vault to differentiate
        vaultpath = os.path.join(IrodsConfig().irods_directory, "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
        subprocess.check_call("echo 'THISISBROEKN' | cat > %s" % (vaultpath), shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN")

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE")

    def test_weighted_passthru__2789(self):

        ### write=1.0;read=1.0
        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=1.0'")

        filename = "some_local_file_A.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand('icp {0} {1}'.format(self.testfile,filename))
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixA',filename])
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixB',filename])
        self.admin.assert_icommand("irm -f " + filename)

        self.admin.assert_icommand("iput " + filepath)
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixA',filename])
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixB',filename])

        # repave a copy in the vault to differentiate
        vaultpath = os.path.join(IrodsConfig().irods_directory, "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
        subprocess.check_call("echo 'THISISBROEKN' | cat > %s" % (vaultpath), shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN")
        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE")
        self.admin.assert_icommand("irm -f " + filename)

        ### write=0.9;read=0.0
        self.admin.assert_icommand("iadmin modresc w_pt context 'write=0.9;read=0.0'")

        filename = "some_local_file_B.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand('icp {0} {1}'.format(self.testfile,filename))
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixA',filename])
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixB',filename])
        self.admin.assert_icommand("irm -f " + filename)

        self.admin.assert_icommand("iput " + filepath)
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixA',filename])
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', ['unixB',filename])

        # repave a copy in the vault to differentiate
        vaultpath = os.path.join(IrodsConfig().irods_directory, "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
        subprocess.check_call("echo 'THISISBROEKN' | cat > %s" % (vaultpath), shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN")
        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE")
        self.admin.assert_icommand("irm -f " + filename)

class Test_Resource_Deferred(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc deferred", 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          IrodsConfig().irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Resource_Deferred, self).setUp()

    def tearDown(self):
        super(Test_Resource_Deferred, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        shutil.rmtree(IrodsConfig().irods_directory + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass


class Test_Resource_Random(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc random", 'STDOUT_SINGLELINE', 'random')
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
        super(Test_Resource_Random, self).setUp()

    def tearDown(self):
        super(Test_Resource_Random, self).tearDown()
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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_unix_filesystem_free_space__3306(self):
        filename = 'test_unix_filesystem_free_space__3306'
        filesize = 3000000
        lib.make_file(filename, filesize)

        free_space = 1000000000
        self.admin.assert_icommand('iadmin modresc unix1Resc freespace {0}'.format(free_space))
        self.admin.assert_icommand('iadmin modresc unix2Resc freespace {0}'.format(free_space))
        self.admin.assert_icommand('iadmin modresc unix3Resc freespace {0}'.format(free_space))

        # minimum below free space - should NOT accept new file on any child
        minimum = free_space + 10
        self.admin.assert_icommand('iadmin modresc unix1Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iadmin modresc unix2Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iadmin modresc unix3Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file1', 'STDERR_SINGLELINE', 'NO_NEXT_RESC_FOUND')

        # minimum above free space plus file size - should accept new file on unix2Resc only
        minimum = free_space + 10
        self.admin.assert_icommand('iadmin modresc unix1Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iadmin modresc unix3Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        minimum = free_space - filesize - 10
        self.admin.assert_icommand('iadmin modresc unix2Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file3')
        self.admin.assert_icommand('ils -l file3', 'STDOUT_SINGLELINE', 'unix2Resc')

class Test_Resource_NonBlocking(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc nonblocking " + test.settings.HOSTNAME_1 + ":" +
                                          IrodsConfig().irods_directory + "/nbVault", 'STDOUT_SINGLELINE', 'nonblocking')
        super(Test_Resource_NonBlocking, self).setUp()

    def tearDown(self):
        super(Test_Resource_NonBlocking, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')


class Test_Resource_CompoundWithMockarchive(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc mockarchive " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'mockarchive')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_CompoundWithMockarchive, self).setUp()

    def tearDown(self):
        super(Test_Resource_CompoundWithMockarchive, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheRescVault", ignore_errors=True)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("NOTSURE / FIXME ... -K not supported, perhaps")
    def test_local_iput_checksum(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                                    #
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource cache should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource cache should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource archive should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource archive should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file... in bulk!
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)


class Test_Resource_CompoundWithUnivmss(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc univmss " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'univmss')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_CompoundWithUnivmss, self).setUp()

    def tearDown(self):
        super(Test_Resource_CompoundWithUnivmss, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheRescVault", ignore_errors=True)

    def test_irm_with_no_stage__2930(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("itrim -n0 -N1 " + self.testfile, 'STDOUT_SINGLELINE', "files trimmed") # trim cache copy
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed

        irods_config = IrodsConfig()
        initial_log_size = lib.get_file_size_by_path(irods_config.server_log_path)
        self.admin.assert_icommand("irm " + self.testfile ) # remove archive replica
        lib.delayAssert(
            lambda: lib.log_message_occurrences_equals_count(
                msg='argv:stageToCache',
                count=0,
                server_log_path=irods_config.server_log_path,
                start_index=initial_log_size))

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                                    #
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource cache should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource cache should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource archive should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource archive should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file... in bulk!
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)


class Test_Resource_Compound(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Resource_Compound'

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/archiveRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_Compound, self).setUp()

    def tearDown(self):
        super(Test_Resource_Compound, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree("rm -rf " + irods_config.irods_directory + "/cacheRescVault", ignore_errors=True)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "local filesystem check")
    def test_ichksum_no_file_modified_under_compound__4085(self):
        filename = 'test_ichksum_no_file_modified_in_compound__4085'
        filepath = lib.create_local_testfile(filename)
        with open(filepath, 'r') as f:
            original_checksum = hashlib.sha256(f.read()).digest().encode("base64").strip()

        self.admin.assert_icommand(['iput', '-K', filepath, filename])
        os.unlink(filename)
        self.admin.assert_icommand(['ils', '-L', filename], 'STDOUT_SINGLELINE', 'archiveResc')

        phypath_for_data_obj = os.path.join(self.admin.get_vault_session_path('archiveResc'), filename)
        original_archive_mtime = str(os.stat(phypath_for_data_obj).st_mtime)

        # trim cache replica
        self.admin.assert_icommand(['itrim', '-n0', '-N1', filename], 'STDOUT_SINGLELINE', "files trimmed")

        # run ichksum on data object, which will replicate to cache
        self.admin.assert_icommand(['ichksum', '-f', filename], 'STDOUT_SINGLELINE', filename + '    sha2:')
        self.admin.assert_icommand(['ils', '-L', filename], 'STDOUT_SINGLELINE', "sha2:" + original_checksum)

        self.assertTrue(str(os.stat(phypath_for_data_obj).st_mtime) == original_archive_mtime, msg='Archive mtime changed after ichksum - not good!')

        #cleanup
        self.admin.assert_icommand(['irm', '-f', filename])

    def test_irsync__2976(self):
        filename = "test_irsync__2976.txt"
        filename_rsync = "test_irsync__2976.txt.rsync"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput -R demoResc " + filename)

        logical_path = os.path.join( self.admin.session_collection, filename )
        logical_path_rsync = os.path.join( self.admin.session_collection, filename_rsync )
        self.admin.assert_icommand("irsync i:" + logical_path + " i:" + logical_path_rsync )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', filename )
        self.admin.assert_icommand("ils -l " + logical_path_rsync, 'STDOUT_SINGLELINE', filename_rsync )

    def test_msiDataObjRsync__2976(self):
        filename = "test_msiDataObjRsync__2976.txt"
        filename_rsync = "test_msiDataObjRsync__2976.txt.rsync"

        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput -R TestResc " + filename)

        logical_path = os.path.join( self.admin.session_collection, filename )
        logical_path_rsync = os.path.join( self.admin.session_collection, filename_rsync )

        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['logical_path_rsync'] = logical_path_rsync
        parameters['dest_resc'] = 'null'
        rule_file_path = 'test_msiDataObjRsync__2976.r'
        rule_str = rule_texts[self.plugin_name][self.class_name]['test_msiDataObjRsync__2976'].format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', filename )
        self.admin.assert_icommand("ils -l " + logical_path_rsync, 'STDOUT_SINGLELINE', filename_rsync )

    def test_irsync_for_collection__2976(self):
        dir_name = 'test_irsync_for_collection__2976'
        dir_name_rsync = dir_name + '_rsync'
        lib.create_directory_of_small_files(dir_name,10)
        self.admin.assert_icommand('iput -rR demoResc ' + dir_name, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        logical_path = os.path.join( self.admin.session_collection, dir_name )
        logical_path_rsync = os.path.join( self.admin.session_collection, dir_name_rsync )

        self.admin.assert_icommand("irsync -r i:" + logical_path + " i:" + logical_path_rsync )
        self.admin.assert_icommand("ils -lr " + logical_path, 'STDOUT_SINGLELINE', dir_name )
        self.admin.assert_icommand("ils -lr " + logical_path_rsync, 'STDOUT_SINGLELINE', dir_name_rsync )

    def test_msiCollRsync__2976(self):
        dir_name = 'test_irsync_for_collection__2976'
        dir_name_rsync = dir_name + '_rsync'
        lib.create_directory_of_small_files(dir_name,10)
        self.admin.assert_icommand('iput -rR demoResc ' + dir_name, "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        logical_path = os.path.join( self.admin.session_collection, dir_name )
        logical_path_rsync = os.path.join( self.admin.session_collection, dir_name_rsync )

        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['logical_path_rsync'] = logical_path_rsync
        parameters['dest_resc'] = 'demoResc'
        rule_file_path = 'test_msiDataObjRsync__2976.r'
        rule_str = rule_texts[self.plugin_name][self.class_name]['test_msiCollRsync__2976'].format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', dir_name )
        self.admin.assert_icommand("ils -l " + logical_path_rsync, 'STDOUT_SINGLELINE', dir_name_rsync )

    def test_iphymv_as_admin__2995(self):
        filename = "test_iphymv_as_admin__2995_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        self.admin.assert_icommand("iphymv -M -S TestResc -R cacheResc " + logical_path )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', ['&', 'cacheResc'])
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', ['&', 'archiveResc'])

    def test_irepl_as_admin__2988(self):
        filename = "test_irepl_as_admin__2988_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        self.admin.assert_icommand("irepl -M -R demoResc " + logical_path )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "local filesystem check")
    def test_msiDataObjUnlink__2983(self):
        filename = "test_test_msiDataObjUnlink__2983.txt"
        filepath = lib.create_local_testfile(filename)
        logical_path = os.path.join( self.admin.session_collection, filename )

        self.admin.assert_icommand("ireg " + filepath + " " + logical_path)

        parameters = {}
        parameters['logical_path'] = logical_path
        rule_file_path = 'test_msiDataObjUnlink__2983.r'
        rule_str = rule_texts[self.plugin_name][self.class_name]['test_msiDataObjUnlink__2983'].format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)
        self.admin.assert_icommand_fail("ils -l " + logical_path, 'STDOUT_SINGLELINE', filename)

        assert os.path.isfile(filepath)

    def test_msiDataObjRepl_as_admin__2988(self):
        filename = "test_msiDataObjRepl_as_admin__2988_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['dest_resc'] = 'demoResc'
        rule_file_path = 'test_msiDataObjRepl_as_admin__2988.r'
        rule_str = rule_texts[self.plugin_name][self.class_name]['test_msiDataObjRepl_as_admin__2988'].format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

    def test_auto_repl_off__2941(self):
        self.admin.assert_icommand("iadmin modresc demoResc context \"auto_repl=off\"" )

        filename = "auto_repl_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)

        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', 'archiveResc')

    def test_msisync_to_archive__2962(self):
        self.admin.assert_icommand("iadmin modresc demoResc context \"auto_repl=off\"" )

        filename = "test_msisync_to_archive__2962.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)

        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'cacheResc')

        physical_path = os.path.join(IrodsConfig().irods_directory, 'cacheRescVault')
        physical_path = os.path.join(physical_path,os.path.basename(self.admin.session_collection))
        physical_path = os.path.join(physical_path,filename)

        logical_path = os.path.join( self.admin.session_collection, filename )
        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['physical_path'] = physical_path
        parameters['resc_hier'] = 'demoResc;cacheResc'

        rule_file_path = 'test_msisync_to_archive__2962.r'
        rule_str = rule_texts[self.plugin_name][self.class_name]['test_msisync_to_archive__2962'].format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

    def test_stage_to_cache(self):
        self.admin.assert_icommand("iadmin modresc demoResc context \"auto_repl=on\"" )

        filename = "test_stage_to_cache_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)

        self.admin.assert_icommand("itrim -N 1 -n 0 " + filename, 'STDOUT_SINGLELINE', "files trimmed")

        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'archiveResc' )

        self.admin.assert_icommand("iget -f " + filename)

        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'cacheResc')

    def test_auto_repl_on__2941(self):
        self.admin.assert_icommand("iadmin modresc demoResc context \"auto_repl=on\"" )

        filename = "auto_repl_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("iput " + filename)

        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'archiveResc')

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iget_prefer_from_archive_corrupt_archive__ticket_3145(self):

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)

        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)        # put file

        # manually remove the replica in the archive vault
        phypath = os.path.join(self.admin.get_vault_session_path('archiveResc'), filename)
        os.remove(phypath)

        # manipulate the core.re to add the new policy
        with temporary_core_file() as core:
            time.sleep(2)  # remove once file hash fix is commited #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(2)  # remove once file hash fix is commited #2279

            self.admin.assert_icommand("irm -f " + filename)

        # local cleanup
        os.remove(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_iget_prefer_from_archive__ticket_1660(self):

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)

        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)        # put file

        # manually update the replica in archive vault
        out, _, _ = self.admin.run_icommand('ils -L ' + filename)
        archivereplicaphypath = filter(lambda x : "archiveRescVault" in x, out.split())[0]
        with open(archivereplicaphypath, 'wt') as f:
            print('MANUALLY UPDATED ON ARCHIVE\n', file=f, end='')
        # get file
        retrievedfile = "retrieved.txt"
        os.system("rm -f %s" % retrievedfile)
        self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from cache

        # confirm retrieved file is same as original
        self.assertEqual(0, os.system("diff %s %s" % (filepath, retrievedfile)))

        # manipulate the core.re to add the new policy
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # restart the server to reread the new core.re
            IrodsController().restart()

            # manually update the replica in archive vault
            out, _, _ = self.admin.run_icommand('ils -L ' + filename)
            archivereplicaphypath = filter(lambda x : "archiveRescVault" in x, out.split())[0]
            with open(archivereplicaphypath, 'wt') as f:
                print('UPDATED ARCHIVE AGAIN\n', file=f, end='')

            # get the file
            self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from archive

            # confirm this is the new archive file
            matchfound = False
            with open(retrievedfile) as f:
                for line in f:
                    if "AGAIN" in line:
                        matchfound = True
            self.assertTrue(matchfound)

        # local cleanup
        os.remove(filepath)
        os.remove(retrievedfile)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource cache should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource cache should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource archive should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource archive should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file... in bulk!
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            # should be listed once - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should be listed
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)


class Test_Resource_ReplicationWithinReplication(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc replResc replication", 'STDOUT_SINGLELINE', 'replication')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + test.settings.HOSTNAME_3 + ":" +
                                          irods_config.irods_directory + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc replResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB2")
        super(Test_Resource_ReplicationWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationWithinReplication, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc replResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc replResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc replResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc replResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unixAVault", ignore_errors=True)

    @unittest.skip("no support for non-compound resources")
    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # replica 2 should be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # default resource should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # default resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 3 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 3 " + self.testresc, " " + doublesize + " ", " & " + filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])  # should not have a replica 4
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)       # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl " + filename)               # replicate to default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from grid
        # replica 1 should be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["1 ", self.testfile])
        # replica 2 should be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 ", self.testfile])
        # replica 3 should be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["3 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)  # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)


class Test_Resource_ReplicationToTwoCompound(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Resource_ReplicationToTwoCompound'

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc compResc1 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc compResc2 compound", 'STDOUT_SINGLELINE', 'compound')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Resource_ReplicationToTwoCompound, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationToTwoCompound, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc compResc2 archiveResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc2 cacheResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc1 archiveResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc1 cacheResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc compResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc compResc1")
            admin_session.assert_icommand("iadmin rmresc archiveResc2")
            admin_session.assert_icommand("iadmin rmresc cacheResc2")
            admin_session.assert_icommand("iadmin rmresc archiveResc1")
            admin_session.assert_icommand("iadmin rmresc cacheResc1")
            admin_session.assert_icommand("iadmin rmresc compResc2")
            admin_session.assert_icommand("iadmin rmresc compResc1")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/archiveResc1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheResc1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/archiveResc2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheResc2Vault", ignore_errors=True)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["4 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iget_prefer_from_archive__ticket_1660(self):

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)
        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)     # put file

        # manually update the replicas in archive vaults
        out, _, _ = self.admin.run_icommand('ils -L ' + filename)
        print(out)
        archive1replicaphypath = filter(lambda x : "archiveResc1Vault" in x, out.split())[0]
        archive2replicaphypath = filter(lambda x : "archiveResc2Vault" in x, out.split())[0]
        print(archive1replicaphypath)
        print(archive2replicaphypath)
        with open(archive1replicaphypath, 'wt') as f:
            print('MANUALLY UPDATED ON ARCHIVE 1\n', file=f, end='')
        with open(archive2replicaphypath, 'wt') as f:
            print('MANUALLY UPDATED ON ARCHIVE 2\n', file=f, end='')

        # get file
        retrievedfile = "retrieved.txt"
        if os.path.exists(retrievedfile):
            os.unlink(retrievedfile)
        self.admin.assert_icommand(['iget', '-f', filename, retrievedfile])  # get file from cache
        # confirm retrieved file is same as original
        self.assertTrue(os.path.exists(retrievedfile))
        self.assertTrue(os.path.exists(filepath))
        self.assertEqual(0, os.system("diff %s %s" % (filepath, retrievedfile))) 
        print("original file diff confirmed")

        # manipulate the core.re to add the new policy
        with temporary_core_file() as core:
            time.sleep(1)  # remove once file hash fix is committed #2279
            core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
            time.sleep(1)  # remove once file hash fix is committed #2279

            # restart the server to reread the new core.re
            IrodsController().restart()

            # manually update the replicas in archive vaults
            out, _, _ = self.admin.run_icommand('ils -L ' + filename)
            archivereplica1phypath = filter(lambda x : "archiveResc1Vault" in x, out.split())[0]
            archivereplica2phypath = filter(lambda x : "archiveResc2Vault" in x, out.split())[0]
            print(archive1replicaphypath)
            print(archive2replicaphypath)
            with open(archivereplica1phypath, 'wt') as f:
                print('UPDATED ARCHIVE 1 AGAIN\n', file=f, end='')
            with open(archivereplica2phypath, 'wt') as f:
                print('UPDATED ARCHIVE 2 AGAIN\n', file=f, end='')

            # confirm the new content is on disk
            with open(archivereplica1phypath) as f:
                for line in f:
                    print(line)
            with open(archivereplica2phypath) as f:
                for line in f:
                    print(line)

            self.admin.assert_icommand(['iget', '-f', filename, retrievedfile])

            # confirm this is the new archive file
            with open(retrievedfile) as f:
                for line in f:
                    if 'AGAIN' in line:
                        break
                else:
                    assert False

        # local cleanup
        os.remove(filepath)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache   1 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive 1 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache   2 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + filename])
        # default resource archive 2 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + filename])
        # default resource cache   1 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive 1 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # default resource cache   2 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " " + filename])
        # default resource archive 2 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 6
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])
        # should not have a replica 7
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 7 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource cache 1 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource cache 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource archive 1 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource archive 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # default resource cache 2 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # default resource cache 2 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " & " + filename])
        # default resource archive 2 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # default resource archive 2 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 4 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 4 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
            # should not have any extra replicas
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file... in bulk!
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
            # should not have any extra replicas
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 4x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 4x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 4x - replica 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])  # should be listed 4x - replica 4
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)


class Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive'

    def setUp(self):
        # back up core file
        core = CoreFile()
        backupcorefilepath = core.filepath + "--" + self._testMethodName
        shutil.copy(core.filepath, backupcorefilepath)

        time.sleep(1)  # remove once file hash fix is committed #2279
        core.add_rule(rule_texts[self.plugin_name][self.class_name][inspect.currentframe().f_code.co_name])
        time.sleep(1)  # remove once file hash fix is committed #2279

        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc compResc1 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc compResc2 compound", 'STDOUT_SINGLELINE', 'compound')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc compResc2 archiveResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc2 cacheResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc1 archiveResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc compResc1 cacheResc1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc compResc2")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc compResc1")
            admin_session.assert_icommand("iadmin rmresc archiveResc2")
            admin_session.assert_icommand("iadmin rmresc cacheResc2")
            admin_session.assert_icommand("iadmin rmresc archiveResc1")
            admin_session.assert_icommand("iadmin rmresc cacheResc1")
            admin_session.assert_icommand("iadmin rmresc compResc2")
            admin_session.assert_icommand("iadmin rmresc compResc1")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/archiveResc1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheResc1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/archiveResc2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/cacheResc2Vault", ignore_errors=True)

        # restore the original core.re
        core = CoreFile()
        backupcorefilepath = core.filepath + "--" + self._testMethodName
        shutil.copy(backupcorefilepath, core.filepath)
        os.remove(backupcorefilepath)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile, 'STDOUT', 'deprecated')  # remove original from cacheResc only
        # replica 2 should still be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["4 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("this is tested elsewhere")
    def test_iget_prefer_from_archive__ticket_1660(self):
        pass

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                      # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource cache   1 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource archive 1 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource cache   2 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + filename])
        # default resource archive 2 should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + filename])
        # default resource cache   1 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource archive 1 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # default resource cache   2 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " " + filename])
        # default resource archive 2 should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # irepl
    ###################

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 6
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])
        # should not have a replica 7
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 7 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource cache 1 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource cache 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource archive 1 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource archive 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # default resource cache 2 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # default resource cache 2 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " & " + filename])
        # default resource archive 2 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # default resource archive 2 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 4 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 4 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
            # should not have any extra replicas
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename], 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # put file... in bulk!
            # should not be listed (trimmed)
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
            # should not have any extra replicas
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 4x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 4x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 4x - replica 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])  # should be listed 4x - replica 4
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)


class Test_Resource_RoundRobin(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
        super(Test_Resource_RoundRobin, self).setUp()

    def tearDown(self):
        super(Test_Resource_RoundRobin, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix2RescVault", ignore_errors=True)

    def test_unix_filesystem_free_space__3306(self):
        filename = 'test_unix_filesystem_free_space__3306'
        filesize = 3000000
        lib.make_file(filename, filesize)

        free_space = 1000000000
        self.admin.assert_icommand('iadmin modresc unix1Resc freespace {0}'.format(free_space))
        self.admin.assert_icommand('iadmin modresc unix2Resc freespace {0}'.format(free_space))

        # minimum above free space - should NOT accept new file on any child
        minimum = free_space + 10
        self.admin.assert_icommand('iadmin modresc unix1Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iadmin modresc unix2Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file1', 'STDERR_SINGLELINE', 'NO_NEXT_RESC_FOUND')

        # minimum below free space after put - should accept new file on unix2Resc only
        minimum = free_space + 10
        self.admin.assert_icommand('iadmin modresc unix1Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        minimum = free_space - filesize - 10
        self.admin.assert_icommand('iadmin modresc unix2Resc context minimum_free_space_for_create_in_bytes={0}'.format(minimum))
        self.admin.assert_icommand('iput ' + filename + ' file3')
        self.admin.assert_icommand('ils -l file3', 'STDOUT_SINGLELINE', 'unix2Resc')

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_round_robin_mechanism(self):
        # local setup
        filename = "rrfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        self.user1.assert_icommand("iput " + filename + " file0.txt")
        self.user1.assert_icommand("iput " + filename + " file1.txt")

        self.user1.assert_icommand("ils -l", 'STDOUT_SINGLELINE', "unix1Resc")
        self.user1.assert_icommand("ils -l", 'STDOUT_SINGLELINE', "unix2Resc")

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

class Test_Resource_Replication(ChunkyDevTest, ResourceSuite, unittest.TestCase):
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
        super(Test_Resource_Replication, self).setUp()

    def tearDown(self):
        super(Test_Resource_Replication, self).tearDown()
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

    def test_ichksum_no_file_modified_under_replication__4099(self):
        filename = 'test_ichksum_no_file_modified_under_replication__4099'
        filepath = lib.create_local_testfile(filename)

        logical_path = os.path.join(self.user0.session_collection, filename)

        # put file into iRODS and clean up local
        self.user0.assert_icommand(['iput', '-K', filepath, logical_path])
        os.unlink(filepath)

        # ensure that number of replicas is correct, trim, and check again
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count))
        self.user0.assert_icommand(['itrim', '-n1', '-N1', logical_path], 'STDOUT_SINGLELINE', 'Number of files trimmed = 1.')
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count - 1))

        # forcibly re-calculate corrupted checksum and ensure that no new replicas were generated
        self.user0.assert_icommand(['ichksum', '-f', '-n0', logical_path], 'STDOUT', 'sha2:')
        self.user0.assert_icommand(['iquest', '%s', '''"select COUNT(DATA_REPL_NUM) where DATA_NAME = '{0}'"'''.format(filename)], 'STDOUT_SINGLELINE', str(self.child_replication_count - 1))

        #cleanup
        self.user0.assert_icommand(['irm', '-f', logical_path])

    def test_ibun_creation_to_replication(self):
        collection_name = 'bundle_me'
        tar_name = 'bundle.tar'
        filename = 'test_ibun_creation_to_replication'
        copy_filename = filename + '_copy'
        test_data_obj_path = os.path.join(collection_name, filename)
        test_data_obj_copy_path = os.path.join(collection_name, copy_filename)

        # put file into iRODS and make a copy on a non-replicating resource
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand(['imkdir', collection_name])
        self.admin.assert_icommand(['iput', filepath, test_data_obj_path])
        os.unlink(filepath)
        self.admin.assert_icommand(['icp', '-R', 'TestResc', test_data_obj_path, test_data_obj_copy_path])

        # bundle the collection and ensure that it and its contents replicated properly
        self.admin.assert_icommand(['ibun', '-c', tar_name, collection_name])
        assert_number_of_replicas(self.admin, tar_name, tar_name, self.child_replication_count)
        assert_number_of_replicas(self.admin, test_data_obj_path, filename, self.child_replication_count)
        self.admin.assert_icommand(['ils', '-l', test_data_obj_copy_path], 'STDOUT_SINGLELINE', [' 0 ', 'TestResc', ' & ', copy_filename])
        assert_number_of_replicas(self.admin, test_data_obj_copy_path, copy_filename, self.child_replication_count + 1)

        # cleanup
        self.admin.assert_icommand(['irm', '-r', '-f', collection_name])
        self.admin.assert_icommand(['irm', '-f', tar_name])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_open_write_close_for_repl__3909(self):
        # Create local test file of a certain expected size
        filename = 'some_local_file.txt'
        filepath = lib.create_local_testfile(filename)
        expected_size_in_bytes = 9
        test_string = 'a' * expected_size_in_bytes
        with open(filepath, 'w') as f:
            f.write(test_string)

        # Put test file into replication resource and verify
        self.admin.assert_icommand(['iput', filepath])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [str(expected_size_in_bytes), ' & ', filename])

        # Create a rule file to open data object, write a string to it, and close it
        logical_path = os.path.join( self.admin.session_collection, filename )
        write_string = 'b' * (expected_size_in_bytes + 1)
        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['write_string'] = write_string
        rule_file_path = 'test_open_write_close_for_repl__3909.r'
        rule_str = '''
test_open_write_close {{
    msiDataObjOpen("objPath={logical_path}++++openFlags=O_WRONLY", *FD)
    msiDataObjWrite(*FD, "{write_string}", *LEN)
    msiDataObjClose(*FD, *status)
}}

#INPUT *data_obj_path="{logical_path}"
INPUT null
OUTPUT ruleExecOut
'''.format(**parameters)
        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # Run rule on replicated data object and verify
        self.admin.assert_icommand(['irule', '-F', rule_file_path])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [str(expected_size_in_bytes + 1), ' & ', filename])

        # Clean up test files
        self.admin.assert_icommand(['irm', '-f', logical_path])
        os.unlink(filepath)
        os.unlink(rule_file_path)

    def test_ibun_extract_to_rebalance(self):
        # Create a tar file
        tar_file = 'pea_pod.tar'
        test_files = ['pea1', 'pea2', 'pea3']
        for f in test_files:
            lib.make_file(f, 1000)
        lib.execute_command(['tar', 'czf', tar_file, test_files[0], test_files[1], test_files[2]])

        # Set up some logical names
        coll_name = 'peas'
        coll_path = self.admin.session_collection + '/' + coll_name
        tar_path = self.admin.session_collection + '/' + tar_file

        # Put the tar file into iRODS
        self.admin.assert_icommand(['iput', tar_file])
        self.admin.assert_icommand(['ils', '-L'], 'STDOUT_SINGLELINE', tar_file)

        # Extract tar file to the replication resource
        self.admin.assert_icommand(['ibun', '-x', tar_path, coll_path])
        for f in test_files:
            self.admin.assert_icommand(['ils', '-L', coll_path], 'STDOUT_SINGLELINE', [' 0 ', ' & ', f])

        # Perform a rebalance to give all the children a copy of each file
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        for f in test_files:
            self.admin.assert_icommand(['ils', '-L', coll_path + '/' + f], 'STDOUT_SINGLELINE', [' 0 ', ' & ', f])
            self.admin.assert_icommand(['ils', '-L', coll_path + '/' + f], 'STDOUT_SINGLELINE', [' 1 ', ' & ', f])
            self.admin.assert_icommand(['ils', '-L', coll_path + '/' + f], 'STDOUT_SINGLELINE', [' 2 ', ' & ', f])

        # Local cleanup
        self.admin.assert_icommand(['irm', '-rf', coll_path])
        self.admin.assert_icommand(['irm', '-rf', tar_path])
        if os.path.exists(tar_file):
            os.unlink(tar_file)
        for f in test_files:
            if os.path.exists(f):
                os.unlink(f)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_file_modified_on_checksum__ticket_3525(self):
        filename = 'test_file_modified_on_checksum__ticket_3525.txt'
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand('iput -K ' + filename)

        u1_path = os.path.join(self.admin.get_vault_session_path('unix1Resc'), filename)
        u2_path = os.path.join(self.admin.get_vault_session_path('unix2Resc'), filename)
        u3_path = os.path.join(self.admin.get_vault_session_path('unix3Resc'), filename)

        self.admin.assert_icommand('ils -L ' + filename, 'STDOUT_SINGLELINE', filename)

        # Overwrite the contents of replica zero.
        # The size of the file must be greater than or equal to the size recorded in
        # the catalog so that the call to ichksum succeeds.
        with open(u1_path, 'wt') as f:
            f.write('UNIX_1' * 20)

        u1_time = os.path.getmtime(u1_path)
        u2_time = os.path.getmtime(u2_path)
        u3_time = os.path.getmtime(u3_path)

        print('u1_time: '+str(u1_time))
        print('u2_time: '+str(u2_time))
        print('u3_time: '+str(u3_time))

        time.sleep(1)

        self.admin.assert_icommand("ichksum -f " + filename, 'STDOUT_SINGLELINE', '    sha2:')

        u1_time2 = os.path.getmtime(u1_path)
        u2_time2 = os.path.getmtime(u2_path)
        u3_time2 = os.path.getmtime(u3_path)

        print('u1_time2: '+str(u1_time2))
        print('u2_time2: '+str(u2_time2))
        print('u3_time2: '+str(u3_time2))

        # Show that computing/updating checksums does not affect the physical object.
        assert(u1_time == u1_time2 and u2_time == u2_time2 and u3_time == u3_time2)

    def test_num_repl_policy__ticket_2851(self):
        self.admin.assert_icommand('iadmin modresc demoResc context "num_repl=2"')
        self.admin.assert_icommand('iadmin lr demoResc', 'STDOUT_SINGLELINE', 'demoResc')
        filename = "test_num_repl_policy__ticket_2851.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("iput " + filepath + ' ' + filename)
        # Count number of lines to determine number of replicas
        linecount = len(self.admin.assert_icommand(['ils', '-l', filename], 'STDOUT_SINGLELINE', filename)[1].splitlines())
        self.assertTrue(2 == linecount, msg='[{}] replicas made, expected 2'.format(linecount))
        self.admin.assert_icommand(['irm', '-f', filename])

        self.admin.assert_icommand('iadmin modresc demoResc context "NO_CONTEXT"')
        os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_random_read_policy__ticket_2851(self):

        self.admin.assert_icommand('iadmin modresc demoResc context "read=random"')
        self.admin.assert_icommand('iadmin lr demoResc', 'STDOUT_SINGLELINE', 'demoResc')
        filename = "test_random_read_policy__ticket_2851.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("iput " + filename + ' ' + filename)

        u1_path = os.path.join(self.admin.get_vault_session_path('unix1Resc'), filename)
        u2_path = os.path.join(self.admin.get_vault_session_path('unix2Resc'), filename)
        u3_path = os.path.join(self.admin.get_vault_session_path('unix3Resc'), filename)

        print( "u1_path ["+u1_path+"]")
        print( "u2_path ["+u2_path+"]")
        print( "u3_path ["+u3_path+"]")

        # modify files in the vaults to signify which resources
        with open(u1_path, 'wt') as f:
            f.write("UNIX_1")
        with open(u2_path, 'wt') as f:
            f.write("UNIX_2")
        with open(u3_path, 'wt') as f:
            f.write("UNIX_3")

        res_ctr = {}
        for i in range(0,10):
            stdout,_,_ = self.admin.run_icommand(['iget', filename, '-'])
            res_ctr[stdout] = 'found_it'

        self.admin.assert_icommand('iadmin modresc demoResc context "NO_CONTEXT"')

        # given random behavior we expect there to be more than one but
        # there are no guarantees
        assert len(res_ctr) <= 3

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_random_read_policy_with_num_repl__ticket_2851(self):

        self.admin.assert_icommand('iadmin modresc demoResc context "num_repl=2;read=random"')
        self.admin.assert_icommand('iadmin lr demoResc', 'STDOUT_SINGLELINE', 'demoResc')
        filename = "test_random_read_policy__ticket_2851.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("iput " + filename + ' ' + filename)

        u1_path = os.path.join(self.admin.get_vault_session_path('unix1Resc'), filename)
        u2_path = os.path.join(self.admin.get_vault_session_path('unix2Resc'), filename)
        u3_path = os.path.join(self.admin.get_vault_session_path('unix3Resc'), filename)

        print( "u1_path ["+u1_path+"]")
        print( "u2_path ["+u2_path+"]")
        print( "u3_path ["+u3_path+"]")

        # modify files in the vaults to signify which resources
        with open(u1_path, 'wt') as f:
            f.write("UNIX_1")
        with open(u2_path, 'wt') as f:
            f.write("UNIX_2")
        with open(u3_path, 'wt') as f:
            f.write("UNIX_3")

        res_ctr = {}
        for i in range(0,10):
            stdout,_,_ = self.admin.run_icommand(['iget', filename, '-'])
            res_ctr[stdout] = 'found_it'

        self.admin.assert_icommand('iadmin modresc demoResc context "NO_CONTEXT"')

        # given random behavior we expect there to be more than one but
        # there are no guarantees
        assert len(res_ctr) <= 2

    def test_irm_specific_replica(self):
        # not allowed here - this is a managed replication resource
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 0 ", " & " + self.testfile])
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 1 ", " & " + self.testfile])
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 2 ", " & " + self.testfile])
        self.admin.assert_icommand("irm -n 1 " + self.testfile, 'STDOUT', 'deprecated')  # try to remove one of the managed replicas
        # should be listed 2x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 0 ", " & " + self.testfile])
        # should not be listed
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 1 ", " & " + self.testfile])
        # should be listed 2x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 2 ", " & " + self.testfile])

    @unittest.skip("--wlock has possible race condition due to Compound/Replication PDMO")
    def test_local_iput_collision_with_wlock(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_iquest_for_resc_hier__3039(self):
        # local setup
        # break the second child resource
        filename = "test_iquest_for_resc_hier__3039.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand_fail("iput " + filename, 'STDOUT_SINGLELINE', "put error")  # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix1Resc")  # should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix2Resc")  # should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix3Resc")  # should be listed
        self.admin.assert_icommand('iquest "select DATA_RESC_HIER where DATA_NAME like \'' + filename + "'\"", 'STDOUT_SINGLELINE', "demoResc;unix1Resc")  # should be listed
        self.admin.assert_icommand('iquest "select DATA_RESC_HIER where DATA_NAME like \'' + filename + "'\"", 'STDOUT_SINGLELINE', "demoResc;unix2Resc")  # should be listed
        self.admin.assert_icommand('iquest "select DATA_RESC_HIER where DATA_NAME like \'' + filename + "'\"", 'STDOUT_SINGLELINE', "demoResc;unix3Resc")  # should be listed


    def test_reliable_iput__ticket_2557(self):
        # local setup
        # break the second child resource
        indicies = [1,2,3]
        success = False
        test = 0
        for i in indicies:
            try:
                c1 = i % len(indicies)
                self.admin.assert_icommand("iadmin modresc unix%dResc path /nopes" % (c1), "STDOUT_SINGLELINE", "Previous resource path")
                filename = "reliableputfile.txt"
                filepath = lib.create_local_testfile(filename)
                self.admin.assert_icommand(['ilsresc', '--ascii'], 'STDOUT_SINGLELINE', "demoResc:replication")
                self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
                self.admin.assert_icommand("iput " + filename, 'STDERR_SINGLELINE', "put error")  # put file
                test = i+1 % len(indicies)
                self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix%dResc"%(test))  # should be listed
                test = i+2 % len(indicies)
                self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix%dResc"%(test))  # should be listed
                success = True
            except:
                success = False
                continue

            if success == True:
                break
        # cleanup
        oldvault = IrodsConfig().irods_directory + "/unix2RescVault"
        self.admin.assert_icommand("iadmin modresc unix2Resc path " + oldvault, "STDOUT_SINGLELINE", "Previous resource path")

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)  # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)                                   # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                            # remove third resource
        self.admin.assert_icommand("iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")          # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)                # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate to default resource
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)                   # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        # assertions
        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                   (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED') # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 6 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        # put file
        self.admin.assert_icommand("iput " + filename)
        # for debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # for debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource 1 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # default resource 1 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        # default resource 2 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource 2 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # default resource 3 should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # default resource 3 should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 3 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 3 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("no support for non-compound resources")
    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename, 'STDOUT', 'Specifying a minimum number of replicas to keep is deprecated.')  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - 1 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - 3 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

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

    def test_rebalance_different_sized_replicas__3486(self):
        filename = 'test_rebalance_different_sized_replicas__3486'
        large_file_size = 40000000
        lib.make_file(filename, large_file_size)
        self.admin.assert_icommand(['iput', filename])
        small_file_size = 20000
        lib.make_file(filename, small_file_size)
        self.update_specific_replica_for_data_objs_in_repl_hier([(filename, filename)])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [filename, str(large_file_size)])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [filename, str(small_file_size)])
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        self.admin.assert_icommand_fail(['ils', '-l'], 'STDOUT_SINGLELINE', str(large_file_size))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing, checks vault")
    def test_ifsck_with_resource_hierarchy__3512(self):
        filename = 'test_ifsck_with_resource_hierarchy__3512'
        lib.make_file(filename, 500)
        self.admin.assert_icommand(['iput', filename])
        vault_path = self.admin.get_vault_path(resource='unix1Resc')
        self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path])
        session_vault_path = self.admin.get_vault_session_path(resource='unix1Resc')
        with open(os.path.join(session_vault_path, filename), 'a') as f:
            f.write('additional file contents to trigger ifsck file size error')
        _, _, rc = self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path], 'STDOUT_SINGLELINE', ['CORRUPTION', filename, 'size'])
        self.assertNotEqual(rc, 0, 'ifsck should have non-zero error code on size mismatch')
        os.unlink(filename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing, checks vault")
    def test_ifsck_checksum_mismatch_print_error__3501(self):
        filename = 'test_ifsck_checksum_mismatch_print_error__3501'
        filesize = 500
        lib.make_file(filename, filesize)
        self.admin.assert_icommand(['iput', '-K', filename])
        vault_path = self.admin.get_vault_path(resource='unix1Resc')
        self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path])
        session_vault_path = self.admin.get_vault_session_path(resource='unix1Resc')
        with open(os.path.join(session_vault_path, filename), 'w') as f:
            f.write('i'*filesize)
        _, _, rc = self.admin.assert_icommand(['ifsck', '-KrR', 'demoResc;unix1Resc', vault_path], 'STDOUT_SINGLELINE', ['CORRUPTION', filename, 'checksum'])
        self.assertNotEqual(rc, 0, 'ifsck should have non-zero error code on checksum mismatch')
        os.unlink(filename)

    def test_rebalance_batching_replica_creation__3570(self):
        filename = 'test_rebalance_batching_replica_creation__3570'
        default_rebalance_batch_size = 500 # from librepl.cpp
        num_data_objects_to_use = default_rebalance_batch_size + 10
        file_size = 400
        lib.make_file(filename, file_size)
        for i in range(num_data_objects_to_use):
            self.admin.assert_icommand(['iput', filename, filename + '_' + str(i)])
            self.admin.assert_icommand(['itrim', '-S', 'demoResc', '-N1', filename + '_' + str(i)], 'STDOUT_SINGLELINE', 'Number of files trimmed = 1.')

        _, out, _ = self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', filename)
        self.assertEqual(len(out.split('\n')), num_data_objects_to_use + 6)
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        _, out, _ = self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', filename)
        self.assertEqual(len(out.split('\n')), 3*num_data_objects_to_use + 6)
        os.unlink(filename)

    def test_rebalance_batching_replica_update__3570(self):
        filename = 'test_rebalance_batching_replica_update__3570'
        default_rebalance_batch_size = 500 # from librepl.cpp
        num_data_objects_to_use = default_rebalance_batch_size + 10
        file_size = 327
        lib.make_file(filename, file_size)
        filename_list = []
        for i in range(num_data_objects_to_use):
            data_obj_name = filename + '_' + str(i)
            self.admin.assert_icommand(['iput', filename, data_obj_name])
            filename_list.append((data_obj_name, filename))
        self.update_specific_replica_for_data_objs_in_repl_hier(filename_list)


        _, out, _ = self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', filename)
        def count_good_and_bad(ils_l_output):
            num_up_to_date = 0
            num_out_of_date = 0
            for line in ils_l_output.split('\n'):
                if filename in line:
                    if '&' in line:
                        num_up_to_date += 1
                    else:
                        num_out_of_date += 1
            return num_up_to_date, num_out_of_date
        num_up_to_date, num_out_of_date = count_good_and_bad(out)
        self.assertEqual(num_up_to_date, num_data_objects_to_use)
        self.assertEqual(num_out_of_date, 2*num_data_objects_to_use)
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        _, out, _ = self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', filename)
        num_up_to_date, num_out_of_date = count_good_and_bad(out)
        self.assertEqual(num_up_to_date, 3*num_data_objects_to_use)
        self.assertEqual(num_out_of_date, 0)
        os.unlink(filename)

    def test_irepl_to_consumer_repl_hier_from_provider__4319(self):
        filename = 'test_irepl_to_consumer_repl_hier_from_provider__4319'
        lib.make_file(filename, 1 * 1024 * 1024 + 1)

        repl_host = test.settings.HOSTNAME_2
        self.admin.assert_icommand(['iadmin', 'modresc', 'unix2Resc', 'host', repl_host])
        self.admin.assert_icommand(['iadmin', 'modresc', 'unix3Resc', 'host', repl_host])
        test_resc = 'catalog_provider_resc'
        self.admin.assert_icommand(['iadmin', 'mkresc', test_resc, 'unixfilesystem', test.settings.ICAT_HOSTNAME + ':/tmp/irods/test_vault'], 'STDOUT_SINGLELINE', "Creating")

        initial_log_size = lib.get_file_size_by_path(IrodsConfig().server_log_path)

        try:
            self.admin.assert_icommand(['iput', '-R', test_resc, filename])
            self.admin.assert_icommand(['irepl', '-R', 'demoResc', filename])

            self.admin.assert_icommand(['ils', '-L', filename], 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
            for msg in ['rcDataCopy failed', 'SYS_BAD_FILE_DESCRIPTOR']:
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg=msg,
                        count=0,
                        server_log_path=IrodsConfig().server_log_path,
                        start_index=initial_log_size))
        finally:
            self.admin.assert_icommand(['irm', '-f', filename])
            os.unlink(filename)
            self.admin.assert_icommand(['iadmin', 'modresc', 'unix2Resc', 'host', test.settings.HOSTNAME_2])
            self.admin.assert_icommand(['iadmin', 'modresc', 'unix3Resc', 'host', test.settings.HOSTNAME_3])
            self.admin.assert_icommand(['iadmin', 'rmresc', test_resc])

class Test_Resource_MultiLayered(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc pass2Resc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc rrResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + test.settings.HOSTNAME_3 + ":" +
                                          irods_config.irods_directory + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc pass2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc pass2Resc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix3Resc")
        super(Test_Resource_MultiLayered, self).setUp()

    def tearDown(self):
        super(Test_Resource_MultiLayered, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unix3Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unix1Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc pass2Resc rrResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc pass2Resc")
            admin_session.assert_icommand("iadmin rmresc unix3Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc rrResc")
            admin_session.assert_icommand("iadmin rmresc pass2Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix2RescVault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix3RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_Resource_RandomWithinRandom(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random0 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random1 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random2 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random3 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unix0 'unixfilesystem' " + test.settings.HOSTNAME_1 + ":" +
                                          irods_config.irods_directory + "/unix0Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix1 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3 'unixfilesystem' " + test.settings.HOSTNAME_2 + ":" +
                                          irods_config.irods_directory + "/unix3Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc random0")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc random1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc random2")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc random3")
            admin_session.assert_icommand("iadmin addchildtoresc random0 unix0")
            admin_session.assert_icommand("iadmin addchildtoresc random1 unix1")
            admin_session.assert_icommand("iadmin addchildtoresc random2 unix2")
            admin_session.assert_icommand("iadmin addchildtoresc random3 unix3")
        super(Test_Resource_RandomWithinRandom, self).setUp()

    def tearDown(self):
        super(Test_Resource_RandomWithinRandom, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc random3 unix3")
            admin_session.assert_icommand("iadmin rmchildfromresc random2 unix2")
            admin_session.assert_icommand("iadmin rmchildfromresc random1 unix1")
            admin_session.assert_icommand("iadmin rmchildfromresc random0 unix0")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc random3")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc random2")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc random1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc random0")
            admin_session.assert_icommand("iadmin rmresc unix3")
            admin_session.assert_icommand("iadmin rmresc unix2")
            admin_session.assert_icommand("iadmin rmresc unix1")
            admin_session.assert_icommand("iadmin rmresc unix0")
            admin_session.assert_icommand("iadmin rmresc random3")
            admin_session.assert_icommand("iadmin rmresc random2")
            admin_session.assert_icommand("iadmin rmresc random1")
            admin_session.assert_icommand("iadmin rmresc random0")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
        irods_config = IrodsConfig()
        shutil.rmtree(irods_config.irods_directory + "/unix0Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix1Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix2Vault", ignore_errors=True)
        shutil.rmtree(irods_config.irods_directory + "/unix3Vault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_trying_next_child_after_child_votes_no__3315(self):
        self.admin.assert_icommand('iadmin modresc unix0 context minimum_free_space_for_create_in_bytes=1000000000000') # should cause unix0 to vote no because free_space is not set
        self.admin.assert_icommand('iadmin modresc unix1 context minimum_free_space_for_create_in_bytes=1000000000000') # should cause unix1 to vote no because free_space is not set
        self.admin.assert_icommand('iadmin modresc unix2 context minimum_free_space_for_create_in_bytes=1000000000000') # should cause unix2 to vote no because free_space is not set
        filename = 'test_trying_next_child_after_child_votes_no__3315.txt'
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand(['iput', filename])
        os.unlink(filepath)
