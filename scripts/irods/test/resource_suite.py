from __future__ import print_function
import base64
import copy
import datetime
import filecmp
import getpass
import hashlib
import itertools
import os
import psutil
import sys
import time

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from .. import test
from . import settings
from .. import lib
from .. import paths
from ..configuration import IrodsConfig
from ..controller import IrodsController
from . import session

plugin_name = IrodsConfig().default_rule_engine_plugin

class ResourceBase(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass'), ('bobby', 'bpass')])):

    def setUp(self):
        super(ResourceBase, self).setUp()
        self.admin = self.admin_sessions[0]
        self.user0 = self.user_sessions[0]
        self.user1 = self.user_sessions[1]

        print("run_resource_setup - BEGIN")
        self.testfile = "testfile.txt"
        self.testdir = "testdir"

        hostname = lib.get_hostname()
        hostuser = getpass.getuser()

        self.testresc = "TestResc"
        self.testvault = "/tmp/" + hostuser + "/" + self.testresc
        self.anotherresc = "AnotherResc"
        self.anothervault = "/tmp/" + hostuser + "/" + self.anotherresc

        self.admin.assert_icommand(
            ['iadmin', "mkresc", self.testresc, 'unixfilesystem', hostname + ":" + self.testvault], 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand(
            ['iadmin', "mkresc", self.anotherresc, 'unixfilesystem', hostname + ":" + self.anothervault], 'STDOUT_SINGLELINE', 'unixfilesystem')
        with open(self.testfile, 'wt') as f:
            print('I AM A TESTFILE -- [' + self.testfile + ']', file=f, end='')
        self.admin.assert_icommand(['imkdir', self.testdir])
        self.admin.assert_icommand(['iput', self.testfile])
        self.admin.assert_icommand(['icp', self.testfile, '../../public/'])
        self.admin.assert_icommand(['ichmod', 'read', self.user0.username, '../../public/' + self.testfile])
        self.admin.assert_icommand(['ichmod', 'write', self.user1.username, '../../public/' + self.testfile])
        print('run_resource_setup - END')

    def tearDown(self):
        print("run_resource_teardown - BEGIN")
        os.unlink(self.testfile)
        self.admin.run_icommand('icd')
        self.admin.run_icommand(['irm', self.testfile, '../public/' + self.testfile])
        self.admin.run_icommand('irm -rf ../../bundle')

        super(ResourceBase, self).tearDown()
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand('irmtrash -M')
            admin_session.run_icommand(['iadmin', 'rmresc', self.testresc])
            admin_session.run_icommand(['iadmin', 'rmresc', self.anotherresc])
            print("run_resource_teardown - END")


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

    def test_ibun_resource_failure_behavior(self):
        lib.touch("file.tar")
        resource = self.testresc
        zone = self.admin.zone_name
        self.user0.assert_icommand('iput file.tar ', 'EMPTY')
        self.admin.assert_icommand('ibun -x -R {resource} file.tar /{zone}/home/rods/doesntmatter'.format(**locals()),
                                   'STDERR_SINGLELINE', 'REPLICA_NOT_IN_RESC')
        self.admin.assert_icommand('ibun -x -R notaResc file.tar /{zone}/home/rods/doesntmatter'.format(**locals()),
                                   'STDERR_SINGLELINE', 'SYS_RESC_DOES_NOT_EXIST')

    def test_local_iget(self):
        # local setup
        localfile = "local.txt"
        # assertions
        self.admin.assert_icommand("iget " + self.testfile + " " + localfile)  # iget
        output, _ = lib.execute_command(['ls', localfile])
        print("  output: [" + output + "]")
        assert output.strip() == localfile
        # local cleanup
        if os.path.exists(localfile):
            os.unlink(localfile)

    def test_local_iget_with_overwrite(self):
        # local setup
        localfile = "local.txt"
        # assertions
        self.admin.assert_icommand("iget " + self.testfile + " " + localfile)  # iget
        self.admin.assert_icommand_fail("iget " + self.testfile + " " + localfile)  # already exists
        self.admin.assert_icommand("iget -f " + self.testfile + " " + localfile)  # already exists, so force
        output, _ = lib.execute_command(['ls', localfile])
        print("  output: [" + output + "]")
        assert output.strip() == localfile
        # local cleanup
        if os.path.exists(localfile):
            os.unlink(localfile)

    def test_local_iget_with_bad_option(self):
        # assertions
        self.admin.assert_icommand_fail("iget -z")  # run iget with bad option

    def test_iget_with_stale_replica(self):  # formerly known as 'dirty'
        # local setup
        filename = "original.txt"
        filepath = lib.create_local_testfile(filename)
        updated_filename = "updated_file_with_longer_filename.txt"
        updated_filepath = lib.create_local_testfile(updated_filename)
        retrievedfile = "retrievedfile.txt"
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)  # replicate file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # debugging
        # force new put on second resource
        self.admin.assert_icommand("iput -f -R " + self.testresc + " " + updated_filename + " " + filename)
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # debugging
        # should get orig file (replica 0)
        self.admin.assert_icommand("iget -f -n 0 " + filename + " " + retrievedfile)

        assert filecmp.cmp(filename, retrievedfile)  # confirm retrieved is correct
        self.admin.assert_icommand("iget -f " + filename + " " + retrievedfile)  # should get updated file
        assert filecmp.cmp(updated_filename, retrievedfile)  # confirm retrieved is correct
        # local cleanup
        if os.path.exists(filename):
            os.unlink(filename)
        if os.path.exists(updated_filename):
            os.unlink(updated_filename)
        if os.path.exists(retrievedfile):
            os.unlink(retrievedfile)

    def test_iget_with_purgec(self):
        # local setup
        filename = "purgecgetfile.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f --purgec " + filename)  # get file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed only once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_iget_specify_resource_with_single_thread__issue_3140(self):
        import subprocess

        # local setup
        filename = "test_file_3140.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput " + filename)  # put file
        self.admin.assert_icommand("iget -f -R demoResc -N0 " + filename)  # get file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # should be listed once

        # local cleanup
        output = subprocess.getstatusoutput('rm ' + filepath)

    ###################
    # imv
    ###################

    def test_local_imv(self):
        # local setup
        movedfile = "moved_file.txt"
        # assertions
        self.admin.assert_icommand("imv " + self.testfile + " " + movedfile)  # move
        self.admin.assert_icommand("ils -L " + movedfile, 'STDOUT_SINGLELINE', movedfile)  # should be listed
        # local cleanup

    def test_local_imv_to_directory(self):
        # local setup
        # assertions
        self.admin.assert_icommand("imv " + self.testfile + " " + self.testdir)  # move
        self.admin.assert_icommand("ils -L " + self.testdir, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        # local cleanup

    def test_local_imv_to_existing_filename(self):
        # local setup
        copyfile = "anotherfile.txt"
        # assertions
        self.admin.assert_icommand("icp " + self.testfile + " " + copyfile)  # icp
        # cannot overwrite existing file
        self.admin.assert_icommand("imv " + self.testfile + " " + copyfile, 'STDERR_SINGLELINE', "CAT_NAME_EXISTS_AS_DATAOBJ")
        # local cleanup

    def test_local_imv_collection_to_sibling_collection__ticket_2448(self):
        self.admin.assert_icommand("imkdir first_dir")  # first collection
        self.admin.assert_icommand("icp " + self.testfile + " first_dir")  # add file
        self.admin.assert_icommand("imkdir second_dir")  # second collection
        self.admin.assert_icommand("imv -v first_dir second_dir", 'STDOUT_SINGLELINE', "first_dir")  # imv into sibling
        self.admin.assert_icommand_fail("ils -L", 'STDOUT_SINGLELINE', "first_dir")  # should not be listed
        self.admin.assert_icommand("ils -L second_dir", 'STDOUT_SINGLELINE', "second_dir/first_dir")  # should be listed
        self.admin.assert_icommand("ils -L second_dir/first_dir", 'STDOUT_SINGLELINE', self.testfile)  # should be listed

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
        self.admin.assert_icommand("ils -rAL " + targetpath, 'STDOUT_SINGLELINE', "own")  # debugging
        self.admin.assert_icommand("ichmod -r write " + self.user0.username + " " + targetpath)  # ichmod
        self.admin.assert_icommand("ils -rAL " + targetpath, 'STDOUT_SINGLELINE', "modify_object")  # debugging
        self.admin.assert_icommand("imkdir " + sourcepath)  # source
        self.admin.assert_icommand("ichmod -r own " + self.user0.username + " " + sourcepath)  # ichmod
        self.admin.assert_icommand("ils -AL " + sourcepath, 'STDOUT_SINGLELINE', "own")  # debugging
        self.user0.assert_icommand("imv " + sourcepath + " " + targetpath)  # imv
        self.admin.assert_icommand("ils -AL " + targetpath, 'STDOUT_SINGLELINE', targetpath + "/source")  # debugging
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

    def test_iphymv_admin_mode(self):
        lib.touch("file.txt")
        for i in range(0, 100):
            self.user0.assert_icommand("iput file.txt " + str(i) + ".txt", "EMPTY")

        listing1,_,_ = self.user0.run_icommand(['ils', '-l', self.user0.session_collection])
        print(listing1)

        # scan each line of the ils and ensure that nothing is on TestResc
        for item in listing1.splitlines()[2:]:
            self.assertNotIn(self.testresc, item,
                'expected not to find [{0}] in line [{1}]'.format(self.testresc, item))

        self.admin.assert_icommand("iphymv -r -M -n0 -R " + self.testresc + " " + self.user0.session_collection)  # creates replica

        listing2,_,_ = self.user0.run_icommand(['ils', '-l', self.user0.session_collection])
        print(listing2)

        # scan each line of the ils and ensure that everything moved to TestResc
        replica_0 = 'alice             0'
        for item in listing2.splitlines():
            if replica_0 in item:
                self.assertIn(self.testresc, item,
                    'expected to find [{0}] in line [{1}]'.format(self.testresc, item))

    ###################
    # iput
    ###################

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ssl_iput_with_rods_env(self):
        server_key_path = os.path.join(self.admin.local_session_dir, 'server.key')
        chain_pem_path = os.path.join(self.admin.local_session_dir, 'chain.pem')
        dhparams_pem_path = os.path.join(self.admin.local_session_dir, 'dhparams.pem')

        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, '2048'])
        lib.execute_command('openssl req -batch -new -x509 -key %s -out %s -days 365' % (server_key_path, chain_pem_path))
        lib.execute_command('openssl dhparam -2 -out %s 2048' % (dhparams_pem_path))

        # make sure acPreConnect's definition matches that of a vanilla installation.
        # this is accomplished by inserting a new rulebase before core.re in the NREP's
        # plugin specific configuration.
        connection_rulebase = 'acPreConnect_rulebase'
        connection_rulebase_path = os.path.join(paths.core_re_directory(), connection_rulebase + '.re')
        with open(connection_rulebase_path, 'w') as f:
            f.write('acPreConnect(*OUT) { *OUT = "CS_NEG_DONT_CARE"; }\n')

        config = IrodsConfig()

        try:
            with lib.file_backed_up(config.server_config_path):
                # prepend the new rulebase to the NREP's rulebase list.
                nrep = config.server_config['plugin_configuration']['rule_engines'][0]
                nrep['plugin_specific_configuration']['re_rulebase_set'].insert(0, connection_rulebase)
                lib.update_json_file_from_dict(config.server_config_path, config.server_config)

                with lib.file_backed_up(config.client_environment_path):
                    server_update = {
                        'irods_ssl_certificate_chain_file': chain_pem_path,
                        'irods_ssl_certificate_key_file': server_key_path,
                        'irods_ssl_dh_params_file': dhparams_pem_path
                    }
                    lib.update_json_file_from_dict(config.client_environment_path, server_update)

                    client_update = {
                        'irods_client_server_policy': 'CS_NEG_REQUIRE',
                        'irods_ssl_verify_server': 'none'
                    }
                    session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
                    self.admin.environment_file_contents.update(client_update)

                    filename = 'encryptedfile.txt'
                    filepath = lib.create_local_testfile(filename)
                    IrodsController().restart(test_mode=True)

                    self.admin.assert_icommand('iinit', 'STDOUT_SINGLELINE',
                                               'Enter your current iRODS password:',
                                               input=f'{self.admin.password}\n')
                    self.admin.assert_icommand(['iput', filename])
                    self.admin.assert_icommand(['ils', '-L', filename], 'STDOUT_SINGLELINE', filename)

                    self.admin.environment_file_contents = session_env_backup
        finally:
            try:
                os.remove(connection_rulebase_path)
            except:
                pass

            IrodsController().restart(test_mode=True)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ssl_iput_small_and_large_files(self):
        # set up client and server side for ssl handshake

        # server side certificate setup
        server_key_path = os.path.join(self.admin.local_session_dir, 'server.key')
        chain_pem_path = os.path.join(self.admin.local_session_dir, 'chain.pem')
        dhparams_pem_path = os.path.join(self.admin.local_session_dir, 'dhparams.pem')

        lib.execute_command(['openssl', 'genrsa', '-out', server_key_path, '2048'])
        lib.execute_command('openssl req -batch -new -x509 -key %s -out %s -days 365' % (server_key_path, chain_pem_path))
        lib.execute_command('openssl dhparam -2 -out %s 2048' % (dhparams_pem_path))

        # client side environment variables
        os.environ['irodsSSLVerifyServer'] = "none"

        client_env_file = self.admin.local_session_dir + "/irods_environment.json"

        try:
            with lib.file_backed_up(client_env_file):
                # update client environment settings.
                client_env_update = {'irods_client_server_policy': 'CS_NEG_REQUIRE'}
                lib.update_json_file_from_dict(client_env_file, client_env_update)

                # make sure acPreConnect's definition matches that of a vanilla installation.
                # this is accomplished by inserting a new rulebase before core.re in the NREP's
                # plugin specific configuration.
                connection_rulebase = 'acPreConnect_rulebase'
                connection_rulebase_path = os.path.join(paths.core_re_directory(), connection_rulebase + '.re')
                with open(connection_rulebase_path, 'w') as f:
                    f.write('acPreConnect(*OUT) { *OUT = "CS_NEG_DONT_CARE"; }\n')

                config = IrodsConfig()

                with lib.file_backed_up(config.server_config_path):
                    # prepend the new rulebase to the NREP's rulebase list.
                    nrep = config.server_config['plugin_configuration']['rule_engines'][0]
                    nrep['plugin_specific_configuration']['re_rulebase_set'].insert(0, connection_rulebase)
                    lib.update_json_file_from_dict(config.server_config_path, config.server_config)

                    # server reboot with new server side environment variables
                    IrodsController(IrodsConfig(
                            injected_environment={
                                'irodsSSLCertificateChainFile': chain_pem_path,
                                'irodsSSLCertificateKeyFile': server_key_path,
                                'irodsSSLDHParamsFile': dhparams_pem_path})
                            ).restart(test_mode=True)

                    # reinitialize
                    self.admin.assert_icommand('iinit', 'STDOUT_SINGLELINE',
                                               'Enter your current iRODS password:',
                                               input=f'{self.admin.password}\n')

                    #
                    # do the encrypted put
                    #

                    # small file
                    filename = "encryptedfile.txt"
                    filepath = lib.create_local_testfile(filename)
                    self.admin.assert_icommand("iput " + filename)  # encrypted put - small file
                    self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should be listed

                    # large file
                    largefilename = "BIGencryptedfile.txt"
                    lib.make_file(largefilename, 60*(2**20))
                    self.admin.assert_icommand("iput " + largefilename)  # encrypted put - large file
                    self.admin.assert_icommand("ils -L " + largefilename, 'STDOUT_SINGLELINE', largefilename)  # should be listed
        finally:
            # reset client environment
            os.unsetenv('irodsSSLVerifyServer')

            try:
                # clean up
                os.remove(filename)
                os.remove(largefilename)
                os.remove(connection_rulebase_path)
            except:
                pass

            # restart iRODS server without altered environment
            IrodsController().restart(test_mode=True)

    @unittest.skipIf(psutil.disk_usage('/').free < 20000000000, "not enough free space for 5 x 2.2GB file ( local + iput + 3 repl children )")
    def test_local_iput_with_really_big_file__ticket_1623(self):
        filename = "reallybigfile.txt"
        # file size larger than 32 bit int
        lib.make_file(filename, pow(2, 31) + 100)
        print("file size = [" + str(os.stat(filename).st_size) + "]")
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', [filename, "does not exist"])
        self.admin.assert_icommand("iput " + filename)  # iput
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should be listed
        if os.path.exists(filename):
            os.unlink(filename)

    def test_local_iput(self):
        '''also needs to count and confirm number of replicas after the put'''
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT_SINGLELINE', datafilename)  # should not be listed
        self.admin.assert_icommand("iput " + datafilename)  # iput
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE', datafilename)  # should be listed
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    def test_local_iput_overwrite(self):
        self.admin.assert_icommand_fail("iput " + self.testfile)  # fail, already exists
        self.admin.assert_icommand("iput -f " + self.testfile)  # iput again, force

    def test_local_iput_recursive(self):
        recursivedirname = "dir"

    def test_local_iput_lower_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        self.admin.assert_icommand("iput -k " + datafilename)  # iput
        with open(datafilename, 'rb') as f:
            checksum = base64.b64encode(hashlib.sha256(f.read()).digest()).decode()
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "sha2:" + checksum)  # check proper checksum
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    def test_local_iput_upper_checksum(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        self.admin.assert_icommand("iput -K " + datafilename)  # iput
        with open(datafilename, 'rb') as f:
            checksum = base64.b64encode(hashlib.sha256(f.read()).digest()).decode()
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', "sha2:" + checksum)  # check proper checksum
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    def test_local_iput_onto_specific_resource(self):
        # local setup
        datafilename = "anotherfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT_SINGLELINE', datafilename)  # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + datafilename)  # iput
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE', datafilename)  # should be listed
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE', self.testresc)  # should be listed
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    @unittest.skipIf(True, 'Enable once #2634 is resolved')
    def test_local_iput_interrupt_directory(self):
        # local setup
        datadir = "newdatadir"
        os.makedirs(datadir)
        datafiles = ["file1", "file2", "file3", "file4", "file5", "file6", "file7"]
        for datafilename in datafiles:
            print("-------------------")
            print("creating " + datafilename + "...")
            localpath = datadir + "/" + datafilename
            lib.make_file(localpath, 2**20)
        restartfile = "collectionrestartfile"
        # assertions
        iputcmd = "iput -X " + restartfile + " -r " + datadir
        if os.path.exists(restartfile):
            os.unlink(restartfile)
        self.admin.interrupt_icommand(iputcmd, restartfile, 10)  # once restartfile reaches 10 bytes
        assert os.path.exists(restartfile), restartfile + " should now exist, but did not"
        print("  restartfile [" + restartfile + "] contents --> [")
        with open(restartfile, 'r') as f:
            for line in f:
                print(line)
        print("]")
        self.admin.assert_icommand("ils -L " + datadir, 'STDOUT_SINGLELINE', datadir)  # just to show contents
        self.admin.assert_icommand(iputcmd, 'STDOUT_SINGLELINE', "File last completed")  # confirm the restart
        for datafilename in datafiles:
            self.admin.assert_icommand("ils -L " + datadir, 'STDOUT_SINGLELINE', datafilename)  # should be listed
        # local cleanup
        lib.execute_command(['rm', '-rf', datadir])
        if os.path.exists(restartfile):
            os.unlink(restartfile)

    @unittest.skipIf(True, 'Enable once race conditions fixed, see #2634')
    def test_local_iput_interrupt_largefile(self):
        # local setup
        datafilename = 'bigfile'
        file_size = int(6 * pow(10, 8))
        lib.make_file(datafilename, file_size)
        restartfile = 'bigrestartfile'
        iputcmd = 'iput --lfrestart {0} {1}'.format(restartfile, datafilename)
        if os.path.exists(restartfile):
            os.unlink(restartfile)
        self.admin.interrupt_icommand(iputcmd, restartfile, 300)  # once restartfile reaches 300 bytes
        time.sleep(2)  # wait for all interrupted threads to exit
        assert os.path.exists(restartfile), restartfile + " should now exist, but did not"
        print("  restartfile [" + restartfile + "] contents --> [")
        with open(restartfile, 'r') as f:
            for line in f:
                print(line)
        print("]")
        today = datetime.date.today()
        # length should not be zero
        self.admin.assert_icommand_fail("ils -L " + datafilename, 'STDOUT_SINGLELINE', [" 0 " + today.isoformat(), datafilename])
        # confirm the restart
        self.admin.assert_icommand(iputcmd, 'STDOUT_SINGLELINE', datafilename + " was restarted successfully")
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE',
                                   [" " + str(os.stat(datafilename).st_size) + " " + today.isoformat(), datafilename])  # length should be size on disk
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)
        if os.path.exists(restartfile):
            os.unlink(restartfile)

    def test_local_iput_physicalpath_no_permission(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        self.admin.assert_icommand("iput -p /newfileinroot.txt " + datafilename, 'STDERR_SINGLELINE',
                                   ["UNIX_FILE_CREATE_ERR", "Permission denied"])  # should fail to write
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Needs investigation refer issue 4932")
    def test_local_iput_physicalpath(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        fullpath = IrodsConfig().irods_directory + "/newphysicalpath.txt"
        self.admin.assert_icommand("iput -p " + fullpath + " " + datafilename)  # should complete
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE', datafilename)  # should be listed
        self.admin.assert_icommand("ils -L " + datafilename, 'STDOUT_SINGLELINE', fullpath)  # should be listed
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)
        if os.path.exists(fullpath):
            os.unlink(fullpath)

    def test_admin_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        relpath = "relativephysicalpath.txt"
        # should disallow relative path
        self.admin.assert_icommand("iput -p " + relpath + " " + datafilename, 'STDERR_SINGLELINE', "absolute")
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    def test_local_iput_relative_physicalpath_into_server_bin(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        relpath = "relativephysicalpath.txt"
        self.user0.assert_icommand("iput -p " + relpath + " " + datafilename, 'STDERR_SINGLELINE', "absolute")  # should error
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    def test_local_iput_with_changed_target_filename(self):
        # local setup
        datafilename = "newfile.txt"
        with open(datafilename, 'wt') as f:
            print("TESTFILE -- [" + datafilename + "]", file=f, end='')
        # assertions
        changedfilename = "different.txt"
        self.admin.assert_icommand("iput " + datafilename + " " + changedfilename)  # should complete
        self.admin.assert_icommand("ils -L " + changedfilename, 'STDOUT_SINGLELINE', changedfilename)  # should be listed
        # local cleanup
        if os.path.exists(datafilename):
            os.unlink(datafilename)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Lists Vault files")
    def test_iput_overwrite_others_file__ticket_2086(self):
        # pre state
        self.admin.assert_icommand("ils -L", 'STDOUT_SINGLELINE', self.testfile)  # should be listed

        # local setup
        filename = "overwritefile.txt"
        filepath = lib.create_local_testfile(filename)

        # alice tries to put
        homepath = "/home/" + self.admin.username + "/" + self.admin._session_id + "/" + self.testfile
        logicalpath = "/" + self.admin.zone_name + homepath
        self.user0.assert_icommand("iput " + filepath + " " + logicalpath, 'STDERR_SINGLELINE', "CAT_NO_ACCESS_PERMISSION")  # iput

        # check physicalpaths (of all replicas)
        out, _, _ = self.admin.run_icommand(['ils', '-L'])
        print("[ils -L]:")
        print("[" + out + "]")
        lines = out.splitlines()
        for i in range(0, len(lines) - 1):
            if "0 demoResc" in lines[i]:
                if "/session-" in lines[i + 1]:
                    l = lines[i + 1]
                    physicalpath = l.split()[1]
                    # check file is on disk
                    print("[ls -l " + physicalpath + "]:")
                    lib.execute_command("ls -l " + physicalpath)
                    assert os.path.exists(physicalpath)

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    def test_iput_with_purgec(self):
        # local setup
        filename = "purgecfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        try:
            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand("iput --purgec " + filename)  # put file
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # should be listed once
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed only once
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once
            self.admin.assert_icommand(['irm', '-f', filename])

            self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
            self.admin.assert_icommand(['iput', '-b', '--purgec', filename])  # put file... in bulk!
            self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # should be listed once
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed only once
            self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        finally:
            if os.path.exists(filepath):
                os.unlink(filepath)

    def test_local_iput_with_force_and_destination_resource__ticket_1706(self):
        # local setup
        filename = "iputwithforceanddestination.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        lib.execute_command("cat %s %s > %s" % (filename, filename, doublefile), use_unsafe_shell=True)
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
        # targeted resource should have new double clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " " + doublesize + " ", "& " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    ###################
    # ireg
    ###################

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsadmin(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("ireg " + filepath + " /" + self.admin.zone_name + "/home/" +
                                   self.admin.username + "/" + self.admin._session_id + "/" + filename)  # ireg
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers local file")
    def test_ireg_as_rodsuser(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.user0.assert_icommand(['ireg', filepath, self.user0.session_collection + '/' + filename], 'STDERR_SINGLELINE', 'PATH_REG_NOT_ALLOWED')
        self.user0.assert_icommand_fail('ils -L ' + filename, 'STDOUT_SINGLELINE', filename)

        # local cleanup
        os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing: Registers file in Vault")
    def test_ireg_as_rodsuser_in_vault(self):
        # get vault base path
        out, _, _ = self.user0.run_icommand(['iquest', '%s', "select RESC_VAULT_PATH where RESC_NAME = 'demoResc'"])
        vaultpath = out.rstrip('\n')

        # make dir in vault if necessary
        dir = os.path.join(vaultpath, 'home', self.user0.username)
        if not os.path.exists(dir):
            os.makedirs(dir)

        # create file in vault
        filename = "newfile.txt"
        filepath = os.path.join(dir, filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.user0.assert_icommand(['ireg', filepath, self.user0.session_collection + '/' + filename], 'STDERR_SINGLELINE', 'PATH_REG_NOT_ALLOWED')
        self.user0.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', filename)  # should not be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_ireg_repl_to_coordinating_resource__issue_3844(self):
        # local setup
        filename = "newfile.txt"
        filepath = os.path.abspath(filename)
        with open(filepath, 'wt') as f:
            print("TESTFILE -- [" + filepath + "]", file=f, end='')

        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("ireg -R " + self.testresc + " " + filepath + " /" + self.admin.zone_name + "/home/" +
                                   self.admin.username + "/" + self.admin._session_id + "/" + filename)  # ireg
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', self.testresc)  # should be listed
        self.admin.assert_icommand("ireg -R demoResc --repl " + " " + filepath + " /" + self.admin.zone_name + "/home/" +
                                   self.admin.username + "/" + self.admin._session_id + "/" + filename)  # ireg
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', 'demoResc')  # should be listed

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)



    ###################
    # irepl
    ###################

    def test_irepl_invalid_input(self):
        # local setup
        filename = "somefile.txt"
        filepath = lib.create_local_testfile(filename)
        # assertions
        # should not be listed
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")
        self.admin.assert_icommand("iput " + filename)                                                 # put file
        # for debugging
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)
        # replicate to bad resource
        self.admin.assert_icommand("irepl -R nonresc " + filename, 'STDERR_SINGLELINE', "SYS_RESC_DOES_NOT_EXIST")
        self.admin.assert_icommand("irm -f " + filename)                                               # cleanup file
        # local cleanup
        os.remove(filepath)

    def test_irepl_multithreaded(self):
        # local setup
        filename = "largefile.txt"
        lib.make_file(filename, 64*1024*1024, 'arbitrary')
        # assertions
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")     # should not be listed
        self.admin.assert_icommand("iput " + filename)                                 # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)             # for debugging
        self.admin.assert_icommand("irepl -R " + self.testresc + " -N 3 " + filename)      # replicate to test resource
        # test resource should be listed
        self.admin.assert_icommand("ils -l " + filename, 'STDOUT_SINGLELINE', self.testresc)
        self.admin.assert_icommand("irm -f " + filename)                               # cleanup file
        # local cleanup
        os.remove(filename)

    def test_irepl_update_replicas(self):
        # local setup
        filename = "updatereplicasfile.txt"
        filepath = lib.create_local_testfile(filename)
        hostname = lib.get_hostname()
        hostuser = getpass.getuser()
        doublefile = "doublefile.txt"
        lib.execute_command("cat %s %s > %s" % (filename, filename, doublefile), use_unsafe_shell=True)
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
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])

        self.admin.assert_icommand(['irepl', '-R', 'fourthresc', filename])                # update last replica

        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a dirty copy
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])

        self.admin.assert_icommand("irepl -a " + filename)                                # update all replicas

        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # should have a clean copy
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])

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
        self.admin.assert_icommand("ils -L " + filename, 'STDERR_SINGLELINE', "does not exist")  # should not be listed
        self.admin.assert_icommand("iput -R " + self.testresc + " " + filename)       # put file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        self.admin.assert_icommand("irepl " + filename)               # replicate to default resource
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
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
        # replicate overtop default resource
        self.admin.assert_icommand(['irepl', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # replicate overtop test resource
        self.admin.assert_icommand(['irepl', '-R', self.testresc, filename], 'STDERR', 'SYS_NOT_ALLOWED')
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # replicate overtop third resource
        self.admin.assert_icommand(['irepl', '-R', 'thirdresc', filename], 'STDERR', 'SYS_NOT_ALLOWED')
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', filename)          # for debugging
        # should not have a replica 3
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 3 ", " & " + filename])
        # should not have a replica 4
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 4 ", " & " + filename])
        self.admin.assert_icommand("irm -f " + filename)                          # cleanup file
        self.admin.assert_icommand("iadmin rmresc thirdresc")                   # remove third resource
        # local cleanup
        os.remove(filepath)

    def test_irepl_over_existing_bad_replica__ticket_1705(self):
        # local setup
        filename = "reploverwritebad.txt"
        filepath = lib.create_local_testfile(filename)
        doublefile = "doublefile.txt"
        lib.execute_command("cat %s %s > %s" % (filename, filename, doublefile), use_unsafe_shell=True)
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
        # test resource should not have doublesize file
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE',
                                        [" 1 " + self.testresc, " " + doublesize + " ", "  " + filename])
        # replicate back onto test resource
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + filename)
        # test resource should have new clean doublesize file
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE',
                                   [" 1 " + self.testresc, " " + doublesize + " ", " & " + filename])
        # should not have a replica 2
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", " & " + filename])
        # local cleanup
        os.remove(filepath)
        os.remove(doublefile)

    # repl update ( repave old copies )
    # walk through command line switches

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
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 0 ", filename])  # should be trimmed
        self.admin.assert_icommand("ils -L " + filename, 'STDOUT_SINGLELINE', [" 1 ", filename])  # should be listed once
        self.admin.assert_icommand_fail("ils -L " + filename, 'STDOUT_SINGLELINE', [" 2 ", filename])  # should be listed only once

        # local cleanup
        if os.path.exists(filepath):
            os.unlink(filepath)

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
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irm " + self.testfile)  # remove from grid
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDERR_SINGLELINE', self.testfile)  # should be deleted
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        # should be in trash
        self.admin.assert_icommand("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE', self.testfile)

    def test_irm_force(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irm -f " + self.testfile)  # remove from grid
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be deleted
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        # should not be in trash
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE', self.testfile)

    def test_irm_specific_replica(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irepl -R " + self.testresc + " " + self.testfile)  # creates replica
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed twice
        self.admin.assert_icommand("itrim -N 1 -n 0 " + self.testfile, 'STDOUT')  # remove original from grid
        # replica 1 should be there
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', ["1 " + self.testresc, self.testfile])
        self.admin.assert_icommand_fail("ils -L " + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica 0 should be gone
        trashpath = "/" + self.admin.zone_name + "/trash/home/" + self.admin.username + \
            "/" + self.admin._session_id
        self.admin.assert_icommand_fail("ils -L " + trashpath + "/" + self.testfile, 'STDOUT_SINGLELINE',
                                        ["0 " + self.admin.default_resource, self.testfile])  # replica should not be in trash

    def test_irm_recursive_file(self):
        self.admin.assert_icommand("ils -L " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irm -r " + self.testfile)  # should not fail, even though a collection

    def test_irm_recursive(self):
        self.admin.assert_icommand("icp -r " + self.testdir + " copydir")  # make a dir copy
        self.admin.assert_icommand("ils -L ", 'STDOUT_SINGLELINE', "copydir")  # should be listed
        self.admin.assert_icommand("irm -r copydir")  # should remove
        self.admin.assert_icommand_fail("ils -L ", 'STDOUT_SINGLELINE', "copydir")  # should not be listed

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_read_permission(self):
        self.user0.assert_icommand("icd ../../public")  # switch to shared area
        self.user0.assert_icommand("ils -AL " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.user0.assert_icommand_fail("irm " + self.testfile)  # read perm should not be allowed to remove
        self.user0.assert_icommand("ils -AL " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should still be listed

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irm_with_write_permission(self):
        self.user1.assert_icommand("icd ../../public")  # switch to shared area
        self.user1.assert_icommand("ils -AL " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.user1.assert_icommand_fail("irm " + self.testfile)  # write perm should not be allowed to remove
        self.user1.assert_icommand("ils -AL " + self.testfile, 'STDOUT_SINGLELINE', self.testfile)  # should still be listed

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
            self.admin.assert_icommand("ils -L " + trashpath, 'STDOUT_SINGLELINE', filename)

    ###################
    # irmtrash
    ###################

    def test_irmtrash_admin(self):
        # assertions
        self.admin.assert_icommand("irm " + self.testfile)  # remove from grid
        self.admin.assert_icommand("ils -rL /" + self.admin.zone_name + "/trash/home/" +
                                   self.admin.username + "/", 'STDOUT_SINGLELINE', self.testfile)  # should be listed
        self.admin.assert_icommand("irmtrash")  # should be listed
        self.admin.assert_icommand_fail("ils -rL /" + self.admin.zone_name + "/trash/home/" +
                                        self.admin.username + "/", 'STDOUT_SINGLELINE', self.testfile)  # should be deleted

    ###################
    # itrim
    ###################

    def test_itrim_with_admin_mode(self):
        lib.make_file("file.txt", 0)
        for i in range(100):
            self.user0.assert_icommand("iput file.txt " + str(i) + ".txt", "EMPTY")

        filename1 = "itrimadminmode1.txt"
        filename2 = "itrimadminmode2.txt"
        filesize = int(pow(2, 20) + pow(10,5))

        filesizeMB = round(float(2 * filesize)/1048576, 3)
        lib.make_file(filename1, filesize)
        lib.make_file(filename2, filesize)

        self.user0.assert_icommand("iput {filename1}".format(**locals()), 'EMPTY')
        self.user0.assert_icommand("iput {filename2}".format(**locals()), 'EMPTY')

        homepath = self.user0.session_collection
        self.user0.assert_icommand("irepl -R " + self.testresc + " -r " + homepath, "EMPTY")  # creates replica
        self.admin.assert_icommand("itrim -M -N1 -r " + homepath, 'STDOUT_SINGLELINE', "Total size trimmed = " + str(filesizeMB) +" MB. Number of files trimmed = 102.")

        #local file cleanup
        os.unlink(os.path.abspath("file.txt"))
        os.unlink(os.path.abspath(filename1))
        os.unlink(os.path.abspath(filename2))

    def test_itrim_no_op(self):
        collection = self.admin.session_collection
        filename = self.testfile
        repl_resource = self.anotherresc

        # check that test file is there
        self.admin.assert_icommand("ils {filename}".format(**locals()), 'STDOUT_SINGLELINE', filename)

        # replicate test file
        self.admin.assert_icommand("irepl -R {repl_resource} {filename}".format(**locals()), 'EMPTY')

        # check replication
        self.admin.assert_icommand("ils -L {filename}".format(**locals()), 'STDOUT_SINGLELINE', repl_resource)

        # count replicas
        repl_count = self.admin.run_icommand('''iquest "%s" "SELECT count(DATA_ID) where COLL_NAME ='{collection}' and DATA_NAME ='{filename}'"'''.format(**locals()))[0]

        # try to trim down to repl_count
        self.admin.assert_icommand("itrim -N {repl_count} {filename}".format(**locals()), 'STDOUT_SINGLELINE', "Total size trimmed = 0.000 MB. Number of files trimmed = 0.")


    def assert_permissions_on_data_object_for_user(self, username, logical_path, permission_value):
        data_access_type = self.admin.run_icommand(['iquest', '%s',
            'select DATA_ACCESS_TYPE where COLL_NAME = \'{}\' and DATA_NAME = \'{}\' and USER_NAME = \'{}\''.format(
                os.path.dirname(logical_path), os.path.basename(logical_path), username)
            ])[0].strip()

        self.assertEqual(str(data_access_type), str(permission_value))


    def test_iget_data_object_as_user_with_read_only_access_and_replica_only_in_archive__issue_6697(self):
        compound_resource = 'demoResc'
        cache_resource = 'cacheResc'
        archive_resource = 'archiveResc'
        cache_hierarchy = compound_resource + ';' + cache_resource
        archive_hierarchy = compound_resource + ';' + archive_resource

        owner_user = self.user0
        readonly_user = self.user1
        filename = 'foo'
        contents = 'jimbo'
        logical_path = os.path.join(owner_user.session_collection, filename)
        READ_OBJECT = 1050

        try:
            # Create a data object which should appear under the compound resource.
            owner_user.assert_icommand(['istream', 'write', logical_path], input=contents)
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

            # Grant read access to another user, ensuring that the other user can see the data object.
            owner_user.assert_icommand(['ichmod', '-r', 'read', readonly_user.username, os.path.dirname(logical_path)])

            # Ensure that the read-only user has read-only permission on the data object.
            self.assert_permissions_on_data_object_for_user(readonly_user.username, logical_path, READ_OBJECT)

            # Trim the replica on the cache resource so that only the replica in the archive remains. Replica 0 resides
            # on the cache resource at this point.
            owner_user.assert_icommand(['itrim', '-N1', '-n0', logical_path], 'STDOUT')
            self.assertFalse(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

            # As the user with read-only access, attempt to get the data object. Replica 1 resides on the archive
            # resource, so the replica on the cache resource which results from the stage-to-cache should be number 2.
            readonly_user.assert_icommand(['iget', logical_path, '-'], 'STDOUT', contents)
            self.assertEqual(str(1), lib.get_replica_status(readonly_user, os.path.basename(logical_path), 1))
            self.assertEqual(str(1), lib.get_replica_status(readonly_user, os.path.basename(logical_path), 2))

            # Ensure that the user has the same permissions on the data object as before getting it.
            self.assert_permissions_on_data_object_for_user(readonly_user.username, logical_path, READ_OBJECT)

        finally:
            self.admin.assert_icommand(['ils', '-Al', logical_path], 'STDOUT') # Debugging

            # Make sure that the data object can be removed by marking both replicas stale before removing.
            self.admin.run_icommand(['ichmod', '-M', 'own', self.admin.username, logical_path])
            self.admin.run_icommand(
                ['iadmin', 'modrepl', 'logical_path', logical_path, 'resource_hierarchy', cache_hierarchy, 'DATA_REPL_STATUS', '0'])
            self.admin.run_icommand(
                ['iadmin', 'modrepl', 'logical_path', logical_path, 'resource_hierarchy', archive_hierarchy, 'DATA_REPL_STATUS', '0'])
            self.admin.run_icommand(['irm', '-f', logical_path])


    def test_iget_data_object_as_user_with_null_access_and_replica_only_in_archive__issue_6697(self):
        compound_resource = 'demoResc'
        cache_resource = 'cacheResc'
        archive_resource = 'archiveResc'
        cache_hierarchy = compound_resource + ';' + cache_resource
        archive_hierarchy = compound_resource + ';' + archive_resource

        owner_user = self.user0
        no_access_user = self.user1
        filename = 'foo'
        contents = 'jimbo'
        not_found_string = 'CAT_NO_ROWS_FOUND: Nothing was found matching your query'
        logical_path = os.path.join(owner_user.session_collection, filename)

        try:
            # Create a data object which should appear under the compound resource.
            owner_user.assert_icommand(['istream', 'write', logical_path], input=contents)
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

            # Ensure that the no-access user has no access permissions on the data object.
            self.assert_permissions_on_data_object_for_user(no_access_user.username, logical_path, not_found_string)

            # Trim the replica on the cache resource so that only the replica in the archive remains. Replica 0 resides
            # on the cache resource at this point.
            owner_user.assert_icommand(['itrim', '-N1', '-n0', logical_path], 'STDOUT')
            self.assertFalse(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

            # As the user with no access, attempt to get the data object. This should fail, and stage-to-cache should
            # not occur. Confirm that no replica exists on the cache resource.
            no_access_user.assert_icommand(
                ['iget', logical_path, '-'], 'STDERR', '{} does not exist'.format(logical_path))
            self.assertFalse(lib.replica_exists_on_resource(owner_user, logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(owner_user, logical_path, archive_resource))

            # Ensure that the no-access user has no access permissions on the data object.
            self.assert_permissions_on_data_object_for_user(no_access_user.username, logical_path, not_found_string)

        finally:
            self.admin.assert_icommand(['ils', '-Al', logical_path], 'STDOUT') # Debugging

            # Make sure that the data object can be removed by marking both replicas stale before removing.
            self.admin.run_icommand(['ichmod', '-M', 'own', self.admin.username, logical_path])
            self.admin.run_icommand(
                ['iadmin', 'modrepl', 'logical_path', logical_path, 'resource_hierarchy', cache_hierarchy, 'DATA_REPL_STATUS', '0'])
            self.admin.run_icommand(
                ['iadmin', 'modrepl', 'logical_path', logical_path, 'resource_hierarchy', archive_hierarchy, 'DATA_REPL_STATUS', '0'])
            self.admin.run_icommand(['irm', '-f', logical_path])
