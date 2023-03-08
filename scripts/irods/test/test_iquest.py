import os
import sys
import shutil
import socket
import re

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from ..configuration import IrodsConfig
from .resource_suite import ResourceBase

def create_large_hierarchy(self, count, hostname, directory):
    self.admin.assert_icommand("iadmin mkresc repRescPrime replication", 'STDOUT_SINGLELINE', 'replication')
    for i in range(count):
        self.admin.assert_icommand("iadmin mkresc repResc{0} replication".format(i), 'STDOUT_SINGLELINE', 'replication')
        self.admin.assert_icommand("iadmin mkresc ptResc{0} passthru".format(i), 'STDOUT_SINGLELINE', 'passthru')
        self.admin.assert_icommand("iadmin mkresc ufs{0} unixfilesystem ".format(i) + hostname + ":" + directory + "/ufs{0}Vault".format(i), 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.admin.assert_icommand("iadmin addchildtoresc repResc{0} ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin addchildtoresc ptResc{0} ufs{0}".format(i))
        self.admin.assert_icommand("iadmin addchildtoresc repRescPrime repResc{0}".format(i))

def remove_large_hierarchy(self, count, directory):
    for i in range(count):
        self.admin.assert_icommand("iadmin rmchildfromresc repRescPrime repResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmchildfromresc ptResc{0} ufs{0}".format(i))
        self.admin.assert_icommand("iadmin rmchildfromresc repResc{0} ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc repResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc ptResc{0}".format(i))
        self.admin.assert_icommand("iadmin rmresc ufs{0}".format(i))
        shutil.rmtree(directory + "/ufs{0}Vault".format(i), ignore_errors=True)
    self.admin.assert_icommand("iadmin rmresc repRescPrime")

class Test_Iquest(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_Iquest, self).setUp()

    def tearDown(self):
        super(Test_Iquest, self).tearDown()

    def test_iquest_MAX_SQL_ROWS_results__3262(self):
        MAX_SQL_ROWS = 256 # needs to be the same as constant in server code
        filename = 'test_iquest_MAX_SQL_ROWS_results__3262'
        lib.make_file(filename, 1)
        data_object_prefix = 'loiuaxnlaskdfpiewrnsliuserd'
        for i in range(MAX_SQL_ROWS):
            self.admin.assert_icommand(['iput', filename, '{0}_file_{1}'.format(data_object_prefix, i)])

        self.admin.assert_icommand(['iquest', "select count(DATA_ID) where DATA_NAME like '{0}%'".format(data_object_prefix)], 'STDOUT_SINGLELINE', 'DATA_ID = {0}'.format(MAX_SQL_ROWS))
        self.admin.assert_icommand_fail(['iquest', '--no-page', "select DATA_ID where DATA_NAME like '{0}%'".format(data_object_prefix)], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')

    def test_iquest_with_data_resc_hier__3705(self):
        filename = 'test_iquest_data_name_with_data_resc_hier__3705'
        lib.make_file(filename, 1)
        self.admin.assert_icommand(['iput', filename])

        # Make sure DATA_RESC_HIER appears
        self.admin.assert_icommand(['iquest', "select DATA_NAME, DATA_RESC_HIER where DATA_RESC_HIER = '{0}'".format(self.admin.default_resource)], 'STDOUT_SINGLELINE', 'DATA_RESC_HIER')

        # Make sure DATA_RESC_ID appears
        self.admin.assert_icommand(['iquest', "select DATA_RESC_ID where DATA_RESC_HIER = '{0}'".format(self.admin.default_resource)], 'STDOUT_SINGLELINE', 'DATA_RESC_ID')

        # Make sure HIER and ID appear in the correct order
        _,out,_ = self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER, DATA_RESC_ID where DATA_NAME = '{0}' and DATA_RESC_HIER = '{1}'".format(filename, self.admin.default_resource)], 'STDOUT_SINGLELINE')
        self.assertTrue("DATA_RESC_HIER" in out.split('\n')[0])
        self.assertTrue("DATA_RESC_ID" in out.split('\n')[1])

        os.remove(filename)

    def test_iquest_resc_hier_with_like__3714(self):
        # Create a hierarchy to test
        LEAF_COUNT = 10
        directory = IrodsConfig().irods_directory
        create_large_hierarchy(self, LEAF_COUNT, socket.gethostname(), directory)
        filename = 'test_iquest_resc_hier_with_like__3714'
        lib.make_file(filename, 1)
        self.admin.assert_icommand("iput -R repRescPrime " + filename)
        expected_hier = 'repRescPrime;repResc3;ptResc3;ufs3'

        # SUCCESS
        # Matches ufs3's hierarchy
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '%ufs3'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches all hierarchies starting with repRescPrime
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'repRescPrime;%'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches all hierarchies containing resource ptResc3
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '%;ptResc3;%'"], 'STDOUT_SINGLELINE', expected_hier)
        # Matches leaf resource from full hierarchy
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = '{0}'".format(expected_hier)], 'STDOUT_SINGLELINE', expected_hier)
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like '{0}'".format(expected_hier)], 'STDOUT_SINGLELINE', expected_hier)

        # FAILURE
        # % needed at front - ufs is not a root
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'ufs%'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # % needed at end - repRescPrime has children
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'repRescPrime'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # = with % results in error
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = '%ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        # Results in direct match - must match full hier
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER = 'ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')
        self.admin.assert_icommand(['iquest', "select DATA_RESC_HIER where DATA_RESC_HIER like 'ufs3'"], 'STDOUT_SINGLELINE', 'CAT_NO_ROWS_FOUND')

        # Clean up
        os.remove(filename)
        self.admin.assert_icommand("irm -f " + filename)
        remove_large_hierarchy(self, LEAF_COUNT, directory)

    def test_iquest_matches_names_containing_apostrophes_via_the_equals_operator__issue_4887(self):
        data_object = "data' object"
        self.admin.assert_icommand(['istream', 'write', data_object], input='hello, world')
        self.admin.assert_icommand(['iquest', "select DATA_NAME where DATA_NAME = '{0}'".format(data_object)], 'STDOUT', ['DATA_NAME = ' + data_object])

        collection = os.path.join(self.admin.session_collection, "coll ect'ion")
        self.admin.assert_icommand(['imkdir', collection])
        self.admin.assert_icommand(['iquest', "select COLL_NAME where COLL_NAME = '{0}'".format(collection)], 'STDOUT', ['COLL_NAME = ' + collection])

    def test_iquest_does_not_fail_when_querying_for_ticket_create_time_and_modify_time__issue_5929(self):
        data_object = 'foo.issue_5929'
        self.admin.assert_icommand(['itouch', data_object])
        self.admin.assert_icommand(['iticket', 'create', 'read', data_object], 'STDOUT', ['ticket:'])

        # Fetch the create time and modify time for the recently created ticket.
        gql = "select TICKET_CREATE_TIME, TICKET_MODIFY_TIME where TICKET_DATA_NAME = '{0}'".format(data_object)
        self.admin.assert_icommand(['iquest', gql], 'STDOUT', ['TICKET_CREATE_TIME = ', 'TICKET_MODIFY_TIME = '])

        # Make sure the values represent iRODS timestamps.
        ec, out, err = self.admin.assert_icommand(['iquest', '%s,%s', gql], 'STDOUT', [','])
        self.assertEqual(ec, 0)

        err = err.strip()
        self.assertEqual(len(err), 0)

        out = out.strip()
        timestamps = out.split(',')
        self.assertTrue(re.match('^\d{11,11}$', timestamps[0]))
        self.assertTrue(re.match('^\d{11,11}$', timestamps[1]))

    def test_iquest_totaldatasize(self):
        self.admin.assert_icommand("iquest \"select sum(DATA_SIZE) where COLL_NAME like '/" +
                                   self.admin.zone_name + "/home/%'\"", 'STDOUT_SINGLELINE', "DATA_SIZE")  # selects total data size

    def test_iquest_bad_format(self):
        self.admin.assert_icommand("iquest \"bad formatting\"", 'STDERR_SINGLELINE',
                                   "INPUT_ARG_NOT_WELL_FORMED_ERR")  # bad request

    def test_iquest_incorrect_format_count(self):
        self.admin.assert_icommand("iquest \"%s %s\" \"select COLL_NAME where COLL_NAME like '%home%'\"",
                                   'STDERR_SINGLELINE', 'boost::too_few_args: format-string referred to more arguments than were passed')


class test_iquest_with_data_resc_hier(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.user = session.mkuser_and_return_session('rodsuser', 'alice', 'apass', lib.get_hostname())

        self.root_resource = 'root'
        self.mid_resource = 'mid'
        self.leaf_resource = 'leaf'
        self.data_name = 'foo'
        self.logical_path = os.path.join(self.user.session_collection, self.data_name)
        self.hierarchy = ';'.join([self.root_resource, self.mid_resource, self.leaf_resource])

        with session.make_session_for_existing_admin() as admin_session:
            lib.create_passthru_resource(self.root_resource, admin_session)
            lib.create_passthru_resource(self.mid_resource, admin_session)
            lib.create_ufs_resource(self.leaf_resource, admin_session)
            lib.add_child_resource(self.root_resource, self.mid_resource, admin_session)
            lib.add_child_resource(self.mid_resource, self.leaf_resource, admin_session)

        self.user.assert_icommand(['itouch', '-R', self.leaf_resource, self.logical_path])


    @classmethod
    def tearDownClass(self):
        self.user.assert_icommand(['irm', '-f', self.logical_path])

        with session.make_session_for_existing_admin() as admin_session:
            self.user.__exit__()
            admin_session.run_icommand(['iadmin', 'rmuser', 'alice'])
            lib.remove_child_resource(self.mid_resource, self.leaf_resource, admin_session)
            lib.remove_child_resource(self.root_resource, self.mid_resource, admin_session)
            lib.remove_resource(self.leaf_resource, admin_session)
            lib.remove_resource(self.mid_resource, admin_session)
            lib.remove_resource(self.root_resource, admin_session)


    def test_invalid_operations(self):
        pattern = self.hierarchy
        invalid_operations = [
            '',
            '==',
            'lik',
            'like not like',
            'not lik',
            'not like like',
            'not not like',
            'not',
            'notlike',
            'ot like'
        ]

        for op in invalid_operations:
            query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER {} \'{}\''.format(self.data_name, op, pattern)
            self.user.assert_icommand(['iquest', '%s', query], 'STDERR', 'CAT_INVALID_ARGUMENT')


    def test_valid_operations_with_matching_glob(self):
        pattern = self.root_resource + '%'
        operations_to_expected_result = {
            'like': self.hierarchy,
            'LIKE': self.hierarchy,
            'not like': 'CAT_NO_ROWS_FOUND',
            'not LIKE': 'CAT_NO_ROWS_FOUND',
            'NOT like': 'CAT_NO_ROWS_FOUND',
            'NOT LIKE': 'CAT_NO_ROWS_FOUND',
            '=': 'CAT_NO_ROWS_FOUND',
            '!=': self.hierarchy
        }

        for op in operations_to_expected_result:
            query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER {} \'{}\''.format(self.data_name, op, pattern)
            self.user.assert_icommand(['iquest', '%s', query], 'STDOUT', operations_to_expected_result[op])


    def test_valid_operations_with_nonmatching_glob(self):
        pattern = 'nope%'
        operations_to_expected_result = {
            'like': 'CAT_NO_ROWS_FOUND',
            'LIKE': 'CAT_NO_ROWS_FOUND',
            'not like': self.hierarchy,
            'not LIKE': self.hierarchy,
            'NOT like': self.hierarchy,
            'NOT LIKE': self.hierarchy,
            '=': 'CAT_NO_ROWS_FOUND',
            '!=': self.hierarchy
        }

        for op in operations_to_expected_result:
            query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER {} \'{}\''.format(self.data_name, op, pattern)
            self.user.assert_icommand(['iquest', '%s', query], 'STDOUT', operations_to_expected_result[op])


    def test_valid_operations_with_exact_match(self):
        pattern = self.hierarchy
        operations_to_expected_result = {
            'like': self.hierarchy,
            'LIKE': self.hierarchy,
            'not like': 'CAT_NO_ROWS_FOUND',
            'not LIKE': 'CAT_NO_ROWS_FOUND',
            'NOT like': 'CAT_NO_ROWS_FOUND',
            'NOT LIKE': 'CAT_NO_ROWS_FOUND',
            '=': self.hierarchy,
            '!=': 'CAT_NO_ROWS_FOUND'
        }

        for op in operations_to_expected_result:
            query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER {} \'{}\''.format(self.data_name, op, pattern)
            self.user.assert_icommand(['iquest', '%s', query], 'STDOUT', operations_to_expected_result[op])


    def test_valid_operations_with_no_match(self):
        pattern = ';'.join(['nope', self.hierarchy])
        operations_to_expected_result = {
            'like': 'CAT_NO_ROWS_FOUND',
            'LIKE': 'CAT_NO_ROWS_FOUND',
            'not like': self.hierarchy,
            'not LIKE': self.hierarchy,
            'NOT like': self.hierarchy,
            'NOT LIKE': self.hierarchy,
            '=': 'CAT_NO_ROWS_FOUND',
            '!=': self.hierarchy
        }

        for op in operations_to_expected_result:
            query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER {} \'{}\''.format(self.data_name, op, pattern)
            self.user.assert_icommand(['iquest', '%s', query], 'STDOUT', operations_to_expected_result[op])


class test_iquest_logical_or_operator_with_data_resc_hier(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.user = session.mkuser_and_return_session('rodsuser', 'alice', 'apass', lib.get_hostname())

        self.root_resource = 'root'
        self.mid_resource = 'mid'
        self.leaf_resource1 = 'leaf1'
        self.leaf_resource2 = 'leaf2'
        self.data_name = 'foo'
        self.logical_path = os.path.join(self.user.session_collection, self.data_name)
        self.hierarchy1 = ';'.join([self.root_resource, self.mid_resource, self.leaf_resource1])
        self.hierarchy2 = ';'.join([self.root_resource, self.mid_resource, self.leaf_resource2])

        with session.make_session_for_existing_admin() as admin_session:
            lib.create_passthru_resource(self.root_resource, admin_session)
            lib.create_replication_resource(self.mid_resource, admin_session)
            lib.create_ufs_resource(self.leaf_resource1, admin_session)
            lib.create_ufs_resource(self.leaf_resource2, admin_session)
            lib.add_child_resource(self.root_resource, self.mid_resource, admin_session)
            lib.add_child_resource(self.mid_resource, self.leaf_resource1, admin_session)
            lib.add_child_resource(self.mid_resource, self.leaf_resource2, admin_session)

        # This should trigger a replication to the other resource.
        self.user.assert_icommand(['itouch', '-R', self.leaf_resource1, self.logical_path])

        self.base_query = 'select DATA_RESC_HIER where DATA_NAME = \'{}\' and DATA_RESC_HIER'.format(self.data_name)


    @classmethod
    def tearDownClass(self):
        self.user.assert_icommand(['irm', '-f', self.logical_path])

        with session.make_session_for_existing_admin() as admin_session:
            self.user.__exit__()
            admin_session.run_icommand(['iadmin', 'rmuser', 'alice'])
            lib.remove_child_resource(self.mid_resource, self.leaf_resource2, admin_session)
            lib.remove_child_resource(self.mid_resource, self.leaf_resource1, admin_session)
            lib.remove_child_resource(self.root_resource, self.mid_resource, admin_session)
            lib.remove_resource(self.leaf_resource2, admin_session)
            lib.remove_resource(self.leaf_resource1, admin_session)
            lib.remove_resource(self.mid_resource, admin_session)
            lib.remove_resource(self.root_resource, admin_session)


    def run_logical_or_query_scenarios(self, scenarios):
        # Each of the scenarios being tested here is a list of lists of tuples. The tuples consist of an operation, a
        # condition, and an expected result set:
        #  - The operation is one of the GenQuery WHERE operations: LIKE, NOT LIKE, =, and !=
        #  - The condition is a string which will be paired with the operation, e.g. 'root;%'
        #  - The expected result set is a list of strings representing resource hierarchies which the query will return
        #
        # Here is an example scenario:
        #   [('LIKE', '%;leaf1', [self.hierarchy1]),
        #    ('NOT LIKE', '%;leaf1', [self.hierarchy2])]
        #
        # This will result in a clause for the GenQuery that looks like this:
        #   LIKE '%;leaf1' || NOT LIKE '%;leaf2'
        #
        # The clause will then be put into a GenQuery string run by iquest, which will look like this:
        #   iquest "%s" "select DATA_RESC_HIER where DATA_NAME = 'foo' and DATA_RESC_HIER LIKE '%;leaf1' || NOT LIKE '%;leaf2'"
        #
        # The expected result set is {self.hierarchy1, self.hierarchy2} because the first condition expects
        # self.hierarchy1 and the second condition expects self.hierarchy2. The || operator combines the expected
        # result sets.
        i = 0
        for scenario in scenarios:
            i = i + 1

            clauses = []
            expected_result_set = set()
            for clause_tuple in scenario:
                operator = clause_tuple[0]
                condition = clause_tuple[1]
                clauses.append('{} \'{}\''.format(operator, condition))

                for expected_result in clause_tuple[2]:
                    expected_result_set.add(expected_result)

            query = self.base_query + ' ' + ' || '.join(clauses)

            print("============= (" + query + "): [" + str(i) + "] =============")
            print(scenario)

            out,err,_ = self.user.run_icommand(['iquest', '%s', query])
            print(out)
            print(err)

            if 0 != len(expected_result_set):
                for result in expected_result_set:
                    self.assertIn(result, out)
            else:
                self.assertIn('CAT_NO_ROWS_FOUND', out)


    def test_like_like(self):
        op1 = 'LIKE'
        op2 = 'LIKE'
        scenarios = [
            [(op1, 'nope;%', []),                                                           (op2, 'nope;%', [])],
            [(op1, 'nope;%', []),                                                           (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                           (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                           (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_like_not_like(self):
        op1 = 'LIKE'
        op2 = 'NOT LIKE'
        scenarios = [
            [(op1, 'nope;%', []),                                                           (op2, '{};%'.format(self.root_resource), [])],
            [(op1, 'nope;%', []),                                                           (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                           (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                           (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_like_equals(self):
        op1 = 'LIKE'
        op2 = '='
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        scenarios = [
            [(op1, 'nope;%', []),                                                           (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', []),                                                           (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                           (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                           (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, 'nope;%', [])],
        #   [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, 'nope;%', [])],
        #   [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, self.hierarchy2, [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_like_not_equals(self):
        op1 = 'LIKE'
        op2 = '!='
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                                           (op2, 'nope;%', [])],
            [(op1, 'nope;%', []),                                                           (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                           (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, 'nope;%', []),                                                           (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op1, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy1]),                  (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy2]),                  (op2, self.hierarchy2, [self.hierarchy1])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_like_like(self):
        op1 = 'NOT LIKE'
        op2 = 'LIKE'
        scenarios = [
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, 'nope;%', [])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, 'nope;%', [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_like_not_like(self):
        op1 = 'NOT LIKE'
        op2 = 'NOT LIKE'
        scenarios = [
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '{};%'.format(self.root_resource), [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_like_equals(self):
        op1 = 'NOT LIKE'
        op2 = '='
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        scenarios = [
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, 'nope;%', [])],
        #   [(op1, '{};%'.format(self.root_resource), []),                  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, 'nope;%', [])],
        #   [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, 'nope;%', [])],
        #   [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_like_not_equals(self):
        op1 = 'NOT LIKE'
        op2 = '!='
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, '{};%'.format(self.root_resource), []),                  (op2, 'nope;%', [])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '{};%'.format(self.root_resource), []),                  (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, 'nope;%', [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),           (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource2), [self.hierarchy1]),  (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, 'nope;%', [])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, '%;{}'.format(self.leaf_resource1), [self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy1])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_equals_like(self):
        op1 = '='
        op2 = 'LIKE'
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        scenarios = [
            [(op1, 'nope;%', []),                                                   (op2, 'nope;%', [])],
            [(op1, 'nope;%', []),                                                   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, 'nope;%', [])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, 'nope;%', [])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_equals_not_like(self):
        op1 = '='
        op2 = 'NOT LIKE'
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        scenarios = [
            [(op1, 'nope;%', []),                                                   (op2, '{};%'.format(self.root_resource), [])],
            [(op1, 'nope;%', []),                                                   (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '{};%'.format(self.root_resource), [])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '{};%'.format(self.root_resource), [])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_equals_equals(self):
        op1 = '='
        op2 = '='
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        scenarios = [
            [(op1, 'nope;%', []),                                                   (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', []),                                                   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                   (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, 'nope;%', []),                                                   (op2, self.hierarchy2, [self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy1])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, 'nope;%', [])],
        #   [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, 'nope;%', [])],
        #   [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, self.hierarchy2, [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_equals_not_equals(self):
        op1 = '='
        op2 = '!='
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                                   (op2, 'nope;%', [])],
            [(op1, 'nope;%', []),                                                   (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', []),                                                   (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, 'nope;%', []),                                                   (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy1, [self.hierarchy2])],
        #   [(op1, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2]),  (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, 'nope;%', [])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy1]),                             (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, 'nope;%', [])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy2]),                             (op2, self.hierarchy2, [self.hierarchy1])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_equals_like(self):
        op1 = '!='
        op2 = 'LIKE'
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                   (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', []),                                   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
        #   [(op1, 'nope;%', []),                                   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
        #   [(op1, 'nope;%', []),                                   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, 'nope;%', [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, 'nope;%', [])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, 'nope;%', [])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_equals_not_like(self):
        op1 = '!='
        op2 = 'NOT LIKE'
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                   (op2, '{};%'.format(self.root_resource), [])],
        #   [(op1, 'nope;%', []),                                   (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
        #   [(op1, 'nope;%', []),                                   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
        #   [(op1, 'nope;%', []),                                   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '{};%'.format(self.root_resource), [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '{};%'.format(self.root_resource), [])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '{};%'.format(self.root_resource), [])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '%;{}'.format(self.leaf_resource2), [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '%;{}'.format(self.leaf_resource1), [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_equals_equals(self):
        op1 = '!='
        op2 = '='
        # All cases where multiple matches are expected with = have been skipped. These are not valid scenarios for = operator.
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                   (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', []),                                   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
        #   [(op1, 'nope;%', []),                                   (op2, self.hierarchy1, [self.hierarchy1])],
        #   [(op1, 'nope;%', []),                                   (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, 'nope;%', [])],
        #   [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, self.hierarchy2, [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, 'nope;%', [])],
        #   [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, self.hierarchy1, [self.hierarchy1])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, self.hierarchy2, [self.hierarchy2])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_not_equals_not_equals(self):
        op1 = '!='
        op2 = '!='
        # All cases where no matches are expected with != have been skipped. These are not valid scenarios for != operator.
        scenarios = [
        #   [(op1, 'nope;%', []),                                   (op2, 'nope;%', [])],
        #   [(op1, 'nope;%', []),                                   (op2, '{};%'.format(self.root_resource), [self.hierarchy1, self.hierarchy2])],
        #   [(op1, 'nope;%', []),                                   (op2, self.hierarchy1, [self.hierarchy1])],
        #   [(op1, 'nope;%', []),                                   (op2, self.hierarchy2, [self.hierarchy2])],
        #   [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, 'nope;%', [])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, 'nope;%', [self.hierarchy1, self.hierarchy2]),   (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, 'nope;%', [])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, self.hierarchy1, [self.hierarchy2]),             (op2, self.hierarchy2, [self.hierarchy1])],
        #   [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, 'nope;%', [])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, 'nope;%', [self.hierarchy1, self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, self.hierarchy1, [self.hierarchy2])],
            [(op1, self.hierarchy2, [self.hierarchy1]),             (op2, self.hierarchy2, [self.hierarchy1])],
        ]

        self.run_logical_or_query_scenarios(scenarios)


    def test_invalid_operations(self):
        pattern = self.hierarchy1
        valid_operations = [
            'like',
            'LIKE',
            'not like',
            'not LIKE',
            'NOT like',
            'NOT LIKE',
            '=',
            '!='
        ]

        invalid_operations = [
            '',
            '==',
            'lik',
            'like not like',
            'not lik',
            'not like like',
            'not not like',
            'not',
            'notlike',
            'ot like'
        ]

        for iop in invalid_operations:
            for vop in valid_operations:
                # First, with the invalid operation as the first clause of the ||...
                query = '{} {} \'{}\' || {} \'{}\''.format(self.base_query, iop, pattern, vop, pattern)
                self.user.assert_icommand(['iquest', '%s', query], 'STDERR', 'CAT_INVALID_ARGUMENT')

                # Now, the valid operation as the first clause of the || and the invalid operation as the second.
                query = '{} {} \'{}\' || {} \'{}\''.format(self.base_query, vop, pattern, iop, pattern)
                self.user.assert_icommand(['iquest', '%s', query], 'STDERR', 'CAT_INVALID_ARGUMENT')
