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

class Test_ibun(resource_suite.ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_ibun, self).setUp()
        self.known_file_name = 'known_file'

    def tearDown(self):
        super(Test_ibun, self).tearDown()

    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_ibun_extraction_of_big_zip_file__issue_4495(self):
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
            lib.make_file(os.path.join(root_name, self.known_file_name), filesize, 'random')

            out,_ = lib.execute_command(['du', '-h', root_name])
            print(out)

            lib.execute_command(['zip', '-r', zip_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', zip_file_name])
            print(out)

            self.admin.assert_icommand(['iput', zip_file_name])

            self.admin.assert_icommand(['ibun', '-x', zip_file_name, unzip_collection_name])
            self.admin.assert_icommand(['ils', '-lr', unzip_collection_name], 'STDOUT', self.known_file_name)

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
        try:
            root_name = tempfile.mkdtemp()
            untar_collection_name = 'my_exploded_coll'
            untar_directory_name = 'my_exploded_dir'
            tar_file_name = 'bigtar.tar'

            # Generate a junk file
            filesize = 524288000 # 500MB
            lib.make_file(self.known_file_name, filesize, 'random')
            out,_ = lib.execute_command(['ls', '-l', self.known_file_name])
            print(out)

            # Copy junk file until a sufficiently large tar file size is reached
            for i in range(0, 13):
                lib.execute_command(['cp', self.known_file_name, os.path.join(root_name, self.known_file_name + str(i))])

            os.unlink(self.known_file_name)

            out,_ = lib.execute_command(['ls', '-l', root_name])
            print(out)

            lib.execute_command(['tar', '-cf', tar_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', tar_file_name])
            print(out)

            self.admin.assert_icommand(['iput', tar_file_name])

            self.admin.assert_icommand(['ibun', '-x', tar_file_name, untar_collection_name])
            self.admin.assert_icommand(['ils', '-lr', untar_collection_name], 'STDOUT', self.known_file_name)

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
                lib.create_replication_resource(replication, self.admin)
                lib.create_ufs_resource(resource_0, self.admin, test.settings.HOSTNAME_2)
                lib.create_ufs_resource(resource_1, self.admin, test.settings.HOSTNAME_3)
                lib.add_child_resource(replication, resource_0, self.admin)
                lib.add_child_resource(replication, resource_1, self.admin)

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
                lib.remove_child_resource(replication, resource_0, self.admin)
                lib.remove_child_resource(replication, resource_1, self.admin)
                lib.remove_resource(replication, self.admin)
                lib.remove_resource(resource_0, self.admin)
                lib.remove_resource(resource_1, self.admin)

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
