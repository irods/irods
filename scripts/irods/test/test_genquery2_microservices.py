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
        rule_file = f'{self.user.local_session_dir}/test_microservices_can_be_used_in_the_native_rule_engine_plugin__issue_7570.nrep.r'

        with open(rule_file, 'w') as f:
            f.write(textwrap.dedent(f'''
                test_issue_7570_nrep {{
                    *END_OF_RESULTSET = -408000;

                    msi_genquery2_execute(*handle, "select COLL_NAME where COLL_NAME = '{self.user.session_collection}'");

                    while (true) {{
                        *ec = errorcode(msi_genquery2_next_row(*handle));

                        if (*ec < 0) {{
                            if (*END_OF_RESULTSET == *ec) {{
                                break;
                            }}

                            failmsg(*ec, "Unexpected error while iterating over GenQuery2 resultset.");
                        }}

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
        rule_file = f'{self.user.local_session_dir}/test_microservices_can_be_used_in_the_python_rule_engine_plugin.prep.r'

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

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Designed for the NREP')
    def test_microservices_support_interleaved_execution__issue_7570(self):
        # This test is about proving GenQuery2 handles are tracked correctly.
        #
        # The original GenQuery2 implementation used a std::vector to track handles. This
        # decision introduced a bug which only appears in the following scenario:
        #
        #     - Two handles exist within the same agent
        #     - The first handle is free'd
        #     - The second handle is used for subsequent operations (unexpected results occur!)
        #
        # Freeing a handle in the original implementation meant erasing the element in the
        # std::vector. The erasure of the element breaks the MSIs because the index of the
        # element was used as the handle for looking up information.

        rule_file = f'{self.user.local_session_dir}/test_microservices_support_interleaved_execution__issue_7570.r'

        with open(rule_file, 'w') as f:
            f.write(textwrap.dedent(f'''
                test_issue_7570_nrep {{
                    msi_genquery2_execute(*zone_handle, "select ZONE_NAME");
                    msi_genquery2_execute(*user_handle, "select USER_NAME where USER_NAME = '{self.user.username}'");
                    msi_genquery2_execute(*resc_handle, "select RESC_NAME where RESC_NAME = 'demoResc'");

                    errorcode(msi_genquery2_next_row(*resc_handle));
                    msi_genquery2_free(*zone_handle); # Remove a handle just because we can.
                    msi_genquery2_column(*resc_handle, '0', *resc_name);
                    msi_genquery2_free(*resc_handle);

                    errorcode(msi_genquery2_next_row(*user_handle));
                    msi_genquery2_column(*user_handle, '0', *user_name);
                    msi_genquery2_free(*user_handle);

                    writeLine('stdout', '*user_name, *resc_name');
                }}

                INPUT *zone_handle="", *user_handle="", *resc_handle="", *user_name="", *resc_name=""
                OUTPUT ruleExecOut
            '''))

        rep_name = self.plugin_name + '-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDOUT', [f'{self.user.username}, demoResc'])
