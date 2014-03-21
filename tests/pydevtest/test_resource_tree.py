import sys
if (sys.version_info >= (2,7)):
    import unittest
else:
    import unittest2 as unittest
from pydevtest_common import getiCmdOutput
import pydevtest_sessions as s

import make_resource_tree, cleanup_resource_tree


class Test_ilsresc(unittest.TestCase):
    width = 300
    max_depth = 100
    filename = 'resourcetree.json'
    
    def setUp(self):
        # Set up admin session
        s.admin_up()
        
        # Make resource tree
        make_resource_tree.main(self.width, self.max_depth)
        
    def tearDown(self):
        # Clean up resource tree
        cleanup_resource_tree.main(self.filename)
        
        # Clean up and exit admin session 
        print "admin session: removing session files"
        s.adminsession.runCmd('icd')
        s.adminsession.runCmd('irm', ['-rf', s.adminsession.sessionId])
        s.admin_down()
    
    def test_ilsresc_tree(self):
        # Run ilsresc --tree
        output = getiCmdOutput(s.adminsession, "ilsresc --tree")
        
        # Print tree (might as well...)
        print output[0]
        
        # Check that stderr is empty
        assert len(output[1]) == 0