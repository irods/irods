from __future__ import print_function
import os
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from . import command
from .. import test
from .. import lib
from ..configuration import IrodsConfig

class Test_IRepl(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_IRepl, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_IRepl, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_with_U_and_R_options__issue_3659(self):
        filename = 'test_file_issue_3659.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3659_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])

            # Main Tests.

            # Test 1: Update the replica under resc_1.
            self.admin.assert_icommand('irepl -R resc_1 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])

            # Test 2: Update the replica under resc_2 using any replica under resc_1.
            self.admin.assert_icommand('irepl -S resc_1 -R resc_2 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])

            # Test 3: Update the replica under resc_3 using replica 1.
            self.admin.assert_icommand('irepl -n1 -R resc_3 {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])

            # Test 4: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -S resc_5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NO_GOOD_REPLICA')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 5: Update the replica under resc_4 using the stale replica under resc_5.
            self.admin.assert_icommand('irepl -n5 -R resc_4 {0}'.format(filename), 'STDERR', 'SYS_NO_GOOD_REPLICA')
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', 'X'])

            # Test 6: Invalid resource name.
            self.admin.assert_icommand('irepl -R invalid_resc {0}'.format(filename), 'STDERR', 'SYS_RESC_DOES_NOT_EXIST')

            # Test 7: Incompatible parameters.
            self.admin.assert_icommand('irepl -S resc_2 -n5 -R resc_5 {0}'.format(filename), 'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # Test 8: Update a replica that does not exist under the destination resource.
            self.admin.assert_icommand('irepl -R demoResc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['demoResc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_irepl_not_honoring_R_option__issue_3885(self):
        filename = 'test_file_issue_3885.txt'
        lib.make_file(filename, 1024)

        hostname = IrodsConfig().client_environment['irods_host']
        replicas = 6

        # Create resources [resc_{0..5}].
        for i in range(replicas):
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + str(i))
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc resc_{0} unixfilesystem {1}'.format(i, path), 'STDOUT', 'unixfilesystem')

        # Create compound resource tree.
        compound_resc_child_names = ['cache', 'archive']

        self.admin.assert_icommand('iadmin mkresc comp_resc compound', 'STDOUT', 'compound')

        for name in compound_resc_child_names:
            vault = os.path.join(self.admin.local_session_dir, 'issue_3885_storage_vault_' + name)
            path = hostname + ':' + vault
            self.admin.assert_icommand('iadmin mkresc {0}_resc unixfilesystem {1}'.format(name, path), 'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand('iadmin addchildtoresc comp_resc {0}_resc {0}'.format(name))

        try:
            self.admin.assert_icommand('iput -R resc_0 {0}'.format(filename))

            # Replicate the file across the remaining resources.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))

            # Replicate the file to the compound resource.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))

            # Make all replicas stale except one.
            self.admin.assert_icommand('iput -fR resc_0 {0}'.format(filename))

            # Verify that all replicas are stale except the one under [resc_0].
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_0', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_1', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_2', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_3', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_4', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_5', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand_fail('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

            # Try to replicate to a child of the compound resource directly.
            # This should fail.
            self.admin.assert_icommand('irepl -R cache_resc {0}'.format(filename), 'STDERR', 'DIRECT_CHILD_ACCESS')

            # Replicate the file in order and verify after each command
            # if the destination resource holds a current replica.
            for i in range(1, replicas):
                self.admin.assert_icommand('irepl -R resc_{0} {1}'.format(i, filename))
                self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['resc_' + str(i), '&'])

            # Replicate to the compound resource and verify the results.
            self.admin.assert_icommand('irepl -R comp_resc {0}'.format(filename))
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['cache_resc', '&'])
            self.admin.assert_icommand('ils -l {0}'.format(filename), 'STDOUT_SINGLELINE', ['archive_resc', '&'])

        finally:
            # Clean up.
            self.admin.assert_icommand('irm -rf {0}'.format(filename))

            for i in range(replicas):
                self.admin.assert_icommand('iadmin rmresc resc_{0}'.format(i))

            for name in compound_resc_child_names:
                self.admin.assert_icommand('iadmin rmchildfromresc comp_resc {0}_resc'.format(name))
                self.admin.assert_icommand('iadmin rmresc {0}_resc'.format(name))

            self.admin.assert_icommand('iadmin rmresc comp_resc')

    def test_irepl_data_object_with_no_permission__4479(self):
        user0 = self.user_sessions[0]
        resource_1 = 'resource1'
        resource_1_vault = os.path.join(user0.local_session_dir, resource_1 + 'vault')
        resource_2 = 'resource2'
        resource_2_vault = os.path.join(user0.local_session_dir, resource_2 + 'vault')

        try:
            self.admin.assert_icommand(
                ['iadmin', 'mkresc', resource_1, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_1, resource_1_vault])],
                'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand(
                ['iadmin', 'mkresc', resource_2, 'unixfilesystem', ':'.join([test.settings.HOSTNAME_2, resource_2_vault])],
                'STDOUT', 'unixfilesystem')

            filename = 'test_irepl_data_object_with_no_permission__4479'
            physical_path = os.path.join(user0.local_session_dir, filename)
            lib.make_file(physical_path, 1024)
            logical_path = os.path.join(user0.session_collection, filename)
            #logical_path_sans_zone = os.sep.join(str(logical_path).split(os.sep)[2:])
            #resource_1_repl_path = os.path.join(resource_1_vault, logical_path_sans_zone)
            #resource_2_repl_path = os.path.join(resource_2_vault, logical_path_sans_zone)

            # Put data object to play with...
            user0.assert_icommand(
                ['iput', '-R', resource_1, physical_path, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_1],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

            # Attempt to replicate data object for which admin has no permissions...
            self.admin.assert_icommand(
                ['irepl', '-R', resource_2, logical_path],
                'STDERR', 'CAT_NO_ACCESS_PERMISSION')
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_2],
                'STDOUT', 'No results found.')
            # TODO: #4770 Use test tool to assert that file was not created on resource_2 (i.e. iscan on remote physical file)

            # Try again with admin flag (with success)
            self.admin.assert_icommand(['irepl', '-M', '-R', resource_2, logical_path])
            self.admin.assert_icommand(
                ['iadmin', 'ls', 'logical_path', logical_path, 'resource_hierarchy', resource_2],
                'STDOUT', 'DATA_REPL_STATUS: 1')
            self.admin.assert_icommand(['iscan', '-d', logical_path])

        finally:
            user0.run_icommand(['irm', '-f', logical_path])
            self.admin.assert_icommand(['iadmin', 'rmresc', resource_1])
            self.admin.assert_icommand(['iadmin', 'rmresc', resource_2])

@unittest.skip('this is an aspirational test suite')
class test_irepl_repl_status(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    repl_statuses = ['X', '&', '?']
    def setUp(self):
        super(test_irepl_repl_status, self).setUp()

        self.admin = self.admin_sessions[0]

        self.leaf_rescs = {
            'a' : {
                'name': 'test_irepl_repl_status_a',
                'vault': os.path.join(self.admin.local_session_dir, 'a'),
                'host': test.settings.HOSTNAME_1
            },
            'b' : {
                'name': 'test_irepl_repl_status_b',
                'vault': os.path.join(self.admin.local_session_dir, 'b'),
                'host': test.settings.HOSTNAME_2
            },
            'c' : {
                'name': 'test_irepl_repl_status_c',
                'vault': os.path.join(self.admin.local_session_dir, 'c'),
                'host': test.settings.HOSTNAME_3
            },
            'f' : {
                'name': 'test_irepl_repl_status_f',
                'vault': os.path.join(self.admin.local_session_dir, 'f'),
                'host': test.settings.HOSTNAME_1
            }
        }

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc['name'], 'unixfilesystem',
                ':'.join([resc['host'], resc['vault']])],
                'STDOUT', resc['name'])

        self.parent_rescs = {
            'd' : 'test_irepl_repl_status_d',
            'e' : 'test_irepl_repl_status_e',
        }

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'mkresc', resc, 'passthru'], 'STDOUT', resc)

        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'addchildtoresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        #self.env_backup = copy.deepcopy(self.admin.environment_file_contents)
        self.admin.environment_file_contents.update({'irods_default_resource': self.leaf_rescs['a']['name']})

        self.filename = 'foo'
        self.local_path = os.path.join(self.admin.local_session_dir, self.filename)
        self.logical_path = os.path.join(self.admin.session_collection, 'subdir', self.filename)

    def tearDown(self):
        #self.admin.environment_file_contents = self.env_backup

        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['d'], self.parent_rescs['e']])
        self.admin.assert_icommand(['iadmin', 'rmchildfromresc', self.parent_rescs['e'], self.leaf_rescs['f']['name']])

        for resc in self.parent_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc])

        for resc in self.leaf_rescs.values():
            self.admin.assert_icommand(['iadmin', 'rmresc', resc['name']])

        super(test_irepl_repl_status, self).tearDown()

    def test_invalid_parameters(self):
        # does not exist
        self.admin.assert_icommand(['irepl', self.logical_path], 'STDERR', 'does not exist')

        # put a file
        if not os.path.exists(self.local_path):
            lib.make_file(self.local_path, 1024)
        self.admin.assert_icommand(['imkdir', os.path.dirname(self.logical_path)])
        self.admin.assert_icommand(['iput', self.local_path, self.logical_path])

        try:
            # INCOMPATIBLE_PARAMETERS
            # irepl -S a -n 0 foo
            self.admin.assert_icommand(['irepl', '-S', self.leaf_rescs['a']['name'], '-n0', self.logical_path],
                'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # irepl -r -S a foo_dir
            self.admin.assert_icommand(['irepl', '-r', '-S', self.leaf_rescs['a']['name'], os.path.dirname(self.logical_path)],
                'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # irepl -a -R
            # TODO: This is valid/compatible
            #self.admin.assert_icommand(['irepl', '-a', '-R', self.leaf_rescs['b']['name'], '-n0', self.logical_path],
                #'STDERR', 'USER_INCOMPATIBLE_PARAMS')

            # DIRECT_CHILD_ACCESS
            # irepl -R f foo
            self.admin.assert_icommand(['irepl', '-R', self.leaf_rescs['f']['name'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R e foo
            self.admin.assert_icommand(['irepl', '-R', self.parent_rescs['e'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'd;e' foo
            hier = ';'.join([self.parent_rescs['d'], self.parent_rescs['e']])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'd;e;f' foo
            hier = ';'.join([hier, self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -R 'e;f' foo
            hier = ';'.join(hier.split(';')[1:-1])
            self.admin.assert_icommand(['irepl', '-R', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S f foo
            self.admin.assert_icommand(['irepl', '-S', self.leaf_rescs['f']['name'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S e foo
            self.admin.assert_icommand(['irepl', '-S', self.parent_rescs['e'], self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'd;e' foo
            hier = ';'.join([self.parent_rescs['d'], self.parent_rescs['e']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'd;e;f' foo
            hier = ';'.join([hier, self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # irepl -S 'e;f' foo
            hier = ';'.join([self.parent_rescs['e'], self.leaf_rescs['f']['name']])
            self.admin.assert_icommand(['irepl', '-S', hier, self.logical_path],
                'STDERR', 'DIRECT_CHILD_ACCESS')

            # ensure no replications took place
            out, _, _ = self.admin.run_icommand(['ils', '-l', self.logical_path])
            print(out)
            for resc in self.leaf_rescs.values()[1:]:
                self.assertNotIn(resc['name'], out, msg='found unwanted replica on [{}]'.format(resc['name']))
            for resc in self.parent_rescs.values():
                self.assertNotIn(resc, out, msg='found replica on coordinating resc [{}]'.format(resc))

        finally:
            self.admin.assert_icommand(['irm', '-rf', os.path.dirname(self.logical_path)])

    def setup_replicas(self, scenario):
        initial_replicas = scenario['start']
        for repl in initial_replicas:
            if initial_replicas[repl] != '-' and repl != 'a':
                # stage replicas
                self.admin.assert_icommand(['irepl', '-R', self.leaf_rescs[repl]['name'], self.logical_path])

        for repl in initial_replicas:
            if initial_replicas[repl] != '-':
                # set replica status for scenario
                self.admin.assert_icommand(
                    ['iadmin', 'modrepl', 'logical_path', self.logical_path, 'resource_hierarchy', self.leaf_rescs[repl]['name'],
                        'DATA_REPL_STATUS', str(self.repl_statuses.index(initial_replicas[repl]))])
            elif repl == 'a':
                # the replica status is to not have a replica - trim it
                self.admin.assert_icommand(['itrim', '-N1', '-S', self.leaf_rescs[repl]['name'], self.logical_path], 'STDOUT', 'files trimmed')

    def assert_result(self, scenario, out=None, err=None, rc=None):
        result = scenario['output']
        scenario_string = scenario['start']
        try:
            if result['out']: self.assertIn(result['out'], out)
            elif out: self.fail('found stdout:[{0}], expected None for scenario [{1}]'.format(out, scenario_string))
        except TypeError:
            self.fail('found stdout:[None], expected [{0}] for scenario [{1}]'.format(result['stdout'], scenario_string))

        try:
            if result['err']: self.assertIn(result['err'], err)
            elif err: self.fail('found stderr:[{0}], expected None for scenario [{1}]'.format(err, scenario_string))
        except TypeError:
            self.fail('found stderr:[None], expected [{0}] for scenario [{1}]'.format(result['err'], scenario_string))

        try:
            if result['rc']: self.assertEqual(result['rc'], rc)
            elif rc: self.fail('found rc:[{0}], expected None for scenario [{1}]'.format(rc, scenario_string))
        except TypeError:
            self.fail('found rc:[None], expected [{0}] for scenario [{1}]'.format(result['rc'], scenario_string))

        ils_out,ils_err,_ = self.admin.run_icommand(['ils', '-l', self.logical_path])
        for repl in scenario['end']:
            expected_status = scenario['end'][repl]

            if expected_status not in self.repl_statuses:
                self.assertNotIn(self.leaf_rescs[repl]['name'], ils_out)
                continue

            expected_results = [self.leaf_rescs[repl]['name'], expected_status]
            result = command.check_command_output(
                None, ils_out, ils_err, check_type='STDOUT_SINGLELINE', expected_results=expected_results)

    def run_command_against_scenarios(self, command, scenarios, name, file_size=1024):
        i = 0
        for scenario in scenarios:
            i = i + 1
            print("============= (" + name + "): [" + str(i) + "] =============")
            print(scenario)
            try:
                if not os.path.exists(self.local_path):
                    lib.make_file(self.local_path, file_size)
                self.admin.assert_icommand(['imkdir', os.path.dirname(self.logical_path)])
                self.admin.assert_icommand(['iput', self.local_path, self.logical_path])

                self.setup_replicas(scenario)
                out,err,_ = self.admin.run_icommand(command)
                print(out)
                print(err)
                self.assert_result(scenario, err=err)

            finally:
                self.admin.assert_icommand(['irm', '-rf', os.path.dirname(self.logical_path)])

    def test_irepl_Ra_foo(self):
        # irepl foo (irepl -R a foo) (default resource case)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(['irepl', self.logical_path], scenarios, 'irepl foo')

    def test_irepl_Rb_foo(self):
        # irepl -R b foo (source unspecified, destination b)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(
            ['irepl', '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -R b foo')

    def test_irepl_Sa_Rb_foo(self):
        # irepl -S a -R b foo (source default resource, destination non-default resource)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(
            ['irepl', '-S', self.leaf_rescs['a']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S a -R b foo')

    def test_irepl_Sc_Rb_foo(self):
        # irepl -S c -R b foo (source non-default resource, destination non-default resource)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'-', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'-', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'X', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'X', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(
            ['irepl', '-S', self.leaf_rescs['c']['name'], '-R', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S c -R b foo')

        # TODO: replica numbers seme to change... might need to detect in special case?
        #self.run_command_against_scenarios(
            #['irepl', '-n', '2', '-R', self.leaf_rescs['b']['name'], self.logical_path],
            #scenarios, 'irepl -n 2 -R b foo')

    def test_irepl_Sb_foo(self):
        # irepl -S b foo (irepl -S b -R a foo) (source non-default resource, destination unspecified)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'-', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'-', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: warning?
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'X', 'b':'-', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'-'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'X'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'X', 'b':'X', 'c':'&'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(
            ['irepl', '-S', self.leaf_rescs['b']['name'], self.logical_path],
            scenarios, 'irepl -S b foo')

        # TODO: replica numbers seme to change... might need to detect in special case?
        #self.run_command_against_scenarios(
            #['irepl', '-n', '1', self.logical_path],
            #scenarios, 'irepl -n 1 foo')

    def test_irepl_a_foo(self):
        # irepl -a foo (source unspecified, destination ALL)
        scenarios = [
            #{'start':{'a':'-', 'b':'-', 'c':'-'}, 'end':{'a':'-', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'-', 'c':'X'}, 'end':{'a':'-', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'-'}, 'end':{'a':'-', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'-', 'b':'X', 'c':'X'}, 'end':{'a':'-', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'-', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}}, # TODO: timestamp?
            {'start':{'a':'&', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'&', 'b':'X', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'-'}, 'end':{'a':'X', 'b':'-', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'-', 'c':'X'}, 'end':{'a':'X', 'b':'-', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'-'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'&', 'c':'X'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'-'}, 'end':{'a':'X', 'b':'X', 'c':'-'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'&'}, 'end':{'a':'&', 'b':'&', 'c':'&'}, 'output':{'out':None, 'err':None, 'rc':None}},
            {'start':{'a':'X', 'b':'X', 'c':'X'}, 'end':{'a':'X', 'b':'X', 'c':'X'}, 'output':{'out':None, 'err':'SYS_NO_GOOD_REPLICA', 'rc':None}}
        ]

        self.run_command_against_scenarios(['irepl', '-a', self.logical_path], scenarios, 'irepl -a foo')
