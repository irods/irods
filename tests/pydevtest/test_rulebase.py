import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import socket
import time  # remove once file hash fix is commited #2279
import lib
import time
import copy
import inspect

import configuration
from resource_suite import ResourceBase


class Test_Rulebase(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Rulebase, self).setUp()

    def tearDown(self):
        super(Test_Rulebase, self).tearDown()

    def test_client_server_negotiation__2564(self):
        corefile = lib.get_core_re_dir() + "/core.re"
        with lib.file_backed_up(corefile):
            client_update = {
                'irods_client_server_policy': 'CS_NEG_REFUSE'
            }

            session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
            self.admin.environment_file_contents.update(client_update)

            time.sleep(2)  # remove once file hash fix is commited #2279
            lib.prepend_string_to_file('\nacPreConnect(*OUT) { *OUT="CS_NEG_REQUIRE"; }\n', corefile)
            time.sleep(2)  # remove once file hash fix is commited #2279

            self.admin.assert_icommand( 'ils','STDERR_SINGLELINE','CLIENT_NEGOTIATION_ERROR')

            self.admin.environment_file_contents = session_env_backup

    def test_msiDataObjWrite__2795(self):
        rule_file = "test_rule_file.r"
        rule_string = """
test_msiDataObjWrite__2795 {
  ### write a string to a file in irods
  msiDataObjCreate("*TEST_ROOT" ++ "/test_file.txt","null",*FD);
  msiDataObjWrite(*FD,"this_is_a_test_string",*LEN);
  msiDataObjClose(*FD,*Status);

}

INPUT *TEST_ROOT=\""""+self.admin.session_collection+"""\"
OUTPUT ruleExecOut
"""
        with open(rule_file, 'w') as f:
            f.write(rule_string)

        test_file = self.admin.session_collection+'/test_file.txt'

        self.admin.assert_icommand('irule -F ' + rule_file)
        self.admin.assert_icommand('ils -l','STDOUT_SINGLELINE','test_file')
        self.admin.assert_icommand('iget -f '+test_file)

        with open("test_file.txt", 'r') as f:
            file_contents = f.read()

        assert( not file_contents.endswith('\0') )

    def test_acPostProcForPut_replicate_to_multiple_resources(self):
        # create new resources
        hostname = socket.gethostname()
        self.admin.assert_icommand("iadmin mkresc r1 unixfilesystem " + hostname + ":/tmp/irods/r1", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc r2 unixfilesystem " + hostname + ":/tmp/irods/r2", 'STDOUT_SINGLELINE', "Creating")

        corefile = os.path.join(lib.get_core_re_dir(), 'core.re')
        with lib.file_backed_up(corefile):
            time.sleep(2)  # remove once file hash fix is commited #2279
            lib.prepend_string_to_file('\nacPostProcForPut { replicateMultiple( \"r1,r2\" ); }\n', corefile)
            time.sleep(2)  # remove once file hash fix is commited #2279

            # add new rule to end of core.re
            newrule = """
# multiple replication rule
replicateMultiple(*destRgStr) {
    *destRgList = split(*destRgStr, ',');
    writeLine("serverLog", " acPostProcForPut multiple replicate $objPath $filePath -> *destRgStr");
    foreach (*destRg in *destRgList) {
        writeLine("serverLog", " acPostProcForPut replicate $objPath $filePath -> *destRg");
        *e = errorcode(msiSysReplDataObj(*destRg,"null"));
        if (*e != 0) {
            if(*e == -808000) {
                writeLine("serverLog", "$objPath cannot be found");
                $status = 0;
                succeed;
            } else {
                fail(*e);
            }
        }
    }
}
"""

            time.sleep(2)  # remove once file hash fix is commited #2279
            lib.prepend_string_to_file(newrule, corefile)
            time.sleep(2)  # remove once file hash fix is commited #2279

            # put data
            tfile = "rulebasetestfile"
            lib.touch(tfile)
            self.admin.assert_icommand(['iput', tfile])

            # check replicas
            self.admin.assert_icommand(['ils', '-L', tfile], 'STDOUT_MULTILINE', [' demoResc ', ' r1 ', ' r2 '])

            # clean up and remove new resources
            self.admin.assert_icommand("irm -rf " + tfile)
            self.admin.assert_icommand("iadmin rmresc r1")
            self.admin.assert_icommand("iadmin rmresc r2")

        time.sleep(2)  # remove once file hash fix is commited #2279

    def test_dynamic_pep_with_rscomm_usage(self):
        # save original core.re
        corefile = os.path.join(lib.get_core_re_dir(), "core.re")
        origcorefile = os.path.join(lib.get_core_re_dir(), "core.re.orig")
        os.system("cp " + corefile + " " + origcorefile)

        # add dynamic PEP with rscomm usage
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system('''echo "pep_resource_open_pre(*OUT) { msiGetSystemTime( *junk, '' ); }" >> ''' + corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

        # check rei functioning
        self.admin.assert_icommand("iget " + self.testfile + " - ", 'STDOUT_SINGLELINE', self.testfile)

        # restore core.re
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp " + origcorefile + " " + corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rulebase_update__2585(self):
        rule_file = 'my_rule.r'
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        my_rule = """
my_rule {
    delay("<PLUSET>1s</PLUSET>") {
        do_some_stuff();
    }
}
INPUT null
OUTPUT ruleExecOut
        """
        with open(rule_file, 'w') as f:
            f.write(my_rule)

        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'
        with lib.file_backed_up(server_config_filename):
            # write new rule file to config dir
            test_rule = 'do_some_stuff() { writeLine( "serverLog", "TEST_STRING_TO_FIND_1_2585" ); }'
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)
            lib.restart_irods_server()

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('re')
            self.admin.assert_icommand('irule -F ' + rule_file)
            time.sleep(35)  # wait for test to fire
            assert lib.count_occurrences_of_string_in_log('re', 'TEST_STRING_TO_FIND_1_2585', start_index=initial_log_size)

            # repave rule with new string
            test_rule = 'do_some_stuff() { writeLine( "serverLog", "TEST_STRING_TO_FIND_2_2585" ); }'
            os.unlink(test_re)
            with open(test_re, 'w') as f:
                f.write(test_rule)
            time.sleep(35)  # wait for delay rule engine to wake

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('re')
            self.admin.assert_icommand('irule -F ' + rule_file)
            time.sleep(35)  # wait for test to fire
            assert lib.count_occurrences_of_string_in_log('re', 'TEST_STRING_TO_FIND_2_2585', start_index=initial_log_size)

        # cleanup
        os.unlink(test_re)
        os.unlink(rule_file)
        lib.restart_irods_server()


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForPut__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # iput test file to trigger PEP
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {testfile}'.format(**locals()))
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acDataDeletePolicy__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # iput test file
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # delete test file to trigger PEP
            sesh.assert_icommand('irm -f {testfile}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForDelete__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # iput test file
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # delete test file to trigger PEP
            sesh.assert_icommand('irm -f {testfile}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acSetChkFilePathPerm__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # test file for ireg
        testfile = os.path.join(lib.get_irods_top_level_dir(), 'VERSION.json')

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # ireg test file to trigger PEP
            target_obj = os.path.join(sesh.home_collection, os.path.basename(testfile))
            sesh.assert_icommand('ireg {testfile} {target_obj}'.format(**locals()), 'STDERR_SINGLELINE', 'PATH_REG_NOT_ALLOWED')

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForFilePathReg__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        # use admin to be allowed to register stuff
        sesh = self.admin

        # test file for ireg
        username = sesh.username
        resc_vault_path = lib.get_vault_path(sesh)
        testfile = '{resc_vault_path}/home/{username}/foo.txt'.format(**locals())
        open(testfile, 'a').close()

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # ireg test file to trigger PEP
            target_obj = os.path.join(sesh.home_collection, os.path.basename(testfile))
            sesh.assert_icommand('ireg {testfile} {target_obj}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {target_obj}'.format(**locals()))
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForCopy__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # iput test file
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # copy test file to trigger PEP
            sesh.assert_icommand('icp {testfile} {testfile}_copy'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {testfile}'.format(**locals()))
        sesh.run_icommand('irm -f {testfile}_copy'.format(**locals()))
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acSetVaultPathPolicy__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # iput test file to trigger PEP
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {testfile}'.format(**locals()))
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPreprocForDataObjOpen__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile
        target_obj = os.path.join(sesh.session_collection, testfile)

        # prepare rule file
        rule_file = "test_rule_file.r"
        rule_string = '''
test_acPostProcForCreate__3024 {{
    msiDataObjOpen("{target_obj}",*FD);
    msiDataObjClose(*FD,*Status);
}}
INPUT null
OUTPUT ruleExecOut
'''.format(**locals())

        with open(rule_file, 'w') as f:
            f.write(rule_string)

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # iput test file
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # invoke irule to trigger PEP
            sesh.assert_icommand('irule -F {rule_file}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {target_obj}'.format(**locals()))
        os.unlink(rule_file)
        os.unlink(test_re)


    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForOpen__3024(self):
        test_re = os.path.join(lib.get_core_re_dir(), 'test.re')
        server_config_filename = lib.get_irods_config_dir() + '/server_config.json'

        # get PEP name from function name
        pep_name = inspect.stack()[0][3].split('_')[1]

        # user session
        sesh = self.user0
        testfile = self.testfile
        target_obj = os.path.join(sesh.session_collection, testfile)

        # prepare rule file
        rule_file = "test_rule_file.r"
        rule_string = '''
test_acPostProcForCreate__3024 {{
    msiDataObjOpen("{target_obj}",*FD);
    msiDataObjClose(*FD,*Status);
}}
INPUT null
OUTPUT ruleExecOut
'''.format(**locals())

        with open(rule_file, 'w') as f:
            f.write(rule_string)

        # query for resource properties
        columns = ('RESC_ZONE_NAME, '
                   'RESC_FREE_SPACE, '
                   'RESC_STATUS, '
                   'RESC_ID, '
                   'RESC_NAME, '
                   'RESC_TYPE_NAME, '
                   'RESC_LOC, '
                   'RESC_CLASS_NAME, '
                   'RESC_VAULT_PATH, '
                   'RESC_INFO, '
                   'RESC_COMMENT, '
                   'RESC_CREATE_TIME, '
                   'RESC_MODIFY_TIME')
        resource = sesh.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = sesh.run_icommand(query)[1]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            rule_body = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            rule_body += ('writeLine("serverLog", $KVPairs.zoneName);'
                          'writeLine("serverLog", $KVPairs.freeSpace);'
                          'writeLine("serverLog", $KVPairs.quotaLimit);'
                          'writeLine("serverLog", $KVPairs.rescStatus);'
                          'writeLine("serverLog", $KVPairs.rescId);'
                          'writeLine("serverLog", $KVPairs.rescName);'
                          'writeLine("serverLog", $KVPairs.rescType);'
                          'writeLine("serverLog", $KVPairs.rescLoc);'
                          'writeLine("serverLog", $KVPairs.rescClass);'
                          'writeLine("serverLog", $KVPairs.rescVaultPath);'
                          'writeLine("serverLog", $KVPairs.rescInfo);'
                          'writeLine("serverLog", $KVPairs.rescComments);'
                          'writeLine("serverLog", $KVPairs.rescCreate);'
                          'writeLine("serverLog", $KVPairs.rescModify);')
            test_rule = '{pep_name} {{ {rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            # iput test file
            sesh.assert_icommand('iput -f {testfile}'.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_log_size('server')

            # invoke irule to trigger PEP
            sesh.assert_icommand('irule -F {rule_file}'.format(**locals()))

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log('server', pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log('server', property, start_index=initial_log_size)

        # cleanup
        sesh.run_icommand('irm -f {target_obj}'.format(**locals()))
        os.unlink(rule_file)
        os.unlink(test_re)

    def test_genquery_foreach_MAX_SQL_ROWS_multiple__3489(self):
        MAX_SQL_ROWS = 256 # needs to be the same as constant in server code
        filename = 'test_genquery_foreach_MAX_SQL_ROWS_multiple__3489_dummy_file'
        lib.make_file(filename, 1)
        data_object_prefix = 'loiuaxnlaskdfpiewrnsliuserd'
        for i in range(MAX_SQL_ROWS):
            self.admin.assert_icommand(['iput', filename, '{0}_file_{1}'.format(data_object_prefix, i)])

        rule_file = 'test_genquery_foreach_MAX_SQL_ROWS_multiple__3489.r'
        rule_string = '''
test_genquery_foreach_MAX_SQL_ROWS_multiple__3489 {{
    foreach(*rows in select DATA_ID where DATA_NAME like '{0}%') {{
        *id = *rows.DATA_ID;
        writeLine("serverLog", "GGGGGGGGGGGGGGG *id");
    }}
}}
INPUT null
OUTPUT ruleExecOut
'''.format(data_object_prefix)

        with open(rule_file, 'w') as f:
            f.write(rule_string)

        self.admin.assert_icommand(['irule', '-F', rule_file])
        os.unlink(rule_file)
