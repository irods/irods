from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
from .. import lib
from . import session

class TestParallelReplication(session.make_sessions_mixin([], []), unittest.TestCase):

    def setUp(self):
        super(TestParallelReplication, self).setUp()
        with session.make_session_for_existing_admin() as admin:
             admin.assert_icommand(['iadmin', "mkresc", "testResc", 'unixfilesystem', lib.get_hostname() + ":" + "/tmp/irods/testResc"], 'STDOUT_SINGLELINE', 'unixfilesystem')
             admin.assert_icommand(['iadmin', "mkresc", "anotherResc", 'unixfilesystem', lib.get_hostname() + ":" + "/tmp/irods/anotherResc"], 'STDOUT_SINGLELINE', 'unixfilesystem')
             admin.assert_icommand(['iadmin', "mkuser", "alice", 'rodsuser'])
             admin.assert_icommand(['iadmin', "moduser", "alice", 'password', 'apass'])

    def tearDown(self):
        super(TestParallelReplication, self).tearDown()
        with session.make_session_for_existing_admin() as admin:
             admin.assert_icommand(['iadmin', "rmresc", "testResc"])
             admin.assert_icommand(['iadmin', "rmresc", "anotherResc"])
             admin.assert_icommand(['iadmin', "rmuser", "alice"])

    def test_replicate_object(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_replicate_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _, _ = admin.run_icommand(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                endpoint = 'replicate'
                options  = """{{"logical_path" : "{lp}",
                                "source_resource" : "{src}",
                               "destination_resource" : "{dst}" }}""".format(lp = logical_path, src = "demoResc", dst = "testResc")

                _, _ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDOUT_SINGLELINE', 'testResc')
                admin.assert_icommand(['irm', '-f', logical_path])
            finally:
                os.remove(file_name)

    def test_replicate_object_with_admin_mode(self):
            logical_path = ""
            file_name = 'test_replicate_object_with_admin_mode'

            try:
                lib.make_file(file_name, 1024)

                with session.make_session_for_existing_user('alice', 'apass', lib.get_hostname(), 'tempZone') as alice:
                    alice.assert_icommand(['iput', '-f', file_name])
                    alice.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)
                    pwd, _, _ = alice.run_icommand(['ipwd'])
                    print(pwd)
                    pwd = pwd.rstrip()
                    logical_path = os.path.join(pwd, file_name)

                endpoint = 'replicate'
                options  = """{{"logical_path" : "{lp}",
                                "admin_mode" : true,
                                "source_resource" : "{src}",
                                "destination_resource" : "{dst}" }}""".format(lp = logical_path, src = "demoResc", dst = "testResc")

                _, _ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                with session.make_session_for_existing_user('alice', 'apass', lib.get_hostname(), 'tempZone') as alice:
                    alice.assert_icommand(['ils', '-l', logical_path], 'STDOUT_SINGLELINE', 'testResc')
            finally:
                os.remove(file_name)
                with session.make_session_for_existing_user('alice', 'apass', lib.get_hostname(), 'tempZone') as alice:
                    alice.assert_icommand(['irm', '-f', logical_path])

    def test_update_single_replica(self):
        with session.make_session_for_existing_admin() as admin:
            logical_path = ""
            try:
                file_name = 'test_replicate_object'
                lib.make_file(file_name, 1024)

                pwd, _, _ = admin.run_icommand(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)
                admin.assert_icommand(['irepl', '-R', 'testResc', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)
                admin.assert_icommand(['iput', '-f', '-n', '0', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)

                endpoint = 'replicate'
                options  = """{{"logical_path" : "{lp}",
                                "update_single_replica" : true,
                                "source_resource" : "{src}",
                                "destination_resource" : "{dst}" }}""".format(lp = logical_path, src = "demoResc", dst = "testResc")

                _, _ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                out, _, _ = admin.run_icommand(['ils', '-l', logical_path])

                print('OUTPUT: '+out)

                assert 2 == out.count('&')
            finally:
                admin.assert_icommand(['irm', '-f', logical_path])
                os.remove(file_name)

    def test_update_all_replicas(self):
        with session.make_session_for_existing_admin() as admin:
            logical_path = ""
            try:
                file_name = 'test_replicate_object'
                lib.make_file(file_name, 1024)

                pwd, _, _ = admin.run_icommand(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)
                admin.assert_icommand(['irepl', '-R', 'testResc', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)
                admin.assert_icommand(['irepl', '-R', 'anotherResc', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)
                admin.assert_icommand(['iput', '-f', '-n', '0', file_name])
                admin.assert_icommand(['ils', '-l', file_name], 'STDOUT_SINGLELINE', file_name)

                endpoint = 'replicate'
                options  = """{{"logical_path" : "{lp}",
                                "update_all_replicas" : true,
                                "source_resource" : "{src}"}}""".format(lp = logical_path, src = "demoResc")

                _, _ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                out, _, _ = admin.run_icommand(['ils', '-l', logical_path])

                print('OUTPUT: '+out)

                assert 3 == out.count('&')
            finally:
                admin.assert_icommand(['irm', '-f', logical_path])
                os.remove(file_name)

