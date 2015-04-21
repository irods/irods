import base64
import commands
import copy
import datetime
import getpass
import hashlib
import itertools
import os
import psutil
import shlex
import sys
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
import lib


class ResourceBase(lib.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass'), ('bobby', 'bpass')])):
    def setUp(self):
        super(ResourceBase, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user0 = self.user_sessions[0]
        self.user1 = self.user_sessions[1]

        print "run_resource_setup - BEGIN"
        self.testfile = "pydevtest_testfile.txt"
        self.testdir = "pydevtest_testdir"
        self.testresc = "pydevtest_TestResc"
        self.anotherresc = "pydevtest_AnotherResc"

        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        self.admin.assert_icommand(['iadmin', "mkresc", self.testresc, 'unixfilesystem', hostname + ":/tmp/" + hostuser + "/pydevtest_" + self.testresc], 'STDOUT', 'unixfilesystem')
        self.admin.assert_icommand(['iadmin', "mkresc", self.anotherresc, 'unixfilesystem', hostname + ":/tmp/" + hostuser + "/pydevtest_" + self.anotherresc], 'STDOUT', 'unixfilesystem')
        with open(self.testfile, 'w') as f:
            f.write('I AM A TESTFILE -- [' + self.testfile + ']')
        self.admin.run_icommand(['imkdir', self.testdir])
        self.admin.run_icommand(['iput', self.testfile])
        self.admin.run_icommand(['icp', self.testfile, '../../public/'])
        self.admin.run_icommand(['ichmod', 'read', self.user0.username, '../../public/' + self.testfile])
        self.admin.run_icommand(['ichmod', 'write', self.user1.username, '../../public/' + self.testfile])
        print 'run_resource_setup - END'

    def tearDown(self):
        print "run_resource_teardown - BEGIN"
        os.unlink(self.testfile)
        self.admin.run_icommand('icd')
        self.admin.run_icommand(['irm', self.testfile, '../public/' + self.testfile])
        self.admin.run_icommand('irm -rf ../../bundle')

        super(ResourceBase, self).tearDown()
        with lib.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand('irmtrash -M')
            admin_session.run_icommand(['iadmin', 'rmresc', self.testresc])
            admin_session.run_icommand(['iadmin', 'rmresc', self.anotherresc])
            print "run_resource_teardown - END"


class ResourceSuite(ResourceBase):
    '''Define the tests to be run for a resource type.

    This class is designed to be used as a base class by developers
    when they write tests for their own resource plugins.

    All these tests will be inherited and the developer can add
    any new tests for new functionality or replace any tests
    they need to modify.
    '''

    ###################
    # iget
    ###################

    def test_local_iget(self):
        # local setup
        localfile = "local.txt"
        # assertions
        self.admin.assert_icommand("iget " + self.testfile + " " + localfile)  # iget
        output = commands.getstatusoutput('ls ' + localfile)
        print "  output: [" + output[1] + "]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput('rm ' + localfile)

    def test_local_iget_with_overwrite(self):
        # local setup
        localfile = "local.txt"
        # assertions
        self.admin.assert_icommand("iget " + self.testfile + " " + localfile)  # iget
        self.admin.assert_icommand_fail("iget " + self.testfile + " " + localfile)  # already exists
        self.admin.assert_icommand("iget -f " + self.testfile + " " + localfile)  # already exists, so force
        output = commands.getstatusoutput('ls ' + localfile)
        print "  output: [" + output[1] + "]"
        assert output[1] == localfile
        # local cleanup
        output = commands.getstatusoutput('rm ' + localfile)

    def test_local_iget_with_bad_option(self):
        # assertions
        self.admin.assert_icommand_fail("iget -z")  # run iget with bad option

    def test_iget_with_dirty_replica(self):
        # local setup
        filename = "original.txt"
        filepath = lib.create_local_testfile(filename)
        updated_filename = "updated_file_with_longer_filename.txt"
        updated_filepath = lib.create_local_testfile(updated_filename)
        retrievedfile = "retrievedfile.txt"
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)  # replicate file
        # force new put on second resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + updated_filename + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, "STDOUT", filename)  # debugging
        # should get orig file (replica 0)
        self.admin.assert_icommand("iget -f -n 0 " + filename + " " + retrievedfile)
        assert 0 == os.system("diff %s %s" % (filename, retrievedfile))  # confirm retrieved is correct
        self.admin.assert_icommand("iget -f " + filename + " " + retrievedfile)  # should get updated file
        assert 0 == os.system("diff %s %s" % (updated_filename, retrievedfile))  # confirm retrieved is correct
        # local cleanup
        output = commands.getstatusoutput('rm ' + filename)
        output = commands.getstatusoutput('rm ' + updated_filename)
        output = commands.getstatusoutput('rm ' + retrievedfile)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 1 ", filename])  # should be listed only once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    ###################
    # imv
    ###################

    def test_local_imv(self):
        # local setup
        movedfile = "moved_file.txt"
        # assertions
        self.admin.assert_icommand("imv " + self.testfile + " " + movedfile)  # move
        self.admin.assert_icommand("ils -L " + movedfile, 'STDOUT', movedfile)  # should be listed
        # local cleanup

    def test_local_imv_to_directory(self):
        # local setup
        # assertions
        self.admin.assert_icommand("imv " + self.testfile + " " + self.testdir)  # move
        self.admin.assert_icommand("ils -L " + self.testdir, 'STDOUT', self.testfile)  # should be listed
        # local cleanup

    def test_local_imv_to_existing_filename(self):
        # local setup
        copyfile = "anotherfile.txt"
        # assertions
        self.admin.assert_icommand("icp " + self.testfile + " " + copyfile)  # icp
        # cannot overwrite existing file
        self.admin.assert_icommand("imv " + self.testfile + " " + copyfile, 'STDERR', "CAT_NAME_EXISTS_AS_DATAOBJ")
        # local cleanup

    def test_local_imv_collection_to_sibling_collection__ticket_2448(self):
        self.admin.assert_icommand("imkdir first_dir")  # first collection
        self.admin.assert_icommand("icp " + self.testfile + " first_dir")  # add file
        self.admin.assert_icommand("imkdir second_dir")  # second collection
        self.admin.assert_icommand("imv -v first_dir second_dir", "STDOUT", "first_dir")  # imv into sibling
        self.admin.assert_icommand_fail("ils -L", "STDOUT", "first_dir")  # should not be listed
        self.admin.assert_icommand("ils -L second_dir", "STDOUT", "second_dir/first_dir")  # should be listed
        self.admin.assert_icommand("ils -L second_dir/first_dir", "STDOUT", self.testfile)  # should be listed

    def test_local_imv_collection_to_collection_with_modify_access_not_own__ticket_2317(self):
        publicpath = "/" + self.admin.zone_name + "/home/public"
        targetpath = publicpath + "/target"
        sourcepath = publicpath + "/source"
        # cleanup
        self.admin.assert_icommand("imkdir -p " + targetpath)  # target
        self.admin.assert_icommand("ichmod -M -r own " + self.admin.username + " " + targetpath)  # ichmod
        self.admin.assert_icommand("irm -rf " + targetpath)  # cleanup
        self.admin.assert_icommand("imkdir -p " + sourcepath)  # source
        self.admin.assert_icommand("ichmod -M -r own " + self.admin.username + " " + sourcepath)  # ichmod
        self.admin.assert_icommand("irm -rf " + sourcepath)  # cleanup
        # setup and test
        self.admin.assert_icommand("imkdir " + targetpath)  # target
        self.admin.assert_icommand("ils -rAL " + targetpath, "STDOUT", "own")  # debugging
        self.admin.assert_icommand("ichmod -r write " + self.user0.username + " " + targetpath)  # ichmod
        self.admin.assert_icommand("ils -rAL " + targetpath, "STDOUT", "modify object")  # debugging
        self.admin.assert_icommand("imkdir " + sourcepath)  # source
        self.admin.assert_icommand("ichmod -r own " + self.user0.username + " " + sourcepath)  # ichmod
        self.admin.assert_icommand("ils -AL " + sourcepath, "STDOUT", "own")  # debugging
        self.user0.assert_icommand("imv " + sourcepath + " " + targetpath)  # imv
        self.admin.assert_icommand("ils -AL " + targetpath, "STDOUT", targetpath + "/source")  # debugging
        self.admin.assert_icommand("irm -rf " + targetpath)  # cleanup
        # cleanup
        self.admin.assert_icommand("imkdir -p " + targetpath)  # target
        self.admin.assert_icommand("ichmod -M -r own " + self.admin.username + " " + targetpath)  # ichmod
        self.admin.assert_icommand("irm -rf " + targetpath)  # cleanup
        self.admin.assert_icommand("imkdir -p " + sourcepath)  # source
        self.admin.assert_icommand("ichmod -M -r own " + self.admin.username + " " + sourcepath)  # ichmod
        self.admin.assert_icommand("irm -rf " + sourcepath)  # cleanup

    ###################
    # iphymv
    ###################

    def test_iphymv_to_nonexistent_resource(self):
        self.admin.assert_icommand("ils -L", "STDOUT", self.testfile)  # debug
        self.admin.assert_icommand("iphymv -R nonexistentresc " + self.testfile,
                   "STDERR", "SYS_RESC_DOES_NOT_EXIST")  # should fail
        self.admin.assert_icommand("ils -L", "STDOUT", self.testfile)  # debug

    def test_iphymv_admin_mode(self):
        lib.touch("file.txt")
        for i in range(0, 100):
            self.user0.assert_icommand("iput file.txt " + str(i) + ".txt", "EMPTY")
        self.admin.assert_icommand("iphymv -r -M -R " + self.testresc + " " + self.admin.session_collection)  # creates replica

    ###################
    # iput
    ###################
    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ssl_iput_with_rods_env(self):
        lib.run_command('openssl genrsa -out server.key')
        lib.run_command('openssl req -batch -new -key server.key -out server.csr')
        lib.run_command('openssl req -batch -new -x509 -key server.key -out chain.pem -days 365')
        lib.run_command('openssl dhparam -2 -out dhparams.pem 100') # normally 2048, but smaller size here for speed

        service_account_environment_file_path = os.path.expanduser('~/.irods/irods_environment.json')
        with lib.file_backed_up(service_account_environment_file_path):
            server_update = {
                'irods_ssl_certificate_chain_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/chain.pem'),
                'irods_ssl_certificate_key_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/server.key'),
                'irods_ssl_dh_params_file': os.path.join(lib.get_irods_top_level_dir(), 'tests/pydevtest/dhparams.pem'),
            }
            lib.update_json_file_from_dict(service_account_environment_file_path, server_update)

            client_update = {
                'irods_client_server_policy': 'CS_NEG_REQUIRE',
                'irods_ssl_verify_server': 'none',
            }

            session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
            self.admin.environment_file_contents.update(client_update)

            filename = 'encryptedfile.txt'
            filepath = lib.create_local_testfile(filename)
            self.admin.assert_icommand(['iinit', self.admin.password])
            self.admin.assert_icommand(['iput', filename])
            self.admin.assert_icommand(['ils', '-L', filename], 'STDOUT', filename)

            self.admin.environment_file_contents = session_env_backup

            for f in ['server.key', 'server.csr', 'chain.pem', 'dhparams.pem']:
                os.unlink(f)

        lib.restart_irods_server()

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
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
        os.environ['irodsSSLCertificateChainFile'] = lib.get_irods_top_level_dir() + "/tests/pydevtest/chain.pem"
        os.environ['irodsSSLCertificateKeyFile'] = lib.get_irods_top_level_dir() + "/tests/pydevtest/server.key"
        os.environ['irodsSSLDHParamsFile'] = lib.get_irods_top_level_dir() + "/tests/pydevtest/dhparams.pem"

        # client side environment variables
        os.environ['irodsSSLVerifyServer'] = "none"

        # add client irodsEnv settings
        clientEnvFile = self.admin.local_session_dir + "/irods_environment.json"
        os.system("cp %s %sOrig" % (clientEnvFile, clientEnvFile))
        env = {}
        env['irods_client_server_policy'] = 'CS_NEG_REQUIRE'
        lib.update_json_file_from_dict(clientEnvFile, env)

        # server reboot to pick up new irodsEnv settings
        lib.restart_irods_server()

        # do the encrypted put
        filename = "encryptedfile.txt"
        filepath = lib.create_local_testfile(filename)
        self.admin.assert_icommand(['iinit', self.admin.password])  # reinitialize
        # small file
        self.admin.assert_icommand("iput " + filename)  # encrypted put - small file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)  # should be listed
        # large file
        largefilename = "BIGencryptedfile.txt"
        output = commands.getstatusoutput('dd if=/dev/zero of=' + largefilename + ' bs=1M count=60')
        assert output[0] == 0, "dd did not successfully exit"
        #os.system("ls -al "+largefilename)
        self.admin.assert_icommand("iput " + largefilename)  # encrypted put - large file
        self.admin.assert_icommand("ils -L " + largefilename, 'STDOUT', largefilename)  # should be listed

        # reset client environment to not require SSL
        os.system("mv %sOrig %s" % (clientEnvFile, clientEnvFile))

        # clean up
        os.system("rm server.key server.csr chain.pem dhparams.pem")
        os.remove(filename)
        os.remove(largefilename)

        # restart iRODS server without altered environment
        lib.restart_irods_server()

    @unittest.skipIf(psutil.disk_usage('/').free < 20000000000, "not enough free space for 5 x 2.2GB file ( local + iput + 3 repl children )")
    def test_local_iput_with_really_big_file__ticket_1623(self):
        filename = "reallybigfile.txt"
        # file size larger than 32 bit int
        lib.make_file(filename, pow(2, 31) + 100)
        print "file size = [" + str(os.stat(filename).st_size) + "]"
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', [filename, "does not exist"])
        self.admin.assert_icommand("iput " + filename)  # iput
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)  # should be listed
        output = commands.getstatusoutput('rm ' + filename)

    def test_local_iput(self):
        '''also needs to count and confirm number of replicas after the put'''
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT', datafilename)  # should not be listed
        self.admin.assert_icommand("iput " + datafilename)  # iput
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT', datafilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_overwrite(self):
        self.admin.assert_icommand_fail("iput " + self.testfile)  # fail, already exists
        self.admin.assert_icommand("iput -f " + self.testfile)  # iput again, force

    def test_local_iput_recursive(self):
        recursivedirname = "dir"

    def test_local_iput_lower_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wb') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        self.admin.assert_icommand("iput -k " + datafilename)  # iput
        with open(datafilename) as f:
            checksum = hashlib.sha256(f.read()).digest().encode("base64").strip()
        self.admin.assert_icommand("ils -L", 'STDOUT', "sha2:" + checksum)  # check proper checksum
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_upper_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wb') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        self.admin.assert_icommand("iput -K " + datafilename)  # iput
        with open(datafilename) as f:
            checksum = hashlib.sha256(f.read()).digest().encode("base64").strip()
        self.admin.assert_icommand("ils -L", 'STDOUT', "sha2:" + checksum)  # check proper checksum
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_onto_specific_resource(self):
        # local setup
        datafilename = "anotherfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT', datafilename)  # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + datafilename)  # iput
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT', datafilename)  # should be listed
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT', self.testresc)  # should be listed
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
        self.admin.interrupt_icommand(iputcmd, rf, 10)  # once restartfile reaches 10 bytes
        assert os.path.exists(rf), rf + " should now exist, but did not"
        output = commands.getstatusoutput('cat ' + rf)
        print "  restartfile [" + rf + "] contents --> [" + output[1] + "]"
        self.admin.assert_icommand("ils -L " + datadir, 'STDOUT', datadir)  # just to show contents
        self.admin.assert_icommand(iputcmd, 'STDOUT', "File last completed")  # confirm the restart
        for datafilename in datafiles:
            self.admin.assert_icommand("ils -L " + datadir, 'STDOUT', datafilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm -rf ' + datadir)
        output = commands.getstatusoutput('rm ' + rf)

    @unittest.skipIf(True, 'Enable once race conditions fixed, see #2634')
    def test_local_iput_interrupt_largefile(self):
        # local setup
        datafilename = 'bigfile'
        file_size = int(6*pow(10, 8))
        lib.make_file(datafilename, file_size)
        rf = 'bigrestartfile'
        iputcmd = 'iput --lfrestart {0} {1}'.format(rf, datafilename)
        if os.path.exists(rf):
            os.unlink(rf)
        self.admin.interrupt_icommand(iputcmd, rf, 300)  # once restartfile reaches 300 bytes
        time.sleep(2)  # wait for all interrupted threads to exit
        assert os.path.exists(rf), rf + " should now exist, but did not"
        output = commands.getstatusoutput('cat ' + rf)
        print "  restartfile [" + rf + "] contents --> [" + output[1] + "]"
        today = datetime.date.today()
        # length should not be zero
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT', [" 0 " + today.isoformat(), datafilename])
        # confirm the restart
        self.admin.assert_icommand(iputcmd, 'STDOUT', datafilename + " was restarted successfully")
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT',
                   [" " + str(os.stat(datafilename).st_size) + " " + today.isoformat(), datafilename])  # length should be size on disk
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)
        output = commands.getstatusoutput('rm ' + rf)

    def test_local_iput_physicalpath_no_permission(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        self.admin.assert_icommand("iput -p /newfileinroot.txt " + datafilename, 'STDERR',
                   ["UNIX_FILE_CREATE_ERR", "Permission denied"])  # should fail to write
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_physicalpath(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        fullpath = lib.get_irods_top_level_dir() + "/newphysicalpath.txt"
        self.admin.assert_icommand("iput -p " + fullpath + " " + datafilename)  # should complete
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT', datafilename)  # should be listed
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT', fullpath)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_admin_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        relpath = "relativephysicalpath.txt"
        # should disallow relative path
        self.admin.assert_icommand("iput -p " + relpath + " " + datafilename, 'STDERR', "absolute")
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        relpath = "relativephysicalpath.txt"
        self.user0.assert_icommand("iput -p " + relpath + " " + datafilename, 'STDERR', "absolute")  # should error
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    def test_local_iput_with_changed_target_filename(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'w') as f:
            f.write("TESTFILE -- [" + datafilename + "]")
        # assertions
        changedfilename = "different.txt"
        self.admin.assert_icommand("iput " + datafilename + " " + changedfilename)  # should complete
        self.admin.assert_icommand("ils -L " + changedfilename, 'STDOUT', changedfilename)  # should be listed
        # local cleanup
        output = commands.getstatusoutput('rm ' + datafilename)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Lists Vault files")
    def test_iput_overwrite_others_file__ticket_2086(self):
        # pre state
        self.admin.assert_icommand("ils -L", 'STDOUT', self.testfile)  # should be listed

        # local setup
        filename = "overwritefile.txt"
        filepath = lib.create_local_testfile(filename)

        # alice tries to put
        homepath = "/home/" + self.admin.username + "/" + self.admin._session_id + "/" + self.testfile
        logicalpath = "/" + self.admin.zone_name + homepath
        self.user0.assert_icommand("iput " + filepath + " " + logicalpath, 'STDERR', "CAT_NO_ACCESS_PERMISSION")  # iput

        # check physicalpaths (of all replicas)
        cmdout = self.admin.run_icommand(['ils', '-L'])
        print "[ils -L]:"
        print "[" + cmdout[1] + "]"
        lines = cmdout[1].splitlines()
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
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput --purgec " + filename)  # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 1 ", filename])  # should be listed only once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        os.system("cat %s %s > %s" % (filename, filename, doublefile))
        doublesize = str(os.stat(doublefile).st_size)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)  # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)
        # overwrite test repl with different data
        self.admin.assert_icommand("iput -f -R %s %s %s" % (self.testresc, doublefile, filename))
        # default resource should have dirty copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", " " + filename])
        # default resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 0 ", " " + doublesize + " ", " " + filename])
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 1 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # ireg
    ###################

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsadmin(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("ireg " + filepath + " /" + self.admin.zone_name + "/home/" +
                   self.admin.username + "/" + self.admin._session_id + "/" + filename)  # ireg
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)  # should be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsuser(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.user0.assert_icommand(['ireg', filepath, self.user0.session_collection + '/' + filename], 'STDERR', 'PATH_REG_NOT_ALLOWED')
        self.user0.assert_icommand_fail('ils -L ' + filename, 'STDOUT', filename)

        # local cleanup
        os.unlink(filepath)

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers file in Vault")
    def test_ireg_as_rodsuser_in_vault(self):
        # get vault base path
        cmdout = self.user0.run_icommand(['iquest', '%s', "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
        vaultpath = cmdout[1].rstrip('\n')

        # make dir in vault if necessary
        dir = os.path.join(vaultpath, 'home', self.user0.username)
        if not os.path.exists(dir):
            os.makedirs(dir)

        # create file in vault
        filename = "newfile.txt"
        filepath = os.path.join(dir, filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.user0.assert_icommand(['ireg', filepath, self.user0.session_collection + '/' + filename], 'STDERR', 'PATH_REG_NOT_ALLOWED')
        self.user0.assert_icommand_fail("ils -L " + filename, 'STDOUT', filename)  # should not be listed

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    ###################
    # irepl
    ###################

    def test_irepl_invalid_input(self):
        # local setup
        filename = "somefile.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, "STDERR", "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                 # put file
        # for debugging
        self.admin.assert_icommand("ils -L " + filename, "STDOUT", filename)
        # replicate to bad resource
        self.admin.assert_icommand("irepl -R nonresc " + filename, "STDERR", "SYS_RESC_DOES_NOT_EXIST")
        self.admin.assert_icommand("irm -f " + filename)                                               # cleanup file
        # local cleanup
        os.remove(filepath)

    def test_irepl_multithreaded(self):
        # local setup
        filename = "largefile.txt"
        filepath = lib.create_local_largefile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, "STDERR", "does not exist")     # should not be listed
        self.admin.assert_icommand("iput " + filename)                                 # put file
        self.admin.assert_icommand("ils -L " + filename, "STDOUT", filename)             # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " -N 3 " + filename)      # replicate to test resource
        # test resource should be listed
        self.admin.assert_icommand("ils -l " + filename, "STDOUT", self.testresc)
        self.admin.assert_icommand("irm -f " + filename)                               # cleanup file
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
                   (hostname, hostuser), 'STDOUT', "Creating")   # create third resource
        self.admin.assert_icommand("iadmin mkresc fourthresc unixfilesystem %s:/tmp/%s/fourthrescVault" %
                   (hostname, hostuser), 'STDOUT', "Creating")  # create fourth resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")              # should not be listed
        self.admin.assert_icommand("iput " + filename)                                         # put file
        # replicate to test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # replicate to third resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)
        # replicate to fourth resource
        self.admin.assert_icommand("irepl -R fourthresc " + filename)
        # repave overtop test resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + doublefile + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)                       # for debugging

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 3 ", " & " + filename])

        self.admin.assert_icommand("irepl -U " + filename)                                 # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 3 ", " & " + filename])

        self.admin.assert_icommand("irepl -aU " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 3 ", " & " + filename])

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
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)       # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        self.admin.assert_icommand("irepl " + filename)               # replicate to default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        self.admin.assert_icommand("irepl " + filename)               # replicate overtop default resource
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
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
                   (hostname, hostuser), 'STDOUT', "Creating")  # create third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate to third resource
        self.admin.assert_icommand("irepl " + filename)                           # replicate overtop default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate overtop test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        self.admin.assert_icommand("irepl -R thirdresc " + filename)              # replicate overtop third resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 3 ", " & " + filename])
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 4 ", " & " + filename])
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
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)                            # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)      # replicate to test resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', filename)          # for debugging
        # overwrite default repl with different data
        self.admin.assert_icommand("iput -f %s %s" % (doublefile, filename))
        # default resource should have clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", " & " + filename])
        # default resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 0 ", " " + doublesize + " ", " & " + filename])
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT',
                       [" 1 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT',
                   [" 1 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    # repl update ( repave old copies )
    # walk through command line switches

    def test_irepl_with_purgec(self):
        # local setup
        filename = "purgecreplfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'w') as f:
            f.write("TESTFILE -- [" + filepath + "]")

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " --purgec " + filename)  # replicate to test resource
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 0 ", filename])  # should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT', [" 2 ", filename])  # should be listed only once

        # local cleanup
        output = commands.getstatusoutput('rm ' + filepath)

    def test_irepl_with_admin_mode(self):
        lib.touch("file.txt")
        for i in range(0, 100):
            self.user0.assert_icommand("iput file.txt " + str(i) + ".txt", "EMPTY")
        homepath = "/" + self.admin.zone_name + "/home/" + \
            self.user0.username + "/" + self.user0._session_id
        self.admin.assert_icommand("irepl -r -M -R " + self.testresc + " " + homepath, "EMPTY")  # creates replica

    ###################
    # irm
    ###################

    def test_irm_doesnotexist(self):
        self.admin.assert_icommand_fail("irm doesnotexist")  # does not exist

    def test_irm(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.admin.assert_icommand("irm " + self.testfile)  # remove from grid
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be deleted
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        # should be in trash
        self.admin.assert_icommand("ils -L " + trashpath + "/" + self.testfile, 'STDOUT', self.testfile)

    def test_irm_force(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.admin.assert_icommand("irm -f " + self.testfile)  # remove from grid
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be deleted
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        # should not be in trash
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT', self.testfile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be listed twice
        self.admin.assert_icommand("irm -n 0 " + self.testfile)  # remove original from grid
        # replica 1 should be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', ["1 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT',
                       ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT',
                       ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    def test_irm_recursive_file(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.admin.assert_icommand("irm -r " + self.testfile)  # should not fail, even though a collection

    def test_irm_recursive(self):
        self.admin.assert_icommand("icp -r " + self.testdir + " copydir")  # make a dir copy
        self.admin.assert_icommand("ils -L ", 'STDOUT', "copydir")  # should be listed
        self.admin.assert_icommand("irm -r copydir")  # should remove
        self.admin.assert_icommand_fail("ils -L ", 'STDOUT', "copydir")  # should not be listed

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_read_permission(self):
        self.user0.assert_icommand("icd ../../public")  # switch to shared area
        self.user0.assert_icommand("ils -AL " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.user0.assert_icommand_fail("irm " + self.testfile)  # read perm should not be allowed to remove
        self.user0.assert_icommand("ils -AL " + self.testfile, 'STDOUT', self.testfile)  # should still be listed

    @unittest.skipIf(configuration.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_write_permission(self):
        self.user1.assert_icommand("icd ../../public")  # switch to shared area
        self.user1.assert_icommand("ils -AL " + self.testfile, 'STDOUT', self.testfile)  # should be listed
        self.user1.assert_icommand_fail("irm " + self.testfile)  # write perm should not be allowed to remove
        self.user1.assert_icommand("ils -AL " + self.testfile, 'STDOUT', self.testfile)  # should still be listed

    def test_irm_repeated_many_times(self):
        # repeat count
        many_times = 50
        # create file
        filename = "originalfile.txt"
        filepath = os.path.abspath(filename)
        lib.make_file(filepath, 15)
        # define
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        # loop
        for i in range(many_times):
            self.admin.assert_icommand("iput " + filename, "EMPTY")  # put the file
            self.admin.assert_icommand("irm " + filename, "EMPTY")  # delete the file
            self.admin.assert_icommand("ils -L " + trashpath, "STDOUT", filename)

    ###################
    # irmtrash
    ###################

    def test_irmtrash_admin(self):
        # assertions
        self.admin.assert_icommand("irm " + self.testfile)  # remove from grid
        self.admin.assert_icommand("ils -rL /" + self.admin.zone_name + "/trash/home/" +
                   self.admin.username + "/", 'STDOUT', self.testfile)  # should be listed
        self.admin.assert_icommand("irmtrash")  # should be listed
        self.admin.assert_icommand_fail("ils -rL /" + self.admin.zone_name + "/trash/home/" +
                       self.admin.username + "/", 'STDOUT', self.testfile)  # should be deleted

    ###################
    # itrim
    ###################

    def test_itrim_with_admin_mode(self):
        lib.touch("file.txt")
        for i in range(100):
            self.user0.assert_icommand("iput file.txt " + str(i) + ".txt", "EMPTY")
        homepath = self.user0.session_collection
        self.user0.assert_icommand("irepl -R " + self.testresc + " -r " + homepath, "EMPTY")  # creates replica
        self.admin.assert_icommand("itrim -M -N1 -r " + homepath, 'STDOUT', "Number of files trimmed = 100.")
