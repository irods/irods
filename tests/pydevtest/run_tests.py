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
    subprocess.check_call([irodsctl, arg])

def restart_irods_server():
    run_irodsctl_with_arg('restart')

def run_devtesty():
    run_irodsctl_with_arg('devtesty')

def run_fastswap_test():
    subprocess.check_call('rulebase_fastswap_test_2276.sh')

def optparse_callback_catch_keyboard_interrupt(*args, **kwargs):
    unittest.installHandler()

def optparse_callback_use_ssl(*args, **kwargs):
    import configuration
    configuration.USE_SSL = True

def optparse_callback_topology_test(option, opt_str, value, parser):
    import configuration
    configuration.RUN_IN_TOPOLOGY = True
    configuration.TOPOLOGY_FROM_RESOURCE_SERVER = value == 'resource'
    configuration.HOSTNAME_1 = 'resource1.example.org'
    configuration.HOSTNAME_2 = 'resource2.example.org'
    configuration.HOSTNAME_3 = 'resource3.example.org'
    configuration.ICAT_HOSTNAME = 'icat.example.org'

def optparse_callback_federation(option, opt_str, value, parser):
    import configuration
    configuration.FEDERATION.REMOTE_IRODS_VERSION = tuple(map(int, value[0].split('.')))
    configuration.FEDERATION.REMOTE_ZONE = value[1]
    configuration.FEDERATION.REMOTE_HOST = value[2]

def run_tests_from_names(names, buffer_test_output, xml_output):
    loader = unittest.TestLoader()
    suites = [loader.loadTestsFromName(name) for name in names]
    super_suite = unittest.TestSuite(suites)
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

if __name__ == '__main__':
    parser = optparse.OptionParser()
    parser.add_option('--run_specific_test', metavar='dotted name')
    parser.add_option('--run_python_suite', action='store_true')
    parser.add_option('--include_auth_tests', action='store_true')
    parser.add_option('--run_devtesty', action='store_true')
    parser.add_option('--topology_test', type='choice', choices=['icat', 'resource'], action='callback', callback=optparse_callback_topology_test, metavar='<icat|resource>')
    parser.add_option('--catch_keyboard_interrupt', action='callback', callback=optparse_callback_catch_keyboard_interrupt)
    parser.add_option('--use_ssl', action='callback', callback=optparse_callback_use_ssl)
    parser.add_option('--no_buffer', action='store_false', dest='buffer_test_output', default=True)
    parser.add_option('--xml_output', action='store_true', dest='xml_output', default=False)
    parser.add_option('--federation', type='str', nargs=3, action='callback', callback=optparse_callback_federation, metavar='<remote irods version, remote zone, remote host>')
    options, _ = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)

    test_identifiers = []
    if options.run_specific_test:
        test_identifiers.append(options.run_specific_test)
    if options.run_python_suite:
        test_identifiers.extend(['test_xmsg', 'test_iadmin', 'test_mso_suite', 'test_resource_types', 'test_catalog', 'test_rulebase',
                                 'test_resource_tree', 'test_load_balanced_suite', 'test_icommands_file_operations', 'test_imeta_set',
                                 'test_all_rules', 'test_iscan', 'test_ichmod', 'test_iput_options', 'test_ireg', 'test_irsync',
                                 'test_control_plane', 'test_iticket', 'test_irodsctl', 'test_quotas', 'test_fuse'])
    if options.include_auth_tests:
        test_identifiers.append('test_auth')

    results = run_tests_from_names(test_identifiers, options.buffer_test_output, options.xml_output)
    print(results)
    if not results.wasSuccessful():
        sys.exit(1)

    if options.run_devtesty:
        run_devtesty()
