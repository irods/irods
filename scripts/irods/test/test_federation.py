from __future__ import print_function
import hashlib
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import tempfile
import time
import shutil

from .. import test
from . import settings
from . import session
from .. import lib
from ..configuration import IrodsConfig

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

    def tearDown(self):
        shutil.rmtree(self.local_test_dir_path, ignore_errors=True)
        super(Test_ICommands, self).tearDown()

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
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'iput_test_file'
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
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

    def test_iput_large_file(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'iput_test_file'
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

        # put file in remote collection, ask for 6 threads
        test_session.assert_icommand(
            "iput -v -N 6 {filepath} {remote_home_collection}/".format(**parameters), 'STDOUT_SINGLELINE', '6 thr')

        # file should be there
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

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
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters))

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
        # e.g: we will look for '16 thr' or '17 thr' in stdout
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
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters))

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

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 0, 3) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 3), 'Fixed in 4.0.3')
    def test_iget_from_bundle(self):
        '''
        WIP
        '''

        # make test dir
        dir_name = 'iphybun_test_dir'
        file_count = 20
        file_size = 4
        dir_path = os.path.join(self.local_test_dir_path, dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, file_count, file_size)

        # make session for existing *remote* user
        user, password = 'rods', 'rods'
        remote_session = session.make_session_for_existing_user(
            user, password, test.settings.FEDERATION.REMOTE_HOST, test.settings.FEDERATION.REMOTE_ZONE)

        # test specific parameters
        parameters = self.config.copy()
        parameters['dir_path'] = dir_path
        parameters['dir_name'] = dir_name
        parameters['user_name'] = remote_session.username
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}".format(
            **parameters)

        # put dir in remote collection
        remote_session.assert_icommand(
            "iput -fr {dir_path} {remote_home_collection}/".format(**parameters))

        # new collection should be there
        remote_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDOUT_SINGLELINE', dir_name)

        # list remote home collection
        remote_session.assert_icommand(
            "ils -L {remote_home_collection}".format(**parameters), 'STDOUT_SINGLELINE', parameters['remote_home_collection'])

        # cleanup
        remote_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

        # close remote session
        remote_session.__exit__()

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
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters))

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
        lib.make_file(filepath, filesize)

        # test specific parameters
        parameters = self.config.copy()
        parameters['filepath'] = filepath
        parameters['filename'] = filename
        parameters['user_name'] = test_session.username
        parameters['local_home_collection'] = test_session.home_collection
        parameters['remote_home_collection'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **parameters)

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

        # cleanup
        test_session.assert_icommand(
            "irm -f {local_home_collection}/{filename}".format(**parameters))
        test_session.assert_icommand(
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
            "iput -r {dir_path} {local_home_collection}/".format(**parameters))

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

        # make test dir
        dir_name = 'irsync_test_dir'
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

        # sync dir with remote collection
        test_session.assert_icommand(
            "irsync -r {dir_path} i:{remote_home_collection}/{dir_name}".format(**parameters))

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

    def test_irsync_r_coll_to_coll(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'irsync_test_dir'
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
            "iput -r {dir_path} {local_home_collection}/".format(**parameters))

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

        # cleanup
        test_session.assert_icommand(
            "irm -rf {local_home_collection}/{dir_name}".format(**parameters))
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    def test_irsync_r_coll_to_dir(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test dir
        dir_name = 'irsync_test_dir'
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

        # sync dir with remote collection
        test_session.assert_icommand(
            "irsync -r {dir_path} i:{remote_home_collection}/{dir_name}".format(**parameters))

        # remove local test dir
        shutil.rmtree(dir_path)

        # sync remote collection back with local dir
        test_session.assert_icommand(
            "irsync -r i:{remote_home_collection}/{dir_name} {dir_path}".format(**parameters))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        test_session.assert_icommand(
            "irm -rf {remote_home_collection}/{dir_name}".format(**parameters))
        shutil.rmtree(dir_path)

    @unittest.skipIf(IrodsConfig().version_tuple < (4, 0, 0) or test.settings.FEDERATION.REMOTE_IRODS_VERSION < (4, 0, 0), 'No resource hierarchies before iRODS 4')
    def test_irsync_passthru_3016(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'irsync_test_file'
        filesize = self.config['test_file_size']
        filepath = os.path.join(self.local_test_dir_path, filename)
        lib.make_file(filepath, filesize)

        # test specific parameters
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

        # create local passthru hierarchy
        parameters['hostname'] = test.settings.ICAT_HOSTNAME
        parameters['local_leaf_resc_path'] = '/tmp/{local_leaf_resc}'.format(
            **parameters)
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

        # cleanup
        test_session.assert_icommand(
            "irm -f {local_home_collection}/{filename}".format(**parameters))
        test_session.assert_icommand(
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
            "ilsresc -z {remote_zone}".format(**self.config), 'STDOUT_SINGLELINE', test.settings.FEDERATION.REMOTE_DEF_RESOURCE)


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
            "iput -r {dir_path} {remote_home_collection}/".format(**parameters))

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

