from __future__ import print_function
import copy
import logging
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import pprint
import time
import json
import tempfile
import shutil

from .. import test
from . import settings
from .. import lib
from ..configuration import IrodsConfig
from ..controller import IrodsController
from .. import paths
from . import resource_suite
from . import session

class TestParallelRecursiveRemove(session.make_sessions_mixin([], []), unittest.TestCase):

    def setUp(self):
        super(TestParallelRecursiveRemove, self).setUp()

    def tearDown(self):
        super(TestParallelRecursiveRemove, self).tearDown()

    def test_remove_object(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_remove_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}"}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDERR_SINGLELINE', 'ERROR')
            finally:
                os.remove(file_name)

    def test_remove_object_no_trash(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                admin.assert_icommand(['irmtrash'])

                file_name = 'test_remove_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['iput', '-f', file_name])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}",
                               "no_trash"     : true}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDERR_SINGLELINE', 'ERROR')
            finally:
                os.remove(file_name)

    def test_unregister_object(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                file_name = 'test_remove_object'
                lib.make_file(file_name, 1024)

                admin.assert_icommand(['ireg', '/var/lib/irods/scripts/test_remove_object', '/tempZone/home/rods/test_remove_object'])
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', file_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, file_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}",
                               "unregister"     : true}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDERR_SINGLELINE', 'ERROR')

                assert(os.path.exists('/var/lib/irods/scripts/test_remove_object'))
            finally:
                os.remove(file_name)

    def test_remove_collection(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dir_name = 'test_remove_collection_directory'

                lib.make_large_local_tmp_dir(dir_name, 10, 1024)

                admin.assert_icommand(['iput', '-f', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}"}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDERR_SINGLELINE', 'ERROR')
            finally:
                shutil.rmtree(dir_name)

    def test_remove_collection_no_trash(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                admin.assert_icommand(['irmtrash'])

                dir_name = 'test_remove_collection_directory'

                lib.make_large_local_tmp_dir(dir_name, 10, 1024)

                admin.assert_icommand(['iput', '-f', '-r', dir_name], 'STDOUT_SINGLELINE', 'Running')
                admin.assert_icommand(['ils', '-l'], 'STDOUT_SINGLELINE', dir_name)

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}",
                                "no_trash"     : true}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', logical_path], 'STDERR_SINGLELINE', 'ERROR')

            finally:
                shutil.rmtree(dir_name)

    def test_unregister_collection(self):
        with session.make_session_for_existing_admin() as admin:
            try:
                dir_name = 'test_remove_collection_directory'

                lib.make_large_local_tmp_dir(dir_name, 10, 1024)

                admin.assert_icommand(['imkdir', dir_name])

                pwd, _ = lib.execute_command(['ipwd'])
                pwd = pwd.rstrip()
                logical_path = os.path.join(pwd, dir_name)

                for filename in os.listdir(dir_name):
                    physical = os.path.abspath(os.path.join(dir_name, filename))
                    logical  = logical_path + '/' + filename
                    admin.assert_icommand(['ireg', physical, logical])

                admin.assert_icommand(['ils', '-l', dir_name], 'STDOUT_SINGLELINE', dir_name)

                endpoint = 'remove'
                options  = """{{"logical_path" : "{lp}",
                               "unregister"   : true}}""".format(lp = logical_path)
                out,_ = lib.execute_command(['irods_api_test_harness', options, endpoint])

                admin.assert_icommand(['ils', '-l', dir_name], 'STDERR_SINGLELINE', 'ERROR')

                ctr = 0
                for filename in os.listdir(dir_name):
                    ctr = ctr + 1
                assert(10 == ctr)
            finally:
                shutil.rmtree(dir_name)

