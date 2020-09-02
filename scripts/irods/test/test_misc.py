import os
import sys
from datetime import datetime, timedelta

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import test
from .. import lib
from ..configuration import IrodsConfig

class Test_Misc(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Misc, self).setUp()
        self.admin = self.admin_sessions[0]

    def tearDown(self):
        super(Test_Misc, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_invalid_client_irodsProt_handled_cleanly__issue_4130(self):
        env = os.environ.copy()
        env['irodsProt'] = '2'
        out, err, ec = lib.execute_command_permissive(['ils'], env=env)
        self.assertTrue('Invalid protocol value.' in err)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_disallow_registration_of_path_with_missing_data_object_name__issue_4494(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    test_registration_issue_4494 {{ 
                        *lpath = '{0}';
                        *ppath = '';
                        msiPhyPathReg(*lpath, 'null', *ppath, 'null', *status);
                    }}
                '''.format(self.admin.session_collection + '/'))

            rep = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.admin.assert_icommand(['irule', '-r', rep, 'test_registration_issue_4494', 'null', 'ruleExecOut'],
                                       'STDERR', ['-317000 USER_INPUT_PATH_ERR'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_disallow_creation_of_collections_with_path_navigation_elements_as_the_name__issue_4750(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    test_dot_issue_4750 {{ 
                        *path = '{0}';
                        *recursive = 0;
                        msiCollCreate(*path, *recursive, *status);
                    }}

                    test_dot_dot_issue_4750 {{ 
                        *path = '{1}';
                        *recursive = 0;
                        msiCollCreate(*path, *recursive, *status);
                    }}

                    test_slash_slash_issue_4750 {{ 
                        *path = '{2}';
                        *recursive = 0;
                        msiCollCreate(*path, *recursive, *status);
                    }}
                '''.format(self.admin.session_collection + '/.',
                           self.admin.session_collection + '/..',
                           self.admin.session_collection + '//'))

            rep = 'irods_rule_engine_plugin-irods_rule_language-instance'

            for rule in ['test_dot_issue_4750', 'test_dot_dot_issue_4750']:
                self.admin.assert_icommand(['irule', '-r', rep, rule, 'null', 'ruleExecOut'], 'STDERR', ['-317000 USER_INPUT_PATH_ERR'])

            self.admin.assert_icommand(['irule', '-r', rep, 'test_slash_slash_issue_4750', 'null', 'ruleExecOut'],
                                       'STDERR', ['-809000 CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_catalog_reflects_truncation_on_open__issues_4483_4628(self):
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            filename = os.path.join(self.admin.local_session_dir, 'truncated_file')
            lib.make_file(filename, 1024, 'arbitrary')
            self.admin.assert_icommand(['iput', filename])

            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    test_truncate_on_open_4483_4628 {{ 
                        *path = '{0}';
                        *ec = 0;

                        msiDataObjOpen('objPath=*path++++openFlags=O_WRONLYO_TRUNC', *fd);

                        msiSplitPath(*path, *parent_path, *data_object);
                        foreach (*row in select DATA_SIZE where COLL_NAME = *parent_path and DATA_NAME = *data_object) {{
                            if (*row.DATA_SIZE != '0') {{
                                *ec = -1;
                            }}
                        }}

                        msiDataObjClose(*fd, *status);

                        *ec;
                    }}
                '''.format(os.path.join(self.admin.session_collection, os.path.basename(filename))))

            rep = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.admin.assert_icommand(['irule', '-r', rep, 'test_truncate_on_open_4483_4628', 'null', 'ruleExecOut'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_return_error_on_incompatible_open_flags__issue_4782(self):
        rep = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule = "msiDataObjOpen('objPath=++++openFlags=O_RDONLYO_TRUNC', *fd)"
        self.admin.assert_icommand(['irule', '-r', rep, rule, 'null', 'ruleExecOut'], 'STDERR', ['-404000 USER_INCOMPATIBLE_OPEN_FLAGS'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_trailing_slashes_are_removed_when_creating_collections__issue_3892(self):
        collection = os.path.join(self.admin.session_collection, 'col_3892.d')
        self.admin.assert_icommand(['imkdir', collection])

        rep = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule = 'msiCollCreate("{0}", 0, *ignored)'.format(collection + '/')
        self.admin.assert_icommand(['irule', '-r', rep, rule, 'null', 'ruleExecOut'], 'STDERR', ['-809000 CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME'])

    def test_trailing_slashes_generate_an_error_when_opening_data_objects__issue_3892(self):
        data_object = os.path.join(self.admin.session_collection, 'dobj_3892')

        rep = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule = 'msiDataObjOpen("objPath={0}++++openFlags=O_WRONLYO_TRUNC", *fd)'.format(data_object + '/')
        self.admin.assert_icommand(['irule', '-r', rep, rule, 'null', 'ruleExecOut'], 'STDERR', ['-317000 USER_INPUT_PATH_ERR'])

        rule = 'msiDataObjCreate("{0}", "", *out)'.format(data_object + '/')
        self.admin.assert_icommand(['irule', '-r', rep, rule, 'null', 'ruleExecOut'], 'STDERR', ['-317000 USER_INPUT_PATH_ERR'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python' or test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_return_error_on_resolution_of_empty_hostname__issue_5085(self):
        resource = 'issue_5085_resc'

        try:
            start = datetime.now()
            self.admin.assert_icommand(['iadmin', 'mkresc', resource, 'replication', ':'], 'STDOUT', ["'' is not a valid DNS entry"])
            self.assertLess(datetime.now() - start, timedelta(seconds=5))
        finally:
            self.admin.assert_icommand(['iadmin', 'rmresc', resource])

