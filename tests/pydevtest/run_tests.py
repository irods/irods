from __future__ import print_function

import optparse
import os
import subprocess
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest


def get_irods_root_directory():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def run_irodsctl_with_arg(arg):
    irodsctl = os.path.join(get_irods_root_directory(), 'iRODS', 'irodsctl')
    print(irodsctl)
    subprocess.check_call([irodsctl, arg])


def restart_irods_server():
    run_irodsctl_with_arg('restart')


def run_devtesty():
    run_irodsctl_with_arg('devtesty')


def run_fastswap_test():
    subprocess.check_call('rulebase_fastswap_test_2276.sh')


def optparse_callback_catch_keyboard_interrupt(*args, **kwargs):
    unittest.installHandler()


def optparse_callback_topology_test(option, opt_str, value, parser):
    import pydevtest_common
    pydevtest_common.irods_test_constants.RUN_IN_TOPOLOGY = True
    pydevtest_common.irods_test_constants.RUN_AS_RESOURCE_SERVER = value == 'resource'
    pydevtest_common.irods_test_constants.HOSTNAME_1 = 'resource1.example.org'
    pydevtest_common.irods_test_constants.HOSTNAME_2 = 'resource2.example.org'
    pydevtest_common.irods_test_constants.HOSTNAME_3 = 'resource3.example.org'


def run_tests_from_names(names):
    loader = unittest.TestLoader()
    suites = [loader.loadTestsFromName(name) for name in names]
    super_suite = unittest.TestSuite(suites)
    runner = unittest.TextTestRunner(verbosity=2, failfast=True, buffer=True, resultclass=RegisteredTestResult)
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

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option("--run_specific_test", metavar='dotted name')
    parser.add_option("--run_python_suite", action='store_true')
    parser.add_option("--include_auth_suite_tests", action='store_true')
    parser.add_option("--include_fuse_suite_tests", action='store_true')
    parser.add_option("--run_devtesty", action='store_true')
    parser.add_option("--topology_test", type='choice',
                      choices=['icat', 'resource'], action='callback', callback=optparse_callback_topology_test, metavar='<icat|resource>')
    parser.add_option("--catch_keyboard_interrupt", action='callback',
                      callback=optparse_callback_catch_keyboard_interrupt)
    options, _ = parser.parse_args()

    test_identifiers = []
    if options.run_specific_test:
        test_identifiers.append(options.run_specific_test)
    if options.run_python_suite:
        test_identifiers.extend(['test_xmsg', 'iadmin_suite', 'test_mso_suite', 'test_resource_types', 'catalog_suite', 'rulebase_suite',
                                 'test_resource_tree', 'test_load_balanced_suite', 'test_icommands_file_operations', 'test_imeta_set', 'test_allrules'])
    if options.include_auth_suite_tests:
        test_identifiers.append('auth_suite')
    if options.include_fuse_suite_tests:
        test_identifiers.append('test_fuse_suite')

    results = run_tests_from_names(test_identifiers)
    print(results)
    if not results.wasSuccessful():
        sys.exit(1)

    if options.run_devtesty:
        run_devtesty()
