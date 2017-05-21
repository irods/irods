from __future__ import print_function
import getpass
import inspect
import os
import psutil
import re
import shutil
import subprocess
import sys
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file, CoreFile
from .. import test
from . import settings
from .. import lib
from .resource_suite import ResourceSuite, ResourceBase
from .test_chunkydevtest import ChunkyDevTest
from . import session
from .rule_texts_for_tests import rule_texts

def statvfs_path_or_parent(path):
    while not os.path.exists(path):
        path = os.path.dirname(path)
    return os.statvfs(path)

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

    @unittest.skip("EMPTY_RESC_PATH - no vault path for coordinating resources")
    def test_ireg_as_rodsuser_in_vault(self):
        pass

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        assert lib.count_occurrences_of_string_in_log(IrodsConfig().server_log_path, 'key [get_key] - value [val3]', start_index=initial_log_size) in [1, 2]  # double print if collection missing
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
        self.admin.assert_icommand('ichksum -K ' + full_logical_path, 'STDOUT_MULTILINE',
                                   ['Total checksum performed = 1, Failed checksum = 0',
                                    'sha2:' + orig_digest])  # ichksum
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
        self.admin.assert_icommand('irm -U ' + full_logical_path)
        self.admin.assert_icommand('ireg ' + file_vault_full_path + ' ' + full_logical_path)
        self.admin.assert_icommand('ifsck -K ' + file_vault_full_path, 'STDOUT_SINGLELINE', ['WARNING: checksum not available'])  # ifsck
        self.admin.assert_icommand('ichksum -f ' + full_logical_path, 'STDOUT_MULTILINE',
                                   ['Total checksum performed = 1, Failed checksum = 0',
                                    'sha2:' + new_digest])
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

    def test_weighted_passthrough(self):
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
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        count = lib.count_occurrences_of_string_in_log(irods_config.server_log_path, 'argv:stageToCache', start_index=initial_log_size)
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
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        self.admin.assert_icommand("iphymv -M -R demoResc " + logical_path )
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'cacheResc')
        self.admin.assert_icommand("ils -l " + logical_path, 'STDOUT_SINGLELINE', 'archiveResc')

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
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))

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
                print('MANUALLY UPDATED ON ARCHIVE **AGAIN**\n', file=f, end='')

            # get the file
            self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from archive

            # confirm this is the new archive file
            matchfound = False
            with open(retrievedfile) as f:
                for line in f:
                    if "**AGAIN**" in line:
                        matchfound = True
            assert matchfound

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
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        # should be listed once - replica 1
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # replica 0 should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # replica 1 should be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # replica 2 should be listed

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
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
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
        os.system("rm -f %s" % retrievedfile)
        self.admin.assert_icommand("iget -f %s %s" % (filename, retrievedfile))  # get file from cache
        # confirm retrieved file is same as original
        assert 0 == os.system("diff %s %s" % (filepath, retrievedfile))
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
                print('MANUALLY UPDATED ON ARCHIVE 1 **AGAIN**\n', file=f, end='')
            with open(archivereplica2phypath, 'wt') as f:
                print('MANUALLY UPDATED ON ARCHIVE 2 **AGAIN**\n', file=f, end='')

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
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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

    def test_num_repl_policy__ticket_2851(self):
        self.admin.assert_icommand('iadmin modresc demoResc context "num_repl=2"')
        self.admin.assert_icommand('iadmin lr demoResc', 'STDOUT_SINGLELINE', 'demoResc')
        filename = "test_num_repl_policy__ticket_2851.txt"
        filepath = lib.create_local_testfile(filename)

        for i in range(0,10):
            self.admin.assert_icommand("iput " + filename + ' ' + filename+str(i))

        hier_ctr = {}
        for i in range(0,10):
            stdout,_,_ = self.admin.run_icommand(['ils', '-l', filename+str(i)])
            res = stdout.split()
            hier_ctr[res[2]] = 'found_it' # first resc_hier
            hier_ctr[res[9]] = 'found_it' # second resc_hier

        print('hier_ctr size: '+str(len(hier_ctr)))

        self.admin.assert_icommand('iadmin modresc demoResc context "NO_CONTEXT"')

        # 10 iputs should hit all the child resources
        assert len(hier_ctr) == 3

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
                self.admin.assert_icommand('ilsresc', 'STDOUT_SINGLELINE', "demoResc:replication")
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

    @unittest.skip("no support for non-compound resources")
    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

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
        if os.path.exists(filepath):
            os.unlink(filepath)

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
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file and purge 'cached' replica
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
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        # should not be listed (trimmed)
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed 3x - 1 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed 3x - 2 of 3
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", filename])  # should be listed 3x - 3 of 3

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

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

class Test_Resource_MultiLayered(ChunkyDevTest, ResourceSuite, unittest.TestCase):

    def setUp(self):
        with session.make_session_for_existing_admin() as admin_session:
            irods_config = IrodsConfig()
            admin_session.assert_icommand("iadmin modresc demoResc name origResc", 'STDOUT_SINGLELINE', 'rename', input='yes\n')
            admin_session.assert_icommand("iadmin mkresc demoResc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc pass2Resc passthru", 'STDOUT_SINGLELINE', 'passthru')
            admin_session.assert_icommand("iadmin mkresc rrResc roundrobin", 'STDOUT_SINGLELINE', 'roundrobin')
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
