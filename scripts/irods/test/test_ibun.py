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

@unittest.skip('Generation of large file causes I/O thrashing... skip for now')
class Test_Ibun(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ibun, self).setUp()
        self.known_file_name = 'known_file'

    def tearDown(self):
        super(Test_Ibun, self).tearDown()

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

