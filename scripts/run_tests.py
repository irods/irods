#!/usr/bin/python3
from __future__ import print_function

try:
    import importlib
except ImportError:
    from __builtin__ import __import__
import itertools
import logging
import optparse
import os
import subprocess
import sys
import fnmatch
import json

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from irods.configuration import IrodsConfig
from irods.controller import IrodsController
import irods.lib
import irods.log
import irods.paths
import irods.test
import irods.test.settings

def run_irodsctl_with_arg(arg):
    irodsctl = os.path.join(IrodsConfig().irods_directory, 'irodsctl')
    subprocess.check_call([irodsctl, arg])

def run_devtesty():
    print('devtesty is currently disabled', file=sys.stderr)
    # run_irodsctl_with_arg('devtesty')

def run_fastswap_test():
    subprocess.check_call('rulebase_fastswap_test_2276.sh')

def optparse_callback_catch_keyboard_interrupt(*args, **kwargs):
    unittest.installHandler()

def optparse_callback_use_ssl(*args, **kwargs):
    irods.test.settings.USE_SSL = True

def optparse_callback_use_mungefs(*args, **kwargs):
    irods.test.settings.USE_MUNGEFS = True

def optparse_callback_federation(option, opt_str, value, parser):
    irods.test.settings.FEDERATION.REMOTE_IRODS_VERSION = tuple(map(int, value[0].split('.')))
    irods.test.settings.FEDERATION.REMOTE_ZONE = value[1]
    irods.test.settings.FEDERATION.REMOTE_HOST = value[2]
    if irods.test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4,2):
        irods.test.settings.FEDERATION.REMOTE_VAULT = '/var/lib/irods/iRODS/Vault'

def add_class_path_prefix(name):
    return "irods.test." + name

def is_testfile(filename):
    name, ext = os.path.splitext(filename)
    return name.startswith('test_') and ext == '.py'

def get_plugin_tests():
    additional_plugin_tests = []
    root = irods.paths.test_directory()
    for curdir, _, entries in os.walk(os.path.join(root, 'plugins')):
        for entry in entries:
            module = '.'.join(os.path.split(os.path.relpath(curdir, root))).strip('.')
            if is_testfile(entry):
                additional_plugin_tests.append('.'.join([module, os.path.splitext(entry)[0]]))
    return additional_plugin_tests

def run_tests_from_names(names, buffer_test_output, xml_output, skipUntil):
    loader = unittest.TestLoader()
    suites = []
    for name in names:
        full_name = add_class_path_prefix(name)
        try:
            suite = loader.loadTestsFromName(full_name)
            suites.append(suite)
        except AttributeError as e:
            raise ImportError("Could not load '" + full_name + "'. Please ensure the file exists and all imports (including imports in imported files) are valid.")

    if skipUntil == None:
        markers = [["",""]]
    else:
        markers = map(lambda x: map(lambda y : y.strip(), x.split("-")), skipUntil.split(","))

    for marker in markers:
        for i in range(0, len(marker)):
            if marker[i] != "":
                marker[i] = add_class_path_prefix(marker[i])

    if skipUntil != None:
        for marker in markers:
            if len(marker) == 1:
                print(marker[0])
            else:
                print(marker[0], "-", marker[1])

    # simulate nonlocal in python 2.x
    filtered_markers = [m for m in markers if m[0] == ""]

    suitelist = []
    def filter_testcase(suite, marker):
        return fnmatch.fnmatch(suite.id(), marker)

    def filter_testsuite(suite, filtered_markers):
        if isinstance(suite, unittest.TestCase):
            if len(filtered_markers) == 0:
                filtered_markers = [m for m in markers if filter_testcase(suite, m[0])]
            if len(filtered_markers) != 0:
                suitelist.append(suite)
                filtered_markers = [m for m in filtered_markers if m[-1] == "" or not filter_testcase(suite, m[-1])]
        else:
            for subsuite in suite:
                filter_testsuite(subsuite, filtered_markers)

    filter_testsuite(suites, filtered_markers)
    super_suite = unittest.TestSuite(suitelist)

    if xml_output:
        import xmlrunner
        runner = xmlrunner.XMLTestRunner(output='test-reports', verbosity=2)
    else:
        runner = unittest.TextTestRunner(verbosity=2, failfast=True, buffer=buffer_test_output, resultclass=RegisteredTestResult)
    results = runner.run(super_suite)
    return results

class RegisteredTestResult(unittest.TextTestResult):
    def __init__(self, *args, **kwargs):
        super(RegisteredTestResult, self).__init__(*args, **kwargs)
        unittest.registerResult(self)

    def startTest(self, test):
        # TextTestResult's impl prints as "test (module.class)" which prevents copy/paste
        print('{0} ... '.format(test.id()), end='', file=self.stream)
        unittest.TestResult.startTest(self, test)

def clear_irods_shared_memory_files():
    for shm_location in irods.paths.possible_shm_locations():
        try:
            for f in os.listdir(shm_location):
                if 'irods' in f.lower():
                    print(f'Warning: Unlinking shared memory file [{f}]')
                    os.unlink(f)
        except OSError:
            pass

if __name__ == '__main__':
    logging.getLogger().setLevel(logging.NOTSET)
    l = logging.getLogger(__name__)

    irods.log.register_tty_handler(sys.stderr, logging.WARNING, None)
    irods.log.register_file_handler(IrodsConfig().test_log_path)

    parser = optparse.OptionParser()
    parser.add_option('--run_specific_test', metavar='dotted name')
    parser.add_option('--skip_until', action="store")
    parser.add_option('--run_python_suite', action='store_true')
    parser.add_option('--run_plugin_tests', action='store_true')
    parser.add_option('--include_auth_tests', action='store_true')
    parser.add_option('--include_timing_tests', action='store_true')
    parser.add_option('--run_devtesty', action='store_true')
    parser.add_option('--topology_test', type='choice', choices=['icat', 'resource'], metavar='<icat|resource>')
    parser.add_option('--catch_keyboard_interrupt', action='callback', callback=optparse_callback_catch_keyboard_interrupt)
    parser.add_option('--use_ssl', action='callback', callback=optparse_callback_use_ssl)
    parser.add_option('--use_mungefs', action='callback', callback=optparse_callback_use_mungefs)
    parser.add_option('--no_buffer', action='store_false', dest='buffer_test_output', default=True)
    parser.add_option('--xml_output', action='store_true', dest='xml_output', default=False)
    parser.add_option('--federation', type='str', nargs=3, action='callback', callback=optparse_callback_federation, metavar='<remote irods version, remote zone, remote host>')
    parser.add_option('--hostnames', type='str', nargs=4, metavar='<ICAT_HOSTNAME HOSTNAME_1 HOSTNAME_2 HOSTNAME_3>')
    options, _ = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)

    univmss_testing = os.path.join(IrodsConfig().irods_directory, 'msiExecCmd_bin', 'univMSSInterface.sh')
    if not os.path.exists(univmss_testing):
        univmss_template = os.path.join(IrodsConfig().irods_directory, 'msiExecCmd_bin', 'univMSSInterface.sh.template')
        with open(univmss_template) as f:
            univmss_contents = f.read().replace('template-','')
        with open(univmss_testing, 'w') as f:
            f.write(univmss_contents)
        os.chmod(univmss_testing, 0o544)

    if options.topology_test:
        irods.test.settings.RUN_IN_TOPOLOGY = True
        irods.test.settings.TOPOLOGY_FROM_RESOURCE_SERVER = options.topology_test == 'resource'
        if options.hostnames:
            irods.test.settings.ICAT_HOSTNAME = options.hostnames[0]
            irods.test.settings.HOSTNAME_1 = options.hostnames[1]
            irods.test.settings.HOSTNAME_2 = options.hostnames[2]
            irods.test.settings.HOSTNAME_3 = options.hostnames[3]
        else:
            irods.test.settings.ICAT_HOSTNAME = 'icat.example.org'
            irods.test.settings.HOSTNAME_1 = 'resource1.example.org'
            irods.test.settings.HOSTNAME_2 = 'resource2.example.org'
            irods.test.settings.HOSTNAME_3 = 'resource3.example.org'

    test_identifiers = []
    if options.run_specific_test:
        test_identifiers.append(options.run_specific_test)
    if options.include_auth_tests:
        test_identifiers.append('test_auth')
    if options.include_timing_tests:
        test_identifiers.append('timing_tests')
    if options.run_python_suite:
        with open(os.path.join(IrodsConfig().scripts_directory, 'core_tests_list.json'), 'r') as f:
            test_identifiers.extend(json.loads(f.read()))
    if options.run_plugin_tests:
        test_identifiers.extend(get_plugin_tests())

    IrodsController().stop()
    # This is a workaround for #6594. If the server did not clean up its shared memory files, these need to be cleared
    # in order for the tests to function properly. If the issue occurs, it is likely that the test which caused the
    # leftover shared memory files failed and will be picked up in the test results.
    clear_irods_shared_memory_files()

    # Some platforms do not include the 'sbin' directory in the path of the service account. There are several
    # executables installed in this directory and used in the tests which need to be accessible, so add the directory
    # to the front of PATH for the duration of the tests.
    os.environ['PATH'] = ':'.join([irods.paths.server_bin_directory(), os.environ['PATH']])

    IrodsController().start(test_mode=True)
    results = run_tests_from_names(test_identifiers, options.buffer_test_output, options.xml_output, options.skip_until)
    print(results)

    os.remove(univmss_testing)

    if not results.wasSuccessful():
        sys.exit(1)

    if options.run_devtesty:
        run_devtesty()
