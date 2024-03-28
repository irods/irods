import textwrap
import unittest

from . import session
from ..configuration import IrodsConfig

rodsadmins = [('otherrods', 'rods')]
rodsusers  = [('alice', 'apass')]

class Test_GenQuery2_Microservices(session.make_sessions_mixin(rodsadmins, rodsusers), unittest.TestCase):

    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_GenQuery2_Microservices, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_GenQuery2_Microservices, self).tearDown()

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Designed for the NREP')
    def test_microservices_can_be_used_in_the_native_rule_engine_plugin__issue_7570(self):
        rule_file = f'{self.user.local_session_dir}/genquery2_issue_7570.nrep.r'

        with open(rule_file, 'w') as f:
            f.write(textwrap.dedent(f'''
                test_issue_7570_nrep {{
                    msi_genquery2_execute(*handle, "select COLL_NAME where COLL_NAME = '{self.user.session_collection}'");

                    while (errorcode(msi_genquery2_next_row(*handle)) == 0) {{
                        msi_genquery2_column(*handle, '0', *coll_name);
                        writeLine('stdout', '*coll_name');
                    }}

                    msi_genquery2_free(*handle);
                }}

                INPUT *handle="", *coll_name=""
                OUTPUT ruleExecOut
            '''))

        rep_name = self.plugin_name + '-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDOUT', [self.user.session_collection])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-python', 'Designed for the PREP')
    def test_microservices_can_be_used_in_the_python_rule_engine_plugin(self):
        rule_file = f'{self.user.local_session_dir}/genquery2_issue_7570.prep.r'

        with open(rule_file, 'w') as f:
            f.write(textwrap.dedent(f'''
                test_issue_7570_prep(rule_args, callback, rei) {{
                    result = callback.msi_genquery2_execute('', "select COLL_NAME where COLL_NAME = '{self.user.session_collection}'");
                    handle = result['arguments'][0]

                    while True:
                        try:
                            msi_genquery2_next_row(*handle)
                        except:
                            break

                        result = callback.msi_genquery2_column(handle, '0', '');
                        coll_name = result['arguments'][2]

                        callback.writeLine('stdout', coll_name);

                    callback.msi_genquery2_free(handle);
                }}

                INPUT *handle=%*coll_name=
                OUTPUT *ruleExecOut
            '''))

        rep_name = self.plugin_name + '-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDOUT', [self.user.session_collection])
