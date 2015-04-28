import os
import re
import sys

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import configuration
from resource_suite import ResourceBase
import lib


class Test_iPut_Options(ResourceBase, unittest.TestCase):
    def setUp(self):
        super(Test_iPut_Options, self).setUp()

    def tearDown(self):
        super(Test_iPut_Options, self).tearDown()

    def test_iput_options(self):
        filepath = os.path.join( self.admin.local_session_dir, 'file' )
        lib.make_file( filepath, 1 )
        self.admin.assert_icommand('iput --metadata "a;v;u;a2;v2" --acl "read ' + self.user0.username + ';" ' + filepath )
                #+ 'write ' + self.user1.username + ';" -- ' + filepath )
        self.admin.assert_icommand('ichmod read ' + self.user0.username + ' ' + self.admin.session_collection )
        self.admin.assert_icommand('ichmod read ' + self.user1.username + ' ' + self.admin.session_collection )
        self.admin.assert_icommand('imeta ls -d file', 'STDOUT',
                '(attribute: *a2?\nvalue: *v2?\nunits: *u?(\n-+ *\n)?){2}', use_regex=True )
        self.admin.assert_icommand('ils -l', 'STDOUT')
        self.admin.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.admin.local_session_dir + '/newfile' )
        self.user0.assert_icommand('iget -- ' + self.admin.session_collection + '/file ' + self.user0.local_session_dir + '/newfile' )
        new_filepath = os.path.join( self.user1.local_session_dir, 'file' )
        #skip the end until the iput -f of unowned files is resolved
        #lib.make_file( new_filepath, 2 )
        #self.user1.assert_icommand('iput -f -- ' + new_filepath + ' ' + self.admin.session_collection + '/file' );
