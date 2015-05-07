from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import shutil
import commands
from configuration import FEDERATION
if FEDERATION.LOCAL_IRODS_VERSION < (4, 1, 0):
    import lib_pre410 as lib
else:
    import lib

SessionsMixin = lib.make_sessions_mixin(
    FEDERATION.RODSADMIN_NAME_PASSWORD_LIST, FEDERATION.RODSUSER_NAME_PASSWORD_LIST)


class Test_ICommands(SessionsMixin, unittest.TestCase):

    def setUp(self):
        # make local test directory
        self.local_test_dir_path = os.path.abspath('federation_test_stuff.tmp')
        os.mkdir(self.local_test_dir_path)

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val

        super(Test_ICommands, self).setUp()

    def tearDown(self):
        # delete local test dir
        shutil.rmtree(self.local_test_dir_path)

        super(Test_ICommands, self).tearDown()

    def test_ils_l(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # make test file
        filename = 'ils_test_file'
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

        # list remote home collection
        test_session.assert_icommand(
            "ils -L {remote_home_collection}".format(**parameters), 'STDOUT_SINGLELINE', parameters['remote_home_collection'])

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # list file info
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', filename)
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', str(filesize))
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', parameters['remote_resource'])
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{filename}".format(**parameters), 'STDOUT_SINGLELINE', parameters['remote_vault'])

        # cleanup
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
        os.remove(filepath)

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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
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
        orig_md5 = commands.getoutput('md5sum ' + filepath)

        # put file in remote collection
        test_session.assert_icommand(
            "iput {filepath} {remote_home_collection}/".format(**parameters))

        # remove local file
        os.remove(filepath)

        # get file back
        test_session.assert_icommand(
            "iget {remote_home_collection}/{filename} {filepath}".format(**parameters))

        # compare checksums
        new_md5 = commands.getoutput('md5sum ' + filepath)
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
        orig_md5 = commands.getoutput('md5sum ' + filepath)

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
        new_md5 = commands.getoutput('md5sum ' + filepath)
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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
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
        orig_md5 = commands.getoutput('md5sum ' + filepath)

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
        new_md5 = commands.getoutput('md5sum ' + filepath)
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        test_session.assert_icommand(
            "irm -f {local_home_collection}/{filename}".format(**parameters))
        test_session.assert_icommand(
            "irm -f {remote_home_collection}/{filename}".format(**parameters))
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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
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

    def test_ilsresc_z(self):
        # pick session(s) for the test
        test_session = self.user_sessions[0]

        # list remote resources
        test_session.assert_icommand(
            "ilsresc -z {remote_zone}".format(**self.config), 'STDOUT_SINGLELINE', self.config['remote_resource'])


class Test_Admin_Commands(unittest.TestCase):

    '''
    Operations requiring administrative privilege,
    run in the remote zone by a local rodsadmin.
    They should all fail (disallowed by remote zone).
    '''

    def setUp(self):
        # make session with existing admin account
        self.admin_session = lib.make_session_for_existing_admin()

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val

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
        # make local test directory
        self.local_test_dir_path = os.path.abspath('federation_test_stuff.tmp')
        os.mkdir(self.local_test_dir_path)

        # load federation settings in dictionary (all lower case)
        self.config = {}
        for key, val in FEDERATION.__dict__.items():
            if not key.startswith('__'):
                self.config[key.lower()] = val

        super(Test_Microservices, self).setUp()

    def tearDown(self):
        # remove test directory
        shutil.rmtree(self.local_test_dir_path)

        super(Test_Microservices, self).tearDown()

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
        rods_files = set(lib.ils_output_to_entries(
            test_session.run_icommand(['ils', "{remote_home_collection}/{dir_name}".format(**parameters)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # prepare irule sequence
        # the rule is simple enough not to need a rule file
        irule_str = '''irule "msiRmColl(*coll, 'forceFlag=', *status); writeLine('stdout', *status)" "*coll={remote_home_collection}/{dir_name}" "ruleExecOut"'''.format(
            **parameters)

        # invoke msiRmColl() and checks that it returns 0
        test_session.assert_icommand(irule_str, 'STDOUT_SINGLELINE', '0')

        # collection should be gone
        test_session.assert_icommand(
            "ils -L {remote_home_collection}/{dir_name}".format(**parameters), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        shutil.rmtree(dir_path)
