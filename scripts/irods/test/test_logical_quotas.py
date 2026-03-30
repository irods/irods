import os
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

from . import session
from .. import lib
from .. import paths
from .resource_suite import ResourceBase
from ..configuration import IrodsConfig
from ..controller import IrodsController

class Test_Logical_Quotas(session.make_sessions_mixin([('otherrods', 'rods')], []), unittest.TestCase):

    def setUp(self):
        super(Test_Logical_Quotas, self).setUp()

        self.admin = self.admin_sessions[0]

        self.quota_user = session.mkuser_and_return_session('rodsuser', 'lquotauser', 'lquotapass', lib.get_hostname())
        self.llq_output_template = 'Collection name: %s\nMaximum bytes: %s\nMaximum objects: %s\nBytes over: %s\nObjects over: %s'

    def tearDown(self):
        self.quota_user.__exit__()
        self.admin.assert_icommand(['iadmin', 'rmuser', 'lquotauser'])
        super(Test_Logical_Quotas, self).tearDown()

    def test_logical_quota_set_nonexistent(self):
        self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}/blah', 'bytes', '10000'], 'STDERR_SINGLELINE', 'CAT_INVALID_ARGUMENT' )

    def test_logical_quota_list_nonexistent(self):
        self.admin.assert_icommand(['iadmin', 'llq', f'{self.quota_user.session_collection}/blah'], 'STDERR_SINGLELINE', 'CAT_UNKNOWN_COLLECTION' )

    def test_logical_quota_set_bad_values(self):
        try:
            # Nonexistent keyword
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', 'potatoes', '10000'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )

            # Negative values
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', 'bytes', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', 'objects', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', '0', '--', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', '--', '-10000', '10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', '--', '-10000', '-10000'], 'STDERR_SINGLELINE', 'USER_INPUT_FORMAT_ERR' )

            # Invalid final parameter
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', 'bytes', 'tomatoes'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )

            # Numeric overflow
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}', 'bytes', '218128128218374562312312412412'], 'STDERR_SINGLELINE', 'SYS_INVALID_INPUT_PARAM' )
        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])

    def test_logical_quota_set(self):
        try: 
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'bytes', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', '-10000', '<unenforced>')) in out)
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'objects', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '10000', '-10000', '0')) in out)
        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])

    def test_logical_quota_enforcement(self):
        file_name = 'test_logical_quota_enforcement'
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')

        try: 
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'bytes', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', '-10000', '<unenforced>')) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', str(4096 - 10000), '<unenforced>')) in out)

            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'objects', '50'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096 - 10000), str(1 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_2'])
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096*3 - 10000), str(3 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), '{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])


    def test_nested_logical_quota_enforcement(self):
        file_name = 'test_nested_logical_quota_enforcement'
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')

        try: 
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'bytes', '10000'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', '-10000', '<unenforced>')) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '<unset>', str(4096 - 10000), '<unenforced>')) in out)

            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'objects', '50'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096 - 10000), str(1 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_2'])
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '10000', '50', str(4096*3 - 10000), str(3 - 50))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), '{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])
