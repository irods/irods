import itertools
import re
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .resource_suite import ResourceBase


class Test_ImetaSet(ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_ImetaSet, self).setUp()

        usernames = [s.username for s in itertools.chain(self.admin_sessions, self.user_sessions)]
        self.admin.assert_icommand('iadmin lu', 'STDOUT_MULTILINE', usernames)

        for u in usernames:
            self.admin.assert_icommand('imeta ls -u ' + u, 'STDOUT_SINGLELINE', 'None')

    def tearDown(self):
        usernames = [s.username for s in itertools.chain(self.admin_sessions, self.user_sessions)]

        for u in usernames:
            self.admin.run_icommand(['imeta', 'rmw', '-u', u, '%', '%', '%'])

        super(Test_ImetaSet, self).tearDown()

    def mod_avu(self, user_name, a, v, u, newv):
        self.admin.assert_icommand('imeta mod -u %s %s %s %s v:%s' % (user_name, a, v, u, newv))

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

    def check_avu_data_obj(self, data_obj_name, a, v, u):
        # If setting unit to empty string then ls output should be blank
        if u == '""':
            u = ''

        a = re.escape(a)
        v = re.escape(v)
        u = re.escape(u)

        self.admin.assert_icommand('imeta ls -d %s %s' % (data_obj_name, a), 'STDOUT_MULTILINE', ['attribute: ' + a + '$',
                                                                                              'value: ' + v + '$',
                                                                                              'units: ' + u + '$'],
                                   use_regex=True)

    def set_and_check_avu(self, user_name, a, v, u):
        self.set_avu(user_name, a, v, u)
        self.check_avu(user_name, a, v, u)

    def add_and_check_avu(self, user_name, a, v, u):
        self.add_avu(user_name, a, v, u)
        self.check_avu(user_name, a, v, u)

    def mod_and_check_avu(self, user_name, a, v, u, newv):
        self.mod_avu(user_name, a, v, u, newv)
        self.check_avu(user_name, a, newv, u)

    def test_imeta_set_and_mod_single_object_triple(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.mod_and_check_avu(user, 'att0', 'val0', 'unt0', 'val5')

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

        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT_SINGLELINE', 'units: unt0')

    def test_imeta_set_single_object_triple_to_double_empty_unit(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.set_and_check_avu(user, 'att0', 'val1', '""')

        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT_SINGLELINE', 'units: unt0')

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
        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT_SINGLELINE', 'units: unt0')

    def test_imeta_set_single_object_abandoned_avu_triple_to_double_empty_unit(self):
        user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.admin.assert_icommand('imeta rm -u %s %s %s %s' % (user, 'att0', 'val0', 'unt0'))
        self.set_and_check_avu(user, 'att0', 'val0', '""')
        self.admin.assert_icommand_fail('imeta ls -u ' + user + ' att0', 'STDOUT_SINGLELINE', 'units: unt0')

    def test_imeta_set_single_object_multi_avu_removal(self):
        user = self.user0.username

        original_avus = [('att' + str(i), 'val' + str(i), 'unt' + str(i)) for i in range(30)]

        for avu in original_avus:
            self.add_and_check_avu(user, *avu)

        self.set_and_check_avu(user, 'att_new', 'val_new', 'unt_new')

        for a, v, u in original_avus:
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT_SINGLELINE', ['attribute: ' + a + '$'])
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT_SINGLELINE', ['value: ' + v + '$'])
            self.admin.assert_icommand_fail('imeta ls -u %s %s' % (user, a), 'STDOUT_SINGLELINE', ['units:' + u + '$'])

    def test_imeta_with_too_long_string(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        num_extra_bind_vars = 10000
        command_str = '''imeta qu -d a in "1'{0}'v"'''.format("'1'" * num_extra_bind_vars)
        self.admin.assert_icommand(command_str, 'STDERR_SINGLELINE', 'USER_STRLEN_TOOLONG')

    def test_imeta_with_many_bind_vars(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        num_extra_bind_vars = 607  # 3848 for postgres and mysql, any more and the argument string is too long
        command_str = '''imeta qu -d a in "1'{0}'v"'''.format("'1'" * num_extra_bind_vars)
        self.admin.assert_icommand(command_str, 'STDOUT_SINGLELINE', self.testfile)

    def test_imeta_addw(self):
        base_name = "file_";
        for i in range(5):
            object_name = base_name + str(i)
            self.admin.assert_icommand(['iput', self.testfile, object_name])

        self.admin.assert_icommand('ils', 'STDOUT_SINGLELINE', 'file_')

        attribute = 'test_imeta_addw_attribute'
        value = 'test_imeta_addw_value'
        wild = base_name+"%"
        self.admin.assert_icommand(['imeta', 'addw', '-d', wild, attribute, value], 'STDOUT_SINGLELINE', 'AVU added to 5 data-objects')

        for i in range(5):
            object_name = base_name + str(i)
            self.admin.assert_icommand('imeta ls -d %s' % (object_name), 'STDOUT_SINGLELINE', ['attribute: ' + attribute])
            self.admin.assert_icommand('imeta ls -d %s' % (object_name), 'STDOUT_SINGLELINE', ['value: ' + value])

    def test_imeta_addw_for_other_user(self):
        base_name = "file_";
        for i in range(5):
            object_name = base_name + str(i)
            self.admin.assert_icommand(['iput', self.testfile, object_name])

        self.admin.assert_icommand('ils', 'STDOUT_SINGLELINE', 'file_')

        attribute = 'test_imeta_addw_attribute'
        value = 'test_imeta_addw_value'
        wild = self.admin.session_collection+'/'+base_name+"%"
        self.user0.assert_icommand(['imeta', 'addw', '-d', wild, attribute, value], 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

    def test_imeta_addw_for_other_user_by_admin(self):
        base_name = "file_";
        for i in range(5):
            object_name = base_name + str(i)
            self.user0.assert_icommand(['iput', self.testfile, object_name])

        self.user0.assert_icommand('ils', 'STDOUT_SINGLELINE', 'file_')

        attribute = 'test_imeta_addw_attribute'
        value = 'test_imeta_addw_value'
        wild = self.user0.session_collection+'/'+base_name+"%"
        self.admin.assert_icommand(['imeta', 'addw', '-d', wild, attribute, value], 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

    def test_imeta_duplicate_attr_3787(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a1', 'v1', 'u1'])
        self.admin.assert_icommand(['imeta', 'mod', '-d', self.testfile, 'a1', 'v1', 'u1', 'n:a2', 'v:v2', 'u:'])
        self.admin.assert_icommand(['imeta', 'mod', '-d', self.testfile, 'a2', 'v2',       'n:a3', 'v:v3', 'u:u3'])
        self.check_avu_data_obj(self.testfile, 'a3', 'v3', 'u3')

    def test_imeta_duplicate_attr_3788(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        self.admin.assert_icommand('imeta mod -d ' + self.testfile + 'a v u n:newa1 n:newa2', 'STDOUT_SINGLELINE', 'Error: New attribute specified more than once')
        self.check_avu_data_obj(self.testfile, 'a', 'v', 'u')

    def test_imeta_duplicate_value_3788(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        self.admin.assert_icommand('imeta mod -d ' + self.testfile + 'a v u v:newa1 v:newa2', 'STDOUT_SINGLELINE', 'Error: New value specified more than once')
        self.check_avu_data_obj(self.testfile, 'a', 'v', 'u')

    def test_imeta_duplicate_unit_3788(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        self.admin.assert_icommand('imeta mod -d ' + self.testfile + 'a v u u:newa1 u:newa2', 'STDOUT_SINGLELINE', 'Error: New unit specified more than once')
        self.check_avu_data_obj(self.testfile, 'a', 'v', 'u')

    def test_imeta_add_missing_value_27(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a'], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to add$', '$'],
                                   use_regex=True)

    def test_imeta_rmi_missing_obj_type_27(self):
        self.admin.assert_icommand(['imeta', 'rmi', '-d'], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to rmi$', '$'],
                                   use_regex=True)

    def test_imeta_rmi_missing_metadata_id_27(self):
        self.admin.assert_icommand(['imeta', 'rmi', '-d', self.testfile], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to rmi$', '$'],
                                   use_regex=True)

    def test_imeta_mod_missing_opt1_27(self):
        self.admin.assert_icommand(['imeta', 'mod', '-d', self.testfile, 'a', 'v'], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to mod$', '$'],
                                   use_regex=True)

    def test_imeta_set_missing_value_27(self):
        self.admin.assert_icommand(['imeta', 'set', '-d', self.testfile, 'a'], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to set$', '$'],
                                   use_regex=True)

    def test_imeta_ls_missing_obj_name_27(self):
        self.admin.assert_icommand(['imeta', 'ls', '-d'], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to ls$', '$'],
                                   use_regex=True)

    def test_imeta_qu_missing_value_27(self):
        self.admin.assert_icommand(['imeta', 'qu', '-d', 'a', '='], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to qu$', '$'],
                                   use_regex=True)

    def test_imeta_cp_missing_obj_name_27(self):
        self.admin.assert_icommand(['imeta', 'cp', '-d', '-d', self.testfile], 'STDOUT_MULTILINE', ['$', 'Error: Not enough arguments provided to cp$', '$'],
                                   use_regex=True)



  


class Test_ImetaQu(ResourceBase, unittest.TestCase):

    def helper_imeta_qu_comparison_2748(self, irods_object_option_flag):
        attribute = 'helper_imeta_qu_comparison_2748_attribute'
        object_name_base = 'helper_imeta_qu_comparison_2748_base_name'
        for i in range(10):
            object_name = object_name_base + str(i)
            value = str(i)
            if irods_object_option_flag == '-C':
                self.admin.assert_icommand(['imkdir', object_name])
            elif irods_object_option_flag == '-d':
                self.admin.assert_icommand(['iput', self.testfile, object_name])
            else:
                assert False, irods_object_option_flag

            self.admin.assert_icommand(['imeta', 'add', irods_object_option_flag, object_name, attribute, value])

        stdout, stderr, rc = self.admin.run_icommand(['imeta', 'qu', irods_object_option_flag, attribute, '<=', '8', 'and', attribute, '>=', '2'])
        assert rc == 0, rc
        assert stderr == '', stderr
        all_objects = set([object_name_base + str(i) for i in range(0, 10)])
        should_find = set([object_name_base + str(i) for i in range(2, 9)])
        should_not_find = all_objects - should_find
        for c in should_find:
            assert c in stdout, c + ' not found in ' + stdout
        for c in should_not_find:
            assert c not in stdout, c + ' found in ' + stdout

    def test_imeta_qu_C_comparison_2748(self):
        self.helper_imeta_qu_comparison_2748('-C')

    def test_imeta_qu_d_comparison_2748(self):
        self.helper_imeta_qu_comparison_2748('-d')

    def test_imeta_qu_d_no_extra_output(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a', 'v', 'u'])
        _, out, _ = self.admin.assert_icommand(['imeta', 'qu', '-d', 'a', 'like', 'v'], 'STDOUT_SINGLELINE', self.testfile)
        split_output = out.split()
        self.assertEqual(split_output[0], 'collection:', out)
        self.assertEqual(split_output[1], self.admin.session_collection, out)
        self.assertEqual(split_output[2], 'dataObj:', out)
        self.assertEqual(split_output[3], 'testfile.txt', out)

