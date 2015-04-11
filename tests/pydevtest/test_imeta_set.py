import itertools
import re
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from resource_suite import ResourceBase


class Test_iMetaSet(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iMetaSet, self).setUp()

        usernames = [s.username for s in itertools.chain(self.admin_sessions, self.user_sessions)]
        self.admin.assert_icommand('iadmin lu', 'STDOUT_MULTILINE', usernames)

        for u in usernames:
            self.admin.assert_icommand('imeta ls -u ' + u, 'STDOUT', 'None')

    def tearDown(self):
        usernames = [s.username for s in itertools.chain(self.admin_sessions, self.user_sessions)]

        for u in usernames:
            self.admin.run_icommand(['imeta', 'rmw', '-u', u, '%', '%', '%'])

        super(Test_iMetaSet, self).tearDown()

    def set_avu(self, user_name, a, v, u):
        self.admin.assert_icommand('imeta set -u %s %s %s %s' % (user_name, a, v, u))

    def add_avu(self, user_name, a, v, u):
        self.admin.assert_icommand('imeta add -u %s %s %s %s' % (user_name, a, v, u))

    def check_avu(self, user_name, a, v, u):
        # If setting unit to empty string then ls output should be blank
        if u == '""':
            u = ''

        a = re.escape(a)
        v = re.escape(v)
        u = re.escape(u)

        self.admin.assert_icommand('imeta ls -u %s %s' % (user_name, a), 'STDOUT_MULTILINE', ['attribute: ' + a + '$',
                                                                                          'value: ' + v + '$',
                                                                                          'units: ' + u + '$'],
                   use_regex=True)

    def set_and_check_avu(self, user_name, a, v, u):
        self.set_avu(user_name, a, v, u)
        self.check_avu(user_name, a, v, u)

    def add_and_check_avu(self, user_name, a, v, u):
        self.add_avu(user_name, a, v, u)
        self.check_avu(user_name, a, v, u)

    def test_imeta_set_single_object_triple(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.set_and_check_avu(user, 'att0', 'val1', 'unt1')

    def test_imeta_set_single_object_double(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', '')
        self.set_and_check_avu(user, 'att0', 'val1', '')

    def test_imeta_set_single_object_double_to_triple(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', '')
        self.set_and_check_avu(user, 'att0', 'val1', 'unt1')

    def test_imeta_set_single_object_triple_to_double_no_unit(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.set_and_check_avu(user, 'att0', 'val1', '')

        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_triple_to_double_empty_unit(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.set_and_check_avu(user, 'att0', 'val1', '""')

        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_multi_object_triple(self):
        user1 = self.user0.username
        user2 = self.user1.username

        self.test_imeta_set_single_object_triple(user=user1)
        self.test_imeta_set_single_object_triple(user=user2)

    def test_imeta_set_multi_object_double(self):
        user1 = self.user0.username
        user2 = self.user1.username

        self.test_imeta_set_single_object_double(user=user1)
        self.test_imeta_set_single_object_double(user=user2)

    def test_imeta_set_multi_object_double_to_triple(self):
        user1 = self.user0.username
        user2 = self.user1.username

        self.test_imeta_set_single_object_double_to_triple(user=user1)
        self.test_imeta_set_single_object_double_to_triple(user=user2)

    def test_imeta_set_multi_object_triple_to_double_no_unit(self):
        user1 = self.user0.username
        user2 = self.user1.username

        self.test_imeta_set_single_object_triple_to_double_no_unit(user=user1)
        self.test_imeta_set_single_object_triple_to_double_no_unit(user=user2)

    def test_imeta_set_multi_object_triple_to_double_empty_unit(self):
        user1 = self.user0.username
        user2 = self.user1.username

        self.test_imeta_set_single_object_triple_to_double_empty_unit(user=user1)
        self.test_imeta_set_single_object_triple_to_double_empty_unit(user=user2)

    def test_imeta_set_single_object_abandoned_avu_triple_to_double_no_unit(self):
        user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.admin.assert_icommand('imeta rm -u %s %s %s %s' % (user, 'att0', 'val0', 'unt0'))
        self.set_and_check_avu(user, 'att0', 'val0', '')
        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_abandoned_avu_triple_to_double_empty_unit(self):
        user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.admin.assert_icommand('imeta rm -u %s %s %s %s' % (user, 'att0', 'val0', 'unt0'))
        self.set_and_check_avu(user, 'att0', 'val0', '""')
        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT', 'units: unt0')

    def test_imeta_set_single_object_multi_avu_removal(self):
        user = self.user0.username

        original_avus = [('att' + str(i), 'val' + str(i), 'unt' + str(i)) for i in range(30)]

        for avu in original_avus:
            self.add_and_check_avu(user, *avu)

        self.set_and_check_avu(user, 'att_new', 'val_new', 'unt_new')

        for a, v, u in original_avus:
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT', ['attribute: ' + a + '$'])
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT', ['value: ' + v + '$'])
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT', ['units:' + u + '$'])
