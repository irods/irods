import commands
import getpass
import os
import re
import shutil
import subprocess
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
import lib
from resource_suite import ResourceSuite, ResourceBase
from test_chunkydevtest import ChunkyDevTest


class Test_Random_within_Replication_Resource(ResourceSuite, ChunkyDevTest, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
        super(Test_Random_within_Replication_Resource, self).setUp()

    def tearDown(self):
        super(Test_Random_within_Replication_Resource, self).tearDown()
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


class Test_RoundRobin_within_Replication_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unixB2")
        super(Test_RoundRobin_within_Replication_Resource, self).setUp()

    def tearDown(self):
        super(Test_RoundRobin_within_Replication_Resource, self).tearDown()
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


class Test_UnixFileSystem_Resource(ResourceSuite, ChunkyDevTest, unittest.TestCase):
    def setUp(self):
        hostname = lib.get_hostname()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc 'unixfilesystem' " + hostname + ":" + lib.get_irods_top_level_dir() + "/demoRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
        super(Test_UnixFileSystem_Resource, self).setUp()

    def tearDown(self):
        super(Test_UnixFileSystem_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/demoRescVault", ignore_errors=True)

    def test_key_value_passthru(self):
        env = os.environ.copy()
        env['spLogLevel'] = '11'
        lib.restart_irods_server(env=env)

        lib.make_file('file.txt', 15)
        initial_log_size = lib.get_log_size('server')
        self.user0.assert_icommand('iput --kv_pass="put_key=val1" file.txt')
        assert lib.count_occurrences_of_string_in_log('server', 'key [put_key] - value [val1]', start_index=initial_log_size) in [1, 2] # double print if collection missing

        initial_log_size = lib.get_log_size('server')
        self.user0.assert_icommand('iget -f --kv_pass="get_key=val3" file.txt other.txt')
        assert lib.count_occurrences_of_string_in_log('server', 'key [get_key] - value [val3]', start_index=initial_log_size) in [1, 2] # double print if collection missing
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
        self.admin.assert_icommand('iput -K ' + filepath)  # iput
        file_vault_full_path = os.path.join(lib.get_vault_session_path(self.admin), filename)
        # method 1
        self.admin.assert_icommand('ichksum -K ' + full_logical_path, 'STDOUT_MULTILINE',
                            ['Total checksum performed = 1, Failed checksum = 0',
                            'sha2:ED5ZjdVDhtL8Th/rYtIraU1m3qMYzT0wNGy53FDrFR4='])  # ichksum
        # method 2
        self.admin.assert_icommand("iquest \"select DATA_CHECKSUM where DATA_NAME = '%s'\"" % filename,
                            'STDOUT_SINGLELINE', ['DATA_CHECKSUM = sha2:ED5ZjdVDhtL8Th/rYtIraU1m3qMYzT0wNGy53FDrFR4='])  # iquest
        # method 3
        self.admin.assert_icommand('ils -L', 'STDOUT_SINGLELINE', filename)  # ils
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # change content in vault
        filename2 = 'samesize.txt'
        filepath2 = lib.create_local_testfile(filename2)
        output = commands.getstatusoutput('cp ' + filename2 + ' ' + file_vault_full_path)
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION','checksum not consistent with iRODS object'])  # ifsck
        # change size in vault
        lib.cat(file_vault_full_path, 'extra letters')
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['CORRUPTION','size not consistent with iRODS object'])  # ifsck
        ## unregister, reregister (to update filesize in iCAT), recalculate checksum, and confirm
        self.admin.assert_icommand('irm -U ' + full_logical_path)
        self.admin.assert_icommand('ireg ' + file_vault_full_path + ' ' + full_logical_path)
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['WARNING: checksum not available'])  # ifsck
        self.admin.assert_icommand('ichksum -f ' + full_logical_path, 'STDOUT_MULTILINE',
                            ['Total checksum performed = 1, Failed checksum = 0',
                            'sha2:o4aFNLNuhWfF1AWiZrZUftnvV84e4hYEfToF5H0uiHI='])
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path)  # ifsck
        # local cleanup
        os.remove(filepath)
        os.remove(filepath2)

class Test_Passthru_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Passthru_Resource, self).setUp()

    def tearDown(self):
        super(Test_Passthru_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_WeightedPassthru_Resource(ResourceBase, unittest.TestCase):
    def setUp(self):
        hostname = lib.get_hostname()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + hostname + ":" + lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB 'unixfilesystem' " + hostname + ":" + lib.get_irods_top_level_dir() + "/unixBVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc w_pt passthru 'write=1.0;read=1.0'", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc w_pt")
            admin_session.assert_icommand("iadmin addchildtoresc w_pt unixB")
        super(Test_WeightedPassthru_Resource, self).setUp()

    def tearDown(self):
        super(Test_WeightedPassthru_Resource, self).tearDown()
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
        vaultpath = os.path.join( lib.get_irods_top_level_dir(), "unixBVault/home/" + self.admin.username, os.path.basename( self.admin._session_id ), filename )
        subprocess.check_call( "echo 'THISISBROEKN' | cat > %s" % (vaultpath),shell=True)

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=2.0'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "THISISBROEKN" )

        self.admin.assert_icommand("iadmin modresc w_pt context 'write=1.0;read=0.01'")
        self.admin.assert_icommand("iget " + filename + " - ", 'STDOUT_SINGLELINE', "TESTFILE" )

class Test_Deferred_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc deferred", 'STDOUT_SINGLELINE', 'deferred')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
        super(Test_Deferred_Resource, self).setUp()

    def tearDown(self):
        super(Test_Deferred_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

class Test_Random_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc random", 'STDOUT_SINGLELINE', 'random')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix3Resc")
        super(Test_Random_Resource, self).setUp()

    def tearDown(self):
        super(Test_Random_Resource, self).tearDown()
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


class Test_NonBlocking_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc nonblocking " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/nbVault", 'STDOUT_SINGLELINE', 'nonblocking')
        super(Test_NonBlocking_Resource, self).setUp()

    def tearDown(self):
        super(Test_NonBlocking_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')


class Test_Compound_with_MockArchive_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc mockarchive " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'mockarchive')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Compound_with_MockArchive_Resource, self).setUp()

    def tearDown(self):
        super(Test_Compound_with_MockArchive_Resource, self).tearDown()
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


class Test_Compound_with_UniversalMSS_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc univmss " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/archiveRescVault univMSSInterface.sh", 'STDOUT_SINGLELINE', 'univmss')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Compound_with_UniversalMSS_Resource, self).setUp()

    def tearDown(self):
        super(Test_Compound_with_UniversalMSS_Resource, self).tearDown()
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


class Test_Compound_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/cacheRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/archiveRescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc cacheResc cache")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc archiveResc archive")
        super(Test_Compound_Resource, self).setUp()

    def tearDown(self):
        super(Test_Compound_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc archiveResc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc cacheResc")
            admin_session.assert_icommand("iadmin rmresc archiveResc")
            admin_session.assert_icommand("iadmin rmresc cacheResc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/archiveRescVault", ignore_errors=True)
        shutil.rmtree("rm -rf " + lib.get_irods_top_level_dir() + "/cacheRescVault", ignore_errors=True)

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

    @unittest.skip("TEMPORARY")
    def test_iget_prefer_from_archive__ticket_1660(self):
        # define core.re filepath
        corefile = lib.get_irods_config_dir() + "/core.re"
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


class Test_Replication_within_Replication_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc replResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unixA 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unixAVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB1 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unixB1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unixB2 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unixB2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc replResc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unixA")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB1")
            admin_session.assert_icommand("iadmin addchildtoresc replResc unixB2")
        super(Test_Replication_within_Replication_Resource, self).setUp()

    def tearDown(self):
        super(Test_Replication_within_Replication_Resource, self).tearDown()
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


class Test_Replication_to_two_Compound_Resources(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc compResc1 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc compResc2 compound", 'STDOUT_SINGLELINE', 'compound')
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Replication_to_two_Compound_Resources, self).setUp()

    def tearDown(self):
        super(Test_Replication_to_two_Compound_Resources, self).tearDown()
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
        corefile = lib.get_irods_config_dir() + "/core.re"
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

class Test_Replication_to_two_Compound_Resources_with_Prefer_Archive(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        # back up core file
        corefile = lib.get_irods_config_dir() + "/core.re"
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
            admin_session.assert_icommand("iadmin mkresc cacheResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/cacheResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc1 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/archiveResc1Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc cacheResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/cacheResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc archiveResc2 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/archiveResc2Vault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc1")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc compResc2")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 cacheResc1 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc1 archiveResc1 archive")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 cacheResc2 cache")
            admin_session.assert_icommand("iadmin addchildtoresc compResc2 archiveResc2 archive")
        super(Test_Replication_to_two_Compound_Resources_with_Prefer_Archive, self).setUp()

    def tearDown(self):
        super(Test_Replication_to_two_Compound_Resources_with_Prefer_Archive, self).tearDown()
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
        corefile = lib.get_irods_config_dir() + "/core.re"
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


class Test_RoundRobin_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
        super(Test_RoundRobin_Resource, self).setUp()

    def tearDown(self):
        super(Test_RoundRobin_Resource, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin rmchildfromresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc unix2Resc")
            admin_session.assert_icommand("iadmin rmresc unix1Resc")
            admin_session.assert_icommand("iadmin rmresc demoResc")
            admin_session.assert_icommand("iadmin modresc origResc name demoResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix1RescVault", ignore_errors=True)
        shutil.rmtree(lib.get_irods_top_level_dir() + "/unix2RescVault", ignore_errors=True)

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


class Test_Replication_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc replication", 'STDOUT_SINGLELINE', 'replication')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc demoResc unix3Resc")
        super(Test_Replication_Resource, self).setUp()

    def tearDown(self):
        super(Test_Replication_Resource, self).tearDown()
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
        self.admin.assert_icommand(   "ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
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
        self.admin.assert_icommand(   "ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand(   "ils -L " + filename, 'STDOUT_SINGLELINE', [" 5 ", " & " + filename])

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


class Test_MultiLayered_Resource(ChunkyDevTest, ResourceSuite, unittest.TestCase):
    def setUp(self):
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', stdin_string='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc pass2Resc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
            admin_session.assert_icommand("iadmin mkresc unix1Resc 'unixfilesystem' " + configuration.HOSTNAME_1 + ":" + lib.get_irods_top_level_dir() + "/unix1RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix2Resc 'unixfilesystem' " + configuration.HOSTNAME_2 + ":" + lib.get_irods_top_level_dir() + "/unix2RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin mkresc unix3Resc 'unixfilesystem' " + configuration.HOSTNAME_3 + ":" + lib.get_irods_top_level_dir() + "/unix3RescVault", 'STDOUT_SINGLELINE', 'unixfilesystem')
            admin_session.assert_icommand("iadmin addchildtoresc demoResc pass2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc pass2Resc rrResc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix1Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix2Resc")
            admin_session.assert_icommand("iadmin addchildtoresc rrResc unix3Resc")
        super(Test_MultiLayered_Resource, self).setUp()

    def tearDown(self):
        super(Test_MultiLayered_Resource, self).tearDown()
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
