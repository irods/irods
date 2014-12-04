import re
import sys
if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest
from resource_suite import ResourceBase
from pydevtest_common import assertiCmd, assertiCmdFail
import pydevtest_sessions as s


def set_avu(user_name, a, v, u):
    assertiCmd(s.adminsession, 'imeta set -u %s %s %s %s' % (user_name, a, v, u))


def add_avu(user_name, a, v, u):
    assertiCmd(s.adminsession, 'imeta add -u %s %s %s %s' % (user_name, a, v, u))


def check_avu(user_name, a, v, u):
    # If setting unit to empty string then ls output should be blank
    if u == '""':
        u = ''

    a = re.escape(a)
    v = re.escape(v)
    u = re.escape(u)

    assertiCmd(s.adminsession, 'imeta ls -u %s %s' % (user_name, a), 'STDOUT_MULTILINE', ['attribute: ' + a + '$',
                                                                                          'value: ' + v + '$',
                                                                                          'units: ' + u + '$'],
               use_regex=True)


def set_and_check_avu(user_name, a, v, u):
    set_avu(user_name, a, v, u)
    check_avu(user_name, a, v, u)


def add_and_check_avu(user_name, a, v, u):
    add_avu(user_name, a, v, u)
    check_avu(user_name, a, v, u)


class Test_iMetaSet(unittest.TestCase, ResourceBase):

    my_test_resource = {'setup': [], 'teardown': []}

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

        user_names = [d.get('name') for d in s.users]
        assertiCmd(s.adminsession, 'iadmin lu', 'STDOUT_MULTILINE', user_names)

        for u in user_names:
            assertiCmd(s.adminsession, 'imeta ls -u ' + u, 'STDOUT', 'None')

    def tearDown(self):
        user_names = [d.get('name') for d in s.users]

        for u in user_names:
            s.adminsession.runCmd('imeta', ['rmw', '-u', u, '%', '%', '%'])
        self.run_resource_teardown()
        s.twousers_down()

    def test_imeta_set_single_object_triple(self, user=None):
        if user is None:
            user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', 'unt0')
        set_and_check_avu(user, 'att0', 'val1', 'unt1')

    def test_imeta_set_single_object_double(self, user=None):
        if user is None:
            user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', '')
        set_and_check_avu(user, 'att0', 'val1', '')

    def test_imeta_set_single_object_double_to_triple(self, user=None):
        if user is None:
            user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', '')
        set_and_check_avu(user, 'att0', 'val1', 'unt1')

    def test_imeta_set_single_object_triple_to_double_no_unit(self, user=None):
        if user is None:
            user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', 'unt0')
        set_and_check_avu(user, 'att0', 'val1', '')

        assertiCmdFail(s.adminsession, 'imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_triple_to_double_empty_unit(self, user=None):
        if user is None:
            user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', 'unt0')
        set_and_check_avu(user, 'att0', 'val1', '""')

        assertiCmdFail(s.adminsession, 'imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_multi_object_triple(self):
        user1 = s.users[1]['name']
        user2 = s.users[2]['name']

        self.test_imeta_set_single_object_triple(user=user1)
        self.test_imeta_set_single_object_triple(user=user2)

    def test_imeta_set_multi_object_double(self):
        user1 = s.users[1]['name']
        user2 = s.users[2]['name']

        self.test_imeta_set_single_object_double(user=user1)
        self.test_imeta_set_single_object_double(user=user2)

    def test_imeta_set_multi_object_double_to_triple(self):
        user1 = s.users[1]['name']
        user2 = s.users[2]['name']

        self.test_imeta_set_single_object_double_to_triple(user=user1)
        self.test_imeta_set_single_object_double_to_triple(user=user2)

    def test_imeta_set_multi_object_triple_to_double_no_unit(self):
        user1 = s.users[1]['name']
        user2 = s.users[2]['name']

        self.test_imeta_set_single_object_triple_to_double_no_unit(user=user1)
        self.test_imeta_set_single_object_triple_to_double_no_unit(user=user2)

    def test_imeta_set_multi_object_triple_to_double_empty_unit(self):
        user1 = s.users[1]['name']
        user2 = s.users[2]['name']

        self.test_imeta_set_single_object_triple_to_double_empty_unit(user=user1)
        self.test_imeta_set_single_object_triple_to_double_empty_unit(user=user2)

    def test_imeta_set_single_object_abandoned_avu_triple_to_double_no_unit(self):
        user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', 'unt0')

        assertiCmd(s.adminsession, 'imeta rm -u %s %s %s %s' % (user, 'att0', 'val0', 'unt0'))

        set_and_check_avu(user, 'att0', 'val0', '')

        assertiCmdFail(s.adminsession, 'imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_abandoned_avu_triple_to_double_empty_unit(self):
        user = s.users[1]['name']

        set_and_check_avu(user, 'att0', 'val0', 'unt0')

        assertiCmd(s.adminsession, 'imeta rm -u %s %s %s %s' % (user, 'att0', 'val0', 'unt0'))

        set_and_check_avu(user, 'att0', 'val0', '""')

        assertiCmdFail(s.adminsession, 'imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_multi_avu_removal(self):
        user = s.users[1]['name']

        original_avus = [('att' + str(i), 'val' + str(i), 'unt' + str(i)) for i in range(30)]

        for avu in original_avus:
            add_and_check_avu(user, *avu)

        set_and_check_avu(user, 'att_new', 'val_new', 'unt_new')

        for a, v, u in original_avus:
            assertiCmdFail(s.adminsession, 'imeta ls -u %s %s' % (user, a), 'STDOUT', ['attribute: ' + a + '$'])
            assertiCmdFail(s.adminsession, 'imeta ls -u %s %s' % (user, a), 'STDOUT', ['value: ' + v + '$'])
            assertiCmdFail(s.adminsession, 'imeta ls -u %s %s' % (user, a), 'STDOUT', ['units:' + u + '$'])
