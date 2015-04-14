import sys
if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

import lib
import make_resource_tree
import cleanup_resource_tree


class Test_ilsresc(lib.make_sessions_mixin([('otherrods', 'pass')], []), unittest.TestCase):
    width = 300
    max_depth = 100
    filename = 'resourcetree.json'

    def setUp(self):
        super(Test_ilsresc, self).setUp()
        make_resource_tree.main(self.width, self.max_depth)

    def tearDown(self):
        cleanup_resource_tree.main(self.filename)
        super(Test_ilsresc, self).tearDown()

    def test_ilsresc_tree(self):
        self.admin_sessions[0].assert_icommand('ilsresc --tree', 'STDOUT', 'resc')

    def test_ilsresc_tree_with_ascii_output(self):
        self.admin_sessions[0].assert_icommand('ilsresc --tree --ascii', 'STDOUT', 'resc')
