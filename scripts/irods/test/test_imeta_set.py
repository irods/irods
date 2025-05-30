import itertools
import os
import re
import sys
import textwrap

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from .. import lib
from .. import test
from . import session

class Test_ImetaSet(ResourceBase, unittest.TestCase):

    plugin_name =  IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_ImetaSet, self).setUp()

        usernames = [s.username for s in itertools.chain(self.admin_sessions, self.user_sessions)]
        self.admin.assert_icommand('iadmin lu', 'STDOUT_MULTILINE', usernames)

        for u in usernames:
            self.admin.assert_icommand('imeta ls -u ' + u, 'STDOUT_SINGLELINE', 'None')

    def tearDown(self):
        # Parent class tearDown method will remove the users which had metadata annotated to them.
        super(Test_ImetaSet, self).tearDown()
        # Clean up the now-unused metadata annotated to the users which have just been removed.
        with session.make_session_for_existing_admin() as admin_session:
            admin_session.run_icommand(["iadmin", "rum"])

    def mod_avu_value(self, user_name, a, v, u, newv):
        self.admin.assert_icommand('imeta mod -u %s %s %s %s v:%s' % (user_name, a, v, u, newv))

    def mod_avu_attr(self, user_name, a, v, u, newa):
        self.admin.assert_icommand('imeta mod -u %s %s %s %s n:%s' % (user_name, a, v, u, newa))

    def mod_avu_unit(self, user_name, a, v, u, newu):
        self.admin.assert_icommand('imeta mod -u %s %s %s %s u:%s' % (user_name, a, v, u, newu))



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

    def mod_and_check_avu_value(self, user_name, a, v, u, newv):
        self.mod_avu_value(user_name, a, v, u, newv)
        self.check_avu(user_name, a, newv, u)

    def mod_and_check_avu_attr(self, user_name, a, v, u, newa):
        self.mod_avu_attr(user_name, a, v, u, newa)
        self.check_avu(user_name, newa, v, u)

    def mod_and_check_avu_unit(self, user_name, a, v, u, newu):
        self.mod_avu_unit(user_name, a, v, u, newu)
        self.check_avu(user_name, a, v, newu)

    def test_imeta_mod_attr_3667(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.mod_and_check_avu_attr(user, 'att0', 'val0', 'unt0', 'newattr')

    def test_imeta_mod_units_3667(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.mod_and_check_avu_unit(user, 'att0', 'val0', 'unt0', 'newunit')

    def test_imeta_set_and_mod_single_object_triple(self, user=None):
        if user is None:
            user = self.user0.username

        self.set_and_check_avu(user, 'att0', 'val0', 'unt0')
        self.mod_and_check_avu_value(user, 'att0', 'val0', 'unt0', 'val5')

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

    def test_imeta_duplicate_attr_3787(self):
        self.admin.assert_icommand(['imeta', 'add', '-d', self.testfile, 'a1', 'v1', 'u1'])
        self.admin.assert_icommand(['imeta', 'mod', '-d', self.testfile, 'a1', 'v1', 'u1', 'n:a2', 'v:v2', 'u:'])
        self.admin.assert_icommand(['imeta', 'mod', '-d', self.testfile, 'a2', 'v2',       'n:a3', 'v:v3', 'u:u3'])
        self.check_avu_data_obj(self.testfile, 'a3', 'v3', 'u3')

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_mod_avu_msvc_4521(self):
        sescoln = self.admin.session_collection
        self.admin.assert_icommand(['imeta', 'set', '-d', self.testfile, 'a1', 'v1'])
        self.admin.assert_icommand(['irule', '-v', '''msiModAVUMetadata("-d","{0}","set","a1","v1","u1")'''.format(sescoln + '/' + self.testfile),'null','null'],
          'STDOUT_SINGLELINE', "completed successfully")
        self.admin.assert_icommand(["iquest", "%s %s %s",
                                    "select META_DATA_ATTR_UNITS,count(META_DATA_ATTR_ID),count(DATA_ID) where DATA_NAME = '{0}'".format(self.testfile)],
                                    'STDOUT_SINGLELINE', 'u1 1 1')
        self.admin.assert_icommand(['irule', '-v', '''msiModAVUMetadata("-d","{0}","add","a1","v1","u2")'''.format(sescoln + '/' + self.testfile),'null','null'],
          'STDOUT_SINGLELINE', "completed successfully")
        self.admin.assert_icommand(["iquest", "%s %s",
                                    "select META_DATA_ATTR_UNITS,count(DATA_ID) where DATA_NAME = '{0}'".format(self.testfile)],
                                    'STDOUT_MULTILINE', ['^u1 1$','^u2 1$'],use_regex=True)
        self.admin.assert_icommand(
            ['irule', '-v', '''msiModAVUMetadata("-d","{0}","rm","a1","v1","u1")'''.format(sescoln + '/' + self.testfile),'null','null'],
            'STDOUT', "completed successfully")
        self.admin.assert_icommand(
            ['irule', '-v', '''msiModAVUMetadata("-d","{0}","rm","a1","v1","u2")'''.format(sescoln + '/' + self.testfile),'null','null'],
            'STDOUT', "completed successfully")
        self.admin.assert_icommand(['iquest', "%s", "select count(META_DATA_ATTR_ID) where DATA_NAME = '{0}'".format(self.testfile)],
          'STDOUT_SINGLELINE',"^0$",use_regex=True)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_mod_avu_msvc_with_rodsuser_invoking_rule_4521(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                test {
                    msiModAVUMetadata("-C", "*homeColln","set","a1","v1","x")
                    *a.a1 = "v1"
                    *a.a2 = "v2"
                    msiAssociateKeyValuePairsToObj(*a ,"*homeColln","-C" )
                    msiModAVUMetadata("-C", "*homeColln","add","a2","v2","u2")
                    printmeta("1",*b)
                    msiModAVUMetadata("-C", "*homeColln","set","a2","v2","u")
                    printmeta("2",*b)
                    msiModAVUMetadata("-C", "*homeColln","rm","a1","v1","")
                    printmeta("3",*b)
                    msiModAVUMetadata("-C", "*homeColln","rm","a1","v1","x")
                    msiModAVUMetadata("-C", "*homeColln","rm","a2","v2","u")
                    printmeta("4",*b)
                }
                printmeta(*id,*z) {
                    msiString2KeyValPair("",*z)
                    foreach (*x in select META_COLL_ATTR_NAME,
                                          META_COLL_ATTR_VALUE,
                                          META_COLL_ATTR_UNITS
                                    where META_COLL_ATTR_NAME like 'a_' and
                                          COLL_NAME like '*homeColln') {
                        *key = *x.META_COLL_ATTR_NAME ++ ":" ++ *x.META_COLL_ATTR_VALUE
                                                      ++ ":" ++ *x.META_COLL_ATTR_UNITS
                        *z.*key = "*id"
                        writeLine("stdout","(*id,*key)")
                    }
                }
                INPUT *homeColln="/$rodsZoneClient/home/$userNameClient"
                OUTPUT ruleExecOut
            ''')
        }

        rule_file = "test_msiModAVUMetadata.r"
        expected_output = sorted([
        "(1,a1:v1:)",
        "(1,a1:v1:x)",
        "(1,a2:v2:)",
        "(1,a2:v2:u2)",
        "(2,a1:v1:)",
        "(2,a1:v1:x)",
        "(2,a2:v2:u)",
        "(3,a1:v1:x)",
        "(3,a2:v2:u)"])
        try:
            with open(rule_file, 'w') as f:
                f.write(pep_map[self.plugin_name])
            dummy_rc,out,_ = self.user0.assert_icommand("irule -F " + rule_file,'STDOUT','')
            output_list = [l for l in sorted(out.split('\n')) if l]
            self.assertTrue( output_list == expected_output )
        finally:
            os.unlink(rule_file)

    def test_imeta_handles_relative_paths__issue_4682(self):
        data_object = 'foo'
        filename = os.path.join(self.admin.local_session_dir, data_object)
        lib.make_file(filename, 1, 'arbitrary')

        self.admin.assert_icommand(['iput', filename])
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('.', data_object)],      'STDOUT', ['None'])
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('.', '.', data_object)], 'STDOUT', ['None'])

        session_collection = os.path.basename(self.admin.session_collection)
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('..', session_collection, data_object)], 'STDOUT', ['None'])

        # Data objects would not normally end in a trailing slash. These tests demonstrate that imeta, like
        # all other icommands, properly removes trailing slashes during processing which allows the server to
        # handle the request without exploding on an invalid path.
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('.', data_object) + '/'],       'STDOUT', ['None'])
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('.', data_object) + '//'],      'STDOUT', ['None'])
        self.admin.assert_icommand(['imeta', 'ls', '-d', os.path.join('.', '.', data_object) + '//'], 'STDOUT', ['None'])

    def test_imeta_handles_trailing_slashes_on_collections__issue_4559(self):
        collection = 'trailing_slash_test.d'
        self.admin.assert_icommand(['imkdir', collection])
        self.admin.assert_icommand(['imeta', 'ls', '-C', os.path.join(collection) + '/'],  'STDOUT', ['None'])
        self.admin.assert_icommand(['imeta', 'ls', '-C', os.path.join(collection) + '//'], 'STDOUT', ['None'])

    def test_imeta_cannot_set_shared_avu_value_to_empty_string__issue_4063(self):
        # Generate collections for test
        collections = ['games', 'super_secret_documents']
        collections = [os.path.join(self.admin.session_collection, col) for col in collections]

        # Create collections
        for collection in collections:
            self.admin.assert_icommand(['imkdir', collection])

        # Set the same AVUs for the collections
        for collection in collections:
            self.admin.assert_icommand(['imeta', 'set', '-C', collection, 'is_cool', 'yes'])

        # Attempt to clear value of second collection
        expected_err = 'ERROR: rcModAVUMetadata failed with error -816000 CAT_INVALID_ARGUMENT'
        self.admin.assert_icommand(['imeta', 'set', '-C', collections[1], 'is_cool', ''], 'STDERR_SINGLELINE', expected_err, desired_rc=4)

        # Check that the value is unaffected for both collections
        for collection in collections:
            expected_value = 'value: yes'
            self.admin.assert_icommand(['imeta', 'ls', '-C', collection], 'STDOUT', expected_value)

# See issue #5111
class Test_ImetaLsLongmode(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_ImetaLsLongmode, self).setUp()
        self.admin = self.admin_sessions[0]
        self.test_data_path = self.admin.session_collection + '/imeta_test_data'
        self.admin.assert_icommand(['itouch', self.test_data_path])
        self.admin.assert_icommand(['iadmin', 'mkresc', 'imeta_test_resc', 'random'], 'STDOUT_SINGLELINE', 'random')
        self.admin.assert_icommand(['imeta', 'add', '-d', self.test_data_path, 'imeta_test_attr', 'imeta_test_value'])
        self.admin.assert_icommand(['imeta', 'add', '-C', self.admin.session_collection, 'imeta_test_attr', 'imeta_test_value'])
        self.admin.assert_icommand(['imeta', 'add', '-R', 'imeta_test_resc', 'imeta_test_attr', 'imeta_test_value'])
        self.admin.assert_icommand(['imeta', 'add', '-u', self.admin.username, 'imeta_test_attr', 'imeta_test_value'])

    def tearDown(self):
        self.admin.assert_icommand(['iadmin', 'rmresc', 'imeta_test_resc'])
        super(Test_ImetaLsLongmode, self).tearDown()

    def test_imeta_ls_ld_mtime_present(self):
        self.admin.assert_icommand(['imeta', 'ls', '-ld', self.test_data_path], 'STDOUT_SINGLELINE', 'time set:')

    def test_imeta_ls_lC_mtime_present(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lC', self.admin.session_collection], 'STDOUT_SINGLELINE', 'time set:')

    def test_imeta_ls_lR_mtime_present(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lR', 'imeta_test_resc'], 'STDOUT_SINGLELINE', 'time set:')

    def test_imeta_ls_lu_mtime_present(self):
        self.admin.assert_icommand(['imeta', 'ls', '-lu', self.admin.username], 'STDOUT_SINGLELINE', 'time set:')

class Test_ImetaCp(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_ImetaCp, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_ImetaCp, self).tearDown()

    def test_subcommand_cp_supports_relative_paths__issue_6174(self):
        current_collection = os.path.basename(self.admin.session_collection)

        # Here are our relative paths.
        src_object = os.path.join('..', current_collection, '.', 'src_object')
        dst_object = os.path.join('..', current_collection, '.', 'dst_object')

        attr_name  = 'issue_6174_attr'
        attr_value = 'issue_6174_value'

        # True : indicates that the object is a data object.
        # False: indicates that the object is a collection.
        #
        # The following table produces the following tests:
        # - data object to data object
        # - data object to collection
        # - collection to collection
        # - collection to data object
        test_cases = [
            {'src': True,  'dst': True},
            {'src': True,  'dst': False},
            {'src': False, 'dst': False},
            {'src': False, 'dst': True}
        ]

        def create_object(_object_name, _is_data_object):
            if _is_data_object:
                self.admin.assert_icommand(['itouch', _object_name])
                return '-d'

            self.admin.assert_icommand(['imkdir', _object_name])
            return '-C'

        for tc in test_cases:
            try:
                # Create the source and destination objects and capture the object type.
                src_object_type_flag = create_object(src_object, tc['src'])
                dst_object_type_flag = create_object(dst_object, tc['dst'])

                # Attach metadata to the source object.
                self.admin.assert_icommand(['imeta', 'set', src_object_type_flag, src_object, attr_name, attr_value])
                self.admin.assert_icommand(['imeta', 'ls', src_object_type_flag, src_object], 'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

                # Copy from source object's metadata to the destination object.
                self.admin.assert_icommand(['imeta', 'cp', src_object_type_flag, dst_object_type_flag, src_object, dst_object])

                # Verify that the metadata was copied from the source object to the destination object.
                self.admin.assert_icommand(['imeta', 'ls', dst_object_type_flag, dst_object], 'STDOUT', ['attribute: ' + attr_name, 'value: ' + attr_value])

            finally:
                self.admin.run_icommand(['irm', '-rf', src_object, dst_object])
