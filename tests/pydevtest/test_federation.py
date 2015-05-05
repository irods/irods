from __future__ import print_function
import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest
import os
import shutil
import commands
import configuration
if configuration.LOCAL_IRODS_VERSION < (4, 1, 0):
    import lib_pre410 as lib
else:
    import lib


class TestCrossZoneICommands(lib.make_sessions_mixin([], [(configuration.CROSS_ZONE_USER_NAME, configuration.CROSS_ZONE_USER_AUTH)]), unittest.TestCase):

    def setUp(self):
        # load federation test parameters
        self.parameters = lib.open_and_load_json_ascii(
            configuration.CROSS_ZONE_TEST_CONFIG_FILE)

        # add home collections
        self.parameters['user_name'] = configuration.CROSS_ZONE_USER_NAME
        self.parameters['local_home_coll'] = "/{local_zone}/home/{user_name}".format(
            **self.parameters)
        self.parameters['remote_home_coll'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **self.parameters)

        super(TestCrossZoneICommands, self).setUp()

    def tearDown(self):
        super(TestCrossZoneICommands, self).tearDown()

    def test_ils_l(self):
        # make test file
        filename = 'ils_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}
        kwargs.update(self.parameters)

        # list remote home collection
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}".format(**kwargs), 'STDOUT_SINGLELINE', kwargs['collection'])

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {collection}/".format(**kwargs))

        # list file info
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', filename)
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', str(filesize))
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', kwargs['remote_resc'])
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', kwargs['remote_vault'])

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_iput(self):
        # make test file
        filename = 'iput_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {collection}/".format(**kwargs))

        # file should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', filename)
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_iput_large_file(self):
        # make test file
        filename = 'iput_test_file'
        filesize = self.parameters['large_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}

        # put file in remote collection, ask for 6 threads
        self.user_sessions[0].assert_icommand(
            "iput -v -N 6 {filename} {collection}/".format(**kwargs), 'STDOUT_SINGLELINE', '6 thr')

        # file should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', filename)
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_iput_r(self):
        # make test dir
        dir_name = 'iput_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'collection': self.parameters['remote_home_coll']}

        # put dir in remote collection
        self.user_sessions[0].assert_icommand(
            "iput -r {dir_name} {collection}/".format(**kwargs))

        # new collection should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{dir_name}".format(**kwargs), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(lib.ils_output_to_entries(
            self.user_sessions[0].run_icommand(['ils', "{collection}/{dir_name}".format(**kwargs)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_iget(self):
        # make test file
        filename = 'iget_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}

        # checksum local file
        orig_md5 = commands.getoutput('md5sum ' + filepath)

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {collection}/".format(**kwargs))

        # remove local file
        os.remove(filepath)

        # get file back
        self.user_sessions[0].assert_icommand(
            "iget {collection}/{filename}".format(**kwargs))

        # compare checksums
        new_md5 = commands.getoutput('md5sum ' + filepath)
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_iget_large_file(self):
        # make test file
        filename = 'iget_test_file'
        filesize = self.parameters['large_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)
        max_threads = self.parameters['max_thread_number']

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}

        # checksum local file
        orig_md5 = commands.getoutput('md5sum ' + filepath)

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {collection}/".format(**kwargs))

        # remove local file
        os.remove(filepath)

        # get file back, ask for too many threads (should be capped)
        self.user_sessions[0].assert_icommand(
            "iget -v -N 600 {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', str(max_threads) + ' thr')

        # compare checksums
        new_md5 = commands.getoutput('md5sum ' + filepath)
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_iget_r(self):
        # make test dir
        dir_name = 'iget_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'collection': self.parameters['remote_home_coll']}

        # put dir in remote collection
        self.user_sessions[0].assert_icommand(
            "iput -r {dir_name} {collection}/".format(**kwargs))

        # remove local test dir
        shutil.rmtree(dir_path)

        # get collection back
        self.user_sessions[0].assert_icommand(
            "iget -r {collection}/{dir_name}".format(**kwargs))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_irm_f(self):
        # make test file
        filename = 'irm_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'collection': self.parameters['remote_home_coll']}

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {collection}/".format(**kwargs))

        # file should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', filename)

        # delete remote file
        self.user_sessions[0].assert_icommand(
            "irm -f {collection}/{filename}".format(**kwargs))

        # file should be gone
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{filename}".format(**kwargs), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        os.remove(filepath)

    def test_irm_rf(self):
        # make test dir
        dir_name = 'irm_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'collection': self.parameters['remote_home_coll']}

        # put dir in remote collection
        self.user_sessions[0].assert_icommand(
            "iput -r {dir_name} {collection}/".format(**kwargs))

        # new collection should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{dir_name}".format(**kwargs), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(lib.ils_output_to_entries(
            self.user_sessions[0].run_icommand(['ils', "{collection}/{dir_name}".format(**kwargs)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # remove remote coll
        self.user_sessions[0].assert_icommand(
            "irm -rf {collection}/{dir_name}".format(**kwargs))

        # coll should be gone
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{dir_name}".format(**kwargs), 'STDERR_SINGLELINE', 'does not exist')

        # cleanup
        shutil.rmtree(dir_path)

    def test_icp(self):
        # make test file
        filename = 'icp_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'remote_collection': self.parameters['remote_home_coll'],
                  'local_collection': self.parameters['local_home_coll']}

        # checksum local file
        orig_md5 = commands.getoutput('md5sum ' + filepath)

        # put file in local collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {local_collection}/".format(**kwargs))

        # remove local file
        os.remove(filepath)

        # copy file to remote home collection
        self.user_sessions[0].assert_icommand(
            "icp {local_collection}/{filename} {remote_collection}/".format(**kwargs))

        # file should show up in remote zone
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', filename)
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{filename}".format(**kwargs), 'STDOUT_SINGLELINE', str(filesize))

        # get file back from remote zone
        self.user_sessions[0].assert_icommand(
            "iget {remote_collection}/{filename}".format(**kwargs))

        # compare checksums
        new_md5 = commands.getoutput('md5sum ' + filepath)
        self.assertEqual(orig_md5, new_md5)

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {local_collection}/{filename}".format(**kwargs))
        self.user_sessions[0].assert_icommand(
            "irm -f {remote_collection}/{filename}".format(**kwargs))
        os.remove(filepath)

    def test_icp_r(self):
        # make test dir
        dir_name = 'icp_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'remote_collection': self.parameters['remote_home_coll'],
                  'local_collection': self.parameters['local_home_coll']}

        # put dir in local collection
        self.user_sessions[0].assert_icommand(
            "iput -r {dir_name} {local_collection}/".format(**kwargs))

        # remove local test dir
        shutil.rmtree(dir_path)

        # copy dir to remote home collection
        self.user_sessions[0].assert_icommand(
            "icp -r {local_collection}/{dir_name} {remote_collection}/{dir_name}".format(**kwargs))

        # collection should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{dir_name}".format(**kwargs), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(lib.ils_output_to_entries(
            self.user_sessions[0].run_icommand(['ils', "{remote_collection}/{dir_name}".format(**kwargs)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # get collection back
        self.user_sessions[0].assert_icommand(
            "iget -r {remote_collection}/{dir_name}".format(**kwargs))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {local_collection}/{dir_name}".format(**kwargs))
        self.user_sessions[0].assert_icommand(
            "irm -rf {remote_collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_imv(self):
        '''
        remote-remote imv test
        (SYS_CROSS_ZONE_MV_NOT_SUPPORTED)
        '''
        # make test file
        filename = 'imv_test_file'
        filesize = self.parameters['test_file_size']
        filepath = os.path.abspath(filename)
        lib.make_file(filename, filesize)

        kwargs = {'filename': filename,
                  'new_name': filename + '_new',
                  'remote_collection': self.parameters['remote_home_coll'],
                  'local_collection': self.parameters['local_home_coll']}

        # put file in remote collection
        self.user_sessions[0].assert_icommand(
            "iput {filename} {remote_collection}/".format(**kwargs))

        # move (rename) remote file
        self.user_sessions[0].assert_icommand(
            "imv {remote_collection}/{filename} {remote_collection}/{new_name}".format(**kwargs))

        # file should have been renamed
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{filename}".format(**kwargs), 'STDERR_SINGLELINE', 'does not exist')
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{new_name}".format(**kwargs), 'STDOUT_SINGLELINE', filename)
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{new_name}".format(**kwargs), 'STDOUT_SINGLELINE', str(filesize))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -f {remote_collection}/{new_name}".format(**kwargs))
        os.remove(filepath)

    def test_irsync_r_dir_to_coll(self):
        # make test dir
        dir_name = 'irsync_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'collection': self.parameters['remote_home_coll']}

        # sync dir with remote collection
        self.user_sessions[0].assert_icommand(
            "irsync -r {dir_name} i:{collection}/{dir_name}".format(**kwargs))

        # new collection should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {collection}/{dir_name}".format(**kwargs), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(lib.ils_output_to_entries(
            self.user_sessions[0].run_icommand(['ils', "{collection}/{dir_name}".format(**kwargs)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_irsync_r_coll_to_coll(self):
        # make test dir
        dir_name = 'irsync_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'remote_collection': self.parameters['remote_home_coll'],
                  'local_collection': self.parameters['local_home_coll']}

        # put dir in local collection
        self.user_sessions[0].assert_icommand(
            "iput -r {dir_name} {local_collection}/".format(**kwargs))

        # remove local test dir
        shutil.rmtree(dir_path)

        # sync local collection with remote collection
        self.user_sessions[0].assert_icommand(
            "irsync -r i:{local_collection}/{dir_name} i:{remote_collection}/{dir_name}".format(**kwargs))

        # collection should be there
        self.user_sessions[0].assert_icommand(
            "ils -L {remote_collection}/{dir_name}".format(**kwargs), 'STDOUT_SINGLELINE', dir_name)

        # files should be there
        rods_files = set(lib.ils_output_to_entries(
            self.user_sessions[0].run_icommand(['ils', "{remote_collection}/{dir_name}".format(**kwargs)])[1]))
        self.assertTrue(set(local_files) == rods_files)

        # get collection back
        self.user_sessions[0].assert_icommand(
            "iget -r {remote_collection}/{dir_name}".format(**kwargs))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {local_collection}/{dir_name}".format(**kwargs))
        self.user_sessions[0].assert_icommand(
            "irm -rf {remote_collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_irsync_r_coll_to_dir(self):
        # make test dir
        dir_name = 'irsync_test_dir'
        dir_path = os.path.abspath(dir_name)
        local_files = lib.make_large_local_tmp_dir(
            dir_path, self.parameters['test_file_count'], self.parameters['test_file_size'])

        kwargs = {'dir_name': dir_name,
                  'collection': self.parameters['remote_home_coll']}

        # sync dir with remote collection
        self.user_sessions[0].assert_icommand(
            "irsync -r {dir_name} i:{collection}/{dir_name}".format(**kwargs))

        # remove local test dir
        shutil.rmtree(dir_path)

        # sync remote collection back with local dir
        self.user_sessions[0].assert_icommand(
            "irsync -r i:{collection}/{dir_name} {dir_name}".format(**kwargs))

        # compare list of files
        received_files = os.listdir(dir_path)
        self.assertTrue(set(local_files) == set(received_files))

        # cleanup
        self.user_sessions[0].assert_icommand(
            "irm -rf {collection}/{dir_name}".format(**kwargs))
        shutil.rmtree(dir_path)

    def test_ilsresc_z(self):
        # list remote resources
        self.user_sessions[0].assert_icommand(
            "ilsresc -z {remote_zone}".format(**self.parameters), 'STDOUT_SINGLELINE', self.parameters['remote_resc'])


class TestCrossZoneAdminCommands(unittest.TestCase):
    '''
    Operations requiring administrative privilege,
    run in the remote zone by a local rodsadmin.
    They should all fail (disallowed by remote zone).
    '''

    def setUp(self):
        # make session with existing admin account
        self.admin_session = lib.make_session_for_existing_admin()

        # load federation test parameters
        self.parameters = lib.open_and_load_json_ascii(
            configuration.CROSS_ZONE_TEST_CONFIG_FILE)

        # add session parameters
        self.parameters['user_name'] = self.admin_session.username
        self.parameters['local_home_coll'] = "/{local_zone}/home/{user_name}".format(
            **self.parameters)
        self.parameters['remote_home_coll'] = "/{remote_zone}/home/{user_name}#{local_zone}".format(
            **self.parameters)


        super(TestCrossZoneAdminCommands, self).setUp()

    def tearDown(self):
        self.admin_session.__exit__()
        super(TestCrossZoneAdminCommands, self).tearDown()

    def test_ichmod(self):
        # try to modify ACLs in the remote zone
        self.admin_session.assert_icommand(
            "ichmod -rM own {user_name}#{local_zone} /{remote_zone}/home".format(**self.parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

        self.admin_session.assert_icommand(
            "ichmod -M own {user_name} /{remote_zone}/home".format(**self.parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')

        self.admin_session.assert_icommand(
            "ichmod -M read {user_name} /{remote_zone}".format(**self.parameters), 'STDERR_SINGLELINE', 'CAT_NO_ACCESS_PERMISSION')


class TestCrossZoneMicroservices(unittest.TestCase):
    pass







