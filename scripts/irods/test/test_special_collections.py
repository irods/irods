import sys
if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

import os
import shutil

from textwrap import dedent

from . import session
from .. import lib
from .. import test
from ..configuration import IrodsConfig

class Test_Special_Collections(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_Special_Collections, self).setUp()

        self.admin = self.admin_sessions[0]

        self.root_resc = 'rootResc'
        self.leaf_resc = 'ufs0'
        lib.create_passthru_resource(self.root_resc, self.admin)
        lib.create_ufs_resource(self.leaf_resc, self.admin)
        lib.add_child_resource(self.root_resc, self.leaf_resc, self.admin)

    def tearDown(self):
        lib.remove_child_resource(self.root_resc, self.leaf_resc, self.admin)
        lib.remove_resource(self.leaf_resc, self.admin)
        lib.remove_resource(self.root_resc, self.admin)
        super(Test_Special_Collections, self).tearDown()

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, 'Uses mounted collection on local filesystem - only works on single-machine deployment')
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_overwrite_and_removal_of_objects_in_mounted_collection__issue_5457(self):
        long_string = ('This is the content for the file that will be written '
                       'to the mounted collection in iRODS. It is longer than 100 bytes.')
        longer_string = ('This is the CHANGED content for the file that will be written '
                         'to the mounted collection in iRODS. It is definitely longer than 100 bytes.')

        filename = 'foo1'
        mounted_collection = os.path.join(self.admin.session_collection, 'mounted_coll')
        mountpoint_directory = os.path.join(self.admin.local_session_dir, 'mounted-coll')
        logical_path = os.path.join(mounted_collection, filename)
        physical_path = os.path.join(mountpoint_directory, filename)

        params = {}
        params['filename'] = filename
        params['root_resc'] = self.root_resc
        params['long_string'] = long_string
        params['longer_string'] = longer_string

        rule_file = os.path.join(self.admin.local_session_dir, 'overwriteFileInMountedColl.r')
        rep_instance_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule_text = dedent('''\
            irule_dummy() {{
                IRULE_overwriteFileInMountedColl(*irodsColl, *irodsResc, *phyDir);
            }}

            IRULE_overwriteFileInMountedColl(*irodsColl, *irodsResc, *phyDir) {{
                # Create the mounted collection
                msiCollCreate(*irodsColl, 0, *status)
                msiPhyPathReg(*irodsColl, *irodsResc, *phyDir, "mountPoint", *status);

                # Write data to file
                *Obj1 = "*irodsColl/{filename}"
                *OFlags1 = "destRescName={root_resc}++++forceFlag="
                *R_BUF1 = "{long_string}"

                msiDataObjCreate(*Obj1,*OFlags1,*D_FD1);
                msiDataObjWrite(*D_FD1,*R_BUF1,*W_LEN1);
                msiDataObjClose(*D_FD1,*Status1);

                writeLine("stdout","Wrote file *Obj1");

                # Overwrite data in file
                *Obj2 = *Obj1
                *OFlags2 = "destRescName={root_resc}++++forceFlag="
                *R_BUF2 = "{longer_string}"

                msiDataObjCreate(*Obj2,*OFlags2,*D_FD2);
                msiDataObjWrite(*D_FD2,*R_BUF2,*W_LEN2);
                msiDataObjClose(*D_FD2,*Status2);

                writeLine("stdout","Overwrote file *Obj2");
            }}

            INPUT *irodsColl="", *irodsResc="", *phyDir=""
            OUTPUT ruleExecOut'''.format(**params))

        with open(rule_file, 'w') as rf:
            rf.write(rule_text)

        try:
            # Create a file in the mounted collection through the rule above
            self.admin.assert_icommand(
                ['irule', '-r', rep_instance_name, '-F', rule_file,
                 '*irodsColl=\'{}\''.format(mounted_collection),
                 '*irodsResc=\'{}\''.format(self.root_resc),
                 '*phyDir=\'{}\''.format(mountpoint_directory)],
                'STDOUT', ['Wrote file {}'.format(logical_path), 'Overwrote file {}'.format(logical_path)])

            # Note: Mounted collections do not actually interact with the catalog, so we must
            # rely on python's os library to stat the file and ensure its correctness
            self.assertTrue(os.path.exists(physical_path))
            self.assertEqual(len(longer_string), os.stat(physical_path).st_size)

            # Ensure iRODS can see the file (even though it's not in the catalog)
            self.admin.assert_icommand(['ils', '-l', mounted_collection], 'STDOUT', filename)

            # Ensure iRODS can remove the file from the mounted collection
            self.admin.run_icommand(['irm', '-f', logical_path])

            # Ensure the file is gone from the mounted collection
            self.admin.assert_icommand_fail(['ils', '-l', mounted_collection], 'STDOUT', filename)

            # Ensure the physical file has been removed
            self.assertFalse(os.path.exists(physical_path))

        finally:
            self.admin.run_icommand(['imcoll', '-U', mounted_collection])
            self.admin.run_icommand(['irm', '-r', '-f', mounted_collection])

            if os.path.exists(rule_file):
                os.unlink(rule_file)

            if os.path.exists(mountpoint_directory):
                shutil.rmtree(mountpoint_directory)
