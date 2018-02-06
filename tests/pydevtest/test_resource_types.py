import commands
import getpass
import os
import psutil
import re
import shutil
import subprocess
import sys
import tempfile
from threading import Timer
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
import lib
from resource_suite import ResourceSuite, ResourceBase
from test_chunkydevtest import ChunkyDevTest

def statvfs_path_or_parent(path):
    while not os.path.exists(path):
        path = os.path.dirname(path)
    return os.statvfs(path)

class Test_Resource_RandomWithinReplication(ResourceSuite, ChunkyDevTest, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
        super(Test_Resource_RandomWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_RandomWithinReplication, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rrResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc rrResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixAVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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


class Test_Resource_RoundRobinWithinReplication(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
        super(Test_Resource_RoundRobinWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_RoundRobinWithinReplication, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc rrResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc rrResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc rrResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixAVault", ignore_errors=True)

    def test_next_child_iteration__2884(self):
        filename="foobar"
        lib.make_file( filename, 100 )

        # extract the next resource in the rr from the context string
        _, out, _ =self.admin.assert_icommand('ilsresc -l rrResc', 'STDOUT_SINGLELINE', 'demoResc')
        for line in out.split('\n'):
            if 'context:' in line:
                _, _, next_resc = line.partition('context:')
                next_resc = next_resc.strip()

        # determine the 'other' resource
        resc_set = set(['unixB1', 'unixB2'])
        remaining_set = resc_set - set([next_resc])
        resc_remaining = remaining_set.pop()

        # resources listed should be 'next_resc'
        self.admin.assert_icommand('iput ' + filename + ' file0')  # put file
        self.admin.assert_icommand('ils -L file0', 'STDOUT_SINGLELINE', next_resc)  # put file

        # resources listed should be 'resc_remaining'
        self.admin.assert_icommand('iput ' + filename + ' file1')  # put file
        self.admin.assert_icommand('ils -L file1', 'STDOUT_SINGLELINE', resc_remaining)  # put file

        # resources listed should be 'next_resc' once again
        self.admin.assert_icommand('iput ' + filename + ' file2')  # put file
        self.admin.assert_icommand('ils -L file2', 'STDOUT_SINGLELINE', next_resc)  # put file

        os.remove(filename)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

    def setUp(self):
        hostname = lib.get_hostname()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc 'unixfilesystem' " + hostname + ":" +
                                          lib.get_irods_top_level_dir() + "/demoRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        super(Test_Resource_Unixfilesystem, self).setUp()

    def tearDown(self):
        super(Test_Resource_Unixfilesystem, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/demoRescVault", ignore_errors=True)

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
        filename = 'test_msi_update_unixfilesystem_resource_free_space_and_acPostProcForParallelTransferReceived'
        filepath = lib.make_file(filename, 50000000)

        # make sure the physical path exists
        lib.mkdir_p(lib.get_vault_path(self.admin, 'demoResc'))

        corefile = lib.get_core_re_dir() + "/core.re"
        with lib.file_backed_up(corefile):
            rules_to_prepend = '''
acPostProcForParallelTransferReceived(*leaf_resource) {msi_update_unixfilesystem_resource_free_space(*leaf_resource);}
            '''
            time.sleep(1)  # remove once file hash fix is committed #2279
            lib.prepend_string_to_file(rules_to_prepend, corefile)
            time.sleep(1)  # remove once file hash fix is committed #2279

            self.user0.assert_icommand(['iput', filename])
            free_space = psutil.disk_usage(lib.get_vault_path(self.admin, 'demoResc')).free

        ilsresc_output = self.admin.run_icommand(['ilsresc', '-l', 'demoResc'])[1]
        for l in ilsresc_output.splitlines():
            if l.startswith('free space:'):
                ilsresc_freespace = int(l.rpartition(':')[2])
        assert abs(free_space - ilsresc_freespace) < 4096*10, 'free_space {0}, ilsresc free space {1}'.format(free_space, ilsresc_freespace)
        os.unlink(filename)

    def test_key_value_passthru(self):
        env = os.environ.copy()
        env['spLogLevel'] = '11'
        lib.restart_irods_server(env=env)

        lib.make_file('file.txt', 15)
        initial_log_size = lib.get_log_size('server')
        self.user0.assert_icommand('iput --kv_pass="put_key=val1" file.txt')
        assert lib.count_occurrences_of_string_in_log('server', 'key [put_key] - value [val1]', start_index=initial_log_size) in [1, 2]  # double print if collection missing

        initial_log_size = lib.get_log_size('server')
        self.user0.assert_icommand('iget -f --kv_pass="get_key=val3" file.txt other.txt')
        assert lib.count_occurrences_of_string_in_log('server', 'key [get_key] - value [val3]', start_index=initial_log_size) in [1, 2]  # double print if collection missing
        lib.restart_irods_server()
        lib.assert_command('rm -f file.txt other.txt')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_ifsck__2650(self):
        # local setup
        filename = 'fsckfile.txt'
        filepath = lib.create_local_testfile(filename)
        full_logical_path = '/' + self.admin.zone_name + '/home/' + self.admin.username + '/' + self.admin._session_id + '/' + filename
        # assertions
        self.admin.assert_icommand('ils -L ' + filename, 'STDERR_SINGLELINE', 'does not exist')  # should not be listed
        self.admin.assert_icommand('iput -K ' + self.testfile + ' ' + full_logical_path)  # iput
        self.admin.assert_icommand('ils -L ' + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        file_vault_full_path = os.path.join(lib.get_vault_session_path(self.admin), filename)
        # method 1
        self.admin.assert_icommand('ichksum -K ' + full_logical_path, 'STDOUT_MULTILINE',
                                   ['Total checksum performed = 1, Failed checksum = 0',
                                    'sha2:0MczF/+UQ4lYmtu417LDmMb4mEarpxPShHfg1PhLtQw='])  # ichksum
        # method 2
        self.admin.assert_icommand("iquest \"select DATA_CHECKSUM where DATA_NAME = '%s'\"" % filename,
                                   'STDOUT_SINGLELINE', ['DATA_CHECKSUM = sha2:0MczF/+UQ4lYmtu417LDmMb4mEarpxPShHfg1PhLtQw='])  # iquest
        # method 3
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', filename)  # ils
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # change content in vault
        with open(file_vault_full_path, 'r+') as f:
            f.seek(0)
            f.write("x")
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION', 'checksum not consistent with iRODS object'])  # ifsck
        # change size in vault
        lib.cat(file_vault_full_path, 'extra letters')
        self.admin.assert_icommand('ifsck ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION', 'size not consistent with iRODS object'])  # ifsck
        # unregister, reregister (to update filesize in iCAT), recalculate checksum, and confirm
        self.admin.assert_icommand('irm -U ' + full_logical_path)
        self.admin.assert_icommand('ireg ' + file_vault_full_path + ' ' + full_logical_path)
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['WARNING: checksum not available'])  # ifsck
        self.admin.assert_icommand('ichksum -f ' + full_logical_path, 'STDOUT_MULTILINE',
                                   ['Total checksum performed = 1, Failed checksum = 0',
                                    'sha2:zJhArM/en4wfI9lVq+AIFAZa6RTqqdC6LVXf6tPbqxI='])
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # local cleanup
        os.remove(filepath)


class Test_Resource_Passthru(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Resource_Passthru, self).setUp()

    def tearDown(self):
        super(Test_Resource_Passthru, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass


class Test_Resource_WeightedPassthru(ResourceBase, unittest.TestCase):

    def setUp(self):
        hostname = lib.get_hostname()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + hostname + ":" +
                                          lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB 'unixfilesystem' " + hostname + ":" +
                                          lib.get_irods_top_level_dir() + "/unixBVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc w_pt passthru '' 'write=1.0;read=1.0'", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc w_pt")
            admin_session.assert_icommand("iadmin addchildtoresc w_pt unixB")
        super(Test_Resource_WeightedPassthru, self).setUp()

    def tearDown(self):
        super(Test_Resource_WeightedPassthru, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc w_pt unixB")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc w_pt")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmresc unixB")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin rmresc w_pt")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixBVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixAVault", ignore_errors=True)

    def test_weighted_passthrough(self):
        filename = "some_local_file.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("iput " + filepath)
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "local")

        # repave a copy in the vault to differentiate
        vaultpath = os.path.join(lib.get_irods_top_level_dir(), "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
        subprocess.check_call("echo 'THISISBROEKN' | cat > %s" % (vaultpath), shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN")

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE")

    def test_weighted_passthrough__2789(self):

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
        vaultpath = os.path.join(lib.get_irods_top_level_dir(), "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
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
        vaultpath = os.path.join(lib.get_irods_top_level_dir(), "unixBVault/home/" + self.admin.username, os.path.basename(self.admin._session_id), filename)
        subprocess.check_call("echo 'THISISBROEKN' | cat > %s" % (vaultpath), shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN")
        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE")
        self.admin.assert_icommand("irm -f " + filename)

class Test_Resource_Deferred(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc deferred", 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Resource_Deferred, self).setUp()

    def tearDown(self):
        super(Test_Resource_Deferred, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass


class Test_Resource_Random(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix3Resc")
        super(Test_Resource_Random, self).setUp()

    def tearDown(self):
        super(Test_Resource_Random, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix3Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix3Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix2RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix3RescVault", ignore_errors=True)

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
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc nonblocking " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/nbVault", 'STDOUT_SINGLELINE', 'nonblocking')
        super(Test_Resource_NonBlocking, self).setUp()

    def tearDown(self):
        super(Test_Resource_NonBlocking, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')


class Test_Resource_CompoundWithMockarchive(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc mockarchive " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'mockarchive')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_CompoundWithMockarchive, self).setUp()

    def tearDown(self):
        super(Test_Resource_CompoundWithMockarchive, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheRescVault", ignore_errors=True)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_CompoundWithUnivmss(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc univmss " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'univmss')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_CompoundWithUnivmss, self).setUp()

    def tearDown(self):
        super(Test_Resource_CompoundWithUnivmss, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheRescVault", ignore_errors=True)

    def test_irm_with_no_stage__2930(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("itrim -n0 -N1 " + self.testfile, 'STDOUT_SINGLELINE', "files trimmed") # trim cache copy
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed

        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand("irm " + self.testfile ) # remove archive replica
        count = lib.count_occurrences_of_string_in_log('server', 'argv:stageToCache', start_index=initial_log_size)
        assert 0 == count

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_Compound(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Resource_Compound, self).setUp()

    def tearDown(self):
        super(Test_Resource_Compound, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree("rm -rf " + lib.get_irods_top_level_dir() + "/cacheRescVault", ignore_errors=True)

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
        self.admin.assert_icommand("iput -R pydevtest_TestResc " + filename)

        logical_path = os.path.join( self.admin.session_collection, filename )
        logical_path_rsync = os.path.join( self.admin.session_collection, filename_rsync )

        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['logical_path_rsync'] = logical_path_rsync
        parameters['dest_resc'] = 'null'
        rule_file_path = 'test_msiDataObjRsync__2976.r'
        rule_str = '''
test_msiDataObjRepl {{
    *err = errormsg( msiDataObjRsync(*SourceFile,"IRODS_TO_IRODS",*Resource,*DestFile,*status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}", *DestFile="{logical_path_rsync}"
OUTPUT ruleExecOut
'''.format(**parameters)

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
        self.admin.assert_icommand('iput -rR demoResc ' + dir_name)

        logical_path = os.path.join( self.admin.session_collection, dir_name )
        logical_path_rsync = os.path.join( self.admin.session_collection, dir_name_rsync )

        self.admin.assert_icommand("irsync -r i:" + logical_path + " i:" + logical_path_rsync )
        self.admin.assert_icommand("ils -lr " + logical_path, 'STDOUT_SINGLELINE', dir_name )
        self.admin.assert_icommand("ils -lr " + logical_path_rsync, 'STDOUT_SINGLELINE', dir_name_rsync )

    def test_msiCollRsync__2976(self):
        dir_name = 'test_irsync_for_collection__2976'
        dir_name_rsync = dir_name + '_rsync'
        lib.create_directory_of_small_files(dir_name,10)
        self.admin.assert_icommand('iput -rR demoResc ' + dir_name)

        logical_path = os.path.join( self.admin.session_collection, dir_name )
        logical_path_rsync = os.path.join( self.admin.session_collection, dir_name_rsync )

        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['logical_path_rsync'] = logical_path_rsync
        parameters['dest_resc'] = 'demoResc'
        rule_file_path = 'test_msiDataObjRsync__2976.r'
        rule_str = '''
test_msiCollRepl {{
    *err = errormsg( msiCollRsync(*SourceColl,*DestColl,*Resource,"IRODS_TO_IRODS",*status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceColl="{logical_path}", *Resource="{dest_resc}", *DestColl="{logical_path_rsync}"
OUTPUT ruleExecOut
'''.format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', dir_name )
        self.admin.assert_icommand("ils -l " + logical_path_rsync, 'STDOUT_SINGLELINE', dir_name_rsync )

    def test_iphymv_as_admin__2995(self):
        filename = "test_iphymv_as_admin__2995_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R pydevtest_TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        self.admin.assert_icommand("iphymv -M -R demoResc " + logical_path )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

    def test_irepl_as_admin__2988(self):
        filename = "test_irepl_as_admin__2988_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R pydevtest_TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        self.admin.assert_icommand("irepl -M -R demoResc " + logical_path )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "local filesystem check")
    def test_msiDataObjUnlink__2983(self):
        filename = "test_test_msiDataObjUnlink__2983.txt"
        filepath = lib.create_local_testfile(filename)
        logical_path = os.path.join( self.admin.session_collection, filename )

        self.admin.assert_icommand("ireg " + filepath + " " + logical_path)

        parameters = {}
        parameters['logical_path'] = logical_path
        rule_file_path = 'test_msiDataObjUnlink__2983.r'
        rule_str = '''
test_msiDataObjUnlink {{
    *err = errormsg( msiDataObjUnlink("objPath=*SourceFile++++unreg=",*Status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceFile="{logical_path}"
OUTPUT ruleExecOut
'''.format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)
        self.admin.assert_icommand_fail("ils -l " + logical_path, 'STDOUT_SINGLELINE', filename)

        assert os.path.isfile(filepath)

    def test_msiDataObjRepl_as_admin__2988(self):
        filename = "test_msiDataObjRepl_as_admin__2988_file.txt"
        filepath = lib.create_local_testfile(filename)
        self.user1.assert_icommand("iput -R pydevtest_TestResc " + filename)

        logical_path = os.path.join( self.user1.session_collection, filename )
        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['dest_resc'] = 'demoResc'
        rule_file_path = 'test_msiDataObjRepl_as_admin__2988.r'
        rule_str = '''
test_msiDataObjRepl {{
    *err = errormsg( msiDataObjRepl(*SourceFile,"destRescName=*Resource++++irodsAdmin=",*Status), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *SourceFile="{logical_path}", *Resource="{dest_resc}"
OUTPUT ruleExecOut
'''.format(**parameters)

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

        physical_path = os.path.join(lib.get_irods_top_level_dir(), 'cacheRescVault')
        physical_path = os.path.join(physical_path,os.path.basename(self.admin.session_collection))
        physical_path = os.path.join(physical_path,filename)

        logical_path = os.path.join( self.admin.session_collection, filename )
        parameters = {}
        parameters['logical_path'] = logical_path
        parameters['physical_path'] = physical_path
        parameters['resc_hier'] = 'demoResc;cacheResc'

        rule_file_path = 'test_msiDataObjRepl_as_admin__2988.r'
        rule_str = '''
test_msiDataObjRepl {{
    *err = errormsg( msisync_to_archive(*RescHier,*PhysicalPath,*LogicalPath), *msg );
    if( 0 != *err ) {{
        writeLine( "stdout", "*err - *msg" );
    }}
}}

INPUT *LogicalPath="{logical_path}", *PhysicalPath="{physical_path}",*RescHier="{resc_hier}"
OUTPUT ruleExecOut
'''.format(**parameters)

        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        # invoke rule
        self.admin.assert_icommand('irule -F ' + rule_file_path)

        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

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
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iget_prefer_from_archive_corrupt_archive__ticket_3145(self):
        # define core.re filepath
        corefile = lib.get_core_re_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)

        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)        # put file

        # manually remove the replica in the archive vault
        phypath = os.path.join(lib.get_vault_session_path(self.admin, 'archiveResc'), filename)
        os.remove(phypath)

        # manipulate the core.re to add the new policy
        shutil.copy(corefile, backupcorefile)
        with open(corefile, 'a') as f:
            f.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')

        self.admin.assert_icommand("irm -f " + filename)

        # restore the original core.re
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

        # local cleanup
        os.remove(filepath)

    @unittest.skip("TEMPORARY")
    def test_iget_prefer_from_archive__ticket_1660(self):
        # define core.re filepath
        corefile = lib.get_core_re_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)

        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)        # put file

        # manually update the replica in archive vault
        output = self.admin.run_icommand('ils -L ' + filename)
        archivereplicaphypath = output[1].split()[-1]  # split into tokens, get the last one
        with open(archivereplicaphypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE\n')
        # get file
        retrievedfile = "retrieved.txt"
        os.system("rm -f %s" % retrievedfile)
        self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from cache

        # confirm retrieved file is same as original
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))

        # manipulate the core.re to add the new policy
        shutil.copy(corefile, backupcorefile)
        with open(corefile, 'a') as f:
            f.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')

        # restart the server to reread the new core.re
        os.system(lib.get_irods_top_level_dir() + "/iRODS/irodsctl stop")
        os.system(lib.get_irods_top_level_dir() + "/tests/zombiereaper.sh")
        os.system(lib.get_irods_top_level_dir() + "/iRODS/irodsctl start")

        # manually update the replica in archive vault
        output = self.admin.run_icommand('ils -L ' + filename)
        archivereplicaphypath = output[1].split()[-1]  # split into tokens, get the last one
        with open(archivereplicaphypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE **AGAIN**\n')

        # get the file
        self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from archive

        # confirm this is the new archive file
        matchfound = False
        with open(retrievedfile) as f:
            for line in f:
                if "**AGAIN**" in line:
                    matchfound = True
        assert matchfound

        # restore the original core.re
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should be listed
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 1 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_ReplicationWithinReplication(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc replResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc replResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB2")
        super(Test_Resource_ReplicationWithinReplication, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationWithinReplication, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc replResc unixB2")
            admin_session.assert_icommand("iadmin rmchildfromresc replResc unixB1")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unixA")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc replResc")
            admin_session.assert_icommand("iadmin rmresc unixB1")
            admin_session.assert_icommand("iadmin rmresc unixB2")
            admin_session.assert_icommand("iadmin rmresc unixA")
            admin_session.assert_icommand("iadmin rmresc replResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB2Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixB1Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unixAVault", ignore_errors=True)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # replica 2 should be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

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
        self.admin.assert_icommand("irepl " + filename)               # replicate overtop default resource
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
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
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from grid
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

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc compResc1 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc compResc2 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Resource_ReplicationToTwoCompound, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationToTwoCompound, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
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
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveResc1Vault", ignore_errors=True)
            shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheResc1Vault", ignore_errors=True)
            shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveResc2Vault", ignore_errors=True)
            shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheResc2Vault", ignore_errors=True)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_iget_prefer_from_archive__ticket_1660(self):
        # define core.re filepath
        corefile = lib.get_core_re_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName

        # new file to put and get
        filename = "archivepolicyfile.txt"
        filepath = lib.create_local_testfile(filename)
        # manipulate core.re (leave as 'when_necessary' - default)

        # put the file
        self.admin.assert_icommand("iput " + filename)     # put file

        # manually update the replicas in archive vaults
        stdout = self.admin.run_icommand('ils -L ' + filename)[1]
        print stdout
        archive1replicaphypath = stdout.split()[-19]  # split into tokens, get the 19th from the end
        archive2replicaphypath = stdout.split()[-1]  # split into tokens, get the last one
        print archive1replicaphypath
        print archive2replicaphypath
        with open(archive1replicaphypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE 1\n')
        with open(archive2replicaphypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE 2\n')

        # get file
        retrievedfile = "retrieved.txt"
        os.system("rm -f %s" % retrievedfile)
        self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from cache
        # confirm retrieved file is same as original
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))
        print "original file diff confirmed"

        # manipulate the core.re to add the new policy
        shutil.copy(corefile, backupcorefile)
        with open(corefile, 'a') as f:
            f.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')

        # restart the server to reread the new core.re
        lib.restart_irods_server()

        # manually update the replicas in archive vaults
        stdout = self.admin.run_icommand('ils -L ' + filename)[1]
        archivereplica1phypath = stdout.split()[-19]  # split into tokens, get the 19th from the end
        archivereplica2phypath = stdout.split()[-1]  # split into tokens, get the last one
        print archive1replicaphypath
        print archive2replicaphypath
        with open(archivereplica1phypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE 1 **AGAIN**\n')
        with open(archivereplica2phypath, 'w') as f:
            f.write('MANUALLY UPDATED ON ARCHIVE 2 **AGAIN**\n')

        # confirm the new content is on disk
        with open(archivereplica1phypath) as f:
            for line in f:
                print line
        with open(archivereplica2phypath) as f:
            for line in f:
                print line

        # confirm the core file has new policy
        print "----- confirm core has new policy ----"
        with open(corefile) as f:
            for line in f:
                if 'pep_' in line:
                    print line
                else:
                    print '.',
        print "----- confirmation done ----"

        self.admin.assert_icommand(['iget', '-f', filename, retrievedfile])

        # confirm this is the new archive file
        with open(retrievedfile) as f:
            for line in f:
                print line
                if 'AGAIN' in line:
                    break
            else:
                assert False

        # restore the original core.re
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 4x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 4x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 4x - replica 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])  # should be listed 4x - replica 4
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        # back up core file
        corefile = lib.get_core_re_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName
        shutil.copy(corefile, backupcorefile)

        # manipulate the core.re to add the new policy
        with open(corefile, 'a') as f:
            f.write('pep_resource_resolve_hierarchy_pre(*OUT){*OUT="compound_resource_cache_refresh_policy=always";}\n')

        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc compResc1 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc compResc2 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive, self).setUp()

    def tearDown(self):
        super(Test_Resource_ReplicationToTwoCompoundResourcesWithPreferArchive, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
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
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveResc1Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheResc1Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveResc2Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/cacheResc2Vault", ignore_errors=True)

        # restore the original core.re
        corefile = lib.get_core_re_dir() + "/core.re"
        backupcorefile = corefile + "--" + self._testMethodName
        shutil.copy(backupcorefile, corefile)
        os.remove(backupcorefile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from cacheResc only
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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - replica 3
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 4x - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 4x - replica 2
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 4x - replica 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", filename])  # should be listed 4x - replica 4
        # should not have any extra replicas
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_RoundRobin(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
        super(Test_Resource_RoundRobin, self).setUp()

    def tearDown(self):
        super(Test_Resource_RoundRobin, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix2RescVault", ignore_errors=True)

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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        self.user1.assert_icommand("iput " + filename + " file0.txt")
        self.user1.assert_icommand("iput " + filename + " file1.txt")

        self.user1.assert_icommand("ils -l", 'STDOUT_SINGLELINE', "unix1Resc")
        self.user1.assert_icommand("ils -l", 'STDOUT_SINGLELINE', "unix2Resc")

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)


class Test_Resource_Replication(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix3Resc")
        super(Test_Resource_Replication, self).setUp()

    def tearDown(self):
        super(Test_Resource_Replication, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix3Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix3Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix2RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix3RescVault", ignore_errors=True)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Checks local file")
    def test_recursive_descent__ticket_3672(self):
        # default behavior - do the recursive descent
        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        self.assertEquals(3, lib.count_occurrences_of_string_in_log('server', 'matching old count', start_index=initial_log_size))

        # default behavior with flag set - do the recursive descent
        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'context', 'recursive_rebalance=true'])
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        self.assertEquals(3, lib.count_occurrences_of_string_in_log('server', 'matching old count', start_index=initial_log_size))

        # disable recursive descent
        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'context', 'recursive_rebalance=false'])
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        self.assertEquals(0, lib.count_occurrences_of_string_in_log('server', 'matching old count', start_index=initial_log_size))

    def test_irm_specific_replica(self):
        # not allowed here - this is a managed replication resource
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 0 ", " & " + self.testfile])
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 1 ", " & " + self.testfile])
        # should be listed 3x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 2 ", " & " + self.testfile])
        self.admin.assert_icommand("irm -n 1 " + self.testfile)  # try to remove one of the managed replicas
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

    def test_reliable_iput__ticket_2557(self):
        # local setup
        # break the second child resource
        self.admin.assert_icommand("iadmin modresc unix2Resc path /nopes", 'STDOUT_SINGLELINE', "unix2RescVault")
        filename = "reliableputfile.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand_fail("iput " + filename, 'STDOUT_SINGLELINE', "put error")  # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix1Resc")  # should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', "unix3Resc")  # should be listed

        # cleanup
        oldvault = lib.get_irods_top_level_dir() + "/unix2RescVault"
        self.admin.assert_icommand("iadmin modresc unix2Resc path " + oldvault, 'STDOUT_SINGLELINE', "/nopes")

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

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

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
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
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

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        # put file, but trim 'cache' copy (purgec) (backwards compatibility)
        self.admin.assert_icommand("iput --purgec " + filename)
        # should not be listed (trimmed first replica)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed twice - replica 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        # should be listed twice - replica 3 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wb') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed twice - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed twice - 2 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wb') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - 1 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - 3 of 3

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing, checks vault")
    def test_ifsck_with_resource_hierarchy__3512(self):
        filename = 'test_ifsck_with_resource_hierarchy__3512'
        lib.make_file(filename, 500)
        self.admin.assert_icommand(['iput', filename])
        vault_path = lib.get_vault_path(self.admin, resource='unix1Resc')
        self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path])
        session_vault_path = lib.get_vault_session_path(self.admin, resource='unix1Resc')
        with open(os.path.join(session_vault_path, filename), 'a') as f:
            f.write('additional file contents to trigger ifsck file size error')
        rc, _, _ = self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path], 'STDOUT_SINGLELINE', ['CORRUPTION', filename, 'size'])
        self.assertNotEqual(rc, 0, 'ifsck should have non-zero error code on size mismatch')
        os.unlink(filename)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing, checks vault")
    def test_ifsck_checksum_mismatch_print_error__3501(self):
        filename = 'test_ifsck_checksum_mismatch_print_error__3501'
        filesize = 500
        lib.make_file(filename, filesize)
        self.admin.assert_icommand(['iput', '-K', filename])
        vault_path = lib.get_vault_path(self.admin, resource='unix1Resc')
        self.admin.assert_icommand(['ifsck', '-rR', 'demoResc;unix1Resc', vault_path])
        session_vault_path = lib.get_vault_session_path(self.admin, resource='unix1Resc')
        with open(os.path.join(session_vault_path, filename), 'w') as f:
            f.write('i'*filesize)
        rc, _, _ = self.admin.assert_icommand(['ifsck', '-KrR', 'demoResc;unix1Resc', vault_path], 'STDOUT_SINGLELINE', ['CORRUPTION', filename, 'checksum'])
        self.assertNotEqual(rc, 0, 'ifsck should have non-zero error code on checksum mismatch')
        os.unlink(filename)

    def test_rebalance_different_sized_replicas__3486(self):
        filename = 'test_rebalance_different_sized_replicas__3486'
        large_file_size = 40000000
        lib.make_file(filename, large_file_size)
        self.admin.assert_icommand(['iput', filename])
        small_file_size = 20000
        lib.make_file(filename, small_file_size)
        self.admin.assert_icommand(['iput', '-f', '-n', '0', filename])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [filename, str(large_file_size)])
        self.admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', [filename, str(small_file_size)])
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        self.admin.assert_icommand_fail(['ils', '-l'], 'STDOUT_SINGLELINE', str(large_file_size))

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Reads server log')
    def test_rebalance_logging_replica_update__3463(self):
        filename = 'test_rebalance_logging_replica_update__3463'
        file_size = 400
        lib.make_file(filename, file_size)
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['iput', '-f', '-n', '0', filename])
        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        data_id = lib.get_data_id(self.admin, self.admin.session_collection, filename)
        self.assertEquals(2, lib.count_occurrences_of_string_in_log('server', 'updating a replica for data id [{0}]'.format(str(data_id)), start_index=initial_log_size))
        os.unlink(filename)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, 'Reads server log')
    def test_rebalance_logging_replica_creation__3463(self):
        filename = 'test_rebalance_logging_replica_creation__3463'
        file_size = 400
        lib.make_file(filename, file_size)
        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['itrim', '-S', 'demoResc', '-N1', filename], 'STDOUT_SINGLELINE', 'Total size trimmed = 0.000 MB. Number of files trimmed = 1.')
        initial_log_size = lib.get_log_size('server')
        self.admin.assert_icommand(['iadmin', 'modresc', 'demoResc', 'rebalance'])
        data_id = lib.get_data_id(self.admin, self.admin.session_collection, filename)
        self.assertEquals(2, lib.count_occurrences_of_string_in_log('server', 'replicating data id [{0}]'.format(str(data_id)), start_index=initial_log_size))
        os.unlink(filename)

@unittest.skipIf(False == configuration.USE_MUNGEFS, "These tests require mungefs")
class Test_Resource_Replication_With_Retry(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            self.munge_mount=tempfile.mkdtemp(prefix='munge_mount_')
            self.munge_target=tempfile.mkdtemp(prefix='munge_target_')
            lib.assert_command('mungefs ' + self.munge_mount + ' -omodules=subdir,subdir=' + self.munge_target)

            self.munge_resc = 'ufs1munge'
            self.munge_passthru = 'pt1'
            self.normal_vault = lib.get_irods_top_level_dir() + '/vault2'

            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand('iadmin mkresc ' + self.munge_passthru + ' passthru', 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand('iadmin mkresc pt2 passthru', 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand('iadmin mkresc ' + self.munge_resc + ' unixfilesystem ' + configuration.HOSTNAME_1 + ':' +
                                          self.munge_mount, 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand('iadmin mkresc ufs2 unixfilesystem ' + configuration.HOSTNAME_2 + ':' +
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
        self.log_message_starting_location = lib.get_log_size('server')

        self.valid_scenarios = [
            self.test_scenario(1, 1, 1, self.make_context()),
            self.test_scenario(3, 1, 1, self.make_context('3')),
            self.test_scenario(1, 3, 1, self.make_context(delay='3')),
            self.test_scenario(1, 3, 1, self.make_context(delay='3')),
            self.test_scenario(3, 2, 2, self.make_context('3', '2', '2')),
            self.test_scenario(3, 2, 1.5, self.make_context('3', '2', '1.5'))
            ]

        self.invalid_scenarios = [
            self.test_scenario(1, 1, 1, self.make_context('-2')),
            self.test_scenario(1, 1, 1, self.make_context('2.0')),
            self.test_scenario(1, 1, 1, self.make_context('one')),
            self.test_scenario(1, 1, 1, self.make_context(delay='0')),
            self.test_scenario(1, 1, 1, self.make_context(delay='-2')),
            self.test_scenario(1, 1, 1, self.make_context(delay='2.0')),
            self.test_scenario(1, 1, 1, self.make_context(delay='one')),
            self.test_scenario(3, 2, 1, self.make_context('3', '2', '0')),
            self.test_scenario(3, 2, 1, self.make_context('3', '2', '0.5')),
            self.test_scenario(3, 2, 1, self.make_context('3', '2', '-2')),
            self.test_scenario(3, 2, 1, self.make_context('3', '2', 'one'))
            ]
        super(Test_Resource_Replication_With_Retry, self).setUp()

    def tearDown(self):
        super(Test_Resource_Replication_With_Retry, self).tearDown()

        # unmount mungefs and destroy directories
        lib.assert_command(['fusermount', '-u', self.munge_mount])
        shutil.rmtree(self.munge_mount, ignore_errors=True)
        shutil.rmtree(self.munge_target, ignore_errors=True)
        shutil.rmtree(self.normal_vault, ignore_errors=True)

        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc " + self.munge_passthru)
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc pt2")
            admin_session.assert_icommand("iadmin rmchildfromresc " + self.munge_passthru + ' ' + self.munge_resc)
            admin_session.assert_icommand("iadmin rmchildfromresc pt2 ufs2")
            admin_session.assert_icommand("iadmin rmresc " + self.munge_resc)
            admin_session.assert_icommand("iadmin rmresc ufs2")
            admin_session.assert_icommand("iadmin rmresc " + self.munge_passthru)
            admin_session.assert_icommand("iadmin rmresc pt2")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')

    # Nested class for containing test case information
    class test_scenario(object):
        def __init__(self, retries, delay, multiplier, context_string=None):
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

        fail_message_count = lib.count_occurrences_of_string_in_log(
                                  'server',
                                  expected_message,
                                  start_index=self.log_message_starting_location)
        self.assertTrue(fail_message_count == expected_count,
                        msg='counted:[{0}]; expected:[{1}]'.format(
                            str(fail_message_count), str(expected_count)))
        self.log_message_starting_location = lib.get_log_size('server')

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
        lib.assert_command(mungefsctl_call)

    def run_rebalance_test(self, scenario, filename):
        # Setup test and context string
        filepath = lib.create_local_testfile(filename)
        if scenario.context_string:
            self.admin.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

        # Get wait time for mungefs to reset
        wait_time = scenario.munge_delay()
        self.assertTrue( wait_time > 0, msg='wait time for mungefs [{0}] <= 0'.format(wait_time))

        # For each test, iput will fail to replicate; then we can run a rebalance
        # Perform corrupt read (checksum) test
        self.run_mungefsctl('read', '--corrupt_data')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message )
        self.verify_new_failure_messages_in_log(scenario.retries + 1)
        munge_thread = Timer(wait_time, self.run_mungefsctl, ['read'])
        munge_thread.start()
        self.admin.assert_icommand('iadmin modresc demoResc rebalance')
        munge_thread.join()
        self.admin.assert_icommand(['irm', '-f', filename])
        self.verify_new_failure_messages_in_log(scenario.retries)

        # Perform corrupt size test
        self.run_mungefsctl('getattr', '--corrupt_size')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(scenario.retries + 1)
        munge_thread = Timer(wait_time, self.run_mungefsctl, ['getattr'])
        munge_thread.start()
        self.admin.assert_icommand('iadmin modresc demoResc rebalance')
        munge_thread.join()
        self.admin.assert_icommand(['irm', '-f', filename])
        self.verify_new_failure_messages_in_log(scenario.retries)

        # Cleanup
        os.unlink(filepath)

    def run_iput_test(self, scenario, filename):
        # Setup test and context string
        filepath = lib.create_local_testfile(filename)
        if scenario.context_string:
            self.admin.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

        # Get wait time for mungefs to reset
        wait_time = scenario.munge_delay()
        self.assertTrue( wait_time > 0, msg='wait time for mungefs [{0}] <= 0'.format(wait_time))

        # Perform corrupt read (checksum) test
        self.run_mungefsctl('read', '--corrupt_data')
        munge_thread = Timer(wait_time, self.run_mungefsctl, ['read'])
        munge_thread.start()
        self.admin.assert_icommand(['iput', '-K', filepath])
        munge_thread.join()
        self.admin.assert_icommand(['irm', '-f', filename])
        self.verify_new_failure_messages_in_log(scenario.retries)

        # Perform corrupt size test
        self.run_mungefsctl('getattr', '--corrupt_size')
        munge_thread = Timer(wait_time, self.run_mungefsctl, ['getattr'])
        munge_thread.start()
        self.admin.assert_icommand(['iput', '-K', filepath])
        munge_thread.join()
        self.admin.assert_icommand(['irm', '-f', filename])
        self.verify_new_failure_messages_in_log(scenario.retries)

        # Cleanup
        os.unlink(filepath)

    # Helper function to recreate replication node to ensure empty context string
    def reset_repl_resource(self):
        self.admin.assert_icommand("iadmin rmchildfromresc demoResc " + self.munge_passthru)
        self.admin.assert_icommand("iadmin rmchildfromresc demoResc pt2")
        self.admin.assert_icommand("iadmin rmresc demoResc")
        self.admin.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
        self.admin.assert_icommand('iadmin addchildtoresc demoResc ' + self.munge_passthru)
        self.admin.assert_icommand('iadmin addchildtoresc demoResc pt2')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_valid(self):
        for scenario in self.valid_scenarios:
            self.run_iput_test(scenario, 'test_repl_retry_iput_valid')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_invalid(self):
        for scenario in self.invalid_scenarios:
            self.run_iput_test(scenario, 'test_repl_retry_iput_invalid')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_valid(self):
        for scenario in self.valid_scenarios:
            self.run_rebalance_test(scenario, 'test_repl_retry_rebalance_valid')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_invalid(self):
        for scenario in self.invalid_scenarios:
            self.run_rebalance_test(scenario, 'test_repl_retry_rebalance_invalid')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_no_context(self):
        self.reset_repl_resource()
        self.run_iput_test(self.test_scenario(1, 1, 1), 'test_repl_retry_iput_no_context')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_no_context(self):
        self.reset_repl_resource()
        self.run_rebalance_test(self.test_scenario(1, 1, 1), 'test_repl_retry_rebalance_no_context')

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_iput_no_retries(self):
        filename = "test_repl_retry_iput_no_retries"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand('iadmin modresc demoResc context "{0}"'.format(self.make_context('0')))

        # Perform corrupt read (checksum) test
        self.run_mungefsctl('read', '--corrupt_data')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.run_mungefsctl('read')
        self.admin.assert_icommand(['irm', '-f', filename])

        # Perform corrupt size test
        self.run_mungefsctl('getattr', '--corrupt_size')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.run_mungefsctl('getattr')

        self.admin.assert_icommand(['irm', '-f', filename])
        os.unlink(filepath)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_rebalance_no_retries(self):
        filename = "test_repl_retry_rebalance_no_retries"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand('iadmin modresc demoResc context "{0}"'.format(self.make_context('0')))

        # Perform corrupt read (checksum) test
        self.run_mungefsctl('read', '--corrupt_data')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.admin.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.run_mungefsctl('read')
        self.admin.assert_icommand(['irm', '-f', filename])

        # Perform corrupt size test
        self.run_mungefsctl('getattr', '--corrupt_size')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.admin.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1)
        self.run_mungefsctl('getattr')

        self.admin.assert_icommand(['irm', '-f', filename])
        os.unlink(filepath)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Reads server log")
    def test_repl_retry_multiplier_overflow(self):
        filename = "test_repl_retry_iput_large_multiplier"
        filepath = lib.create_local_testfile(filename)
        large_number = pow(2, 32)
        scenario = self.test_scenario(2, 1, large_number, self.make_context('2', '1', str(large_number)))
        failure_message = 'bad numeric conversion'
        self.admin.assert_icommand('iadmin modresc demoResc context "{0}"'.format(scenario.context_string))

        self.run_mungefsctl('read', '--corrupt_data')
        self.admin.assert_icommand_fail(['iput', '-K', filepath], 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1, failure_message)
        self.admin.assert_icommand_fail('iadmin modresc demoResc rebalance', 'STDOUT_SINGLELINE', self.failure_message)
        self.verify_new_failure_messages_in_log(1, failure_message)
        self.run_mungefsctl('read')

        self.admin.assert_icommand(['irm', '-f', filename])
        os.unlink(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource should have replicated 2 clean copies
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # default resource should have replicated new double clean copies
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])

        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = lib.create_local_testfile(filename)

        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # replicate to default resource (default resource is replication)
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # replicate overtop default resource (default resource is replication)
        self.admin.assert_icommand("irepl " + filename)
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
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

        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" % (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        # put to default resource (creates 2 replicas, 0 and 1)
        self.admin.assert_icommand("iput " + filename)
        # replicate to test resource (creates replica 2)
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource (creates replica 3)
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        self.admin.assert_icommand("irepl " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should not have a replica 5
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)
        self.admin.assert_icommand("iadmin rmresc thirdresc")

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

        self.admin.assert_icommand("iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                                           (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                                           (hostname, hostuser), 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        # put to default resource (creates two replicas, 0 and 1)
        self.admin.assert_icommand("iput " + filename)
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)

        # should have dirty copies
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have dirty copies
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -U " + filename)

        # should have dirty copies
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)

        # should have clean copies
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])

        self.admin.assert_icommand("irm -f " + filename)
        self.admin.assert_icommand("iadmin rmresc thirdresc")
        self.admin.assert_icommand("iadmin rmresc fourthresc")

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irm_specific_replica(self):
        # should be listed 2x
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 0 ", " & " + self.testfile])
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 1 ", " & " + self.testfile])

        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)
        # should be listed a third time
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', [" 2 ", " & " + self.testfile])
        # Remove one of the original replicas
        self.admin.assert_icommand("irm -n 1 " + self.testfile)
        # replicas 0 and 2 should be there (replica 0 goes to ufs2)
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["0 ", "ufs2", self.testfile])
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["2 " + self.testresc, self.testfile])
        # replica 1 should be gone
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                                ["1 ", self.munge_resc, self.testfile])
        # replica should not be in trash
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                                ["1 ", self.munge_resc, self.testfile])

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        # put to default resource creates 2 replicas
        self.admin.assert_icommand("iput " + filename)
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource should have dirty copies
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + filename])
        # default resource should not have any doublesize files
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " " + doublesize + " ", " " + filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " " + doublesize + " ", "& " + filename])

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    @unittest.skip("no support for non-compound resources")
    def test_iget_with_purgec(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_irepl_with_purgec(self):
        pass

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_Resource_MultiLayered(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc pass2Resc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc pass2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc pass2Resc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix3Resc")
        super(Test_Resource_MultiLayered, self).setUp()

    def tearDown(self):
        super(Test_Resource_MultiLayered, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
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
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix2RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix3RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_Resource_RandomWithinRandom(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random0 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random1 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random2 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc random3 random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unix0 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix0Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" +
                                          lib.get_irods_top_level_dir() + "/unix3Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
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
        with lib.make_session_for_existing_admin() as admin_session:
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
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix0Vault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1Vault", ignore_errors=True)

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
