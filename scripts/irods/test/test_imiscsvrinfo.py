import unittest

from . import session
from .. import test

class Test_Imiscsvrinfo(session.make_sessions_mixin([], [('alice', 'apass')]), unittest.TestCase):

    def setUp(self):
        super(Test_Imiscsvrinfo, self).setUp()
        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_Imiscsvrinfo, self).tearDown()

    @unittest.skipUnless(test.settings.USE_SSL, 'Test case assumes use of SSL/TLS')
    def test_imiscsvrinfo_prints_ssl_info_with_ssl_enabled__issue_7986(self):
        _, out, _ = self.user.assert_icommand(['imiscsvrinfo'], 'STDOUT')
        expected_outputs = ['enabled: true', 'issuer_name', 'not_after', 'not_before', 'public_key', 'subject_alternative_names', 'subject_name']
        for output in expected_outputs:
            self.assertIn(output, out);

    @unittest.skipIf(test.settings.USE_SSL, 'Test case assumes absence of SSL/TLS')
    def test_imiscsvrinfo_prints_ssl_info_with_ssl_disabled__issue_7986(self):

        # Attributes should be missing with SSL disabled
        expected_missing = ['enabled: true', 'issuer_name', 'not_after', 'not_before', 'public_key', 'subject_alternative_names', 'subject_name']
        _, out, _ = self.user.assert_icommand(['imiscsvrinfo'], 'STDOUT', 'enabled: false')

        for output in expected_missing:
            self.assertNotIn(output, out);
