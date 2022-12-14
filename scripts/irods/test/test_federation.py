from __future__ import print_function
import hashlib
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import shutil
import socket
import tempfile
import time

from . import session
from . import settings
from .. import lib
from .. import paths
from .. import test
from . import ustrings
from ..configuration import IrodsConfig
from ..core_file import temporary_core_file

SessionsMixin = session.make_sessions_mixin(
    test.settings.FEDERATION.RODSADMIN_NAME_PASSWORD_LIST, test.settings.FEDERATION.RODSUSER_NAME_PASSWORD_LIST)


class Test_ICommands(SessionsMixin, unittest.TestCase):

    def setUp(self):
        super(Test_ICommands, self).setUp()

        # make local test directory
        self.local_test_dir_path = '/tmp/federation_test_stuff'
        os.mkdir(self.local_test_dir_path)

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in test.settings.FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val
        self.config['local_zone'] = self.user_sessions[0].zone_name
        if test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 0):
            test.settings.FEDERATION.REMOTE_VAULT = '/home/irods/irods-legacy/iRODS/Vault'

    def tearDown(self):
        shutil.rmtree(self.local_test_dir_path, ignore_errors=True)
        super(Test_ICommands, self).tearDown()

    def test_iquest__3466(self):
        if 'otherZone' == test.settings.FEDERATION.REMOTE_ZONE:
            self.admin_sessions[0].assert_icommand('iquest -z otherZone --sql bug_3466_query', 'STDOUT_SINGLELINE', 'bug_3466_query')

    def test_ils_l(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        with tempfile.NamedTemporaryFile() as f:
            filename = os.path.basename(f.name)
            filesize = test.settings.FEDERATION.TEST_FILE_SIZE
            lib.make_file(f.name, filesize, 'arbitrary')
            remote_home_collection = test_session.remote_home_collection(
                test.settings.FEDERATION.REMOTE_ZONE)

            test_session.assert_icommand(
                ['ils', '-L', remote_home_collection], 'STDOUT_SINGLELINE', remote_home_collection)
            test_session.assert_icommand(
                ['iput', f.name, remote_home_collection])

            # list file info
            test_session.assert_icommand(
                ['ils', '-L', '{0}/{1}'.format(remote_home_collection, filename)], 'STDOUT_SINGLELINE', filename)
            test_session.assert_icommand(
                ['ils', '-L', '{0}/{1}'.format(remote_home_collection, filename)], 'STDOUT_SINGLELINE', str(filesize))
            test_session.assert_icommand(
                ['ils', '-L', '{0}/{1}'.format(remote_home_collection, filename)], 'STDOUT_SINGLELINE', test.settings.FEDERATION.REMOTE_DEF_RESOURCE)
            test_session.assert_icommand(
                ['ils', '-L', '{0}/{1}'.format(remote_home_collection, filename)], 'STDOUT_SINGLELINE', test.settings.FEDERATION.REMOTE_VAULT)

            # cleanup
            test_session.assert_icommand(
                ['irm', '-f', '{0}/{1}'.format(remote_home_collection, filename)])

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 0), 'Fixed in 4.2.0')
    def test_ils_A(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        with tempfile.NamedTemporaryFile() as f:
            filename = os.path.basename(f.name)
            filesize = test.settings.FEDERATION.TEST_FILE_SIZE
            lib.make_file(f.name, filesize, 'arbitrary')
            remote_home_collection = test_session.remote_home_collection(test.settings.FEDERATION.REMOTE_ZONE)
            username = test_session.username
            local_zone = test_session.zone_name

            # put file in remote collection
            test_session.assert_icommand(
                ['iput', f.name, remote_home_collection])

            # icd to remote collection
            test_session.assert_icommand(['icd', remote_home_collection])

            # list object's ACLs
            test_session.assert_icommand(
                ['ils', '-A', filename], 'STDOUT_SINGLELINE', "ACL - {username}#{local_zone}:own".format(**locals()))

            # cleanup
            test_session.assert_icommand(
                ['irm', '-f', '{0}/{1}'.format(remote_home_collection, filename)])


    @unittest.skipUnless(test.settings.USE_SSL, 'This test uses SSL and so it is required in order to run.')
    def test_ils_with_misconfigured_ssl_catches_exceptions__issue_6365(self):
        test_session = self.user_sessions[0]
        remote_home_collection = test_session.remote_home_collection(test.settings.FEDERATION.REMOTE_ZONE)
        test_session.assert_icommand(['ils', remote_home_collection], 'STDOUT_SINGLELINE', remote_home_collection)
        with temporary_core_file() as core:
            # Disable SSL communications in the local server. This should break communications with the remote zone,
            # which is supposed to be configured for SSL communications.
            core.add_rule('acPreConnect(*OUT) { *OUT = "CS_NEG_REFUSE"; }')

            # Disable SSL communications in the service account client environment so that it can communicate with
            # the local server, which has just disabled SSL communications.
            env_update = {'irods_client_server_policy': 'CS_NEG_REFUSE'}
            service_account_env_file = os.path.join(paths.irods_directory(), '.irods', "irods_environment.json")
            with lib.file_backed_up(service_account_env_file):
                lib.update_json_file_from_dict(service_account_env_file, env_update)

                # Disable SSL communications in the test session client environment so that it can communicate with
                # the local server, which has just disabled SSL communications.
                client_env_file = os.path.join(test_session.local_session_dir, "irods_environment.json")
                with lib.file_backed_up(client_env_file):
                    lib.update_json_file_from_dict(client_env_file, env_update)

                    # Make sure communications with the local zone are in working order...
                    _, pwd, _ = test_session.assert_icommand(['ipwd'], 'STDOUT', test_session.zone_name)
                    test_session.assert_icommand(['ils'], 'STDOUT_SINGLELINE', pwd.strip())

                    # ils in the remote zone should fail due to the misconfigured SSL settings, but not explode.
                    out, err, rc = test_session.run_icommand(['ils', remote_home_collection])
                    self.assertNotEqual(0, rc)
                    self.assertEqual(0, len(out))
                    self.assertIn('iRODS filesystem error occurred', err)
                    self.assertNotIn('terminating with uncaught exception', err)

    def test_ils_subcolls(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # test specific parameters
        parameters = self.config.copy()
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)
        parameters['subcoll0'] = "{remote_home_collection}/subcoll0".format(
            **parameters)
        parameters['subcoll1'] = "{remote_home_collection}/subcoll1".format(
            **parameters)

        # make subcollections in remote coll
        test_session.assert_icommand("imkdir {subcoll0}".format(**parameters))
        test_session.assert_icommand("imkdir {subcoll1}".format(**parameters))

        # list remote home collection and look for subcollections
        test_session.assert_icommand(
            "ils {remote_home_collection}".format(**parameters), 'STDOUT_MULTILINE', [parameters['subcoll0'], parameters['subcoll1']])

        # cleanup
        test_session.assert_icommand("irm -r {subcoll0}".format(**parameters))
        test_session.assert_icommand("irm -r {subcoll1}".format(**parameters))

    def test_iput(self):
        self.basic_iput_test(self.config['test_file_size'])

    def test_iput_large_file(self):
        self.basic_iput_test(self.config['large_file_size'])

    def basic_iput_test(self, filesize):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'iput_test_file'
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        if filesize >= self.config['large_file_size']:
            # put file in remote collection, ask for 4 threads
            test_session.assert_icommand(
                "iput -v -N 4 {filepath} {remote_home_collection}/".format(**parameters), 'STDOUT_SINGLELINE', '4 thr')
        else:
            # put file in remote collection
            test_session.assert_icommand(
                "iput {filepath} {remote_home_collection}/".format(**parameters))

        # file should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 1, 9), 'Fixed in 4.1.9')
    def test_slow_ils_over_federation__ticket_3215(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'iput_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, 500, 30)

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in remote collection
        test_session.assert_icommand(
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # time listing of collection
        t0 = time.time()
        test_session.assert_icommand(
            "ils -AL {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)
        t1 = time.time()

        diff = t1 - t0
        self.assertTrue(diff<20)

        # cleanup
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    def test_iput_r(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'iput_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.config['test_file_count'], self.config['test_file_size'])

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in remote collection
        test_session.assert_icommand(
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # new collection should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
        self.assertTrue(set(local_files) == rods_files)

        # cleanup
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    def test_iget(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'iget_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # checksum local file
        orig_md5 = lib.file_digest(filepath, 'md5')

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # remove local file
        os.remove(filepath)

        # get file back
        test_session.assert_icommand(
            "iget {remote_home_collection}/{filename} {filepath}".format(**parameters))

        # compare checksums
        new_md5 = lib.file_digest(filepath, 'md5')
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

    def test_iget_large_file(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'iget_test_file'
        filesize = self.config['large_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # checksum local file
        orig_md5 = lib.file_digest(filepath, 'md5')

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # remove local file
        os.remove(filepath)

        # for the next transfer we expect the number of threads
        # to be capped at max_threads or max_threads+1,
        # e.g: we will look for '4 thr' or '5 thr' in stdout
        parameters['max_threads_plus_one'] = parameters['max_threads'] + 1
        expected_output_regex = '[{max_threads}|{max_threads_plus_one}] thr'.format(
            **parameters)

        # get file back, ask for too many threads (should be capped)
        test_session.assert_icommand(
            "iget -v -N 600 {remote_home_collection}/{filename} {filepath}".format(**parameters), 'STDOUT_SINGLELINE', expected_output_regex, use_regex=True)

        # compare checksums
        new_md5 = lib.file_digest(filepath, 'md5')
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

    def test_iget_r(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'iget_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.config['test_file_count'], self.config['test_file_size'])

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in remote collection
        test_session.assert_icommand(
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # remove local test dir
        shutil.rmtree(dir_path)

        # get collection back
        test_session.assert_icommand(
            "iget -r {remote_home_collection}/{dir_name} {dir_path}".format(**parameters))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    def test_irm_f(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'irm_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # file should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)

        # delete remote file
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))

        # file should be gone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        os.remove(filepath)

    def test_irm_rf(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'irm_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.config['test_file_count'], self.config['test_file_size'])

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in remote collection
        test_session.assert_icommand(
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # new collection should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
        self.assertTrue(set(local_files) == rods_files)

        # remove remote coll
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))

        # coll should be gone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        shutil.rmtree(dir_path)

    def test_icp(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'icp_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        try:
            lib.make_file(filepath, filesize)

            # checksum local file
            orig_md5 = lib.file_digest(filepath, 'md5')

            # put file in local collection
            test_session.assert_icommand(
                "iput {filepath} {local_home_collection}/".format(**parameters))

            # remove local file
            os.remove(filepath)

            # copy file to remote home collection
            test_session.assert_icommand(
                "icp {local_home_collection}/{filename} {remote_home_collection}/".format(**parameters))

            # file should show up in remote zone
            test_session.assert_icommand(
                "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)
            test_session.assert_icommand(
                "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))

            # get file back from remote zone
            test_session.assert_icommand(
                "iget {remote_home_collection}/{filename} {filepath}".format(**parameters))

            # compare checksums
            new_md5 = lib.file_digest(filepath, 'md5')
            self.assertEqual(orig_md5, new_md5)

        finally:
            print(test_session.run_icommand("ils -L {remote_home_collection}")[0])

            # cleanup
            test_session.run_icommand(
                "irm -f {local_home_collection}/{filename}".format(**parameters))
            test_session.run_icommand(
                "irm -f {remote_home_collection}/{filename}".format(**parameters))
            os.remove(filepath)

    def test_icp_large(self):
        # test settings
        remote_zone = self.config['remote_zone']
        test_session = self.user_sessions[0]
        local_zone = test_session.zone_name
        user_name = test_session.username
        local_home_collection = test_session.home_collection
        remote_home_collection = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **locals())

        # make test file
        filename = 'icp_test_file'
        filesize = self.config['large_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # checksum local file
        orig_md5 = lib.file_digest(filepath, 'md5')

        # put file in local collection
        test_session.assert_icommand(
            "iput {filepath} {local_home_collection}/".format(**locals()))

        # remove local file
        os.remove(filepath)

        # copy file to remote home collection
        test_session.assert_icommand(
            "icp {local_home_collection}/{filename} {remote_home_collection}/".format(**locals()))

        # file should show up in remote zone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**locals()), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**locals()), 'STDOUT_SINGLELINE', str(filesize))

        # get file back from remote zone
        test_session.assert_icommand(
            "iget {remote_home_collection}/{filename} {filepath}".format(**locals()))

        # compare checksums
        new_md5 = lib.file_digest(filepath, 'md5')
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        test_session.assert_icommand(
            "irm -f {local_home_collection}/{filename}".format(**locals()))
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**locals()))
        os.remove(filepath)

    def test_icp_f_large(self):
        # test settings
        remote_zone = self.config['remote_zone']
        test_session = self.user_sessions[0]
        local_zone = test_session.zone_name
        user_name = test_session.username
        local_home_collection = test_session.home_collection
        remote_home_collection = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **locals())

        # make test file
        filename = 'icp_test_file'
        filesize = self.config['large_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # checksum local file
        orig_md5 = lib.file_digest(filepath, 'md5')

        # put file in local collection
        test_session.assert_icommand(
            "iput {filepath} {local_home_collection}/".format(**locals()))

        # remove local file
        os.remove(filepath)

        # copy file to remote home collection
        test_session.assert_icommand(
            "icp -f {local_home_collection}/{filename} {remote_home_collection}/".format(**locals()))

        # file should show up in remote zone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**locals()), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**locals()), 'STDOUT_SINGLELINE', str(filesize))

        # get file back from remote zone
        test_session.assert_icommand(
            "iget {remote_home_collection}/{filename} {filepath}".format(**locals()))

        # compare checksums
        new_md5 = lib.file_digest(filepath, 'md5')
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        test_session.assert_icommand(
            "irm -f {local_home_collection}/{filename}".format(**locals()))
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**locals()))
        os.remove(filepath)

    def test_icp_r(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'icp_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.config['test_file_count'], self.config['test_file_size'])

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in local collection
        test_session.assert_icommand(
            "iput -r {dir_path} {local_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # remove local test dir
        shutil.rmtree(dir_path)

        # copy dir to remote home collection
        test_session.assert_icommand(
            "icp -r {local_home_collection}/{dir_name} {remote_home_collection}/{dir_name}".format(**parameters))

        # collection should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
        self.assertTrue(set(local_files) == rods_files)

        # get collection back
        test_session.assert_icommand(
            "iget -r {remote_home_collection}/{dir_name} {dir_path}".format(**parameters))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        test_session.assert_icommand(
            "irm -rf {local_home_collection}/{dir_name}".format(**parameters))
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    def test_imv(self):
        '''
        remote-remote imv test
        (SYS_CROSS_ZONE_MV_NOT_SUPPORTED)
        '''
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'imv_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['new_name'] = filename = '_new'
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # move (rename) remote file
        test_session.assert_icommand(
            "imv {remote_home_collection}/{filename} {remote_home_collection}/{new_name}".format(**parameters))

        # file should have been renamed
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDERR_SINGLELINE', 'does not exist')
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{new_name}".format(**parameters), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{new_name}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{new_name}".format(**parameters))
        os.remove(filepath)

    def test_irsync_r_dir_to_coll(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # test specific parameters
        dir_name = 'irsync_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)

        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        try:
            # make test dir
            local_files = lib.make_large_local_tmp_dir(
                dir_path, self.config['test_file_count'], self.config['test_file_size'])

            # sync dir with remote collection
            test_session.assert_icommand(
                "irsync -r {dir_path} i:{remote_home_collection}/{dir_name}".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

            # new collection should be there
            test_session.assert_icommand(
                "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

            # files should be there
            rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
            self.assertTrue(set(local_files) == rods_files)

        finally:
            # cleanup
            test_session.run_icommand(
                "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
            shutil.rmtree(dir_path)

    def test_irsync_r_coll_to_coll(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # test specific parameters
        dir_name = 'irsync_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)

        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        try:
            # make test dir
            local_files = lib.make_large_local_tmp_dir(
                dir_path, self.config['test_file_count'], self.config['test_file_size'])

            # put dir in local collection
            test_session.assert_icommand(
                "iput -r {dir_path} {local_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

            # remove local test dir
            shutil.rmtree(dir_path)

            # sync local collection with remote collection
            test_session.assert_icommand(
                "irsync -r i:{local_home_collection}/{dir_name} i:{remote_home_collection}/{dir_name}".format(**parameters))

            # collection should be there
            test_session.assert_icommand(
                "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

            # files should be there
            rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
            self.assertTrue(set(local_files) == rods_files)

            # get collection back
            test_session.assert_icommand(
                "iget -r {remote_home_collection}/{dir_name} {dir_path}".format(**parameters))

            # compare list of files
            received_files = os.listdir(dir_path)
            self.assertTrue(set(local_files) == set(received_files))

        finally:
            # cleanup
            test_session.run_icommand(
                "irm -rf {local_home_collection}/{dir_name}".format(**parameters))
            test_session.run_icommand(
                "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
            shutil.rmtree(dir_path)

    def test_irsync_r_coll_to_dir(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # test specific parameters
        dir_name = 'irsync_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)

        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        try:
            # make test dir
            local_files = lib.make_large_local_tmp_dir(
                dir_path, self.config['test_file_count'], self.config['test_file_size'])

            # sync dir with remote collection
            test_session.assert_icommand(
                "irsync -r {dir_path} i:{remote_home_collection}/{dir_name}".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

            # remove local test dir
            shutil.rmtree(dir_path)

            # sync remote collection back with local dir
            test_session.assert_icommand(
                "irsync -r i:{remote_home_collection}/{dir_name} {dir_path}".format(**parameters))

            # compare list of files
            received_files = os.listdir(dir_path)
            self.assertTrue(set(local_files) == set(received_files))

        finally:
            # cleanup
            test_session.run_icommand(
                "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
            shutil.rmtree(dir_path)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 0, 0) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 0), 'No resource hierarchies before iRODS 4')
    def test_irsync_passthru_3016(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # test specific parameters
        filename = 'irsync_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)

        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # extract resources from hierarchies
        (parameters['local_pt_resc'], parameters['local_leaf_resc']) = tuple(
            parameters['local_pt_resc_hier'].split(';'))
        parameters['remote_pt_resc'] = parameters[
            'remote_pt_resc_hier'].split(';')[0]

        parameters['hostname'] = test.settings.ICAT_HOSTNAME
        parameters['local_leaf_resc_path'] = '/tmp/{local_leaf_resc}'.format(
            **parameters)

        try:
            # make test file
            lib.make_file(filepath, filesize)

            # create local passthru hierarchy
            self.admin_sessions[0].run_icommand(
                "iadmin mkresc {local_pt_resc} passthru".format(**parameters))
            self.admin_sessions[0].run_icommand(
                "iadmin mkresc {local_leaf_resc} unixfilesystem {hostname}:{local_leaf_resc_path}".format(**parameters))
            self.admin_sessions[0].run_icommand(
                "iadmin addchildtoresc {local_pt_resc} {local_leaf_resc}".format(**parameters))

            # checksum local file
            orig_md5 = lib.file_digest(filepath, 'md5')

            # put file in local collection, using local passthru resource
            test_session.assert_icommand(
                "iput -R {local_pt_resc} {filepath} {local_home_collection}/".format(**parameters))

            # remove local file
            os.remove(filepath)

            # rsync file into remote coll, using remote passthru resource
            test_session.assert_icommand(
                "irsync -R {remote_pt_resc} i:{local_home_collection}/{filename} i:{remote_home_collection}/{filename}".format(**parameters))

            # check that file is on remote zone's resource hierarchy
            test_session.assert_icommand(
                "ils -L {remote_home_collection}/{filename}".format(**parameters),  'STDOUT_MULTILINE', [filename, parameters['remote_pt_resc_hier']])

            # get file back and compare checksums
            if test.settings.FEDERATION.REMOTE_IRODS_VERSION != (4, 0, 3):
                test_session.assert_icommand(
                    "iget {remote_home_collection}/{filename} {filepath}".format(**parameters))
                new_md5 = lib.file_digest(filepath, 'md5')
                self.assertEqual(orig_md5, new_md5)
            else:
                test_session.assert_icommand(
                    "iget {remote_home_collection}/{filename} {filepath}".format(**parameters), 'STDERR_SINGLELINE', 'USER_RODS_HOSTNAME_ERR')

        finally:
            # cleanup
            test_session.run_icommand(
                "irm -f {local_home_collection}/{filename}".format(**parameters))
            test_session.run_icommand(
                "irm -f {remote_home_collection}/{filename}".format(**parameters))
            try:
                os.remove(filepath)
            except OSError:
                pass
            self.admin_sessions[0].run_icommand(
                "iadmin rmchildfromresc {local_pt_resc} {local_leaf_resc}".format(**parameters))
            self.admin_sessions[0].run_icommand(
                "iadmin rmresc {local_pt_resc}".format(**parameters))
            self.admin_sessions[0].run_icommand(
                "iadmin rmresc {local_leaf_resc}".format(**parameters))

    def test_ilsresc_z(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # list remote resources
        test_session.assert_icommand(
            "ilsresc --ascii -z {remote_zone}".format(**self.config), 'STDOUT_SINGLELINE', test.settings.FEDERATION.REMOTE_DEF_RESOURCE)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 0) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 0), 'No resource hierarchies before iRODS 4')
    def test_ilsresc_z_child_resc(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]
        test_session.assert_icommand(
            "ilsresc --ascii -z {remote_zone}".format(**self.config), 'STDOUT_SINGLELINE', test.settings.FEDERATION.REMOTE_PT_RESC_HIER.split(';')[1])

    def run_remote_writeLine_test(self, config, zone_info):
        # Some inputs and expected values
        localnum_before = 1
        localnum_after = 3
        localstring = 'one'
        inputnum_before = 4
        inputnum_after = 6
        inputstring = 'four'
        remotenum = 0
        remotestring = 'zero'
        expected_before_remote = 'stdout before remote: {0}, {1}, {2}, {3}'.format(
            inputnum_before, inputstring, localnum_before, localstring)
        expected_from_remote = 'stdout from remote: {0}, {1}, {2}, {3}, {4}, {5}'.format(
            inputnum_before, inputstring, localnum_before, localstring, remotenum, remotestring)
        expected_after_remote = 'stdout after remote: {0}, {1}, {2}, {3}'.format(
            inputnum_after, inputstring, localnum_after, localstring)

        if zone_info == 'local':
            zone = config['local_zone']
            host = socket.gethostname()
            # TODO: Add support for remote with #4164
            expected_from_remote_log = 'serverLog from remote: {0}, {1}, {2}, {3}, {4}, {5}'.format(
                inputnum_before, inputstring, localnum_before, localstring, remotenum, remotestring)
        else:
            zone = config['remote_zone']
            host = config['remote_host']

        # Write a line to the serverLog in the local zone using remote execution block
        rule_string = '''
myTestRule {{
    *localnum = {0};
    *localstring = "{1}";
    writeLine("stdout", "stdout before remote: *inputnum, *inputstring, *localnum, *localstring");
    remote("{2}", "<ZONE>{3}</ZONE>") {{
        *remotenum = {4};
        *remotestring = "{5}";
        writeLine("serverLog", "serverLog from remote: *inputnum, *inputstring, *localnum, *localstring, *remotenum, *remotestring");
        writeLine("stdout", "stdout from remote: *inputnum, *inputstring, *localnum, *localstring, *remotenum, *remotestring");
        *inputnum = *inputnum + 1
        *localnum = *localnum + 1
    }}
    *inputnum = *inputnum + 1
    *localnum = *localnum + 1
    writeLine("stdout", "stdout after remote: *inputnum, *inputstring, *localnum, *localstring");
}}
INPUT *inputnum={6}, *inputstring="{7}"
OUTPUT ruleExecOut
            '''.format(localnum_before, localstring, host, zone, remotenum, remotestring, inputnum_before, inputstring)

        rule_file = "test_rule_file.r"
        with open(rule_file, 'w') as f:
            f.write(rule_string)

        # TODO: Add support for remote with #4164
        if zone_info == 'local':
            initial_log_size = lib.get_file_size_by_path(paths.server_log_path())

        # Execute rule and ensure that output is empty (success)
        self.user_sessions[0].assert_icommand(['irule', '-F', rule_file], 'STDOUT_MULTILINE', [expected_before_remote, expected_from_remote, expected_after_remote])

        # TODO: Add support for remote with #4164
        if zone_info == 'local':
            lib.delayAssert(
                lambda: lib.log_message_occurrences_equals_count(
                    msg=expected_from_remote_log,
                    start_index=initial_log_size))
        os.remove(rule_file)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 3) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 2, 3), 'Fixed in 4.2.3')
    def test_remote_writeLine_localzone_3722(self):
        self.run_remote_writeLine_test(self.config.copy(), 'local')

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 3) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 2, 3), 'Fixed in 4.2.3')
    def test_remote_writeLine_remotezone_3722(self):
        self.run_remote_writeLine_test(self.config.copy(), 'remote')

    def test_imeta_qu_with_zone_name_option__issue_4426(self):
        user = self.user_sessions[0]
        parameters = self.config.copy()

        # Create a file and put it into the remote zone.
        parameters['filename'] = 'foo'
        parameters['filepath'] = os.path.join(user.local_session_dir, parameters['filename'])

        try:
            lib.make_file(parameters['filepath'], 1, 'arbitrary')

            parameters['user_name'] = user.username
            parameters['remote_home_collection'] = '/{remote_zone}/home/{user_name}#{local_zone}'.format(**parameters)
            parameters['remote_data_object'] = '{remote_home_collection}/{filename}'.format(**parameters)
            user.assert_icommand('iput {filepath} {remote_data_object}'.format(**parameters))

            # Add metadata to the new data object.
            user.assert_icommand('imeta add -d {remote_data_object} n1 v1 u1'.format(**parameters))
            user.assert_icommand('imeta ls -d {remote_data_object}'.format(**parameters), 'STDOUT', ['attribute: n1', 'value: v1', 'units: u1'])

            # Show that the remote data object can be found via "imeta -z qu".
            user.assert_icommand('imeta -z {remote_zone} qu -d n1 = v1'.format(**parameters), 'STDOUT', [parameters['filename']])

        finally:
            user.run_icommand(['irm', '-f', parameters['remote_data_object']])

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 9) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 2, 9), 'Only available in 4.2.9 and later')
    def test_federation_support_for_replica_open_close_and_get_file_descriptor_info(self):
        user = self.user_sessions[0]
        parameters = self.config.copy()

        # Create a new data object via istream.
        # istream proves that the following API plugins work in a federated environment.
        # - rx_get_file_descriptor_info
        # - rx_replica_open
        # - rx_replica_close
        parameters['filename'] = 'istream_test_file.txt'
        parameters['user_name'] = user.username
        parameters['remote_home_collection'] = '/{remote_zone}/home/{user_name}#{local_zone}'.format(**parameters)
        parameters['remote_data_object'] = '{remote_home_collection}/{filename}'.format(**parameters)
        contents = 'Hello, iRODS!'
        try:
            user.assert_icommand('istream write {remote_data_object}'.format(**parameters), input=contents)

            # Show that the data object exists and contains the expected content.
            user.assert_icommand('istream read {remote_data_object}'.format(**parameters), 'STDOUT', [contents])

        finally:
            user.run_icommand(['irm', '-f', parameters['remote_data_object']])

class Test_Admin_Commands(unittest.TestCase):

    '''
    Operations requiring administrative privilege,
    run in the remote zone by a local rodsadmin.
    They should all fail (disallowed by remote zone).
    '''

    def setUp(self):
        # make session with existing admin account
        self.admin_session = session.make_session_for_existing_admin()

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in test.settings.FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val
        self.config['local_zone'] = self.admin_session.zone_name

        super(Test_Admin_Commands, self).setUp()

    def tearDown(self):
        self.admin_session.__exit__()
        super(Test_Admin_Commands, self).tearDown()

    def test_ichmod(self):
        # test specific parameters
        parameters = self.config.copy()
        parameters['user_name'] = self.admin_session.username

        # try to modify ACLs in the remote zone
        self.admin_session.assert_icommand(
            "ichmod -rM own {user_name}#{local_zone} /{remote_zone}/home".format(**parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

        self.admin_session.assert_icommand(
            "ichmod -M own {user_name} /{remote_zone}/home".format(**parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

        self.admin_session.assert_icommand(
            "ichmod -M read {user_name} /{remote_zone}".format(**parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

    def test_iadmin(self):
        pass


class Test_Microservices(SessionsMixin, unittest.TestCase):

    def setUp(self):
        super(Test_Microservices, self).setUp()

        # make local test directory
        self.local_test_dir_path = os.path.abspath('federation_test_stuff.tmp')
        os.mkdir(self.local_test_dir_path)

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in test.settings.FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val
        self.config['local_zone'] = self.user_sessions[0].zone_name

    def tearDown(self):
        # remove test directory
        shutil.rmtree(self.local_test_dir_path)

        super(Test_Microservices, self).tearDown()

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 1, 0), 'Fixed in 4.1.0')
    def test_msirmcoll(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'msiRmColl_test_dir'
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.config['test_file_count'], self.config['test_file_size'])

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put dir in remote collection
        test_session.assert_icommand(
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters), "STDOUT_SINGLELINE", ustrings.recurse_ok_string())

        # new collection should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(test_session.get_entries_in_collection("{remote_home_collection}/{dir_name}".format(**parameters)))
        self.assertTrue(set(local_files) == rods_files)

        # prepare irule sequence
        # the rule is simple enough not to need a rule file
        irule_str = '''irule "msiRmColl(*coll, 'forceFlag=', *status); writeLine('stdout', *status)" "*coll={remote_home_collection}/{dir_name}" "ruleExecOut"'''.format(
            **parameters)

        # invoke msiRmColl() and checks that it returns 0
        test_session.assert_icommand(
            irule_str, 'STDOUT', '^0$', use_regex=True)

        # collection should be gone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        shutil.rmtree(dir_path)

    def test_delay_msiobjstat(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'delay_msiobjstat_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

        # put file in remote collection
        test_session.assert_icommand(
            "iput -f {filepath} {remote_home_collection}/".format(**parameters))

        # file should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)

        # prepare rule file
        rule_file_path = os.path.join(
            self.local_test_dir_path, 'delay_msiobjstat.r')
        with open(rule_file_path, 'wt') as rule_file:
            rule_str = '''
delay_msiobjstat {{
    delay("<PLUSET>30s</PLUSET>") {{
# Perform a stat on the object
# Save the stat operation's error code as metadata associated with the object
        *attr."delay_msiobjstat_return_value" = str(errorcode(msiObjStat(*obj,*out)));
        msiAssociateKeyValuePairsToObj(*attr, *obj, "-d")
    }}
}}
INPUT *obj="{remote_home_collection}/{filename}"
OUTPUT ruleExecOut
'''.format(**parameters)
            print(rule_str, file=rule_file, end='')

        # invoke rule
        test_session.assert_icommand('irule -F ' + rule_file_path)

        # give it time to complete
        time.sleep(60)

        # look for AVU set by delay rule
        attr = "delay_msiobjstat_return_value"
        value = "0"
        test_session.assert_icommand('imeta ls -d {remote_home_collection}/{filename}'.format(
            **parameters), 'STDOUT_MULTILINE', ['attribute: ' + attr + '$', 'value: ' + value + '$'], use_regex=True)

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 1, 5), 'Fixed in 4.1.5')
    def test_msiRemoveKeyValuePairsFromObj(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'msiRemoveKeyValuePairsFromObj_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)
        parameters['attribute'] = "test_attr"
        parameters['value'] = "test_value"

        # put file in remote collection
        test_session.assert_icommand(
            "iput -f {filepath} {remote_home_collection}/".format(**parameters))

        # file should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)

        # prepare first rule file to add kvp metadata
        rule_file_path = os.path.join(
            self.local_test_dir_path, 'msiAssociateKeyValuePairsToObj.r')
        with open(rule_file_path, 'w') as rule_file:
            rule_str = '''
msiAssociateKeyValuePairsToObj {{
    *attr."{attribute}" = "{value}";
    msiAssociateKeyValuePairsToObj(*attr, *obj, "-d")
}}
INPUT *obj="{remote_home_collection}/{filename}"
OUTPUT ruleExecOut
'''.format(**parameters)
            rule_file.write(rule_str)

        # invoke rule
        test_session.assert_icommand('irule -F ' + rule_file_path)

        # look for AVU set by msiAssociateKeyValuePairsToObj
        test_session.assert_icommand(
                                     'imeta ls -d {remote_home_collection}/{filename}'.format(**parameters),
                                     'STDOUT_MULTILINE',
                                     ['attribute: {attribute}$'.format(**parameters),
                                      'value: {value}$'.format(**parameters)],
                                     use_regex=True)

        # prepare second rule file to remove kvp metadata
        rule_file_path = os.path.join(
            self.local_test_dir_path, 'msiRemoveKeyValuePairsFromObj.r')
        with open(rule_file_path, 'w') as rule_file:
            rule_str = '''
msiRemoveKeyValuePairsFromObj {{
    *attr."{attribute}" = "{value}";
    msiRemoveKeyValuePairsFromObj(*attr, *obj, "-d")
}}
INPUT *obj="{remote_home_collection}/{filename}"
OUTPUT ruleExecOut
'''.format(**parameters)
            rule_file.write(rule_str)

        # invoke rule
        test_session.assert_icommand('irule -F ' + rule_file_path)

        # confirm that AVU is gone
        test_session.assert_icommand(
                                     'imeta ls -d {remote_home_collection}/{filename}'.format(**parameters),
                                     'STDOUT_MULTILINE',
                                     ['AVUs defined for dataObj {remote_home_collection}/{filename}:$'.format(**parameters),
                                      'None$'],
                                     use_regex=True)

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

class Test_Recursive_Icp(SessionsMixin, unittest.TestCase):

    def setUp(self):
        super(Test_Recursive_Icp, self).setUp()

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in test.settings.FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val
        self.config['local_zone'] = self.user_sessions[0].zone_name

        # Use 10 files; expected value is -1 due to 0-based indexing
        self.file_count1 = 10
        self.file_count2 = self.file_count1 * 2
        self.local_dir1 = 'test_recursive_icp1'
        self.local_dir2 = 'test_recursive_icp2'
        lib.create_directory_of_small_files(self.local_dir1, self.file_count1)
        lib.create_directory_of_small_files(self.local_dir2, self.file_count2)

        test_session = self.user_sessions[0]
        source_coll_name = 'source_coll'
        self.local_source_coll = os.path.join(test_session.home_collection, source_coll_name)
        test_session.assert_icommand(['imkdir', self.local_source_coll])

        test_session.assert_icommand(['irsync', '-r', self.local_dir1, self.local_dir2, 'i:{}'.format(self.local_source_coll)], 'STDOUT_SINGLELINE', ustrings.recurse_ok_string())

    def tearDown(self):
        test_session = self.user_sessions[0]
        test_session.assert_icommand(['irm', '-rf', self.local_source_coll])
        shutil.rmtree(self.local_dir1)
        shutil.rmtree(self.local_dir2)
        super(Test_Recursive_Icp, self).tearDown()

    # recursive icp of local source collection
    def cp_recursive_local_source_test(self, source_coll, flat_coll=True, remote_zone=True, in_target=False):
        test_session = self.user_sessions[0]
        try:
            # prepare names for home collection and target collection
            if remote_zone:
                home_coll = test_session.remote_home_collection(test.settings.FEDERATION.REMOTE_ZONE)
            else:
                home_coll = test_session.home_collection
            target_coll = os.path.join(home_coll, 'cp_recursive_local_source_test')
            # create target collection and copy source into it
            test_session.assert_icommand(['imkdir', target_coll])
            if in_target:
                test_session.assert_icommand(['icd', target_coll])
                test_session.assert_icommand(['icp', '-r', source_coll, '.'])
            else:
                test_session.assert_icommand(['icp', '-r', source_coll, target_coll])
            # ensure all files are in the collection
            test_session.assert_icommand(['ils', '-lr', target_coll], 'STDOUT_SINGLELINE', '& {}'.format(str(self.file_count1 - 1)))
            if flat_coll:
                test_session.assert_icommand_fail(['ils', '-lr', target_coll], 'STDOUT_SINGLELINE', '& {}'.format(str(self.file_count2 - 1)))
            else:
                test_session.assert_icommand(['ils', '-lr', target_coll], 'STDOUT_SINGLELINE', '& {}'.format(str(self.file_count2 - 1)))
        finally:
            # cleanup
            test_session.assert_icommand(['irm', '-rf', target_coll])

    def test_icp_single_dir_localzone_in_home(self):
        source_coll = os.path.join(self.local_source_coll, self.local_dir1)
        self.cp_recursive_local_source_test(source_coll, flat_coll=True, remote_zone=False, in_target=False)

    def test_icp_single_dir_localzone_in_target(self):
        source_coll = os.path.join(self.local_source_coll, self.local_dir1)
        self.cp_recursive_local_source_test(source_coll, flat_coll=True, remote_zone=False, in_target=True)

    def test_icp_single_dir_remotezone_in_home(self):
        source_coll = os.path.join(self.local_source_coll, self.local_dir1)
        self.cp_recursive_local_source_test(source_coll, flat_coll=True, remote_zone=True, in_target=False)

    def test_icp_single_dir_remotezone_in_target(self):
        source_coll = os.path.join(self.local_source_coll, self.local_dir1)
        self.cp_recursive_local_source_test(source_coll, flat_coll=True, remote_zone=True, in_target=True)

    def test_icp_tree_localzone_in_home(self):
        self.cp_recursive_local_source_test(self.local_source_coll, flat_coll=False, remote_zone=False, in_target=False)

    def test_icp_tree_localzone_in_target(self):
        self.cp_recursive_local_source_test(self.local_source_coll, flat_coll=False, remote_zone=False, in_target=True)

    def test_icp_tree_remotezone_in_home(self):
        self.cp_recursive_local_source_test(self.local_source_coll, flat_coll=False, remote_zone=True, in_target=False)

    def test_icp_tree_remotezone_in_target(self):
        self.cp_recursive_local_source_test(self.local_source_coll, flat_coll=False, remote_zone=True, in_target=True)

class test_dynamic_peps_in_federation(SessionsMixin, unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin
    class_name = 'Test_Native_Rule_Engine_Plugin'

    def setUp(self):
        super(test_dynamic_peps_in_federation, self).setUp()

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in test.settings.FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val

        self.config['local_zone'] = self.user_sessions[0].zone_name
        if test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 0):
            test.settings.FEDERATION.REMOTE_VAULT = '/home/irods/irods-legacy/iRODS/Vault'

        self.admin = session.make_session_for_existing_admin()

    def tearDown(self):
        self.admin.__exit__()
        super(test_dynamic_peps_in_federation, self).tearDown()

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 2, 9), 'Fixed in 4.2.9')
    def test_peps_for_parallel_mode_transfers__issue_5017(self):
        test_session = self.user_sessions[0]
        remote_home_collection = test_session.remote_home_collection(test.settings.FEDERATION.REMOTE_ZONE)
        filename = 'test_peps_for_parallel_mode_transfers__issue_5017'
        local_file = os.path.join(self.admin.local_session_dir, filename)
        logical_path = os.path.join(remote_home_collection, filename)
        local_logical_path = os.path.join(test_session.home_collection, filename)
        file_size = 40 * 1024 * 1024 # 40MB
        attr = 'test_peps_for_parallel_mode_transfers__issue_5017'

        try:
            if not os.path.exists(local_file):
                lib.make_file(local_file, file_size)

            # PEPs will fire locally on connected server, so metadata will be applied to local data object
            parameters = {}
            parameters['logical_path'] = local_logical_path
            put_peps = '''
pep_api_data_obj_put_pre (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__issue_5017","data-obj-put-pre");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
}}
pep_api_data_obj_put_post (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__issue_5017","data-obj-put-post");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
}}
pep_api_data_obj_put_except (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__issue_5017","data-obj-put-except");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
}}
pep_api_data_obj_put_finally (*INSTANCE_NAME, *COMM, *DATAOBJINP, *BUFFER, *PORTAL_OPR_OUT)
{{
    msiAddKeyVal(*key_val_pair,"test_peps_for_parallel_mode_transfers__issue_5017","data-obj-put-finally");
    msiAssociateKeyValuePairsToObj(*key_val_pair,"{logical_path}","-d");
}}
'''.format(**parameters)

            print(put_peps)

            # put a new data object so that the PEPs have an object to which metadata can be associated
            test_session.assert_icommand(['iput', local_file, local_logical_path])

            with temporary_core_file() as core:
                core.add_rule(put_peps)

                # peps to check for the first, successful put
                peps = ['data-obj-put-pre', 'data-obj-put-post', 'data-obj-put-finally']

                # put a new data object and ensure success
                test_session.assert_icommand(['iput', local_file, logical_path])

                for pep in peps:
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(test_session, attr, pep),
                        interval=1,
                        maxrep=10
                    )

                self.assertFalse(lib.metadata_attr_with_value_exists(test_session, attr, 'pep-obj-put-except'))

                # clean up metadata for next test
                for pep in peps:
                    test_session.assert_icommand(['imeta', 'rm', '-d', local_logical_path, attr, pep])

                test_session.assert_icommand(['imeta', 'ls', '-d', local_logical_path], 'STDOUT', 'None')

                # peps to check for the second, unsuccessful put
                peps = ['data-obj-put-pre', 'data-obj-put-except', 'data-obj-put-finally']

                # put to same logical path without force flag, resulting in error and (hopefully) triggering except PEP
                test_session.assert_icommand(['iput', local_file, logical_path], 'STDERR', 'OVERWRITE_WITHOUT_FORCE_FLAG')

                for pep in peps:
                    lib.delayAssert(
                        lambda: lib.metadata_attr_with_value_exists(test_session, attr, pep),
                        interval=1,
                        maxrep=10
                    )

                self.assertFalse(lib.metadata_attr_with_value_exists(test_session, attr, 'pep-obj-put-post'))

        finally:
            test_session.run_icommand(['irm', '-f', local_logical_path])
            test_session.run_icommand(['irm', '-f', logical_path])
            self.admin.assert_icommand(['iadmin', 'rum'])
