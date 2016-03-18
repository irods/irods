from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import commands
import getpass
import imp
import json
import os
import shutil
import socket
import stat
import subprocess
import time
import tempfile

import configuration
import lib
import resource_suite

# path to server_config.py
pydevtestdir = os.path.dirname(os.path.realpath(__file__))
topdir = os.path.dirname(os.path.dirname(pydevtestdir))
packagingdir = os.path.join(topdir, 'packaging')
module_tuple = imp.find_module('server_config', [packagingdir])
imp.load_module('server_config', *module_tuple)
from server_config import ServerConfig

class Test_Native_Rule_Engine_Plugin(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Native_Rule_Engine_Plugin, self).setUp()

    def tearDown(self):
        super(Test_Native_Rule_Engine_Plugin, self).tearDown()

    def test_network_pep(self):
        rules_to_prepend = """
pep_network_agent_start_pre(*INST,*CTX,*OUT) {
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_network_agent_start_post(*INST,*CTX,*OUT){
writeLine( 'serverLog', '*OUT')
}
"""
        corefile = lib.get_core_re_dir() + "/core.re"

        with lib.file_backed_up(corefile):
	    time.sleep(1)  # remove once file hash fix is commited #2279
	    lib.prepend_string_to_file(rules_to_prepend, corefile)
	    time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_log_size('server')
            
            filename = "test_re_serialization.txt"
            lib.make_file(filename, 1000)

            self.admin.assert_icommand("iput -f --metadata ATTR;VALUE;UNIT "+filename)

            out_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'THIS IS AN OUT VARIABLE',
                            start_index=initial_size_of_server_log)
        output = commands.getstatusoutput('rm ' + filename)
       
        print( "counts: " + str(out_count) )

        assert 1 == out_count

    def test_auth_pep(self):
        rules_to_prepend = """
pep_resource_resolve_hierarchy_pre(*A,*B,*OUT,*E,*F,*G,*H){
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*A,*B,*OUT,*E,*F,*G,*H){
writeLine( 'serverLog', '*OUT')
}
"""
        corefile = lib.get_core_re_dir() + "/core.re"

        with lib.file_backed_up(corefile):
	    time.sleep(1)  # remove once file hash fix is commited #2279
	    lib.prepend_string_to_file(rules_to_prepend, corefile)
	    time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_log_size('server')
            
            filename = "test_re_serialization.txt"
            lib.make_file(filename, 1000)

            self.admin.assert_icommand("iput -f --metadata ATTR;VALUE;UNIT "+filename)

            out_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'THIS IS AN OUT VARIABLE',
                            start_index=initial_size_of_server_log)
        output = commands.getstatusoutput('rm ' + filename)
       
        print( "counts: " + str(out_count) )

        assert 1 == out_count

    def test_out_variable(self):
        rules_to_prepend = """
pep_resource_resolve_hierarchy_pre(*A,*B,*OUT,*E,*F,*G,*H){
*OUT = "THIS IS AN OUT VARIABLE"
}
pep_resource_resolve_hierarchy_post(*A,*B,*OUT,*E,*F,*G,*H){
writeLine( 'serverLog', '*OUT')
}
"""
        corefile = lib.get_core_re_dir() + "/core.re"

        with lib.file_backed_up(corefile):
	    time.sleep(1)  # remove once file hash fix is commited #2279
	    lib.prepend_string_to_file(rules_to_prepend, corefile)
	    time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_log_size('server')
            
            filename = "test_re_serialization.txt"
            lib.make_file(filename, 1000)

            self.admin.assert_icommand("iput -f --metadata ATTR;VALUE;UNIT "+filename)

            out_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'THIS IS AN OUT VARIABLE',
                            start_index=initial_size_of_server_log)
        output = commands.getstatusoutput('rm ' + filename)
       
        print( "counts: " + str(out_count) )

        assert 1 == out_count

    def test_re_serialization(self):
        rules_to_prepend = """
pep_resource_resolve_hierarchy_pre(*A,*B,*OUT,*E,*F,*G,*H){
writeLine("serverLog", "pep_resource_resolve_hierarchy_pre - [*A] [*B] [*OUT] [*E] [*F] [*G] [*H]");
}
"""
        corefile = lib.get_core_re_dir() + "/core.re"

        with lib.file_backed_up(corefile):
	    time.sleep(1)  # remove once file hash fix is commited #2279
	    lib.prepend_string_to_file(rules_to_prepend, corefile)
	    time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_log_size('server')
            
            filename = "test_re_serialization.txt"
            lib.make_file(filename, 1000)

            self.admin.assert_icommand("iput -f --metadata ATTR;VALUE;UNIT "+filename)

            auth_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'user_auth_info_auth_flag=5',
                            start_index=initial_size_of_server_log)
            zone_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'user_rods_zone=tempZone',
                            start_index=initial_size_of_server_log)
            user_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'user_user_name=otherrods',
                            start_index=initial_size_of_server_log)
            mdata_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'ATTR;VALUE;UNIT',
                            start_index=initial_size_of_server_log)
        output = commands.getstatusoutput('rm ' + filename)
       
        print( "counts: " + str(auth_count) + " " + str(zone_count) + " " + str(user_count) + " " + str(mdata_count) )

        assert 1 == auth_count
        assert 1 == zone_count
        assert 1 == user_count
        assert 1 == mdata_count

    def test_api_plugin(self):
        rules_to_prepend = """
pep_rs_hello_world_pre(*INST,*OUT,*COMM,*HELLO_IN,*HELLO_OUT) {
    writeLine("serverLog", "pep_rs_hello_world_pre - *INST *OUT *HELLO_IN, *HELLO_OUT");
}
pep_rs_hello_world_post(*INST,*OUT,*COMM,*HELLO_IN,*HELLO_OUT) {
    writeLine("serverLog", "pep_rs_hello_world_post - *INST *OUT *HELLO_IN, *HELLO_OUT");
}
"""
        corefile = lib.get_core_re_dir() + "/core.re"
        with lib.file_backed_up(corefile):
	    time.sleep(1)  # remove once file hash fix is commited #2279
	    lib.prepend_string_to_file(rules_to_prepend, corefile)
	    time.sleep(1)  # remove once file hash fix is commited #2279

            initial_size_of_server_log = lib.get_log_size('server')
            self.admin.assert_icommand("iapitest", 'STDOUT_SINGLELINE', 'this')

            pre_count = lib.count_occurrences_of_string_in_log(
                            'server',
                            'pep_rs_hello_world_pre - api_instance <unconvertible> that=hello, world.++++this=42, null_value',
                            start_index=initial_size_of_server_log)
            hello_count = lib.count_occurrences_of_string_in_log(
                             'server',
                             'HELLO WORLD',
                             start_index=initial_size_of_server_log)
            post_count = lib.count_occurrences_of_string_in_log(
                             'server',
                             'pep_rs_hello_world_post - api_instance <unconvertible> that=hello, world.++++this=42, that=hello, world.++++this=42++++value=128',
                             start_index=initial_size_of_server_log)

        assert 1 == pre_count
        assert 1 == hello_count
        assert 1 == post_count

    def test_rule_engine_2242(self):
        self.admin.assert_icommand("irule -F rule1_2242.r", 'STDOUT_SINGLELINE', "Update session variable $userNameClient not allowed")
        self.admin.assert_icommand("irule -F rule2_2242.r", "EMPTY")

    @unittest.skipIf(configuration.TOPOLOGY_FROM_RESOURCE_SERVER, 'Skip for topology testing from resource server: reads re server log')
    def test_rule_engine_2309(self):
        corefile = lib.get_core_re_dir() + "/core.re"
        coredvm = lib.get_core_re_dir() + "/core.dvm"
        with lib.file_backed_up(coredvm):
            lib.prepend_string_to_file('oprType||rei->doinp->oprType\n', coredvm)
            with lib.file_backed_up(corefile):
                initial_size_of_server_log = lib.get_log_size('server')
                rules_to_prepend = '''
 acSetNumThreads() {
     writeLine("serverLog","test_rule_engine_2309: put: acSetNumThreads oprType [$oprType]");
 }
 '''
                time.sleep(1)  # remove once file hash fix is commited #2279
                lib.prepend_string_to_file(rules_to_prepend, corefile)
                time.sleep(1)  # remove once file hash fix is commited #2279
                trigger_file = 'file_to_trigger_acSetNumThreads'
                lib.make_file(trigger_file, 4 * pow(10, 7))
                self.admin.assert_icommand('iput {0}'.format(trigger_file))
                assert 1 == lib.count_occurrences_of_string_in_log(
                    'server', 'writeLine: inString = test_rule_engine_2309: put: acSetNumThreads oprType [1]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log('server', 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)

            with lib.file_backed_up(corefile):
                initial_size_of_server_log = lib.get_log_size('server')
                rules_to_prepend = '''
acSetNumThreads() {
    writeLine("serverLog","test_rule_engine_2309: get: acSetNumThreads oprType [$oprType]");
}
'''
                time.sleep(1)  # remove once file hash fix is commited #2279
                lib.prepend_string_to_file(rules_to_prepend, corefile)
                time.sleep(1)  # remove once file hash fix is commited #2279
                self.admin.assert_icommand('iget {0}'.format(trigger_file), use_unsafe_shell=True)
                assert 1 == lib.count_occurrences_of_string_in_log(
                    'server', 'writeLine: inString = test_rule_engine_2309: get: acSetNumThreads oprType [2]', start_index=initial_size_of_server_log)
                assert 0 == lib.count_occurrences_of_string_in_log('server', 'RE_UNABLE_TO_READ_SESSION_VAR', start_index=initial_size_of_server_log)
                os.unlink(trigger_file)

