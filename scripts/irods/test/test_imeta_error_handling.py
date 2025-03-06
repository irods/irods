from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import string
from .. import lib
from . import session
from random import Random

class Test_Imeta_Error_Handling(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.admin = session.mkuser_and_return_session('rodsadmin', 'otherrods', 'rods', lib.get_hostname())
        cls.test_data_path_base = cls.admin.session_collection + '/imeta_test_data'
        cls.test_data_paths_wildcard = cls.test_data_path_base + '%'
        cls.test_coll_path_base = cls.admin.session_collection + '/imeta_test_coll'
        cls.test_coll_paths_wildcard = cls.test_coll_path_base + '%'
        cls.test_resc_name_base = 'imeta_test_resc'
        cls.test_resc_names_wildcard = cls.test_resc_name_base + '%'
        cls.test_user_name_base = 'imeta_test_user'
        cls.test_user_names_wildcard = cls.test_user_name_base + '%'
        cls.test_univ_attr = 'some_attr'
        cls.test_data_qty = 5
        for n in range(cls.test_data_qty):
            data_path = cls.test_data_path_base + str(n)
            coll_path = cls.test_coll_path_base + str(n)
            resc_name = cls.test_resc_name_base + str(n)
            user_name = cls.test_user_name_base + str(n)
            cls.admin.assert_icommand(['itouch', data_path])
            cls.admin.assert_icommand(['imkdir', coll_path])
            cls.admin.assert_icommand(['iadmin', 'mkresc', resc_name, 'random'], 'STDOUT')
            cls.admin.assert_icommand(['iadmin', 'mkuser', user_name, 'rodsuser'])
            cls.admin.assert_icommand(['imeta', 'add', '-d', data_path, cls.test_univ_attr, str(n * 5)])
            cls.admin.assert_icommand(['imeta', 'add', '-C', coll_path, cls.test_univ_attr, str(n * 5)])
            cls.admin.assert_icommand(['imeta', 'add', '-R', resc_name, cls.test_univ_attr, str(n * 5)])
            cls.admin.assert_icommand(['imeta', 'add', '-u', user_name, cls.test_univ_attr, str(n * 5)])
        r = Random()
        r.seed(60221415)
        cls.long_name = ''.join(r.choice(string.ascii_lowercase) for i in range(1056))
        cls.longer_name = ''.join(r.choice(string.ascii_lowercase) for i in range(1536))
        cls.long_but_valid_name = ''.join(r.choice(string.ascii_lowercase) for i in range(1020))

    @classmethod
    def tearDownClass(cls):
        for n in range(cls.test_data_qty):
            resc_name = cls.test_resc_name_base + str(n)
            user_name = cls.test_user_name_base + str(n)
            cls.admin.assert_icommand(['iadmin', 'rmresc', resc_name])
            cls.admin.assert_icommand(['iadmin', 'rmuser', user_name])
        with session.make_session_for_existing_admin() as admin_session:
            cls.admin.__exit__()
            admin_session.assert_icommand(['iadmin', 'rmuser', cls.admin.username])

    def test_imeta_too_many_args(self):
        self.admin.assert_icommand(['imeta'] + (['a'] * 48), 'STDERR_SINGLELINE', 'Unrecognized input, too many input tokens', desired_rc=4)

    def test_imeta_badcmd(self):
        self.admin.assert_icommand(['imeta', 'badcmd'], 'STDERR_SINGLELINE', 'Unrecognized subcommand', desired_rc=4)

    def test_imeta_add_return_code(self):
        data_path = f'{self.test_data_path_base}0'
        attr = 'test_imeta_add_return_code_attr'
        val = 'test_imeta_add_return_code_val'
        self.admin.assert_icommand(['imeta', 'add', '-d', data_path, attr, val], 'STDOUT', desired_rc=0)
        _, err, ec = self.admin.run_icommand(['imeta', 'add', '-d', data_path, attr, val])
        self.assertEqual(4, ec)
        self.assertIn('CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME', err)

    def test_imeta_rmi_insuff_args(self):
        self.admin.assert_icommand(['imeta', 'rmi', '-d'], 'STDERR_SINGLELINE', 'Not enough arguments provided', desired_rc=4)
    def test_imeta_rmi_no_args(self):
        self.admin.assert_icommand(['imeta', 'rmi'], 'STDERR_SINGLELINE', 'No object type descriptor', desired_rc=4)

    def test_imeta_mod_insuff_args(self):
        self.admin.assert_icommand(['imeta', 'mod', '-d'], 'STDERR_SINGLELINE', 'Not enough arguments provided', desired_rc=4)
    def test_imeta_mod_no_args(self):
        self.admin.assert_icommand(['imeta', 'mod'], 'STDERR_SINGLELINE', 'No object type descriptor', desired_rc=4)
    def test_imeta_mod_dup_attr(self):
        self.admin.assert_icommand(['imeta', 'mod', '-d', 'path', 'attr', 'val', 'n:attr2', 'n:attr2'], 'STDERR_SINGLELINE', 'New attribute specified more than once', desired_rc=4)
    def test_imeta_mod_dup_val(self):
        self.admin.assert_icommand(['imeta', 'mod', '-d', 'path', 'attr', 'val', 'v:val2', 'v:val2'], 'STDERR_SINGLELINE', 'New value specified more than once', desired_rc=4)
    def test_imeta_mod_dup_unit(self):
        self.admin.assert_icommand(['imeta', 'mod', '-d', 'path', 'attr', 'val', 'u:unit', 'u:unit'], 'STDERR_SINGLELINE', 'New unit specified more than once', desired_rc=4)

    def test_imeta_set_insuff_args(self):
        self.admin.assert_icommand(['imeta', 'set', '-d'], 'STDERR_SINGLELINE', 'Not enough arguments provided', desired_rc=4)
    def test_imeta_set_no_args(self):
        self.admin.assert_icommand(['imeta', 'set'], 'STDERR_SINGLELINE', 'No object type descriptor', desired_rc=4)

    def test_imeta_ls_d_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-d', 'nonexist'], 'STDERR', 'does not exist', desired_rc=4)
    def test_imeta_ls_ld_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-ld', 'nonexist'], 'STDERR', 'does not exist', desired_rc=4)
    def test_imeta_ls_C_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-C', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)
    def test_imeta_ls_lC_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lC', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)
    def test_imeta_ls_R_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-R', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)
    def test_imeta_ls_lR_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lR', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)
    def test_imeta_ls_u_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-u', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)
    def test_imeta_ls_lu_nonexist(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lu', 'nonexist'], 'STDERR_SINGLELINE', 'does not exist', desired_rc=4)

    def test_imeta_ls_u_name_inval(self):
        self.admin.assert_icommand(['imeta', 'ls', '-u', '##'], 'STDERR_SINGLELINE', 'Invalid username format', desired_rc=4)
    def test_imeta_ls_u_name_toolong(self):
        self.admin.assert_icommand(['imeta', 'ls', '-u', self.long_name], 'STDERR_SINGLELINE', 'Invalid username format', desired_rc=4)

    def test_imeta_qu_insuff_args(self):
        self.admin.assert_icommand(['imeta', 'qu', '-d'], 'STDERR_SINGLELINE', 'Not enough arguments provided', desired_rc=4)
    def test_imeta_qu_no_args(self):
        self.admin.assert_icommand(['imeta', 'qu'], 'STDERR_SINGLELINE', 'No object type descriptor', desired_rc=4)
    def test_imeta_qu_badop_arg(self):
        self.admin.assert_icommand(['imeta', 'qu', '-d', self.test_univ_attr, 'gt', '17'], 'STDERR_SINGLELINE', 'CAT_INVALID_ARGUMENT', desired_rc=4)
    def test_imeta_qu_d_badquery(self):
        self.admin.assert_icommand(['imeta', 'qu', '-d', self.test_univ_attr, 'n>', '6', '26'], 'STDERR_SINGLELINE', 'Unrecognized input', desired_rc=4)
    def test_imeta_qu_C_badquery(self):
        self.admin.assert_icommand(['imeta', 'qu', '-C', self.test_univ_attr, 'n>', '6', '26'], 'STDERR_SINGLELINE', 'Unrecognized input', desired_rc=4)
    def test_imeta_qu_R_extra_arg(self):
        self.admin.assert_icommand(['imeta', 'qu', '-R', self.test_univ_attr, 'n>', '6', 'and', self.test_univ_attr, 'n<', '17'], 'STDERR_SINGLELINE', 'Too many arguments provided to imeta qu for the -R option', desired_rc=4)
    def test_imeta_qu_u_extra_arg(self):
        self.admin.assert_icommand(['imeta', 'qu', '-u', self.test_univ_attr, 'n>', '6', 'and', self.test_univ_attr, 'n<', '17'], 'STDERR_SINGLELINE', 'Too many arguments provided to imeta qu for the -u option', desired_rc=4)

    def test_imeta_cp_one_arg(self):
        self.admin.assert_icommand(['imeta', 'cp', '-d'], 'STDERR_SINGLELINE', 'No second object type descriptor', desired_rc=4)
    def test_imeta_cp_insuff_args(self):
        self.admin.assert_icommand(['imeta', 'cp', '-d', '-d'], 'STDERR_SINGLELINE', 'Not enough arguments provided', desired_rc=4)
    def test_imeta_cp_no_args(self):
        self.admin.assert_icommand(['imeta', 'cp'], 'STDERR_SINGLELINE', 'No first object type descriptor', desired_rc=4)

    def test_imeta_returns_error_code_on_invalid_subcommand__issue_5316(self):
        ec, _, _ = self.admin.assert_icommand(['imeta', 'ls'], 'STDERR_SINGLELINE', ['Error: No object type descriptor (-d/C/R/u) specified'])
        self.assertEqual(ec, 4)

        subcmd = 'non_existent_subcommand'
        ec, _, _ = self.admin.assert_icommand(['imeta', subcmd], 'STDERR_SINGLELINE', ["Unrecognized subcommand '{0}', try 'imeta help'".format(subcmd)])
        self.assertEqual(ec, 4)

