from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import shutil
import tempfile

from . import resource_suite
from .. import lib
from .. import test
from ..test.command import assert_command

class Test_ibun(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_ibun, self).setUp()

    def tearDown(self):
        super(Test_ibun, self).tearDown()

    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_ibun_extraction_of_big_zip_file__issue_4495(self):
        known_file_name = 'known_file'

        try:
            root_name = tempfile.mkdtemp()
            unzip_collection_name = 'my_exploded_coll'
            unzip_directory_name = 'my_exploded_dir'
            zip_file_name = 'bigzip.zip'

            source_dir = '/var/lib/irods/scripts'
            for i in range(0, 10):
                dest_dir = os.path.join(root_name, str(i))
                shutil.copytree(source_dir, dest_dir)

            filesize = 3900000000
            lib.make_file(os.path.join(root_name, known_file_name), filesize, 'random')

            out,_ = lib.execute_command(['du', '-h', root_name])
            print(out)

            lib.execute_command(['zip', '-r', zip_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', zip_file_name])
            print(out)

            self.admin.assert_icommand(['iput', zip_file_name])

            self.admin.assert_icommand(['ibun', '-x', zip_file_name, unzip_collection_name])
            self.admin.assert_icommand(['ils', '-lr', unzip_collection_name], 'STDOUT', known_file_name)

            self.admin.assert_icommand(['iget', '-r', unzip_collection_name, unzip_directory_name])
            lib.execute_command(['diff', '-r', root_name, os.path.join(unzip_directory_name, root_name)])
        finally:
            self.admin.run_icommand(['irm', '-f', zip_file_name])
            self.admin.run_icommand(['irm', '-rf', unzip_collection_name])
            shutil.rmtree(root_name, ignore_errors=True)
            shutil.rmtree(unzip_directory_name, ignore_errors=True)
            if os.path.exists(zip_file_name):
                os.unlink(zip_file_name)

    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_ibun_extraction_of_big_tar_file__issue_4118(self):
        known_file_name = 'known_file'

        try:
            root_name = tempfile.mkdtemp()
            untar_collection_name = 'my_exploded_coll'
            untar_directory_name = 'my_exploded_dir'
            tar_file_name = 'bigtar.tar'

            # Generate a junk file
            filesize = 524288000 # 500MB
            lib.make_file(known_file_name, filesize, 'random')
            out,_ = lib.execute_command(['ls', '-l', known_file_name])
            print(out)

            # Copy junk file until a sufficiently large tar file size is reached
            for i in range(0, 13):
                lib.execute_command(['cp', known_file_name, os.path.join(root_name, known_file_name + str(i))])

            os.unlink(known_file_name)

            out,_ = lib.execute_command(['ls', '-l', root_name])
            print(out)

            lib.execute_command(['tar', '-cf', tar_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', tar_file_name])
            print(out)

            self.admin.assert_icommand(['iput', tar_file_name])

            self.admin.assert_icommand(['ibun', '-x', tar_file_name, untar_collection_name])
            self.admin.assert_icommand(['ils', '-lr', untar_collection_name], 'STDOUT', known_file_name)

            self.admin.assert_icommand(['iget', '-r', untar_collection_name, untar_directory_name])
            lib.execute_command(['diff', '-r', root_name, os.path.join(untar_directory_name, root_name)])
        finally:
            self.admin.run_icommand(['irm', '-f', tar_file_name])
            self.admin.run_icommand(['irm', '-rf', untar_collection_name])
            shutil.rmtree(root_name, ignore_errors=True)
            shutil.rmtree(untar_directory_name, ignore_errors=True)
            if os.path.exists(tar_file_name):
                os.unlink(tar_file_name)

    def test_ibun_atop_existing_archive_file_in_replication_hierarchy__issue_5426(self):
        def do_test_ibun_atop_existing_archive_file_in_replication_hierarchy(self, filesize, compressed_size):
            replication = 'repl0'
            resource_0 = 'resc0'
            resource_1 = 'resc1'
            filename = 'thefile'
            local_file = os.path.join(self.admin.local_session_dir, filename)
            collection_name = 'thecoll'
            collection_path = os.path.join(self.admin.session_collection, collection_name)
            put_path = os.path.join(collection_path, filename)

            archive_name = collection_name + '.bzip2'
            archive_path = os.path.join(self.admin.session_collection, archive_name)

            try:
                # generate a replication hierarchy to two unixfilesystem resources
                lib.create_replication_resource(self.admin, replication)
                lib.create_ufs_resource(self.admin, resource_0, test.settings.HOSTNAME_2)
                lib.create_ufs_resource(self.admin, resource_1, test.settings.HOSTNAME_3)
                lib.add_child_resource(self.admin, replication, resource_0)
                lib.add_child_resource(self.admin, replication, resource_1)

                # generate (uncompressible) contents for the collection
                lib.make_file(local_file, filesize, 'random')
                out,_ = lib.execute_command(['ls', '-l', local_file])
                print(out)

                # generate the collection to bundle up
                self.admin.assert_icommand(['imkdir', collection_path])
                self.admin.assert_icommand(['iput', '-f', '-R', replication, local_file, put_path])

                # bundle the collection to an archive file in the replication hierarchy
                self.admin.assert_icommand(['ibun', '-c', '-Dbzip2', '-R', replication, archive_path, collection_path])
                print(self.admin.run_icommand(['ils', '-l', archive_path])[0])

                # ensure that the replicas were created and are good
                query = 'select DATA_SIZE, DATA_REPL_STATUS, DATA_REPL_NUM where COLL_NAME = \'{}\' and DATA_NAME = \'{}\''.format(
                    os.path.dirname(archive_path), os.path.basename(archive_path))

                out, err, ec = self.admin.run_icommand(['iquest', '%s...%s...%s', query])

                self.assertEqual(0, ec)
                self.assertEqual(0, len(err))
                self.assertEqual(2, len(out.splitlines()))

                for result in out.splitlines():
                    r = result.split('...')
                    self.assertEqual(compressed_size, int(r[0]))
                    self.assertEqual(1, int(r[1]))

                # overwrite archive file via a forced ibun of the same collection to the same logical path
                self.admin.assert_icommand(['ibun', '-c', '-Dbzip2', '-f', '-R', replication, archive_path, collection_path])
                print(self.admin.run_icommand(['ils', '-l', archive_path])[0])

                # ensure that the replicas were overwritten and are good
                out, err, ec = self.admin.run_icommand(['iquest', '%s...%s...%s', query])

                self.assertEqual(0, ec)
                self.assertEqual(0, len(err))
                self.assertEqual(2, len(out.splitlines()))

                for result in out.splitlines():
                    r = result.split('...')
                    self.assertEqual(compressed_size, int(r[0]))
                    self.assertEqual(1, int(r[1]))

            finally:
                self.admin.run_icommand(['irm', '-r', '-f', collection_path])
                self.admin.run_icommand(['irm', '-f', archive_path])
                lib.remove_child_resource(self.admin, replication, resource_0)
                lib.remove_child_resource(self.admin, replication, resource_1)
                lib.remove_resource(self.admin, replication)
                lib.remove_resource(self.admin, resource_0)
                lib.remove_resource(self.admin, resource_1)

        do_test_ibun_atop_existing_archive_file_in_replication_hierarchy(self, 1038425, 1044480)
        do_test_ibun_atop_existing_archive_file_in_replication_hierarchy(self, 1040425, 1054720)

    def test_ibun(self):
        test_file = "ibun_test_file"
        lib.make_file(test_file, 1000)
        cmd = "tar cf somefile.tar " + test_file
        lib.execute_command(['tar', 'cf', 'somefile.tar', test_file])

        tar_path = self.admin.session_collection + '/somefile.tar'
        dir_path = self.admin.session_collection + '/somedir'

        self.admin.assert_icommand("iput somefile.tar")
        self.admin.assert_icommand("imkdir " + dir_path)
        self.admin.assert_icommand("iput %s %s/foo0" % (test_file, dir_path))
        self.admin.assert_icommand("iput %s %s/foo1" % (test_file, dir_path))

        self.admin.assert_icommand("ibun -cD tar " + tar_path + " " +
                                   dir_path, 'STDERR_SINGLELINE', "OVERWRITE_WITHOUT_FORCE_FLAG")

        self.admin.assert_icommand("irm -rf " + dir_path)
        self.admin.assert_icommand("irm -rf " + tar_path)

    def test_ibun__issue_3571(self):
        test_file = "ibun_test_file"
        lib.make_file(test_file, 1000)

        tar_path = self.admin.session_collection + '/somefile.tar'
        dir_path = self.admin.session_collection + '/somedir'

        self.admin.assert_icommand("imkdir " + dir_path)
        for i in range(257):
            self.admin.assert_icommand("iput %s %s/foo%d" % (test_file, dir_path, i))

        self.admin.assert_icommand("ibun -cD tar " + tar_path + " " + dir_path)

        self.admin.assert_icommand("irm -rf " + dir_path)
        self.admin.assert_icommand("irm -rf " + tar_path)


    def test_ibun_x_tarfile_only_on_remote_archive_resource__issue_6997(self):
        def count_objects_in_collection(collection_path):
            return self.admin.run_icommand(['iquest', '%s',
                "select COUNT(DATA_ID) where COLL_NAME like '{}%'"
                .format(collection_path)])[0].strip()

        filename = 'test_ibun_x_tarfile_on_remote_archive_resource'
        file_count = 10
        dir_path = os.path.join(self.user0.local_session_dir, filename)
        tar_path = os.path.join(self.user0.local_session_dir, '{}.tar'.format(filename))
        tar_logical_path = os.path.join(self.user0.session_collection, '{}.tar'.format(filename))
        untar_logical_path = os.path.join(self.user0.session_collection, filename)

        compound_resource = 'comp'
        cache_resource = 'cache'
        archive_resource = 'archive'

        try:
            # Set up a simple compound resource hierarchy. The default policy is sufficient for this test because it
            # simply requires that a stage-to-cache be triggered. This can be forced by purging the cache replica.
            self.admin.assert_icommand(['iadmin', 'mkresc', compound_resource, 'compound'], 'STDOUT')
            lib.create_ufs_resource(self.admin, cache_resource, test.settings.HOSTNAME_2)
            lib.create_ufs_resource(self.admin, archive_resource, test.settings.HOSTNAME_2)
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', compound_resource, cache_resource, 'cache'])
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', compound_resource, archive_resource, 'archive'])

            # Create some test data and tar it up so iRODS can extract it for the test.
            lib.create_directory_of_small_files(dir_path, file_count)
            lib.execute_command(['tar', 'cf', tar_path, dir_path])

            # Put the test tar file to the compound resource and ensure that it exists in the cache and the archive.
            self.user0.assert_icommand(['iput', '-R', compound_resource, tar_path, tar_logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, tar_logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, tar_logical_path, archive_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(tar_logical_path), 0))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(tar_logical_path), 1))

            # Trim the replica in the cache to ensure that a stage-to-cache will occur when extracting the file via
            # ibun. Replica 0 should be the replica in the cache, so trim that one.
            self.user0.assert_icommand(
                ['itrim', '-n0', '-N1', tar_logical_path], 'STDOUT', 'Number of data objects trimmed = 1.')
            self.assertFalse(lib.replica_exists_on_resource(self.user0, tar_logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, tar_logical_path, archive_resource))

            # Extract the tar file via ibun -x and ensure that the stage-to-cache was successful. Also make sure that
            # the extracted contents at least has the same number of objects as there were files. Note that replica 2
            # is the replica in the cache because replica 1 is the replica in the archive and was staged to cache.
            self.user0.assert_icommand(['ibun', '-x', tar_logical_path, untar_logical_path])
            self.assertTrue(lib.replica_exists_on_resource(self.user0, tar_logical_path, cache_resource))
            self.assertTrue(lib.replica_exists_on_resource(self.user0, tar_logical_path, archive_resource))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(tar_logical_path), 1))
            self.assertEqual(str(1), lib.get_replica_status(self.user0, os.path.basename(tar_logical_path), 2))

            # There should be two times the number of results in the resulting collection than the original directory
            # because there is one replica in the cache and one in the archive.
            self.assertEqual(str(2 * file_count), count_objects_in_collection(untar_logical_path))

        finally:
            self.user0.assert_icommand(['ils', '-ALr', os.path.dirname(tar_logical_path)], 'STDOUT') # Debugging

            self.user0.run_icommand(['irm', '-rf', tar_logical_path, untar_logical_path])

            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', compound_resource, cache_resource])
            self.admin.assert_icommand(['iadmin', 'rmchildfromresc', compound_resource, archive_resource])
            lib.remove_resource(self.admin, compound_resource)
            lib.remove_resource(self.admin, cache_resource)
            lib.remove_resource(self.admin, archive_resource)

    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_ibun_tar_with_file_over_8GB__issue_7474(self):
        try:
            test_file_small = "ibun_test_file_small"
            test_file_medium = "ibun_test_file_medium"
            test_file_large = "ibun_test_file_large"
            lib.make_arbitrary_file(test_file_small, 1000)
            lib.make_arbitrary_file(test_file_medium, 100*1024*1024)

            # for this very large file, just making it with truncate as make_arbitrary_file
            # would take too long and contents='random' would use too much entropy
            lib.make_file(test_file_large, 12*1024*1024*1024, 'arbitrary')   # make_arbitrary

            tar_path = f'{self.admin.session_collection}/somefile.tar'
            collection_to_tar = 'tardir'
            extract_collection = 'extracted'
            collection_to_tar_fullpath = f'{self.admin.session_collection}/{collection_to_tar}'
            extract_collection_fullpath = f'{self.admin.session_collection}/{extract_collection}'

            # create directories to tar up and extract the tar to
            self.admin.assert_icommand(f'imkdir {collection_to_tar_fullpath}')
            self.admin.assert_icommand(f'imkdir {extract_collection_fullpath}')

            # put files into tar colleciton
            self.admin.assert_icommand(f'iput {test_file_small} {collection_to_tar_fullpath}')
            self.admin.assert_icommand(f'iput {test_file_medium} {collection_to_tar_fullpath}')
            self.admin.assert_icommand(f'iput {test_file_large} {collection_to_tar_fullpath}')

            # create tar file
            self.admin.assert_icommand(f'ibun -cDtar {tar_path} {collection_to_tar_fullpath}')

            # extract tar file
            self.admin.assert_icommand(f'ibun -x {tar_path} {extract_collection_fullpath}')

            # verify contents of extracted file into irods
            self.admin.assert_icommand(f'ils -l {extract_collection_fullpath}/{collection_to_tar}', 'STDOUT_MULTILINE', [test_file_small, test_file_medium, test_file_large])

            # get the files and compare contents
            self.admin.assert_icommand(f'iget {extract_collection_fullpath}/{collection_to_tar}/{test_file_small} {test_file_small}.downloaded')
            assert_command(f'diff {test_file_small} {test_file_small}.downloaded', 'EMPTY')
            self.admin.assert_icommand(f'iget {extract_collection_fullpath}/{collection_to_tar}/{test_file_medium} {test_file_medium}.downloaded')
            assert_command(f'diff {test_file_medium} {test_file_medium}.downloaded', 'EMPTY')
            self.admin.assert_icommand(f'iget {extract_collection_fullpath}/{collection_to_tar}/{test_file_large} {test_file_large}.downloaded')
            assert_command(f'diff {test_file_large} {test_file_large}.downloaded', 'EMPTY')

        finally:
            self.admin.assert_icommand(f'irm -f {tar_path}')
            self.admin.assert_icommand(f'irm -rf {collection_to_tar_fullpath}')
            self.admin.assert_icommand(f'irm -rf {extract_collection_fullpath}')
            lib.remove_file_if_exists(test_file_small)
            lib.remove_file_if_exists(f'{test_file_small}.downloaded')
            lib.remove_file_if_exists(test_file_medium)
            lib.remove_file_if_exists(f'{test_file_medium}.downloaded')
            lib.remove_file_if_exists(test_file_large)
            lib.remove_file_if_exists(f'{test_file_large}.downloaded')
