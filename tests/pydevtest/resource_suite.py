import base64
import commands
import datetime
import getpass
import hashlib
import os
import psutil
import shlex
import sys
import time


if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import pydevtest_common
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd, create_local_testfile, create_local_largefile, get_hostname, get_irods_top_level_dir, get_irods_config_dir, mod_json_file
import pydevtest_sessions as s


class ResourceBase(object):

    def __init__(self):
        print "in ResourceBase.__init__"
        self.testfile = "pydevtest_testfile.txt"
        self.testdir = "pydevtest_testdir"
        self.testresc = "pydevtest_TestResc"
        self.anotherresc = "pydevtest_AnotherResc"

    def run_resource_setup(self):
        print "run_resource_setup - BEGIN"
        # set up resource itself
        for i in self.my_test_resource["setup"]:
            parameters = shlex.split(i)  # preserves quoted substrings
            if parameters[0] == "iadmin":
                print s.adminsession.runAdminCmd(parameters[0], parameters[1:])
            else:
                output = commands.getstatusoutput(" ".join(parameters))
                print output
        # set up test resource
        print "run_resource_setup - creating test resources"
        output = commands.getstatusoutput("hostname")
        hostname = output[1]
        hostuser = getpass.getuser()
        s.adminsession.runAdminCmd(
            'iadmin', ["mkresc", self.testresc, "unix file system", hostname + ":/tmp/" + hostuser + "/pydevtest_" + self.testresc])
        s.adminsession.runAdminCmd(
            'iadmin', ["mkresc", self.anotherresc, "unix file system", hostname + ":/tmp/" + hostuser + "/pydevtest_" + self.anotherresc])
        # set up test files
        print "run_resource_setup - generating local testfile"
        f = open(self.testfile, 'wb')
        f.write("I AM A TESTFILE -- [" + self.testfile + "]")
        f.close()
        print "run_resource_setup - adding testfile to grid"
        s.adminsession.runCmd('imkdir', [self.testdir])
        s.adminsession.runCmd('iput', [self.testfile])
        print "run_resource_setup - adding testfile to grid public directory"
        s.adminsession.runCmd('icp', [self.testfile, "../../public/"])  # copy of testfile into public
        print "run_resource_setup - setting permissions"
        # permissions
        s.adminsession.runCmd(
            'ichmod', ["read", s.users[1]['name'], "../../public/" + self.testfile])  # read for user1
        s.adminsession.runCmd(
            'ichmod', ["write", s.users[2]['name'], "../../public/" + self.testfile])  # write for user2
        # set test group
        self.testgroup = s.testgroup
        print "run_resource_setup - END"

    def run_resource_teardown(self):
        print "run_resource_teardown - BEGIN"
        # local file cleanup
        print "run_resource_teardown - removing local testfile"
        os.unlink(self.testfile)
        # remove grid test files
        print "run_resource_teardown  - removing testfile from grid public directory"
        s.adminsession.runCmd('icd')
        s.adminsession.runCmd('irm', [self.testfile, "../public/" + self.testfile])
        # remove any bundle files
        print "run_resource_teardown  - removing any bundle files"
        s.adminsession.runCmd('irm -rf ../../bundle')
        # tear down admin session files
        print "run_resource_teardown  - admin session removing session files"
        for session in s.sessions:
            session.runCmd('icd')
            session.runCmd('irm', ['-r', session.sessionId])
        # clean trash
        print "run_resource_teardown  - clean trash"
        s.adminsession.runCmd('irmtrash', ['-M'])
        # remove resc
        print "run_resource_teardown  - removing test resources"
        s.adminsession.runAdminCmd('iadmin', ['rmresc', self.testresc])
        s.adminsession.runAdminCmd('iadmin', ['rmresc', self.anotherresc])
        # tear down resource itself
        print "run_resource_teardown  - tearing down actual resource"
        for i in self.my_test_resource["teardown"]:
            parameters = shlex.split(i)  # preserves quoted substrings
            if parameters[0] == "iadmin":
                print s.adminsession.runAdminCmd(parameters[0], parameters[1:])
            else:
                output = commands.getstatusoutput(" ".join(parameters))
                print output
        print "run_resource_teardown - END"


class ShortAndSuite(ResourceBase):

    def __init__(self):
        print "in ShortAndSuite.__init__"
        ResourceBase.__init__(self)

    def test_awesome(self):
        print "AWESOME!"

    def test_local_iget(self):
        print self.testfile

        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession, "iget " + self.testfile + " " + localfile)  # iget
        output = commands.getstatusoutput('ls ' + localfile)
        print "  output: [" + output[1] + "]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput('rm ' + localfile)


class ResourceSuite(ResourceBase):

    '''Define the tests to be run for a resource type.

    This class is designed to be used as a base class by developers
    when they write tests for their own resource plugins.

    All these tests will be inherited and the developer can add
    any new tests for new functionality or replace any tests
    they need to modify.
    '''

    def __init__(self):
        print "in ResourceSuite.__init__"
        ResourceBase.__init__(self)

    ###################
    # iget
    ###################

    def test_local_iget(self):
        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession, "iget " + self.testfile + " " + localfile)  # iget
        output = commands.getstatusoutput('ls ' + localfile)
        print "  output: [" + output[1] + "]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput('rm ' + localfile)

    def test_local_iget_with_overwrite(self):
        # local setup
        localfile = "local.txt"
        # assertions
        assertiCmd(s.adminsession, "iget " + self.testfile + " " + localfile)  # iget
        assertiCmdFail(s.adminsession, "iget " + self.testfile + " " + localfile)  # already exists
        assertiCmd(s.adminsession, "iget -f " + self.testfile + " " + localfile)  # already exists, so force
        output = commands.getstatusoutput('ls ' + localfile)
        print "  output: [" + output[1] + "]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput('rm ' + localfile)

    def test_local_iget_with_bad_option(self):
        # assertions
        assertiCmdFail(s.adminsession, "iget -z")  # run iget with bad option

    def test_iget_with_dirty_replica(self):
        # local setup
        filename = "original.txt"
        filepath = pydevtest_common.create_local_testfile(filename)
        updated_filename = "updated_file_with_longer_filename.txt"
        updated_filepath = pydevtest_common.create_local_testfile(updated_filename)
        retrievedfile = "retrievedfile.txt"
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)  # put file
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)  # replicate file
        # force new put on second resource
        assertiCmd(s.adminsession, "iput -f -R " + self.testresc + " " + updated_filename + " " + filename)
        assertiCmd(s.adminsession, "ils -L " + filename, "STDOUT", filename)  # debugging
        # should get orig file (replica 0)
        assertiCmd(s.adminsession, "iget -f -n 0 " + filename + " " + retrievedfile)
        assert 0 == os.system("diff %s %s" % (filename, retrievedfile))  # confirm retrieved is correct
        assertiCmd(s.adminsession, "iget -f " + filename + " " + retrievedfile)  # should get updated file
        assert 0 == os.system("diff %s %s" % (updated_filename, retrievedfile))  # confirm retrieved is correct
        # local cleanup
        output = commands.getstatusoutput('rm ' + filename)
        output = commands.getstatusoutput('rm ' + updated_filename)
        output = commands.getstatusoutput('rm ' + retrievedfile)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = pydevtest_common.create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)  # put file
        assertiCmd(s.adminsession, "iget -f --purgec " + filename)  # get file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", filename])  # should be listed once
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", filename])  # should be listed only once
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    ###################
    # imv
    ###################

    def test_local_imv(self):
        # local setup
        movedfile = "moved_file.txt"
        # assertions
        assertiCmd(s.adminsession, "imv " + self.testfile + " " + movedfile)  # move
        assertiCmd(s.adminsession, "ils -L " + movedfile, "LIST", movedfile)  # should be listed
        # local cleanup

    def test_local_imv_to_directory(self):
        # local setup
        # assertions
        assertiCmd(s.adminsession, "imv " + self.testfile + " " + self.testdir)  # move
        assertiCmd(s.adminsession, "ils -L " + self.testdir, "LIST", self.testfile)  # should be listed
        # local cleanup

    def test_local_imv_to_existing_filename(self):
        # local setup
        copyfile = "anotherfile.txt"
        # assertions
        assertiCmd(s.adminsession, "icp " + self.testfile + " " + copyfile)  # icp
        # cannot overwrite existing file
        assertiCmd(s.adminsession, "imv " + self.testfile + " " + copyfile, "ERROR", "CAT_NAME_EXISTS_AS_DATAOBJ")
        # local cleanup

    def test_local_imv_collection_to_sibling_collection__ticket_2448(self):
        assertiCmd(s.adminsession, "imkdir first_dir")  # first collection
        assertiCmd(s.adminsession, "icp " + self.testfile + " first_dir")  # add file
        assertiCmd(s.adminsession, "imkdir second_dir")  # second collection
        assertiCmd(s.adminsession, "imv -v first_dir second_dir", "STDOUT", "first_dir")  # imv into sibling
        assertiCmdFail(s.adminsession, "ils -L", "STDOUT", "first_dir")  # should not be listed
        assertiCmd(s.adminsession, "ils -L second_dir", "STDOUT", "second_dir/first_dir")  # should be listed
        assertiCmd(s.adminsession, "ils -L second_dir/first_dir", "STDOUT", self.testfile)  # should be listed

    def test_local_imv_collection_to_collection_with_modify_access_not_own__ticket_2317(self):
        publicpath = "/" + s.adminsession.getZoneName() + "/home/public"
        targetpath = publicpath + "/target"
        sourcepath = publicpath + "/source"
        # cleanup
        assertiCmd(s.adminsession, "imkdir -p " + targetpath)  # target
        assertiCmd(s.adminsession, "ichmod -M -r own " + s.adminsession.getUserName() + " " + targetpath)  # ichmod
        assertiCmd(s.adminsession, "irm -rf " + targetpath)  # cleanup
        assertiCmd(s.adminsession, "imkdir -p " + sourcepath)  # source
        assertiCmd(s.adminsession, "ichmod -M -r own " + s.adminsession.getUserName() + " " + sourcepath)  # ichmod
        assertiCmd(s.adminsession, "irm -rf " + sourcepath)  # cleanup
        # setup and test
        assertiCmd(s.adminsession, "imkdir " + targetpath)  # target
        assertiCmd(s.adminsession, "ils -rAL " + targetpath, "STDOUT", "own")  # debugging
        assertiCmd(s.adminsession, "ichmod -r write " + s.sessions[1].getUserName() + " " + targetpath)  # ichmod
        assertiCmd(s.adminsession, "ils -rAL " + targetpath, "STDOUT", "modify object")  # debugging
        assertiCmd(s.adminsession, "imkdir " + sourcepath)  # source
        assertiCmd(s.adminsession, "ichmod -r own " + s.sessions[1].getUserName() + " " + sourcepath)  # ichmod
        assertiCmd(s.adminsession, "ils -AL " + sourcepath, "STDOUT", "own")  # debugging
        assertiCmd(s.sessions[1], "imv " + sourcepath + " " + targetpath)  # imv
        assertiCmd(s.adminsession, "ils -AL " + targetpath, "STDOUT", targetpath + "/source")  # debugging
        assertiCmd(s.adminsession, "irm -rf " + targetpath)  # cleanup
        # cleanup
        assertiCmd(s.adminsession, "imkdir -p " + targetpath)  # target
        assertiCmd(s.adminsession, "ichmod -M -r own " + s.adminsession.getUserName() + " " + targetpath)  # ichmod
        assertiCmd(s.adminsession, "irm -rf " + targetpath)  # cleanup
        assertiCmd(s.adminsession, "imkdir -p " + sourcepath)  # source
        assertiCmd(s.adminsession, "ichmod -M -r own " + s.adminsession.getUserName() + " " + sourcepath)  # ichmod
        assertiCmd(s.adminsession, "irm -rf " + sourcepath)  # cleanup

    ###################
    # iphymv
    ###################

    def test_iphymv_to_nonexistent_resource(self):
        assertiCmd(s.adminsession, "ils -L", "STDOUT", self.testfile)  # debug
        assertiCmd(s.adminsession, "iphymv -R nonexistentresc " + self.testfile,
                   "STDERR", "SYS_RESC_DOES_NOT_EXIST")  # should fail
        assertiCmd(s.adminsession, "ils -L", "STDOUT", self.testfile)  # debug

    def test_iphymv_admin_mode(self):
        pydevtest_common.touch("file.txt")
        for i in range(0, 100):
            assertiCmd(s.sessions[1], "iput file.txt " + str(i) + ".txt", "EMPTY")
        homepath = "/" + s.adminsession.getZoneName() + "/home/" + \
            s.sessions[1].getUserName() + "/" + s.sessions[1].sessionId
        assertiCmd(s.adminsession, "iphymv -r -M -R " + self.testresc + " " + homepath, "EMPTY")  # creates replica

    ###################
    # iput
    ###################
    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ssl_iput_with_rods_env(self):
        # set up client and server side for ssl handshake

        # server side certificate setup
        os.system("openssl genrsa -out server.key 2> /dev/null")
        os.system("openssl req -batch -new -key server.key -out server.csr")
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")
        os.system("mv server.crt chain.pem")
        # normally 2048, but smaller size here for speed
        os.system("openssl dhparam -2 -out dhparams.pem 100 2> /dev/null")

        # add client irodsEnv settings
        clientEnvFile = s.adminsession.sessionDir + "/irods_environment.json"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))

        env = {}
        env['irods_client_server_policy'] = 'CS_NEG_REQUIRE'
        env['irods_ssl_certificate_chain_file'] = get_irods_top_level_dir() + "/tests/pydevtest/chain.pem"
        env['irods_ssl_certificate_key_file'] = get_irods_top_level_dir() + "/tests/pydevtest/server.key"
        env['irods_ssl_dh_params_file'] = get_irods_top_level_dir() + "/tests/pydevtest/dhparams.pem"
        env['irods_ssl_verify_server'] = "none"

        mod_json_file(clientEnvFile, env)

        # server needs the environment variable to
        # read the correctly changed environment

        # server reboot to pick up new irodsEnv settings
        env_val = s.adminsession.sessionDir + "/irods_environment.json"
        sys_cmd = "export IRODS_ENVIRONMENT_FILE=" + env_val + ";" + \
            get_irods_top_level_dir() + "/iRODS/irodsctl restart"
        os.system(sys_cmd)

        # do the encrypted put
        filename = "encryptedfile.txt"
        filepath = create_local_testfile(filename)
        assertiCmd(s.adminsession, "iinit rods")  # reinitialize
        # small file
        assertiCmd(s.adminsession, "iput " + filename)  # encrypted put - small file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)  # should be listed
        # reset client environment to not require SSL
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))

        # clean up
        os.system("rm server.key server.csr chain.pem dhparams.pem")
        os.remove(filename)

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ssl_iput_small_and_large_files(self):
        # set up client and server side for ssl handshake

        # server side certificate setup
        os.system("openssl genrsa -out server.key 2> /dev/null")
        os.system("openssl req -batch -new -key server.key -out server.csr")
        os.system("openssl req -batch -new -x509 -key server.key -out server.crt -days 365")
        os.system("mv server.crt chain.pem")
        # normally 2048, but smaller size here for speed
        os.system("openssl dhparam -2 -out dhparams.pem 100 2> /dev/null")

        # server side environment variables
        os.environ['irodsSSLCertificateChainFile'] = get_irods_top_level_dir() + "/tests/pydevtest/chain.pem"
        os.environ['irodsSSLCertificateKeyFile'] = get_irods_top_level_dir() + "/tests/pydevtest/server.key"
        os.environ['irodsSSLDHParamsFile'] = get_irods_top_level_dir() + "/tests/pydevtest/dhparams.pem"

        # client side environment variables
        os.environ['irodsSSLVerifyServer'] = "none"

        # add client irodsEnv settings
        clientEnvFile = s.adminsession.sessionDir + "/irods_environment.json"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))
        env = {}
        env['irods_client_server_policy'] = 'CS_NEG_REQUIRE'
        mod_json_file(clientEnvFile, env)

        # server reboot to pick up new irodsEnv settings
        os.system(get_irods_top_level_dir() + "/iRODS/irodsctl restart")

        # do the encrypted put
        filename = "encryptedfile.txt"
        filepath = create_local_testfile(filename)
        assertiCmd(s.adminsession, "iinit rods")  # reinitialize
        # small file
        assertiCmd(s.adminsession, "iput " + filename)  # encrypted put - small file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)  # should be listed
        # large file
        largefilename = "BIGencryptedfile.txt"
        output = commands.getstatusoutput('dd if=/dev/zero of=' + largefilename + ' bs=1M count=60')
        assert output[0] == 0, "dd did not successfully exit"
        #os.system("ls -al "+largefilename)
        assertiCmd(s.adminsession, "iput " + largefilename)  # encrypted put - large file
        assertiCmd(s.adminsession, "ils -L " + largefilename, "LIST", largefilename)  # should be listed

        # reset client environment to not require SSL
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))

        # clean up
        os.system("rm server.key server.csr chain.pem dhparams.pem")
        os.remove(filename)
        os.remove(largefilename)

    @unittest.skipIf(psutil.disk_usage('/').free < 20000000000, "not enough free space for 5 x 2.2GB file ( local + iput + 3 repl children )")
    def test_local_iput_with_really_big_file__ticket_1623(self):
        filename = "reallybigfile.txt"
        # file size larger than 32 bit int
        pydevtest_common.make_file(filename, pow(2, 31) + 100)
        print "file size = [" + str(os.stat(filename).st_size) + "]"
        # should not be listed
        assertiCmd(s.adminsession, "ils -L " + filename, 'STDERR', [filename, "does not exist"])
        assertiCmd(s.adminsession, "iput " + filename)  # iput
        assertiCmd(s.adminsession, "ils -L " + filename, 'STDOUT', filename)  # should be listed
        output = commands.getstatusoutput('rm ' + filename)

    def test_local_iput(self):
        '''also needs to count and confirm number of replicas after the put'''
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        assertiCmdFail(s.adminsession, "ils -L " + datafilename, "LIST", datafilename)  # should not be listed
        assertiCmd(s.adminsession, "iput " + datafilename)  # iput
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST", datafilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_overwrite(self):
        assertiCmdFail(s.adminsession, "iput " + self.testfile)  # fail, already exists
        assertiCmd(s.adminsession, "iput -f " + self.testfile)  # iput again, force

    def test_local_iput_recursive(self):
        recursivedirname = "dir"

    def test_local_iput_lower_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wb') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        assertiCmd(s.adminsession, "iput -k " + datafilename)  # iput
        with open(datafilename) as f:
            checksum = hashlib.sha256(f.read()).digest().encode("base64").strip()
        assertiCmd(s.adminsession, "ils -L", "LIST", "sha2:" + checksum)  # check proper checksum
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_upper_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wb') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        assertiCmd(s.adminsession, "iput -K " + datafilename)  # iput
        with open(datafilename) as f:
            checksum = hashlib.sha256(f.read()).digest().encode("base64").strip()
        assertiCmd(s.adminsession, "ils -L", "LIST", "sha2:" + checksum)  # check proper checksum
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_onto_specific_resource(self):
        # local setup
        datafilename = "anotherfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        assertiCmdFail(s.adminsession, "ils -L " + datafilename, "LIST", datafilename)  # should not be listed
        assertiCmd(s.adminsession, "iput -R " + self.testresc + " " + datafilename)  # iput
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST", datafilename)  # should be listed
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST", self.testresc)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_interrupt_directory(self):
        # local setup
        datadir = "newdatadir"
        output = commands.getstatusoutput('mkdir ' + datadir)
        datafiles = ["file1", "file2", "file3", "file4", "file5", "file6", "file7"]
        for datafilename in datafiles:
            print "-------------------"
            print "creating " + datafilename + "..."
            localpath = datadir + "/" + datafilename
            output = commands.getstatusoutput('dd if=/dev/zero of=' + localpath + ' bs=1M count=20')
            print output[1]
            assert output[0] == 0, "dd did not successfully exit"
        rf = "collectionrestartfile"
        # assertions
        iputcmd = "iput -X " + rf + " -r " + datadir
        if os.path.exists(rf):
            os.unlink(rf)
        interruptiCmd(s.adminsession, iputcmd, rf, 10)  # once restartfile reaches 10 bytes
        assert os.path.exists(rf), rf + " should now exist, but did not"
        output = commands.getstatusoutput('cat ' + rf)
        print "  restartfile [" + rf + "] contents --> [" + output[1] + "]"
        assertiCmd(s.adminsession, "ils -L " + datadir, "LIST", datadir)  # just to show contents
        assertiCmd(s.adminsession, iputcmd, "LIST", "File last completed")  # confirm the restart
        for datafilename in datafiles:
            assertiCmd(s.adminsession, "ils -L " + datadir, "LIST", datafilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm -rf ' + datadir)
        output = commands.getstatusoutput('rm ' + rf)

    def test_local_iput_interrupt_largefile(self):
        # local setup
        datafilename = "bigfile"
        print "-------------------"
        print "creating " + datafilename + "..."
        output = commands.getstatusoutput('dd if=/dev/zero of=' + datafilename + ' bs=1M count=150')
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        rf = "bigrestartfile"
        # assertions
        iputcmd = "iput --lfrestart " + rf + " " + datafilename
        if os.path.exists(rf):
            os.unlink(rf)
        interruptiCmd(s.adminsession, iputcmd, rf, 10)  # once restartfile reaches 10 bytes
        time.sleep(2)  # wait for all interrupted threads to exit
        assert os.path.exists(rf), rf + " should now exist, but did not"
        output = commands.getstatusoutput('cat ' + rf)
        print "  restartfile [" + rf + "] contents --> [" + output[1] + "]"
        today = datetime.date.today()
        # length should not be zero
        assertiCmdFail(s.adminsession, "ils -L " + datafilename, "LIST", [" 0 " + today.isoformat(), datafilename])
        # confirm the restart
        assertiCmd(s.adminsession, iputcmd, "LIST", datafilename + " was restarted successfully")
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST",
                   [" " + str(os.stat(datafilename).st_size) + " " + today.isoformat(), datafilename])  # length should be size on disk
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)
        output = commands.getstatusoutput('rm ' + rf)

    def test_local_iput_physicalpath_no_permission(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        assertiCmd(s.adminsession, "iput -p /newfileinroot.txt " + datafilename, "ERROR",
                   ["UNIX_FILE_CREATE_ERR", "Permission denied"])  # should fail to write
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_physicalpath(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        fullpath = get_irods_top_level_dir() + "/newphysicalpath.txt"
        assertiCmd(s.adminsession, "iput -p " + fullpath + " " + datafilename)  # should complete
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST", datafilename)  # should be listed
        assertiCmd(s.adminsession, "ils -L " + datafilename, "LIST", fullpath)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_admin_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        relpath = "relativephysicalpath.txt"
        # should disallow relative path
        assertiCmd(s.adminsession, "iput -p " + relpath + " " + datafilename, "ERROR", "absolute")
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        relpath = "relativephysicalpath.txt"
        assertiCmd(s.sessions[1], "iput -p " + relpath + " " + datafilename, "ERROR", "absolute")  # should error
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_with_changed_target_filename(self):
        # local setup
        datafilename = "newfile.txt"
        f = open(datafilename, 'wb')
        f.write("TESTFILE -- [" + datafilename + "]")
        f.close()
        # assertions
        changedfilename = "different.txt"
        assertiCmd(s.adminsession, "iput " + datafilename + " " + changedfilename)  # should complete
        assertiCmd(s.adminsession, "ils -L " + changedfilename, "LIST", changedfilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    @unittest.skip("TODO: revisit later, this is causing issues with mysql")
    def test_local_iput_collision_with_wlock(self):
        # local setup
        datafilename = "collisionfile1.txt"
        print "-------------------"
        print "creating " + datafilename + "..."
        output = commands.getstatusoutput('dd if=/dev/zero of=' + datafilename + ' bs=1M count=30')
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        # assertions

        begin = time.time()
        errorflag = False

        procs = set()
        pids = set()

        # start multiple icommands in parallel
        initialdelay = 3  # seconds
        for i in range(5):
            if i == 0:
                # add a three second delay before the first icommand
                p = s.adminsession.runCmd(
                    'iput', ["-vf", "--wlock", datafilename], waitforresult=False, delay=initialdelay)
            else:
                p = s.adminsession.runCmd('iput', ["-vf", "--wlock", datafilename], waitforresult=False, delay=0)
            procs.add(p)
            pids.add(p.pid)

        while pids:
            pid, retval = os.wait()
            for proc in procs:
                if proc.pid == pid:
                    print "pid " + str(pid) + ":"
                    if retval != 0:
                        print "  * ERROR occurred *  <------------"
                        errorflag = True
                    print "  retval [" + str(retval) + "]"
                    print "  stdout [" + proc.stdout.read().strip() + "]"
                    print "  stderr [" + proc.stderr.read().strip() + "]"
                    pids.remove(pid)

        elapsed = time.time() - begin
        print "\ntotal time [" + str(elapsed) + "]"

        assert elapsed > initialdelay

        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

        assert errorflag == False, "oops, had an error"

    @unittest.skip("TODO: revisit later, not sure this is testing anything of interest")
    def test_local_iput_collision_without_wlock(self):
        # local setup
        datafilename = "collisionfile2.txt"
        print "-------------------"
        print "creating " + datafilename + "..."
        output = commands.getstatusoutput('dd if=/dev/zero of=' + datafilename + ' bs=1M count=30')
        print output[1]
        assert output[0] == 0, "dd did not successfully exit"
        # assertions

        begin = time.time()
        errorflag = False

        procs = set()
        pids = set()

        # start multiple icommands in parallel
        initialdelay = 3  # seconds
        for i in range(7):
            if i == 0:
                # add a three second delay before the first icommand
                p = s.adminsession.runCmd('iput', ["-vf", datafilename], waitforresult=False, delay=initialdelay)
            else:
                p = s.adminsession.runCmd('iput', ["-vf", datafilename], waitforresult=False, delay=0)
            procs.add(p)
            pids.add(p.pid)

        while pids:
            pid, retval = os.wait()
            for proc in procs:
                if proc.pid == pid:
                    print "pid " + str(pid) + ":"
                    if retval != 0:
                        errorflag = True
                    else:
                        print "  * Unexpectedly, No ERROR occurred *  <------------"
                    print "  retval [" + str(retval) + "]"
                    print "  stdout [" + proc.stdout.read().strip() + "]"
                    print "  stderr [" + proc.stderr.read().strip() + "]"
                    pids.remove(pid)

        elapsed = time.time() - begin
        print "\ntotal time [" + str(elapsed) + "]"

        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

        assert errorflag, "Expected ERRORs did not occur"

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Lists Vault files")
    def test_iput_overwrite_others_file__ticket_2086(self):
        # pre state
        assertiCmd(s.adminsession, "ils -L", "LIST", self.testfile)  # should be listed

        # local setup
        filename = "overwritefile.txt"
        filepath = create_local_testfile(filename)

        # alice tries to put
        homepath = "/home/" + s.adminsession.getUserName() + "/" + s.adminsession.sessionId + "/" + self.testfile
        logicalpath = "/" + s.adminsession.getZoneName() + homepath
        assertiCmd(s.sessions[1], "iput " + filepath + " " + logicalpath, "ERROR", "CAT_NO_ACCESS_PERMISSION")  # iput

        # check physicalpaths (of all replicas)
        cmdout = s.adminsession.runCmd('ils', ['-L'])
        print "[ils -L]:"
        print "[" + cmdout[0] + "]"
        lines = cmdout[0].splitlines()
        for i in range(0, len(lines) - 1):
            if "0 demoResc" in lines[i]:
                if "/session-" in lines[i + 1]:
                    l = lines[i + 1]
                    physicalpath = l.split()[1]
                    # check file is on disk
                    print "[ls -l " + physicalpath + "]:"
                    os.system("ls -l " + physicalpath)
                    assert os.path.exists(physicalpath)

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath, 'wb')
        f.write("TESTFILE -- [" + filepath + "]")
        f.close()

        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput --purgec " + filename)  # put file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", filename])  # should be listed once
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", filename])  # should be listed only once
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)  # put file
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)  # replicate to test resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)
        # overwrite test repl with different data
        assertiCmd(s.adminsession, "iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource should have dirty copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " " + filename])
        # default resource should not have doublesize file
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # ireg
    ###################

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsadmin(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath, 'wb')
        f.write("TESTFILE -- [" + filepath + "]")
        f.close()

        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "ireg " + filepath + " /" + s.adminsession.getZoneName() + "/home/" +
                   s.adminsession.getUserName() + "/" + s.adminsession.sessionId + "/" + filename)  # ireg
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)  # should be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsuser(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath, 'wb')
        f.write("TESTFILE -- [" + filepath + "]")
        f.close()

        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.sessions[1], "ireg " + filepath + " /" + s.sessions[1].getZoneName() + "/home/" + s.sessions[
                   1].getUserName() + "/" + s.sessions[1].sessionId + "/" + filename, "ERROR", "PATH_REG_NOT_ALLOWED")  # ireg
        assertiCmdFail(s.sessions[1], "ils -L " + filename, "LIST", filename)  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers file in Vault")
    def test_ireg_as_rodsuser_in_vault(self):
        # get vault base path
        cmdout = s.sessions[1].runCmd('iquest', ["%s", "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
        vaultpath = cmdout[0].rstrip('\n')

        # make dir in vault if necessary
        dir = os.path.join(vaultpath, 'home', s.sessions[1].getUserName())
        if not os.path.exists(dir):
            os.makedirs(dir)

        # create file in vault
        filename = "newfile.txt"
        filepath = os.path.join(dir, filename)
        f = open(filepath, 'wb')
        f.write("TESTFILE -- [" + filepath + "]")
        f.close()

        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.sessions[1], "ireg " + filepath + " /" + s.sessions[1].getZoneName() + "/home/" + s.sessions[
                   1].getUserName() + "/" + s.sessions[1].sessionId + "/" + filename, "ERROR", "PATH_REG_NOT_ALLOWED")  # ireg
        assertiCmdFail(s.sessions[1], "ils -L " + filename, "LIST", filename)  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    ###################
    # irepl
    ###################

    def test_irepl_invalid_input(self):
        # local setup
        filename = "somefile.txt"
        filepath = create_local_testfile(filename)
        # assertions
        # should not be listed
        assertiCmd(s.adminsession, "ils -L " + filename, "STDERR", "does not exist")
        assertiCmd(s.adminsession, "iput " + filename)                                                 # put file
        # for debugging
        assertiCmd(s.adminsession, "ils -L " + filename, "STDOUT", filename)
        # replicate to bad resource
        assertiCmd(s.adminsession, "irepl -R nonresc " + filename, "STDERR", "SYS_RESC_DOES_NOT_EXIST")
        assertiCmd(s.adminsession, "irm -f " + filename)                                               # cleanup file
        # local cleanup
        os.remove(filepath)

    def test_irepl_multithreaded(self):
        # local setup
        filename = "largefile.txt"
        filepath = create_local_largefile(filename)
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "STDERR", "does not exist")     # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)                                 # put file
        assertiCmd(s.adminsession, "ils -L " + filename, "STDOUT", filename)             # for debugging
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " -N 3 " + filename)      # replicate to test resource
        # test resource should be listed
        assertiCmd(s.adminsession, "ils -l " + filename, "STDOUT", self.testresc)
        assertiCmd(s.adminsession, "irm -f " + filename)                               # cleanup file
        # local cleanup
        os.remove(filepath)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)

        # assertions
        assertiCmd(s.adminsession, "iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                   (hostname, hostuser), "LIST", "Creating")   # create third resource
        assertiCmd(s.adminsession, "iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                   (hostname, hostuser), "LIST", "Creating")  # create fourth resource
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")              # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)                                         # put file
        # replicate to test resource
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        assertiCmd(s.adminsession, "irepl -R thirdresc " + filename)
        # replicate to fourth resource
        assertiCmd(s.adminsession, "irepl -R fourthresc " + filename)
        # repave overtop test resource
        assertiCmd(s.adminsession, "iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)                       # for debugging

        # should have a dirty copy
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", " & " + filename])
        # should have a dirty copy
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        # should have a dirty copy
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 3 ", " & " + filename])

        assertiCmd(s.adminsession, "irepl -U " + filename)                                 # update last replica

        # should have a dirty copy
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", " & " + filename])
        # should have a dirty copy
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 3 ", " & " + filename])

        assertiCmd(s.adminsession, "irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        # should have a clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 3 ", " & " + filename])

        assertiCmd(s.adminsession, "irm -f " + filename)                                   # cleanup file
        assertiCmd(s.adminsession, "iadmin rmresc thirdresc")                            # remove third resource
        assertiCmd(s.adminsession, "iadmin rmresc fourthresc")                           # remove third resource

        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    def test_irepl_over_existing_second_replica__ticket_1705(self):
        # local setup
        filename = "secondreplicatest.txt"
        filepath = create_local_testfile(filename)
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput -R " + self.testresc + " " + filename)       # put file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        assertiCmd(s.adminsession, "irepl " + filename)               # replicate to default resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        assertiCmd(s.adminsession, "irepl " + filename)               # replicate overtop default resource
        # should not have a replica 2
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        # should not have a replica 2
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_third_replica__ticket_1705(self):
        # local setup
        filename = "thirdreplicatest.txt"
        filepath = create_local_testfile(filename)
        hostname = get_hostname()
        hostuser = getpass.getuser()
        # assertions
        assertiCmd(s.adminsession, "iadmin mkresc thirdresc unixfilesystem %s:/tmp/%s/thirdrescVault" %
                   (hostname, hostuser), "LIST", "Creating")  # create third resource
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)                            # put file
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        assertiCmd(s.adminsession, "irepl -R thirdresc " + filename)              # replicate to third resource
        assertiCmd(s.adminsession, "irepl " + filename)                           # replicate overtop default resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        assertiCmd(s.adminsession, "irepl -R thirdresc " + filename)              # replicate overtop third resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        # should not have a replica 3
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 3 ", " & " + filename])
        # should not have a replica 4
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 4 ", " & " + filename])
        assertiCmd(s.adminsession, "irm -f " + filename)                          # cleanup file
        assertiCmd(s.adminsession, "iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)                            # put file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", filename)          # for debugging
        # overwrite default repl with different data
        assertiCmd(s.adminsession, "iput -f %s %s" % (doublefile, filename))
        # default resource should have clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " & " + filename])
        # default resource should have new double clean copy
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST",
                       [" 1 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST",
                   [" 1 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 2
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    # repl update ( repave old copies )
    # walk through command line switches

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        f = open(filepath, 'wb')
        f.write("TESTFILE -- [" + filepath + "]")
        f.close()

        # assertions
        assertiCmd(s.adminsession, "ils -L " + filename, "ERROR", "does not exist")  # should not be listed
        assertiCmd(s.adminsession, "iput " + filename)  # put file
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 0 ", filename])  # should be trimmed
        assertiCmd(s.adminsession, "ils -L " + filename, "LIST", [" 1 ", filename])  # should be listed once
        assertiCmdFail(s.adminsession, "ils -L " + filename, "LIST", [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_admin_mode(self):
        pydevtest_common.touch("file.txt")
        for i in range(0, 100):
            assertiCmd(s.sessions[1], "iput file.txt " + str(i) + ".txt", "EMPTY")
        homepath = "/" + s.adminsession.getZoneName() + "/home/" + \
            s.sessions[1].getUserName() + "/" + s.sessions[1].sessionId
        assertiCmd(s.adminsession, "irepl -r -M -R " + self.testresc + " " + homepath, "EMPTY")  # creates replica

    ###################
    # irm
    ###################

    def test_irm_doesnotexist(self):
        assertiCmdFail(s.adminsession, "irm doesnotexist")  # does not exist

    def test_irm(self):
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmd(s.adminsession, "irm " + self.testfile)  # remove from grid
        assertiCmdFail(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be deleted
        trashpath = "/" + s.adminsession.getZoneName() + "/trash/home/" + s.adminsession.getUserName() + \
            "/" + s.adminsession.sessionId
        # should be in trash
        assertiCmd(s.adminsession, "ils -L " + trashpath + "/" + self.testfile, "LIST", self.testfile)

    def test_irm_force(self):
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmd(s.adminsession, "irm -f " + self.testfile)  # remove from grid
        assertiCmdFail(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be deleted
        trashpath = "/" + s.adminsession.getZoneName() + "/trash/home/" + s.adminsession.getUserName() + \
            "/" + s.adminsession.sessionId
        # should not be in trash
        assertiCmdFail(s.adminsession, "ils -L " + trashpath + "/" + self.testfile, "LIST", self.testfile)

    def test_irm_specific_replica(self):
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmd(s.adminsession, "irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be listed twice
        assertiCmd(s.adminsession, "irm -n 0 " + self.testfile)  # remove original from grid
        # replica 1 should be there
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", ["1 " + self.testresc, self.testfile])
        assertiCmdFail(s.adminsession, "ils -L " + self.testfile, "LIST",
                       ["0 " + s.adminsession.getDefResource(), self.testfile])  # replica 0 should be gone
        trashpath = "/" + s.adminsession.getZoneName() + "/trash/home/" + s.adminsession.getUserName() + \
            "/" + s.adminsession.sessionId
        assertiCmdFail(s.adminsession, "ils -L " + trashpath + "/" + self.testfile, "LIST",
                       ["0 " + s.adminsession.getDefResource(), self.testfile])  # replica should not be in trash

    def test_irm_recursive_file(self):
        assertiCmd(s.adminsession, "ils -L " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmd(s.adminsession, "irm -r " + self.testfile)  # should not fail, even though a collection

    def test_irm_recursive(self):
        assertiCmd(s.adminsession, "icp -r " + self.testdir + " copydir")  # make a dir copy
        assertiCmd(s.adminsession, "ils -L ", "LIST", "copydir")  # should be listed
        assertiCmd(s.adminsession, "irm -r copydir")  # should remove
        assertiCmdFail(s.adminsession, "ils -L ", "LIST", "copydir")  # should not be listed

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_read_permission(self):
        assertiCmd(s.sessions[1], "icd ../../public")  # switch to shared area
        assertiCmd(s.sessions[1], "ils -AL " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmdFail(s.sessions[1], "irm " + self.testfile)  # read perm should not be allowed to remove
        assertiCmd(s.sessions[1], "ils -AL " + self.testfile, "LIST", self.testfile)  # should still be listed

    @unittest.skipIf(pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_write_permission(self):
        assertiCmd(s.sessions[2], "icd ../../public")  # switch to shared area
        assertiCmd(s.sessions[2], "ils -AL " + self.testfile, "LIST", self.testfile)  # should be listed
        assertiCmdFail(s.sessions[2], "irm " + self.testfile)  # write perm should not be allowed to remove
        assertiCmd(s.sessions[2], "ils -AL " + self.testfile, "LIST", self.testfile)  # should still be listed

    def test_irm_repeated_many_times(self):
        # repeat count
        many_times = 50
        # create file
        filename = "originalfile.txt"
        filepath = os.path.abspath(filename)
        pydevtest_common.make_file(filepath, 15)
        # define
        trashpath = "/" + s.adminsession.getZoneName() + "/trash/home/" + s.adminsession.getUserName() + \
            "/" + s.adminsession.sessionId
        # loop
        for i in range(many_times):
            assertiCmd(s.adminsession, "iput " + filename, "EMPTY")  # put the file
            assertiCmd(s.adminsession, "irm " + filename, "EMPTY")  # delete the file
            assertiCmd(s.adminsession, "ils -L " + trashpath, "STDOUT", filename)

    ###################
    # irmtrash
    ###################

    def test_irmtrash_admin(self):
        # assertions
        assertiCmd(s.adminsession, "irm " + self.testfile)  # remove from grid
        assertiCmd(s.adminsession, "ils -rL /" + s.adminsession.getZoneName() + "/trash/home/" +
                   s.adminsession.getUserName() + "/", "LIST", self.testfile)  # should be listed
        assertiCmd(s.adminsession, "irmtrash")  # should be listed
        assertiCmdFail(s.adminsession, "ils -rL /" + s.adminsession.getZoneName() + "/trash/home/" +
                       s.adminsession.getUserName() + "/", "LIST", self.testfile)  # should be deleted

    ###################
    # itrim
    ###################

    def test_itrim_with_admin_mode(self):
        pydevtest_common.touch("file.txt")
        for i in range(0, 100):
            assertiCmd(s.sessions[1], "iput file.txt " + str(i) + ".txt", "EMPTY")
        homepath = "/" + s.adminsession.getZoneName() + "/home/" + \
            s.sessions[1].getUserName() + "/" + s.sessions[1].sessionId
        assertiCmd(s.sessions[1], "irepl -R " + self.testresc + " -r " + homepath, "EMPTY")  # creates replica
        assertiCmd(s.adminsession, "itrim -M -N1 -r " + homepath, "LIST", "Number of files trimmed = 100.")
