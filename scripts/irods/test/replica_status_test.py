from __future__ import print_function
import os

from . import command
from .. import lib

#repl_statuses = ['X', '&', '?']
repl_statuses = ['X', '&']

def setup_replicas(cls, scenario):
    initial_replicas = scenario['start']
    ordered_keys = sorted(initial_replicas)

    replica_zero_resource = None

    # create the new data object
    for repl in ordered_keys:
        if initial_replicas[repl] != '-':
            replica_zero_resource = cls.leaf_rescs[repl]['name']
            cls.admin.assert_icommand(['iput', '-R', replica_zero_resource, cls.local_path, cls.logical_path])
            break

    # stage replicas and set replica status for scenario
    for repl in ordered_keys:
        if initial_replicas[repl] != '-':
            if cls.leaf_rescs[repl]['name'] != replica_zero_resource:
                cls.admin.assert_icommand(['irepl', '-n0', '-R', cls.leaf_rescs[repl]['name'], cls.logical_path])

            cls.admin.assert_icommand(
                ['iadmin', 'modrepl', 'logical_path', cls.logical_path, 'resource_hierarchy', cls.leaf_rescs[repl]['name'],
                    'DATA_REPL_STATUS', str(repl_statuses.index(initial_replicas[repl]))])

def assert_result(cls, scenario, out=None, err=None, rc=None):
    result = scenario['output']
    scenario_string = scenario['start']
    try:
        if result['out']: cls.assertIn(result['out'], out)
        elif out: cls.fail('found stdout:[{0}], expected None for scenario [{1}]'.format(out, scenario_string))
    except TypeError:
        cls.fail('found stdout:[None], expected [{0}] for scenario [{1}]'.format(result['stdout'], scenario_string))

    try:
        if result['err']: cls.assertIn(result['err'], err)
        elif err: cls.fail('found stderr:[{0}], expected None for scenario [{1}]'.format(err, scenario_string))
    except TypeError:
        cls.fail('found stderr:[None], expected [{0}] for scenario [{1}]'.format(result['err'], scenario_string))

    try:
        if result['rc']: cls.assertEqual(result['rc'], rc)
        elif rc: cls.fail('found rc:[{0}], expected None for scenario [{1}]'.format(rc, scenario_string))
    except TypeError:
        cls.fail('found rc:[None], expected [{0}] for scenario [{1}]'.format(result['rc'], scenario_string))

    ils_out,ils_err,_ = cls.admin.run_icommand(['ils', '-l', cls.logical_path])
    for repl in scenario['end']:
        expected_status = scenario['end'][repl]

        if expected_status not in repl_statuses:
            cls.assertNotIn(cls.leaf_rescs[repl]['name'], ils_out)
            continue

        expected_results = [cls.leaf_rescs[repl]['name'], expected_status]
        result = command.check_command_output(
            None, ils_out, ils_err, check_type='STDOUT_SINGLELINE', expected_results=expected_results)
        cls.assertTrue(result)

def run_command_against_scenarios(cls, command, scenarios, name, file_size=1024):
    i = 0
    for scenario in scenarios:
        i = i + 1
        subtest_name = name.replace(' ', '_').replace('-', '') + f'_{i}'
        with cls.subTest(subtest_name):
            print("============= (" + name + "): [" + str(i) + "] =============")
            print(scenario)
            try:
                if not os.path.exists(cls.local_path):
                    lib.make_file(cls.local_path, file_size)
                cls.admin.assert_icommand(['imkdir', os.path.dirname(cls.logical_path)])

                setup_replicas(cls, scenario)

                out,err,_ = cls.admin.run_icommand(command)
                print(out)
                print(err)
                assert_result(cls, scenario, err=err)

            finally:
                cls.admin.assert_icommand(['irm', '-rf', os.path.dirname(cls.logical_path)])

