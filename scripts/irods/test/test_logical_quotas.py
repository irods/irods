import os
import sys
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
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '1'])

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

            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'bytes', '100000'])
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'objects', '2'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '100000', '2', str(4096*3 - 100000), str(3 - 2))) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), '{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, 'objects', '10'])
            self.admin.assert_icommand(['iadmin', 'clu'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '100000', '10', str(4096*3 - 100000), str(3 - 10))) in out)

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), '{file_name}_4'])

        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '0'])


    def test_nested_logical_quota_enforcement(self):
        nesting_depth = 5
        self.quota_user.assert_icommand(['imkdir', f'{self.quota_user.session_collection}/test_nested_logical_quota_coll'])
        def subcoll_path_generator():
            path = 'test_nested_logical_quota_coll'
            count = 0
            while True:
                path = f'{path}/{count}'
                count = count + 1
                yield path

        file_name = 'test_nested_logical_quota_enforcement'
        # Violate quota in one shot
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 10001, contents='arbitrary')

        try:
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '1'])

            path_gen = subcoll_path_generator()
            innermost_subcoll = ''

            max_bytes = 10000
            # Set nested quotas
            # Make outermost quota most restrictive
            for i in range(0, nesting_depth):
                subcoll = next(path_gen)
                self.quota_user.assert_icommand(['imkdir', f'{self.quota_user.session_collection}/{subcoll}'])
                self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}/{subcoll}', 'bytes', str(max_bytes) ])
                _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', f'{self.quota_user.session_collection}/{subcoll}')
                self.assertTrue((self.llq_output_template % (f'{self.quota_user.session_collection}/{subcoll}', str(max_bytes), '<unset>', str(-max_bytes), '<unenforced>')) in out)
                max_bytes = max_bytes * 2
                innermost_subcoll = subcoll

            path_gen = subcoll_path_generator()
            subcoll = next(path_gen)

            # Put to innermost subcoll
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', f'{self.quota_user.session_collection}/{subcoll}')
            self.assertTrue((self.llq_output_template % (f'{self.quota_user.session_collection}/{subcoll}', '10000', '<unset>', str(10001 - 10000), '<unenforced>')) in out)

            # Putting to innermost subcoll should violate the outermost subcoll's quota
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # Lift byte restriction on outermost subcoll
            # Add 2-object quota on outermost
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}/{subcoll}', '0', '2'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            # Should succeed now
            # Should violate byte quota one level deep after recalculation
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_2'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

            # Lift byte restriction on one-level-deep subcoll
            self.admin.assert_icommand(['iadmin', 'slq', f'{self.quota_user.session_collection}/{next(path_gen)}', 'bytes', '0'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            # Should succeed now
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_3'])
            self.admin.assert_icommand(['iadmin', 'clu'])

            # Should now violate object quota on outermost collection
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{innermost_subcoll}/{file_name}_4'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')

        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '0'])

    def test_logical_quota_msi(self):
        file_name = 'test_logical_quota_msi'
        lib.make_file(os.path.join(self.quota_user.local_session_dir, file_name), 4096, contents='arbitrary')
        self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '1'])

        try:
            self.admin.assert_icommand(['irule', f'msi_set_logical_quota("{self.quota_user.session_collection}", "bytes", "4000")', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '4000', '<unset>', '-4000', '<unenforced>')) in out)

            # Check that the alternate invocation works too
            self.admin.assert_icommand(['irule', f'msi_set_logical_quota("{self.quota_user.session_collection}", "3000", "10")', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            # Bytes over still at -4000 since no recalculation done yet
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '3000', '10', '-4000', '0')) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), f'{self.quota_user.session_collection}/{file_name}'])
            self.admin.assert_icommand(['irule', f'msi_calc_logical_usage', 'null', 'null'])
            _, out, _ = self.admin.assert_icommand(['iadmin', 'llq'], 'STDOUT_SINGLELINE', self.quota_user.session_collection)
            self.assertTrue((self.llq_output_template % (self.quota_user.session_collection, '3000', '10', str(4096 - 3000), str(1 - 10))) in out)
            self.quota_user.assert_icommand(['iput', os.path.join(self.quota_user.local_session_dir, file_name), '{file_name}_2'],'STDERR_SINGLELINE', 'LOGICAL_QUOTA_EXCEEDED')
        finally:
            self.admin.assert_icommand(['iadmin', 'slq', self.quota_user.session_collection, '0', '0'])
            self.admin.assert_icommand(['iadmin', 'set_grid_configuration', 'logical_quotas', 'enforcement_enabled', '0'])
