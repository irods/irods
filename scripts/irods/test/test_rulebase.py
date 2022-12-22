from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import copy
import json
import os
import socket
import tempfile
import time
import subprocess
import textwrap

from .. import lib
from .. import paths
from .. import test
from . import session
from . import settings
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController
from ..core_file import temporary_core_file

class Test_Rulebase(ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Rulebase'

    def setUp(self):
        super(Test_Rulebase, self).setUp()

    def tearDown(self):
        super(Test_Rulebase, self).tearDown()

    def test_client_server_negotiation__2564(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                acPreConnect(*OUT) {
                    *OUT = 'CS_NEG_REQUIRE';
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def acPreConnect(rule_args, callback, rei):
                    rule_args[0] = 'CS_NEG_REQUIRE'
            ''')
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])

            client_update = {
                'irods_client_server_policy': 'CS_NEG_REFUSE'
            }

            session_env_backup = copy.deepcopy(self.admin.environment_file_contents)
            self.admin.environment_file_contents.update(client_update)

            self.admin.assert_icommand( 'ils','STDERR_SINGLELINE','CLIENT_NEGOTIATION_ERROR')

            self.admin.environment_file_contents = session_env_backup

    def test_msiDataObjWrite__2795(self):
        filename = 'test_file.txt'
        logical_path = os.path.join(self.admin.session_collection, filename)

        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                test_msiDataObjWrite__2795 {{
                    ### write a string to a file in irods
                    msiDataObjCreate("*TEST_ROOT" ++ "/" ++ "{filename}","null",*FD);
                    msiDataObjWrite(*FD,"this_is_a_test_string",*LEN);
                    msiDataObjClose(*FD,*Status);
                }}
                INPUT *TEST_ROOT="{self.admin.session_collection}"
                OUTPUT ruleExecOut
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent(f'''
                def main(rule_args, callback, rei):
                    out_dict = callback.msiDataObjCreate(global_vars['*TEST_ROOT'][1:-1] + '/' + '{filename}', 'null', 0)
                    file_desc = out_dict['arguments'][2]

                    out_dict = callback.msiDataObjWrite(file_desc, 'this_is_a_test_string', 0)

                    out_dict = callback.msiDataObjClose(file_desc, 0)

                INPUT *TEST_ROOT="{self.admin.session_collection}"
                OUTPUT ruleExecOut
            ''')
        }

        rule_file = "test_rule_file.r"
        rule_string = rule_map[self.plugin_name]
        with open(rule_file, 'wt') as f:
            print(rule_string, file=f, end='')

        self.admin.assert_icommand(['irule', '-r', self.plugin_name + '-instance', '-F', rule_file])
        self.admin.assert_icommand('ils -l','STDOUT_SINGLELINE', filename)
        self.admin.assert_icommand('iget -f '+ logical_path)

        with open(filename, 'r') as f:
            file_contents = f.read()

        self.assertFalse(file_contents.endswith('\0'))

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Skip for non-rule-language REP')
    def test_irods_re_infinite_recursion_3169(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                call_with_wrong_number_of_string_arguments(*A, *B, *C) {
                }

                acPostProcForPut {
                    call_with_wrong_number_of_string_arguments("a", "b");
                }

                acPostProcForPut { }
            ''')
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])

            test_file = 'rulebasetestfile'
            lib.touch(test_file)
            self.admin.assert_icommand(['iput', test_file])

    def test_acPostProcForPut_replicate_to_multiple_resources(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                # multiple replication rule
                replicateMultiple(*destRgStr) {
                    *destRgList = split(*destRgStr, ',');
                    writeLine("serverLog", " acPostProcForPut multiple replicate $objPath $filePath -> *destRgStr");
                    foreach (*destRg in *destRgList) {
                        writeLine("serverLog", " acPostProcForPut replicate $objPath $filePath -> *destRg");
                        *err = errormsg(msiDataObjRepl($objPath,"destRescName=*destRg++++irodsAdmin=",*Status), *msg );
                        if( 0 != *err ) {
                            if(*err == -808000) {
                                writeLine("serverLog", "$objPath cannot be found");
                                $status = 0;
                                succeed;
                            } else {
                                fail(*err);
                            }
                        }
                    }
                }
                acPostProcForPut {
                    replicateMultiple( "r1,r2" )
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                # multiple replication rule
                def replicateMultiple(dest_list, callback, rei):
                    obj_path = str(rei.doi.objPath if rei.doi is not None else rei.doinp.objPath)
                    filepath = str(rei.doi.filePath)
                    callback.writeLine('serverLog', ' acPostProcForPut multiple replicate ' + obj_path + ' ' + filepath + ' -> ' + str(dest_list))
                    for dest in dest_list:
                        callback.writeLine('serverLog', 'acPostProcForPut replicate ' + obj_path + ' ' + filepath + ' -> ' + dest)
                        out_dict = callback.msiDataObjRepl(obj_path,"destRescName=" + dest + "++++irodsAdmin=", 0);
                        if not out_dict['code'] == 0:
                            if out_dict['code'] == -808000:
                                callback.writeLine('serverLog', obj_path + ' cannot be found')
                                return 0
                            else:
                                callback.writeLine('serverLog', 'ERROR: ' + out_dict['code'])
                                return int(out_dict['code'])
                def acPostProcForPut(rule_args, callback, rei):
                    replicateMultiple(["r1","r2"], callback, rei)
            '''),
        }

        # create new resources
        hostname = socket.gethostname()
        self.admin.assert_icommand("iadmin mkresc r1 unixfilesystem " + hostname + ":/tmp/irods/r1", 'STDOUT_SINGLELINE', "Creating")
        self.admin.assert_icommand("iadmin mkresc r2 unixfilesystem " + hostname + ":/tmp/irods/r2", 'STDOUT_SINGLELINE', "Creating")
        tfile = "rulebasetestfile"
        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])

                # put data
                lib.touch(tfile)

                # Note that this test was failing on PREP in iRODS 4.3.0, due to an inconsistency in how the iput client outputs diagnostics.
                # The failing was due to this message being output on STDOUT during the iput:
                #
                #    Level 0: selected destination hierarchy [r1] is not stale; replication is not allowed.
                #    Level 1: DEBUG: msiDataObjRepl: rsDataObjRepl failed /tempZone/home/otherrods/bbbbbb, status = -169000
                #
                #    Level 2: selected destination hierarchy [r1] is not stale; replication is not allowed.
                #    Level 3: DEBUG: msiDataObjRepl: rsDataObjRepl failed /tempZone/home/otherrods/bbbbbb, status = -169000
                #
                # Presumed Explanation: acPostProcForPut fires twice during the iput, once on the PUT itself and once on the msiDataObjRepl invocation,
                # the 2nd repl attempt causes a SYS_NOT_ALLOWED (as we can no longer repl to a non-stale hierarchy.) So it failed under PREP, due to
                # the outputted diagnostic's and its stated reasons. NREP passed because no such STDOUT eruptions occur, though the error itself does.

                iput_command = ['iput', tfile]
                if self.plugin_name == 'irods_rule_engine_plugin-irods_rule_language':
                    self.admin.assert_icommand(iput_command)
                else:
                    self.admin.run_icommand(iput_command) # prepend "stdout, stderr, rc = " to see the stdout message for PREP testing on 4.3.0

                # check replicas
                self.admin.assert_icommand(['ils', '-L', tfile], 'STDOUT_MULTILINE', [' demoResc ', ' r1 ', ' r2 '])


        finally:
            # clean up and remove new resources
            self.admin.run_icommand("irm -rf " + tfile)
            self.admin.assert_icommand("iadmin rmresc r1")
            self.admin.assert_icommand("iadmin rmresc r2")

    def test_dynamic_pep_with_rscomm_usage(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_resource_open_pre(*OUT, *FOO, *BAR) {
                    msiGetSystemTime( *junk, '' );
                }
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def pep_resource_open_pre(rule_args, callback, rei):
                    retStr = ''
                    callback.msiGetSystemTime( retStr, '' )
            '''),
        }

        with temporary_core_file() as core:
            core.add_rule(pep_map[self.plugin_name])

            # check rei functioning
            self.admin.assert_icommand("iget " + self.testfile + " - ", 'STDOUT_SINGLELINE', self.testfile)


    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'tests cache update - only applicable for irods_rule_language REP')
    def test_rulebase_update__2585(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': [
                textwrap.dedent('''
                    my_rule {
                        delay("<PLUSET>1s</PLUSET>") {
                            do_some_stuff();
                        }
                    }
                    INPUT null
                    OUTPUT ruleExecOut
                '''),
                textwrap.dedent('''
                    do_some_stuff() {
                        writeLine("serverLog", "TEST_STRING_TO_FIND_1_2585");
                    }
                '''),
                textwrap.dedent('''
                    do_some_stuff() {
                        writeLine("serverLog", "TEST_STRING_TO_FIND_2_2585");
                    }
                ''')
            ]
        }

        rules = rule_map[self.plugin_name]

        my_rule = rules[0]
        rule_file = 'my_rule.r'
        with open(rule_file, 'wt') as f:
            print(my_rule, file=f, end='')

        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # inject a new rule base into the native rule engine
        svr_cfg['plugin_configuration']['rule_engines'][0]['plugin_specific_configuration']['re_rulebase_set'] = ["test", "core"]

        # dump to a string to repave the existing server_config.json
        new_server_config=json.dumps(svr_cfg, sort_keys=True,indent=4, separators=(',', ': '))

        with lib.file_backed_up(paths.server_config_path()):
            test_re = os.path.join(paths.core_re_directory(), 'test.re')
            # write new rule file to config dir
            with open(test_re, 'wt') as f:
                print(rules[1], file=f, end='')

            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            IrodsController().restart()
            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand('irule -F ' + rule_file)

            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'TEST_STRING_TO_FIND_1_2585', start_index=initial_log_size))

            # repave rule with new string
            os.unlink(test_re)
            with open(test_re, 'wt') as f:
                print(rules[2], file=f, end='')

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand('irule -F ' + rule_file)
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'TEST_STRING_TO_FIND_2_2585', start_index=initial_log_size))

        # cleanup
        IrodsController().restart()
        os.unlink(test_re)
        os.unlink(rule_file)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'tests cache update - only applicable for irods_rule_language REP')
    def test_update_core_multiple_agents__3184(self):
        with temporary_core_file() as core:
            for l in range(100):
                core.add_rule("multiple_agents {}")

                processes = []
                initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
                for i in range(100):
                    processes.append(subprocess.Popen(["ils"]))
                for p in processes:
                    p.wait()
                lib.delayAssert(
                    lambda: lib.log_message_occurrences_equals_count(
                        msg='stack trace',
                        count=0,
                        start_index=initial_log_size))

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'tests cache update - only applicable for irods_rule_language REP')
    def test_fast_updates__2279(self):
            fastswap_test_script = os.path.join('/var', 'lib', 'irods', 'scripts', 'rulebase_fastswap_test_2276.sh')
            rc, _, _ = self.admin.assert_icommand(['bash', fastswap_test_script], 'STDOUT_SINGLELINE', 'etc')
            assert rc == 0

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'tests cache update - only applicable for irods_rule_language REP')
    def test_rulebase_update_without_delay(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': [
                textwrap.dedent('''
                    my_rule {
                        do_some_stuff();
                    }
                    INPUT null
                    OUTPUT ruleExecOut
                '''),
                textwrap.dedent('''
                    do_some_stuff() {
                        writeLine("serverLog", "TEST_STRING_TO_FIND_1_NODELAY");
                    }
                '''),
                textwrap.dedent('''
                    do_some_stuff() {
                        writeLine("serverLog", "TEST_STRING_TO_FIND_2_NODELAY");
                    }
                ''')
            ]
        }

        rules = rule_map[self.plugin_name]

        my_rule = rules[0]
        rule_file = 'my_rule.r'
        with open(rule_file, 'wt') as f:
            print(my_rule, file=f, end='')

        server_config_filename = paths.server_config_path()

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # inject a new rule base into the native rule engine
        svr_cfg['plugin_configuration']['rule_engines'][0]['plugin_specific_configuration']['re_rulebase_set'] = ["test", "core"]

        # dump to a string to repave the existing server_config.json
        new_server_config=json.dumps(svr_cfg, sort_keys=True,indent=4, separators=(',', ': '))

        with lib.file_backed_up(paths.server_config_path()):
            test_re = os.path.join(paths.core_re_directory(), 'test.re')
            # write new rule file to config dir
            with open(test_re, 'wt') as f:
                print(rules[1], file=f, end='')

            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand('irule -F ' + rule_file)
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'TEST_STRING_TO_FIND_1_NODELAY', start_index=initial_log_size))

            time.sleep(5) # ensure modify time is sufficiently different

            # repave rule with new string
            os.unlink(test_re)
            with open(test_re, 'wt') as f:
                print(rules[2], file=f, end='')

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand('irule -F ' + rule_file)
            #time.sleep(35)  # wait for test to fire
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'TEST_STRING_TO_FIND_2_NODELAY', start_index=initial_log_size))

        # cleanup
        os.unlink(test_re)
        os.unlink(rule_file)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Python REP does not guarantee argument preservation')
    def test_argument_preservation__3236(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_msiDataObjWrite__3236 {
                    msiTakeThreeArgumentsAndDoNothing(*arg1, *arg2, *arg3);
                    writeLine("stdout", "AFTER arg1=*arg1 arg2=*arg2 arg3=*arg3");
                }
                INPUT *arg1="abc", *arg2="def", *arg3="ghi"
                OUTPUT ruleExecOut
            ''')
        }

        with tempfile.NamedTemporaryFile(suffix='.r', mode='w+t') as f:
            rule_string = rule_map[self.plugin_name]
            f.write(rule_string)
            f.flush()

            self.admin.assert_icommand('irule -F ' + f.name, 'STDOUT_SINGLELINE', 'AFTER arg1=abc arg2=def arg3=ghi')

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Skip for non-rule-language REP')
    def test_rulebase_update_sixty_four_chars__3560(self):
        irods_config = IrodsConfig()

        server_config_filename = irods_config.server_config_path

        # load server_config.json to inject a new rule base
        with open(server_config_filename) as f:
            svr_cfg = json.load(f)

        # inject several new rules into the native rule engine
        svr_cfg['plugin_configuration']['rule_engines'][0]['plugin_specific_configuration']['re_rulebase_set'] = [
            "rulefile1", "rulefile2", "rulefile3", "rulefile4", "rulefile5", "rulefile6", "rulefile7","rulefile8", "core"]

        # dump to a string to repave the existing server_config.json
        new_server_config = json.dumps(svr_cfg, sort_keys=True, indent=4, separators=(',', ': '))

        with lib.file_backed_up(irods_config.server_config_path):
            rulefile1 = os.path.join(irods_config.core_re_directory, 'rulefile1.re')
            rulefile2 = os.path.join(irods_config.core_re_directory, 'rulefile2.re')
            rulefile3 = os.path.join(irods_config.core_re_directory, 'rulefile3.re')
            rulefile4 = os.path.join(irods_config.core_re_directory, 'rulefile4.re')
            rulefile5 = os.path.join(irods_config.core_re_directory, 'rulefile5.re')
            rulefile6 = os.path.join(irods_config.core_re_directory, 'rulefile6.re')
            rulefile7 = os.path.join(irods_config.core_re_directory, 'rulefile7.re')
            rulefile8 = os.path.join(irods_config.core_re_directory, 'rulefile8.re')

            # write new rule files to config dir
            with open(rulefile1, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile2, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile3, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile4, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile5, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile6, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile7, 'wt') as f:
                print('dummyRule{ }', file=f, end='')

            with open(rulefile8, 'wt') as f:
                print('acPostProcForPut{ writeLine( "serverLog", "TEST_STRING_TO_FIND_DEFECT_3560" ); }', file=f, end='')

            # repave the existing server_config.json
            with open(server_config_filename, 'w') as f:
                f.write(new_server_config)

            IrodsController().restart()
            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(irods_config.server_log_path)

            filename = "defect3560.txt"
            filepath = lib.create_local_testfile(filename)
            put_resource = self.testresc
            self.user0.assert_icommand("iput -R {put_resource} {filename}".format(**locals()), "EMPTY")

            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(irods_config.server_log_path,
                                                          "TEST_STRING_TO_FIND_DEFECT_3560", start_index=initial_log_size))

        # cleanup
        IrodsController().restart()
        os.unlink(rulefile1)
        os.unlink(rulefile2)
        os.unlink(rulefile3)
        os.unlink(rulefile4)
        os.unlink(rulefile5)
        os.unlink(rulefile6)
        os.unlink(rulefile7)
        os.unlink(rulefile8)
 
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_acPreProcForExecCmd__3867(self):
        with temporary_core_file() as core:
            core.add_rule('acPreProcForExecCmd(*cmd, *args, *addr, *hint){ writeLine("serverLog", "TEST_MARKER_test_acPreProcForExecCmd__3867")}')

            rule_file = 'test_acPreProcForExecCmd__3867.r'
            rule_string = '''
test_acPreProcForExecCmd__3867_rule {
    msiExecCmd('hello', '', '', '', '', *out)
}

INPUT null
OUTPUT ruleExecOut
'''
            with open(rule_file, 'w') as f:
                f.write(rule_string)

            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand('irule -F ' + rule_file)
            os.unlink(rule_file)

            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg='TEST_MARKER_test_acPreProcForExecCmd__3867',
                    start_index=initial_log_size))

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    def test_create_close__issue_5018(self):
        logical_path = os.path.join(self.admin.session_collection, 'test_create_close__issue_5018')

        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(f'''
                test_create_close__issue_5018 {{
                    msiDataObjCreate("{logical_path}", "destRescName=demoResc", *fd)
                    msiDataObjClose(*fd, *status)
                    writeLine("stdout", "created [{logical_path}]");
                }}
                INPUT null
                OUTPUT ruleExecOut
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent(f'''
                def main(rule_args, callback, rei):
                    out_dict = callback.msiDataObjCreate('{logical_path}', 'destRescName=demoResc', 0)
                    fd = out_dict['arguments'][2]
                    out_dict = callback.msiDataObjClose(fd, 0)
                    callback.writeLine('stdout', 'created [{logical_path}]');

                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_file = os.path.join(self.admin.local_session_dir, 'test_create_close__issue_5018.r')
        rule_string = rule_map[self.plugin_name]
        with open(rule_file, 'w') as f:
            f.write(rule_string)

        self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT', 'created [{}]'.format(logical_path))
        os.unlink(rule_file)

        self.admin.assert_icommand(['iadmin', 'ls', 'logical_path', logical_path, 'replica_number', '0'], 'STDOUT', 'DATA_REPL_STATUS: 1')


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiExecCmd_closeAllL1Desc__issue_6623(self):
        logical_path = os.path.join(self.admin.session_collection, 'foo')
        attr = 'test_msiExecCmd_closeAllL1Desc__issue_6623'

        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(
                f'''
                pep_resource_close_post(*inst, *ctx, *out) {{
                    msiGetSystemTime(*time_str, '');
                    msiAddKeyVal(*key_val_pair,"{attr}","*time_str");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
                    # Sleep to ensure that the actual close doesn't occur in the same second
                    msiSleep("1", "0");
                }}''')
        }

        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(
                f'''
                execute_hello {{
                    msiDataObjOpen("{logical_path}", *fd);
                    msiExecCmd('hello', '', '', '', '', *out);
                    msiDataObjClose(*fd, *unused_status);
                }}
                INPUT null
                OUTPUT ruleExecOut''')
        }

        self.assertFalse(lib.replica_exists(self.admin, logical_path, 0))
        try:
            with temporary_core_file() as core:
                self.admin.assert_icommand(['itouch', logical_path])
                self.assertTrue(lib.replica_exists(self.admin, logical_path, 0))

                # Add a PEP which fires after the "close" resource operation. The PEP adds an AVU to the object at logical_path.
                core.add_rule(pep_map[self.plugin_name])

                # Create an iRODS rule which opens a data object at logical_path, calls msiExecCmd, and then closes the data object.
                rule_file = os.path.join(self.admin.local_session_dir, 'rule.r')
                with open(rule_file, 'w') as f:
                    f.write(rule_map[self.plugin_name])

                self.admin.assert_icommand(['irule', '-r', self.plugin_name + '-instance', '-F', rule_file])

                # msiExecCmd should not execute any policy. Since the rule calls msiDataObjClose only once, there should be only one
                # AVU associated with the data object. If this were not the case, there would be two: once for the msiDataObjClose
                # call, and once for the close resource operation invoked by the execCmd API.
                expected_count = 1
                q = f"select COUNT(META_DATA_ATTR_ID) where META_DATA_ATTR_NAME = '{attr}'"
                self.admin.assert_icommand(['iquest', '%s', q], 'STDOUT', str(expected_count))

        finally:
            self.admin.run_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rum'])


    def test_rsDataObjRepl_populates_input_struct__issue_6100(self):
        resc1 = 'resc1'
        resc2 = 'resc2'

        filename = 'test_rsDataObjRepl_populates_input_struct__issue_6100'
        logical_path = os.path.join(self.user0.session_collection, filename)

        metadata_attr = 'keyword_from_repl'
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent(
                f'''pep_api_data_obj_repl_post(*inst, *comm, *inp, *stat) {{
                    msiGetValByKey(*inp, "resc_hier", *src);
                    msiAddKeyVal(*src_kvp,"{metadata_attr}_src", "*src");
                    msiAssociateKeyValuePairsToObj(*src_kvp, "{logical_path}", "-d");

                    msiGetValByKey(*inp, "dest_resc_hier", *dest);
                    msiAddKeyVal(*dest_kvp,"{metadata_attr}_dest", "*dest");
                    msiAssociateKeyValuePairsToObj(*dest_kvp, "{logical_path}", "-d");
                }}
            ''')
        }

        try:
            with temporary_core_file() as core:
                core.add_rule(pep_map[self.plugin_name])

                lib.create_ufs_resource(resc1, self.admin, hostname=test.settings.HOSTNAME_2)
                lib.create_ufs_resource(resc2, self.admin, hostname=test.settings.HOSTNAME_3)

                self.user0.assert_icommand(['itouch', '-R', resc1, logical_path])
                self.user0.assert_icommand(['irepl', '-R', resc2, logical_path])

                self.assertTrue(lib.metadata_attr_with_value_exists(self.admin, f'{metadata_attr}_src', resc1))
                self.assertTrue(lib.metadata_attr_with_value_exists(self.admin, f'{metadata_attr}_dest', resc2))

        finally:
            self.user0.assert_icommand(['irm', '-f', logical_path])
            lib.remove_resource(resc2, self.admin)
            lib.remove_resource(resc1, self.admin)
            self.admin.assert_icommand(['iadmin', 'rum'])


@unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads rods server log')
class Test_Resource_Session_Vars__3024(ResourceBase, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Resource_Session_Vars__3024'

    def setUp(self):
        super(Test_Resource_Session_Vars__3024, self).setUp()

        # get PEP name
        self.pep_name = self._testMethodName.split('_')[1]

        # make large test file
        self.large_file = '/tmp/largefile'
        lib.make_file(self.large_file, '64M', 'arbitrary')

    def tearDown(self):
        del self.pep_name

        # remove large test file
        os.unlink(self.large_file)

        super(Test_Resource_Session_Vars__3024, self).tearDown()

    def test_acPostProcForPut(self):
        self.pep_test_helper(commands=['iput -f {testfile}'])

    def test_acSetNumThreads(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': 'msiSetNumThreads("default", "0", "default");',
            'irods_rule_engine_plugin-python': "    callback.msiSetNumThreads('default', '0', 'default')"
        }
        self.pep_test_helper(commands=['iput -f {large_file}'], rule_body=rule_map[self.plugin_name])

    def test_acDataDeletePolicy(self):
        self.pep_test_helper(precommands=['iput -f {testfile}'], commands=['irm -f {testfile}'])

    def test_acPostProcForDelete(self):
        self.pep_test_helper(precommands=['iput -f {testfile}'], commands=['irm -f {testfile}'])

    def test_acSetChkFilePathPerm(self):
        # regular user will try to register a system file
        # e.g: /var/lib/irods/version.json
        path_to_register = paths.version_path()
        commands = [('ireg {path_to_register} {{target_obj}}'.format(**locals()), 'STDERR_SINGLELINE', 'PATH_REG_NOT_ALLOWED')]
        self.pep_test_helper(commands=commands, target_name=os.path.basename(path_to_register))

    def test_acPostProcForFilePathReg(self):
        # admin user will register a file
        sess=self.admin

        # make new physical file in user's vault
        reg_file_path = os.path.join(sess.get_vault_session_path(), 'reg_test_file')
        lib.make_dir_p(os.path.dirname(reg_file_path))
        lib.touch(reg_file_path)

        commands = ['ireg {reg_file_path} {{target_obj}}'.format(**locals())]
        self.pep_test_helper(commands=commands, target_name=os.path.basename(reg_file_path), user_session=sess)

    def test_acPostProcForCopy(self):
        self.pep_test_helper(precommands=['iput -f {testfile}'], commands=['icp {testfile} {testfile}_copy'])

    def test_acSetVaultPathPolicy(self):
        self.pep_test_helper(commands=['iput -f {testfile}'])

    def test_acPreprocForDataObjOpen(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_{pep_name} {{
                    msiDataObjOpen("{target_obj}", *FD);
                    msiDataObjClose(*FD, *Status);
                }}
                INPUT null
                OUTPUT ruleExecOut
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def main(rule_args, callback, rei):
                    out_dict = callback.msiDataObjOpen("{target_obj}", 0)
                    file_desc = out_dict['arguments'][1]

                    out_dict = callback.msiDataObjClose(file_desc, 0)

                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        self.pep_test_helper(precommands=['iput -f {testfile}'], commands=['irule -F {client_rule_file}'], client_rule=rule_map[self.plugin_name])

    def test_acPostProcForOpen(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_{pep_name} {{
                    msiDataObjOpen("{target_obj}", *FD);
                    msiDataObjClose(*FD, *Status);
                }}
                INPUT null
                OUTPUT ruleExecOut
            '''),
            'irods_rule_engine_plugin-python': textwrap.dedent('''
                def main(rule_args, callback, rei):
                    out_dict = callback.msiDataObjOpen("{target_obj}", 0)
                    file_desc = out_dict['arguments'][1]

                    out_dict = callback.msiDataObjClose(file_desc, 0)

                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        self.pep_test_helper(precommands=['iput -f {testfile}'], commands=['irule -F {client_rule_file}'], client_rule=rule_map[self.plugin_name])

    def get_resource_property_list(self, session):
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
        resource = session.default_resource
        query = '''iquest "SELECT {columns} WHERE RESC_NAME ='{resource}'"'''.format(**locals())
        result = session.run_icommand(query)[0]

        # last line is iquest default formatting separator
        resource_property_list = result.splitlines()[:-1]

        # make sure property list is not empty
        self.assertTrue(len(resource_property_list))

        return resource_property_list

    def make_pep_rule(self, pep_name, rule_body):
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

        return '{pep_name} {{ {write_statements};{rule_body} }}'.format(**locals())

       
    def make_pep_rule_python(self, pep_name, rule_body):
        # prepare rule
        # rule will write PEP name as well as
        # resource related rule session vars to server log
        write_statements = '    callback.writeLine("serverLog", "{pep_name}")\n'.format(**locals())
        write_statements += ('    callback.writeLine("serverLog", rei.condInputData["zoneName"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["freeSpace"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["quotaLimit"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescStatus"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescId"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescName"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescType"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescLoc"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescClass"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescVaultPath"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescInfo"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescComments"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescCreate"])\n'
                      '    callback.writeLine("serverLog", rei.condInputData["rescModify"])\n')

        return 'def {pep_name}(rule_args, callback, rei):\n{write_statements}\n{rule_body}'.format(**locals())

#    def make_new_server_config_json(self, server_config_filename):
#        # load server_config.json to inject a new rule base
#        with open(server_config_filename) as f:
#            svr_cfg = json.load(f)
#
#        # inject a new rule base into the native rule engine
#        svr_cfg['plugin_configuration']['rule_engines'][0]['plugin_specific_configuration']['re_rulebase_set'] = ["test", "core"]
#
#        # dump to a string to repave the existing server_config.json
#        return json.dumps(svr_cfg, sort_keys=True,indent=4, separators=(',', ': '))

    def pep_test_helper(self, precommands=[], commands=[], rule_body='', client_rule=None, target_name=None, user_session=None):
        pep_name = self.pep_name

        # user session
        if user_session is None:
            if client_rule is None:
                user_session = self.user0
            else:
                # Python rule engine needs admin privileges to run irule
                user_session = self.admin

        # local vars to format command strings
        testfile = self.testfile
        large_file = self.large_file
        if target_name is None:
            target_name = testfile
        target_obj = '/'.join([user_session.session_collection, target_name])

        # query for resource properties
        resource_property_list = self.get_resource_property_list(user_session)

        with temporary_core_file() as core:

            # make pep rule
            if self.plugin_name == 'irods_rule_engine_plugin-irods_rule_language':
                test_rule = self.make_pep_rule(pep_name, rule_body)
            elif self.plugin_name == 'irods_rule_engine_plugin-python':
                test_rule = self.make_pep_rule_python(pep_name, rule_body)

            # write pep rule into test_re
#            with open(test_re, 'w') as f:
#                f.write(test_rule)
            core.add_rule(test_rule)

#            # repave the existing server_config.json to add test_re
#            with open(server_config_filename, 'w') as f:
#                f.write(new_server_config)

            # make client-side rule file
            if client_rule is not None:
                client_rule_file = "test_rule_file.r"
                with open(client_rule_file, 'w') as f:
                    f.write(client_rule.format(**locals()))

            # perform precommands
            for c in precommands:
                if isinstance(c, tuple):
                    user_session.assert_icommand(c[0].format(**locals()), c[1], c[2])
                else:
                    user_session.assert_icommand(c.format(**locals()))

            # checkpoint log to know where to look for the string
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())

            # perform commands to hit PEP
            for c in commands:
                if isinstance(c, tuple):
                    user_session.assert_icommand(c[0].format(**locals()), c[1], c[2])
                else:
                    user_session.assert_icommand(c.format(**locals()))

            # delete client-side rule file
            if client_rule is not None:
                os.unlink(client_rule_file)

            # confirm that PEP was hit by looking for pep name in server log
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), pep_name, start_index=initial_log_size))

            # check that resource session vars were written to the server log
            for line in resource_property_list:
                column = line.rsplit('=', 1)[0].strip()
                property = line.rsplit('=', 1)[1].strip()
                if property:
                    if column != 'RESC_MODIFY_TIME':
                        lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), property, start_index=initial_log_size))

        # cleanup
        user_session.run_icommand('irm -f {target_obj}'.format(**locals()))
#        os.unlink(test_re)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
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

class Test_Remote_Exec(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Remote_Exec'

    def setUp(self):
        super(Test_Remote_Exec, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Remote_Exec, self).tearDown()

    @staticmethod
    def create_rule_file(rule_text_key, rule_text):
        rule_file_path = rule_text_key + '.r'
        parameters = {}
        parameters['host'] = test.settings.ICAT_HOSTNAME
        parameters['zone'] = 'tempZone'
        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_text.format(**parameters))
        return rule_file_path

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_remote_no_writeline(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_no_writeLine {{
                    remote("{host}", "<ZONE>{zone}</ZONE>") {{
                        *a = "remote";
                    }}
                    writeLine("stdout", "a=*a");
                }}
                INPUT *a="input"
                OUTPUT ruleExecOut
            ''')
        }

        rule_file_path = Test_Remote_Exec.create_rule_file('test_remote_no_writeline', rule_map[self.plugin_name])
        try:
            self.admin.assert_icommand(['irule', '-F', rule_file_path], 'STDOUT', 'a=remote')
        finally:
            if os.path.exists(rule_file_path):
                os.unlink(rule_file_path)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_remote_writeline(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_writeLine {{
                    remote("{host}", "<ZONE>{zone}</ZONE>") {{
                        writeLine("stdout", "Remote writeLine");
                    }}
                }}
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_file_path = Test_Remote_Exec.create_rule_file('test_remote_writeline', rule_map[self.plugin_name])
        try:
            self.admin.assert_icommand(['irule', '-F', rule_file_path], 'STDOUT', 'Remote writeLine')
        finally:
            if os.path.exists(rule_file_path):
                os.unlink(rule_file_path)

    @unittest.skip('Remove skip with resolution of #4262')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_remote_in_remote_writeline(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_writeLine {{
                    remote("{host}", "<ZONE>{zone}</ZONE>") {{
                        remote("{host}", "<ZONE>{zone}</ZONE>") {{
                            writeLine("stdout", "Remote in remote writeLine");
                        }}
                        writeLine("stdout", "Remote writeLine");
                    }}
                }}
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_file_path = Test_Remote_Exec.create_rule_file('test_remote_in_remote_writeline', rule_map[self.plugin_name])
        try:
            self.admin.assert_icommand(['irule', '-F', rule_file_path], 'STDOUT_MULTILINE', ['Remote in remote writeLine', 'Remote writeLine'])
        finally:
            if os.path.exists(rule_file_path):
                os.unlink(rule_file_path)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads server log')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_delay_in_remote_writeline(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_writeLine {{
                    remote("{host}", "<ZONE>{zone}</ZONE>") {{
                        delay("<PLUSET>1s</PLUSET>") {{
                            writeLine("serverLog", "Delay in remote writeLine");
                        }}
                        writeLine("stdout", "Remote writeLine");
                    }}
                }}
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_file_path = Test_Remote_Exec.create_rule_file('test_delay_in_remote_writeline', rule_map[self.plugin_name])
        try:
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file_path], 'STDOUT', 'Remote writeLine')
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Delay in remote writeLine', start_index=initial_log_size))
        finally:
            if os.path.exists(rule_file_path):
                os.unlink(rule_file_path)

    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads server log')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_remote_in_delay_writeline(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_writeLine {{
                    delay("<PLUSET>1s</PLUSET>") {{
                        remote("{host}", "<ZONE>{zone}</ZONE>") {{
                            writeLine("serverLog", "Remote in delay writeLine");
                        }}
                        writeLine("serverLog", "Delay writeLine");
                    }}
                }}
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_file_path = Test_Remote_Exec.create_rule_file('test_remote_in_delay_writeline', rule_map[self.plugin_name])
        try:
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())
            self.admin.assert_icommand(['irule', '-F', rule_file_path])
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Remote in delay writeLine', start_index=initial_log_size))
            lib.delayAssert(lambda: lib.count_occurrences_of_string_in_log(paths.server_log_path(), 'Delay writeLine', start_index=initial_log_size))
        finally:
            if os.path.exists(rule_file_path):
                os.unlink(rule_file_path)


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for Python REP')
    def test_remote_returns_appropriate_error_on_bad_hostname__issue_4260(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test_remote_with_bad_hostname {{
                    # this will execute
                    msiAddKeyVal(*key_val_pair,"{metadata_attr}","{metadata_value_true}");
                    msiAssociateKeyValuePairsToObj(*key_val_pair,"{username}","-u");

                    # this block definitely should not execute
                    remote("{hostname}", "<ZONE>{zone}</ZONE>") {{
                        msiAddKeyVal(*key_val_pair,"{metadata_attr}","{metadata_value_false}");
                        msiAssociateKeyValuePairsToObj(*key_val_pair,"{username}","-u");
                    }}
                }}
                INPUT null
                OUTPUT ruleExecOut
            ''')
        }

        rule_text_key = 'test_remote_returns_appropriate_error_on_bad_hostname__issue_4260'
        rule_file_path = os.path.join(self.admin.local_session_dir, rule_text_key + '.r')
        parameters = {}
        parameters['hostname'] = 'badhostname'
        parameters['zone'] = 'tempZone'
        parameters['username'] = self.admin.username
        parameters['metadata_attr'] = rule_text_key
        parameters['metadata_value_true'] = 'this executed!'
        parameters['metadata_value_false'] = 'this will never execute!'
        rule_str = rule_map[self.plugin_name].format(**parameters)
        with open(rule_file_path, 'w') as rule_file:
            rule_file.write(rule_str)

        out, err, rc = self.admin.run_icommand(['irule', '-r', self.plugin_name + '-instance', '-F', rule_file_path])
        self.assertNotEqual(0, rc)
        self.assertTrue('Failed to resolve hostname: [{}]'.format(parameters['hostname']) in out)
        self.assertTrue('SYS_INVALID_SERVER_HOST' in err)

        lib.delayAssert(
            lambda: lib.metadata_attr_with_value_exists(
                self.admin,
                parameters['metadata_attr'],
                parameters['metadata_value_true'])
        )

        self.assertFalse(
            lib.metadata_attr_with_value_exists(
                self.admin,
                parameters['metadata_attr'],
                parameters['metadata_value_false'])
        )
