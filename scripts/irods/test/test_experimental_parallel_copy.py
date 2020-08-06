from __future__ import print_function


import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os




import shutil



from .. import lib




from . import session

class TestParallelCopy(session.make_sessions_mixin([], []), unittest.TestCase):

    def setUp(self):
        super(TestParallelCopy, self).setUp()

    def tearDown(self):
        super(TestParallelCopy, self).tearDown()

    def test_copy_object(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_copy_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)
                destination  = logical_path+"_COPY"

                endpoint = 'copy'
                options  = """{{"logical_path" : "{lp}", "destination_logical_path" : "{d}"}}""".format(lp = logical_path, d=destination)
                lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', destination], 'STDOUT_SINGLELINE', 'demoResc')
                admin.assert_icommand(['ips'], 'STDOUT_SINGLELINE', 'ips')
            finally:
                os.remove(file_name)
                admin.assert_icommand(['irm', destination])
                admin.assert_icommand(['irm', logical_path])

    def test_copy_collection(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dir_name = 'test_copy_collection_directory'

                lib.make_large_local_tmp_dir(dir_name, 10, 1024)

                admin.assert_icommand(['iput', '-f', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)
                destination  = logical_path+"_COPY"

                endpoint = 'copy'
                options  = """{{"logical_path" : "{lp}", "destination_logical_path" : "{d}"}}""".format(lp = logical_path, d = destination)
                lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', destination], 'STDOUT_SINGLELINE', destination)
                admin.assert_icommand(['ips'], 'STDOUT_SINGLELINE', 'ips')
            finally:
                shutil.rmtree(dir_name)
                admin.assert_icommand(['irm', '-r', destination])
                admin.assert_icommand(['irm', '-r', logical_path])

    def test_copy_nested_collection(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dir_name = 'test_copy_nested_collection_directory'

                # test settings
                depth = 4
                files_per_level = 4
                file_size = 1024*1024*40

                lib.make_deep_local_tmp_dir(dir_name, depth, files_per_level, file_size)

                admin.assert_icommand(['iput', '-f', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)
                destination  = logical_path+"_COPY"

                endpoint = 'copy'
                options  = """{{"logical_path" : "{lp}", "destination_logical_path" : "{d}"}}""".format(lp = logical_path, d = destination)
                lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', destination], 'STDOUT_SINGLELINE', destination)
                admin.assert_icommand(['ips'], 'STDOUT_SINGLELINE', 'ips')
            finally:
                shutil.rmtree(dir_name)
                admin.assert_icommand(['irm', '-r', destination])
                admin.assert_icommand(['irm', '-r', logical_path])

