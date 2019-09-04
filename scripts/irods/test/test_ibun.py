from __future__ import print_function
import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import shutil

from . import resource_suite
from .. import lib

class Test_Ibun(resource_suite.ResourceBase, unittest.TestCase):

    def setUp(self):
        super(Test_Ibun, self).setUp()

    def tearDown(self):
        super(Test_Ibun, self).tearDown()

    def test_ibun_extraction_of_big_zip_file__issue_4495(self):
        try:
            root_name = 'test_ibun_extraction_of_big_zip_file__issue_4495_dir'
            unzip_collection_name = 'my_exploded_coll'
            unzip_directory_name = 'my_exploded_dir'
            known_file = 'known_file'
            zip_file_name = 'bigzip.zip'

            source_dir = '/var/lib/irods/scripts'
            for i in range(0, 10):
                dest_dir = os.path.join(root_name, str(i))
                shutil.copytree(source_dir, dest_dir)

            filesize = 3900000000
            lib.make_file(os.path.join(root_name, known_file), filesize, 'random')

            out,_ = lib.execute_command(['du', '-h', root_name])
            print(out)

            lib.execute_command(['zip', '-r', zip_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', zip_file_name])
            print(out)

            self.admin.assert_icommand(['iput', zip_file_name])

            self.admin.assert_icommand(['ibun', '-x', zip_file_name, unzip_collection_name])
            self.admin.assert_icommand(['ils', '-lr', unzip_collection_name], 'STDOUT', known_file)

            self.admin.assert_icommand(['iget', '-r', unzip_collection_name, unzip_directory_name])
            lib.execute_command(['diff', '-r', root_name, os.path.join(unzip_directory_name, root_name)])
        finally:
            self.admin.run_icommand(['irm', '-f', zip_file_name])
            self.admin.run_icommand(['irm', '-rf', unzip_collection_name])
            shutil.rmtree(root_name, ignore_errors=True)
            shutil.rmtree(unzip_directory_name, ignore_errors=True)
            os.unlink(zip_file_name)
