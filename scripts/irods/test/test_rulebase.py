from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import socket
import time  # remove once file hash fix is commited #2279
import copy
import inspect

from .. import lib
from .. import test
from . import settings
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController


class Test_Rulebase(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Rulebase, self).setUp()

        # make large test file
        self.large_file = '/tmp/largefile'
        lib.make_file(self.large_file, '64M')

    def tearDown(self):
        # remove large test file
        os.unlink(self.large_file)

        super(Test_Rulebase, self).tearDown()

    def test_client_server_negotiation__2564(self):
        corefile = IrodsConfig().core_re_directory + "/core.re"
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
        with open(rule_file, 'wt') as f:
            print(rule_string, file=f, end='')

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

        corefile = os.path.join(IrodsConfig().core_re_directory, 'core.re')
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
        corefile = os.path.join(IrodsConfig().core_re_directory, "core.re")
        origcorefile = os.path.join(IrodsConfig().core_re_directory, "core.re.orig")
        os.system("cp " + corefile + " " + origcorefile)

        # add dynamic PEP with rscomm usage
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system('''echo "pep_resource_open_pre(*OUT,*FOO,*BAR) { msiGetSystemTime( *junk, '' ); }" >> ''' + corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

        # check rei functioning
        self.admin.assert_icommand("iget " + self.testfile + " - ", 'STDOUT_SINGLELINE', self.testfile)

        # restore core.re
        time.sleep(1)  # remove once file hash fix is commited #2279
        os.system("cp " + origcorefile + " " + corefile)
        time.sleep(1)  # remove once file hash fix is commited #2279

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rulebase_update__2585(self):
        my_rule = """
my_rule {
    delay("<PLUSET>1s</PLUSET>") {
        do_some_stuff();
    }
}
INPUT null
OUTPUT ruleExecOut
"""
        rule_file = 'my_rule.r'
        with open(rule_file, 'wt') as f:
            print(my_rule, file=f, end='')

        irods_config = IrodsConfig()
        with lib.file_backed_up(irods_config.server_config_path):
            test_re = os.path.join(irods_config.core_re_directory, 'test.re')
            # write new rule file to config dir
            with open(test_re, 'wt') as f:
                print('do_some_stuff() { writeLine( "serverLog", "TEST_STRING_TO_FIND_1_2585" ); }', file=f, end='')

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(irods_config.server_config_path, server_config_update)
            IrodsController().restart()
            time.sleep(35)  # wait for delay rule engine to wake

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(irods_config.re_log_path)
            self.admin.assert_icommand('irule -F ' + rule_file)
            time.sleep(35)  # wait for test to fire
            assert lib.count_occurrences_of_string_in_log(irods_config.re_log_path, 'TEST_STRING_TO_FIND_1_2585', start_index=initial_log_size)

            # repave rule with new string
            os.unlink(test_re)
            with open(test_re, 'wt') as f:
                print('do_some_stuff() { writeLine( "serverLog", "TEST_STRING_TO_FIND_2_2585" ); }', file=f, end='')
            time.sleep(35)  # wait for delay rule engine to wake

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(irods_config.re_log_path)
            self.admin.assert_icommand('irule -F ' + rule_file)
            time.sleep(35)  # wait for test to fire
            assert lib.count_occurrences_of_string_in_log(irods_config.re_log_path, 'TEST_STRING_TO_FIND_2_2585', start_index=initial_log_size)

        # cleanup
        os.unlink(test_re)
        os.unlink(rule_file)


    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForPut__3024(self):
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], commands=['iput -f {testfile}'])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acSetNumThreads__3024(self):
        rule_body = 'msiSetNumThreads("default","0","default");'
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], commands=['iput -f {large_file}'], rule_body=rule_body)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acDataDeletePolicy__3024(self):
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], precommands=['iput -f {testfile}'], commands=['irm -f {testfile}'])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForDelete__3024(self):
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], precommands=['iput -f {testfile}'], commands=['irm -f {testfile}'])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acSetChkFilePathPerm__3024(self):
        irods_config = IrodsConfig()
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], commands=[('ireg %s {target_obj}' % irods_config.version_path, 'STDERR_SINGLELINE', 'PATH_REG_NOT_ALLOWED')], target_name=os.path.basename(irods_config.version_path))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForFilePathReg__3024(self):
        reg_file_path = os.path.join(self.user0.get_vault_session_path(), 'reg_test_file')
        lib.make_dir_p(os.path.dirname(reg_file_path))
        lib.touch(reg_file_path)
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], commands=['ireg %s {target_obj}' % reg_file_path], target_name=os.path.basename(reg_file_path), user_session=self.admin)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForCopy__3024(self):
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], precommands=['iput -f {testfile}'], commands=['icp {testfile} {testfile}_copy'])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acSetVaultPathPolicy__3024(self):
        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], commands=['iput -f {testfile}'])

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPreprocForDataObjOpen__3024(self):
        rule_string = '''
test_{pep_name}__3024 {{
    msiDataObjOpen("{target_obj}",*FD);
    msiDataObjClose(*FD,*Status);
}}
INPUT null
OUTPUT ruleExecOut
'''

        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], precommands=['iput -f {testfile}'], commands=['irule -F {rule_file}'], rule_string=rule_string)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
    def test_acPostProcForOpen__3024(self):
        # prepare rule file
        rule_string = '''
test_{pep_name}__3024 {{
    msiDataObjOpen("{target_obj}",*FD);
    msiDataObjClose(*FD,*Status);
}}
INPUT null
OUTPUT ruleExecOut
'''

        self.pep_test_helper__3024(inspect.stack()[0][3].split('_')[1], precommands=['iput -f {testfile}'], commands=['irule -F {rule_file}'], rule_string=rule_string)

    def pep_test_helper__3024(self, pep_name, precommands=[], commands=[], rule_body='', rule_string=None, target_name=None, user_session=None):
        irods_config = IrodsConfig()
        test_re = os.path.join(irods_config.core_re_directory, 'test.re')
        server_config_filename = irods_config.server_config_path

        # user session
        if user_session is None:
            user_session = self.user0
        testfile = self.testfile
        large_file = self.large_file
        target_name = target_name if target_name is not None else testfile
        target_obj = '/'.join([user_session.session_collection, target_name])

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
        resource = user_session.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = user_session.run_icommand(query)[0]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        # make sure property list is not empty
        self.assertTrue(len(resource_property_list))

        with lib.file_backed_up(server_config_filename):
            # prepare rule
            # rule will write PEP name as well as
            # resource related rule session vars to server log
            write_statements = 'writeLine("serverLog", "{pep_name}");'.format(**locals())
            write_statements += ('writeLine("serverLog", $KVPairs.zoneName);'
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
                          'writeLine("serverLog", $KVPairs.rescModify)')
            test_rule = '{pep_name} {{ {write_statements};{rule_body} }}'.format(**locals())

            # write new rule file
            with open(test_re, 'w') as f:
                f.write(test_rule)

            # update server config with additional rule file
            server_config_update = {
                "re_rulebase_set": [{"filename": "test"}, {"filename": "core"}]
            }
            lib.update_json_file_from_dict(server_config_filename, server_config_update)

            if rule_string is not None:
                rule_file = "test_rule_file.r"
                with open(rule_file, 'w') as f:
                    f.write(rule_string.format(**locals()))

            # perform precommands
            for c in precommands:
                if isinstance(c, tuple):
                    user_session.assert_icommand(c[0].format(**locals()), c[1], c[2])
                else:
                    user_session.assert_icommand(c.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(irods_config.server_log_path)

            # perform commands to hit PEP
            for c in commands:
                if isinstance(c, tuple):
                    user_session.assert_icommand(c[0].format(**locals()), c[1], c[2])
                else:
                    user_session.assert_icommand(c.format(**locals()))

            if rule_string is not None:
                os.unlink(rule_file)

            # confirm that PEP was hit by looking for pep name in server log
            assert lib.count_occurrences_of_string_in_log(irods_config.server_log_path, pep_name, start_index=initial_log_size)

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        assert lib.count_occurrences_of_string_in_log(irods_config.server_log_path, property, start_index=initial_log_size)

        # cleanup
        user_session.run_icommand('irm -f {target_obj}'.format(**locals()))
        os.unlink(test_re)
