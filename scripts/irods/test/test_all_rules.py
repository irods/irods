from __future__ import print_function
import getpass
import json
import os
import shutil
import six
import socket
import sys
import tempfile
import textwrap

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import lib
from .. import test
from .. import paths
from ..configuration import IrodsConfig
from ..controller import IrodsController
from . import metaclass_unittest_test_case_generator
from . import resource_suite
from . import session

@six.add_metaclass(metaclass_unittest_test_case_generator.MetaclassUnittestTestCaseGenerator)
class Test_AllRules(resource_suite.ResourceBase, unittest.TestCase):

    class_name = 'Test_AllRules'

    global plugin_name
    plugin_name = IrodsConfig().default_rule_engine_plugin

    global database_instance_name
    if test.settings.TOPOLOGY_FROM_RESOURCE_SERVER:
        database_instance_name = 'none'
    else:
        database_instance_name = next(iter(IrodsConfig().server_config['plugin_configuration']['database']))

    global rulesdir
    currentdir = os.path.dirname(os.path.realpath(__file__))
    if plugin_name == 'irods_rule_engine_plugin-irods_rule_language':
        rulesdir = os.path.join(IrodsConfig().irods_directory, 'clients', 'icommands', 'test', 'rules')
    elif plugin_name == 'irods_rule_engine_plugin-python':
        rulesdir = os.path.join(IrodsConfig().irods_directory, 'scripts', 'irods', 'test', 'python_rules')
    conf_dir = IrodsConfig().core_re_directory

    def setUp(self):
        super(Test_AllRules, self).setUp()

        self.rods_session = session.make_session_for_existing_admin()  # some rules hardcode 'rods' and 'tempZone'

        hostname = socket.gethostname()
        hostuser = getpass.getuser()
        progname = __file__
        dir_w = os.path.join(IrodsConfig().irods_directory, 'clients', 'icommands', 'test')
        self.rods_session.assert_icommand('icd')  # to get into the home directory (for testallrules assumption)
        self.rods_session.assert_icommand('iadmin mkuser devtestuser rodsuser')
        self.rods_session.assert_icommand('iadmin mkresc testallrulesResc unixfilesystem ' + hostname + ':/tmp/' +
                                          hostuser + '/testallrulesResc', 'STDOUT_SINGLELINE', 'unixfilesystem')
        self.rods_session.assert_icommand('imkdir sub1')
        self.rods_session.assert_icommand('imkdir sub3')
        self.rods_session.assert_icommand('imkdir forphymv')
        self.rods_session.assert_icommand('imkdir ruletest')
        self.rods_session.assert_icommand('imkdir test')
        self.rods_session.assert_icommand('imkdir test/phypathreg')
        self.rods_session.assert_icommand('imkdir ruletest/subforrmcoll')
        self.rods_session.assert_icommand('iput ' + progname + ' test/foo1')
        self.rods_session.assert_icommand('icp test/foo1 sub1/dcmetadatatarget')
        self.rods_session.assert_icommand('icp test/foo1 sub1/mdcopysource')
        self.rods_session.assert_icommand('icp test/foo1 sub1/mdcopydest')
        self.rods_session.assert_icommand('icp test/foo1 sub1/foo1')
        self.rods_session.assert_icommand('icp test/foo1 sub1/foo2')
        self.rods_session.assert_icommand('icp test/foo1 sub1/foo3')
        self.rods_session.assert_icommand('icp test/foo1 forphymv/phymvfile')
        self.rods_session.assert_icommand('icp test/foo1 sub1/objunlink1')
        self.rods_session.assert_icommand('irm sub1/objunlink1')  # put it in the trash
        self.rods_session.assert_icommand('icp test/foo1 sub1/objunlink2')
        self.rods_session.assert_icommand('irepl -R testallrulesResc sub1/objunlink2')
        self.rods_session.assert_icommand('icp test/foo1 sub1/freebuffer')
        self.rods_session.assert_icommand('icp test/foo1 sub1/automove')
        self.rods_session.assert_icommand('icp test/foo1 test/versiontest.txt')
        self.rods_session.assert_icommand('icp test/foo1 test/metadata-target.txt')
        self.rods_session.assert_icommand('icp test/foo1 test/ERAtestfile.txt')
        self.rods_session.assert_icommand('ichmod read devtestuser test/ERAtestfile.txt')
        self.rods_session.assert_icommand('imeta add -d test/ERAtestfile.txt Fun 99 Balloons')
        self.rods_session.assert_icommand('icp test/foo1 sub1/for_versioning.txt')
        self.rods_session.assert_icommand('imkdir sub1/SaveVersions')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/devtestuser-account-ACL.txt test')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/load-metadata.txt test')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/load-usermods.txt test')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/sample.email test')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/email.tag test')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/sample.email test/sample2.email')
        self.rods_session.assert_icommand('iput ' + dir_w + '/misc/email.tag test/email2.tag')

        # setup for rulemsiAdmChangeCoreRE and the likes
        empty_core_file_name = 'empty.test.re'
        new_core_file_name = 'new.test.re'
        with open(self.conf_dir + '/' + empty_core_file_name, 'wt'):
            pass
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/core.re.bckp")           # back up core.re
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/" + new_core_file_name)   # copy core.re

    def tearDown(self):
        self.rods_session.assert_icommand('icd')  # for home directory assumption
        self.rods_session.assert_icommand(['ichmod', '-r', 'own', self.rods_session.username, '.'])
        self.rods_session.run_icommand(['imcoll', '-U', self.rods_session.home_collection + '/test/phypathreg'])
        self.rods_session.run_icommand('irm -rf test ruletest forphymv sub1 sub2 sub3 bagit rules bagit.tar /' +
                                       self.rods_session.zone_name + '/bundle/home/' + self.rods_session.username)
        self.rods_session.assert_icommand(['irmtrash', '-M'])
        self.rods_session.assert_icommand('iadmin rmresc testallrulesResc')
        self.rods_session.assert_icommand('iadmin rmuser devtestuser')
        self.rods_session.assert_icommand('iqdel -a')  # remove all/any queued rules

        # cleanup mods in iRODS config dir
        lib.execute_command('mv -f {0}/core.re.bckp {0}/core.re'.format(self.conf_dir))
        lib.execute_command('rm -f {0}/*.test.re'.format(self.conf_dir))

        self.rods_session.__exit__()
        super(Test_AllRules, self).tearDown()

    def generate_tests_allrules():

        def filter_rulefiles(rulefile):

            # Skip rules that do not work in the Python rule language
            #   Generally, this is because they use types (such as ExecCmdOut) that are not yet implemented
            if plugin_name == 'irods_rule_engine_plugin-python':
                names_to_skip = [
                    'rulemsiDoSomething',
                    'rulemsiExecCmd',
                    'rulemsiExecStrCondQuery',
                    'rulemsiExit',
                    'rulemsiExtractTemplateMDFromBuf',
                    'rulemsiGetStderrInExecCmdOut',
                    'rulemsiGetStdoutInExecCmdOut',
                    'rulemsiGetValByKey',
                    'rulemsiListEnabledMS',
                    'rulemsiMakeQuery',
                    'rulemsiPrintGenQueryOutToBuffer',
                    'rulemsiPrintKeyValPair',
                    'rulemsiReadMDTemplateIntoTagStruct',
                    'rulemsiStrToBytesBuf',
                    'rulewriteBytesBuf',
                    'test_no_memory_error_patch_2242',
                    'testsuite1',
                    'testsuite2',
                    'testsuite3',
                    'testsuiteForLcov'
                ]
                for n in names_to_skip:
                    if n in rulefile:
                        return False

            # only works for package install, rule file hardcodes source directory
            if rulefile == 'rulemsiPhyPathReg.r':
                if os.path.dirname(os.path.abspath(__file__)) != '/var/lib/irods/scripts/irods/test':
                    return False

            # skip rules that fail by design
            names_to_skip = [
                "GoodFailure"
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- failbydesign")
                    return False

            # skip if an action (run in the core.re), not enough input/output for irule
            names_to_skip = [
                "rulemsiAclPolicy",
                "rulemsiAddUserToGroup",
                "rulemsiCheckHostAccessControl",
                "rulemsiCheckOwner",
                "rulemsiCheckPermission",
                "rulemsiCommit",
                "rulemsiCreateCollByAdmin",
                "rulemsiCreateUser",
                "rulemsiDeleteCollByAdmin",
                "rulemsiDeleteDisallowed",
                "rulemsiDeleteUser",
                "rulemsiExtractNaraMetadata",
                "rulemsiOprDisallowed",
                "rulemsiRenameCollection",
                "rulemsiRenameLocalZone",
                "rulemsiRollback",
                "rulemsiSetBulkPutPostProcPolicy",
                "rulemsiSetDataObjAvoidResc",
                "rulemsiSetDataObjPreferredResc",
                "rulemsiSetDataTypeFromExt",
                "rulemsiSetDefaultResc",
                "rulemsiSetGraftPathScheme",
                "rulemsiSetNoDirectRescInp",
                "rulemsiSetNumThreads",
                "rulemsiSetPublicUserOpr",
                "rulemsiSetRandomScheme",
                "rulemsiSetRescQuotaPolicy",
                "rulemsiSetResource",
                "rulemsiNoChkFilePathPerm",
                "rulemsiNoTrashCan",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- input/output")
                    return False

            # skip rules we are not yet supporting
            names_to_skip = [
                "rulemsiobj",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- msiobj")
                    return False

            # transition to core microservices only
            names_to_skip = [
                "rulemsiAddKeyVal.r",
                "rulemsiApplyDCMetadataTemplate.r",
                "rulemsiAssociateKeyValuePairsToObj.r",
                "rulemsiCollectionSpider.r",
                "rulemsiGetObjectPath.r",
                "rulemsiPropertiesGet.r",
                "rulemsiRemoveKeyValuePairsFromObj.r",
                "rulemsiSetDataType.r",
                "rulemsiString2KeyValPair.r",
                "rulewriteKeyValPairs.r",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- transition to core")
                    return False

            # skipping rules requiring additional .re files in community code
            names_to_skip = [
                "rulemsiAdmAddAppRuleStruct.r",
                "rulemsiAdmClearAppRuleStruct.r",
                "rulemsiAdmInsertRulesFromStructIntoDB.r",
                "rulemsiAdmReadRulesFromFileIntoStruct.r",
                "rulemsiAdmRetrieveRulesFromDBIntoStruct.r",
                "rulemsiAdmWriteRulesFromStructIntoFile.r",
                "ruleTestChangeSessionVar.r", # only fails in RIP install
                "rulemsiStoreVersionWithTS.r", # only fails in RIP install
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- community")
                    return False

            # skipping for now, not sure why it's throwing a stacktrace at the moment
            if "rulemsiPropertiesToString" in rulefile:
                # print("skipping " + rulefile + " ----- b/c of stacktrace")
                return False

            # misc / other
            if "rulemsiCollRepl" in rulefile:
                # print("skipping " + rulefile + " ----- deprecated")
                return False
            if "rulemsiTarFileExtract" in rulefile:
                # print("skipping " + rulefile + " ----- CAT_NO_ROWS_FOUND - failed in)
                # call to getDataObjInfoIncSpecColl"
                return False
            if "rulemsiDataObjRsync" in rulefile:
                # print("skipping " + rulefile + " ----- tested separately")
                return False

            return True

        for rulefile in filter(filter_rulefiles, sorted(os.listdir(rulesdir))):
            def make_test(rulefile):
                def test(self):
                    self.rods_session.assert_icommand("icd")
                    self.rods_session.assert_icommand("irule -vF " + os.path.join(rulesdir, rulefile) + " -r " + plugin_name + "-instance"
                                                     ,'STDOUT_SINGLELINE', "completed successfully")
                return test

            yield 'test_' + rulefile.replace('.', '_'), make_test(rulefile)

    def test_rulemsiDataObjRsync(self):
        rulefile = 'rulemsiDataObjRsync.r'
        src_filename = 'source.txt'
        dest_filename = 'dest.txt'
        test_dir = '/tmp'
        test_coll = self.rods_session.home_collection + '/synctest'
        src_file = os.path.join(test_dir, src_filename)
        src_obj = test_coll + '/' + src_filename
        dest_obj = test_coll + '/' + dest_filename

        # create test collection
        self.rods_session.run_icommand(['imkdir', test_coll])

        # create source test file
        with open(src_file, 'at') as f:
            print('blah\n', file=f, end='')

        # upload source test file
        self.rods_session.run_icommand(['iput', src_file, test_coll])

        # first rsync rule test
        self.rods_session.assert_icommand("irule -F " + os.path.join(rulesdir, rulefile), 'STDOUT_SINGLELINE', "status = 99999992")

        # modify the source and try again
        for i in range(1, 5):
            with open(src_file, 'at') as f:
                print('blah_' + str(i) + '\n', file=f, end='')

            # force upload source
            self.rods_session.run_icommand(['iput', '-fK', src_file, test_coll])

            # sync test
            self.rods_session.assert_icommand("irule -F " + os.path.join(rulesdir, rulefile), 'STDOUT_SINGLELINE', "status = 99999992")

        # cleanup
        self.rods_session.run_icommand(['irm', '-rf', test_coll])
        os.remove(src_file)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_str_2528(self):
        self.rods_session.assert_icommand('''irule "*a.a = 'A'; *a.b = 'B'; writeLine('stdout', str(*a))" null ruleExecOut''', 'STDOUT_SINGLELINE', "a=A++++b=B")

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_datetimef_3767(self):
        self.rods_session.assert_icommand('''irule "*RetTime = '2018-01-01';writeLine('stdout',datetimef(*RetTime,'%Y-%m-%d'));" null ruleExecOut''', 'STDOUT_SINGLELINE', "Jan 01 2018")

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_type_3575(self):
        rule_file = "test_rule_file.r"
        rule_string = '''
main{
  *a:string;
  *a="0";
  func1(*a);
  *a == "-1";
  int(*a);
}
func1(*i) {
}
input null
output ruleExecOut
'''
        with open(rule_file, 'wt') as f:
            print(rule_string, file=f, end='')

        self.admin.assert_icommand('irule -F ' + rule_file)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_pattern_3575(self):
        rule_file = "test_rule_file.r"
        rule_string = '''
main {
  *a."a" = "a";
  asdf("a");
}
asdf(*a) {}
input null
output ruleExecOut
'''
        with open(rule_file, 'wt') as f:
            print(rule_string, file=f, end='')

        self.admin.assert_icommand('irule -F ' + rule_file)

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'msiGetStderrInExecCmdOut not available in Python yet - RTS')
    def test_return_data_structure_non_null_2604(self):
        self.rods_session.assert_icommand(
            '''irule "*Err = errorcode(msiExecCmd('cmd', '', '', '', '', *Out)); msiGetStderrInExecCmdOut(*Out, *Stderr); writeLine('stdout', 'stderr: *Err*Stderr')" null ruleExecOut''', 'STDOUT_SINGLELINE', "stderr")

    @unittest.skip('Delete this line upon resolving #3957')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_writeLine_config_last_3477(self):
        irods_config = IrodsConfig()
        orig = irods_config.server_config['plugin_configuration']['rule_engines']
        try:
            irods_config.server_config['plugin_configuration']['rule_engines'] = orig + [{
                    'instance_name': 'irods_rule_engine_plugin-irods_rule_language-instance',
                    'plugin_name': 'irods_rule_engine_plugin-irods_rule_language',
                    'plugin_specific_configuration': {
                        "re_data_variable_mapping_set": [
                            "core"
                        ],
                        "re_function_name_mapping_set": [
                            "core"
                        ],
                        "re_rulebase_set": [
                            "core"
                        ],
                        "regexes_for_supported_peps": [
                            "ac[^ ]*",
                            "msi[^ ]*",
                            "[^ ]*pep_[^ ]*_(pre|post)"
                        ]
                    },
                    "shared_memory_instance": "irods_rule_language_rule_engine"
                }]
            irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=True)

            rule_file = "test_rule_file.r"
            rule_string = '''def main(args, cb, rei):
    cb.writeLine("stdout", "from_prep")

input null
output ruleExecOut
'''
            with open(rule_file, 'wt') as f:
                print(rule_string, file=f, end='')

            self.rods_session.assert_icommand(
                '''irule -F test_rule_file.r''', 'STDOUT_SINGLELINE', "from_prep")
        finally:

            IrodsController().stop()

            irods_config.server_config['plugin_configuration']['rule_engines'] = orig
            irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=True)

            IrodsController().start()

    @unittest.skip('Delete this line upon resolving #3957')
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for python REP')
    def test_writeLine_config_first_3477(self):
        irods_config = IrodsConfig()
        orig = irods_config.server_config['plugin_configuration']['rule_engines']
        try:
            irods_config.server_config['plugin_configuration']['rule_engines'] = [{
                    'instance_name': 'irods_rule_engine_plugin-irods_rule_language-instance',
                    'plugin_name': 'irods_rule_engine_plugin-irods_rule_language',
                    'plugin_specific_configuration': {
                        "re_data_variable_mapping_set": [
                            "core"
                        ],
                        "re_function_name_mapping_set": [
                            "core"
                        ],
                        "re_rulebase_set": [
                            "core"
                        ],
                        "regexes_for_supported_peps": [
                            "ac[^ ]*",
                            "msi[^ ]*",
                            "[^ ]*pep_[^ ]*_(pre|post)"
                        ]
                    },
                    "shared_memory_instance": "irods_rule_language_rule_engine"
                }] + orig
            irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=True)

            rule_file = "test_rule_file.r"
            rule_string = '''def main(args, cb, rei):
    cb.writeLine("stdout", "from_prep")

input null
output ruleExecOut
'''
            with open(rule_file, 'wt') as f:
                print(rule_string, file=f, end='')

            self.rods_session.assert_icommand(
                '''irule -r ''' + orig[0]["instance_name"] + ''' -F test_rule_file.r''', 'STDOUT_SINGLELINE', "from_prep")
        finally:

            IrodsController().stop()

            irods_config.server_config['plugin_configuration']['rule_engines'] = orig
            irods_config.commit(irods_config.server_config, irods_config.server_config_path, make_backup=True)

            IrodsController().start()


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiServerMonPerf_default_3736(self):
        rule_file="test_msiServerMonPerf.r"
        rule_string= '''
msiTestServerMonPerf {{
    msiServerMonPerf("default", "default");
}}

INPUT null
OUTPUT ruleExecOut
'''

        with open(rule_file, 'w') as f:
            f.write(rule_string)

        self.rods_session.assert_icommand("irule -F " + rule_file);

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiCheckAccess_3309(self):
        # setup for test_msiCheckAccess_3309
        self.rods_session.assert_icommand('icd')
        lib.make_file('test_file_3309', 1)
        self.rods_session.assert_icommand('iput test_file_3309')
        self.rods_session.assert_icommand('imkdir test_collection_3309')
        self.rods_session.assert_icommand('iadmin mkgroup group1_3309')  # group rods does not belong to 
        self.rods_session.assert_icommand('iadmin mkgroup group2_3309')  # group rods belongs to 
        self.rods_session.assert_icommand('iadmin atg group2_3309 rods')  

        try:
            data_obj_rule_file="test_msiCheckAccess_data_obj_3309.r"
            rule_string= '''
testMsiCheckAccess {
     msiCheckAccess("/tempZone/home/rods/test_file_3309", "read_object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_file_3309", "modify_object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_file_3309", "own", *result);
     writeLine("stdout", "result=*result");
}

INPUT null
OUTPUT ruleExecOut
'''

            with open(data_obj_rule_file, 'w') as f:
                f.write(rule_string)

            collection_rule_file="test_msiCheckAccess_collection_3309.r"
            rule_string= '''
testMsiCheckAccess {
     msiCheckAccess("/tempZone/home/rods/test_collection_3309", "read_object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_collection_3309", "modify_object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_collection_3309", "own", *result);
     writeLine("stdout", "result=*result");
}

INPUT null
OUTPUT ruleExecOut
'''

            with open(collection_rule_file, 'w') as f:
                f.write(rule_string)



            self.rods_session.assert_icommand('ichmod own group1_3309 test_file_3309')  # just to make sure we don't pick up ownership 

            # --------- check permissions on file -----------

            # check permissions by user 

            self.rods_session.assert_icommand('ichmod null rods test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=0$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M read rods test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M write rods test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M own rods test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
                use_regex=True)


            # remove user permission and check permissions by group 

            self.rods_session.assert_icommand('ichmod null rods test_file_3309')

            self.rods_session.assert_icommand('ichmod -M read group2_3309 test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M write group2_3309 test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M own group2_3309 test_file_3309')
            self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
                use_regex=True)


            # --------- check permissions on collection -----------

            # check permissions by user 

            self.rods_session.assert_icommand('ichmod null rods test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=0$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M read rods test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M write rods test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M own rods test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
                use_regex=True)


            # remove user permission and check permissions by group 

            self.rods_session.assert_icommand('ichmod null rods test_collection_3309')

            self.rods_session.assert_icommand('ichmod -M read group2_3309 test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M write group2_3309 test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
                use_regex=True)

            self.rods_session.assert_icommand('ichmod -M own group2_3309 test_collection_3309')
            self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
                use_regex=True)

        finally:
            # cleanup for test_msiCheckAccess_3309
            self.rods_session.assert_icommand('irm -f test_file_3309')
            self.rods_session.assert_icommand('irm -rf test_collection_3309')
            self.rods_session.assert_icommand('iadmin rmgroup group1_3309')
            self.rods_session.assert_icommand('iadmin rmgroup group2_3309')


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_msiTarFileExtract_big_file__issue_4118(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                myTestRule {{
                    msiTarFileExtract(*File,*Coll,*Resc,*Status);
                    writeLine("stdout","Extract files from a tar file *File into collection *Coll on resource *Resc");
                }}
                INPUT *File="{logical_path_to_tar_file}", *Coll="{logical_path_to_untar_coll}", *Resc="demoResc"
                OUTPUT ruleExecOut
            ''')
        }

        try:
            test_name = 'test_msiTarFileExtract_big_file__issue_4118'
            root_name = test_name + '_dir'
            untar_collection_name = 'my_exploded_coll'
            untar_directory_name = 'my_exploded_dir'
            tar_file_name = 'bigtar.tar'
            known_file_name = 'known_file'

            # Generate a junk file
            filesize = 524288000 # 500MB
            lib.make_file(known_file_name, filesize, 'random')
            out,_ = lib.execute_command(['ls', '-l', known_file_name])
            print(out)

            # Copy junk file until a sufficiently large tar file size is reached
            os.mkdir(root_name)
            for i in range(0, 13):
                lib.execute_command(['cp', known_file_name, os.path.join(root_name, known_file_name + str(i))])

            out,_ = lib.execute_command(['ls', '-l', root_name])
            print(out)

            lib.execute_command(['tar', '-cf', tar_file_name, root_name])
            out,_ = lib.execute_command(['ls', '-l', tar_file_name])
            print(out)

            self.admin.assert_icommand(['iput', tar_file_name])

            try:
                logical_path_to_tar_file = os.path.join(self.admin.session_collection, tar_file_name)
                logical_path_to_untar_coll = os.path.join(self.admin.session_collection, untar_collection_name)
                rule_file = test_name + '.r'
                with open(rule_file, 'w') as f:
                    f.write(rule_map[plugin_name].format(**locals()))
                self.admin.assert_icommand(['irule', '-F', rule_file], 'STDOUT', 'Extract files from a tar file')
            finally:
                if os.path.exists(rule_file):
                    os.unlink(rule_file)

            self.admin.assert_icommand(['ils', '-lr', untar_collection_name], 'STDOUT', known_file_name)

            self.admin.assert_icommand(['iget', '-r', untar_collection_name, untar_directory_name])
            lib.execute_command(['diff', '-r', root_name, os.path.join(untar_directory_name, root_name)])
        finally:
            self.admin.run_icommand(['irm', '-f', tar_file_name])
            self.admin.run_icommand(['irm', '-rf', untar_collection_name])
            shutil.rmtree(root_name, ignore_errors=True)
            shutil.rmtree(untar_directory_name, ignore_errors=True)
            if os.path.exists(known_file_name):
                os.unlink(known_file_name)
            if os.path.exists(tar_file_name):
                os.unlink(tar_file_name)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_msiRenameCollection_does_rename_collections__issue_4597(self):
        # Create a collection, "col.a".
        src = os.path.join(self.admin.session_collection, 'col.a')
        dst = os.path.join(self.admin.session_collection, 'col.a.renamed')
        self.admin.assert_icommand(['imkdir', src])

        # The following is a rule template for invoking msiRenameCollection.
        rule_file = os.path.join(self.admin.local_session_dir, 'test_rule_file.r')
        rule_string = '''
main {{
    *src = "{0}";
    *dst = "{1}";
    msiRenameCollection(*src, *dst);
}}
INPUT null
OUTPUT ruleExecOut'''

        # Create a rule file, using the template, that renames "col.a" to "col.a.renamed".
        with open(rule_file, 'wt') as f:
            contents = rule_string.format(src, dst)
            print(contents, file=f, end='')
            print(contents)

        # Invoke the rule and verify that the rename was successful.
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file])
        self.admin.assert_icommand(['ils', dst], 'STDOUT', [dst])
        self.admin.assert_icommand(['ils', src], 'STDERR', [src + ' does not exist'])

        # Overwrite the existing rule file that attempts to renames "col.a.renamed" back to "col.a".
        # Notice the call to os.path.basename.
        with open(rule_file, 'wt') as f:
            contents = rule_string.format(dst, os.path.basename(src))
            print(contents, file=f, end='')
            print(contents)

        # Show that the rule will fail because a relative path was being passed as the destination.
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', ['-358000 OBJ_PATH_DOES_NOT_EXIST'])
        self.admin.assert_icommand(['ils', dst], 'STDOUT', [dst])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_msiRenameCollection_does_not_support_renaming_data_objects__issue_5452(self):
        # Create a new data object.
        src = os.path.join(self.admin.session_collection, 'foo')
        dst = os.path.join(self.admin.session_collection, 'bar')
        self.admin.assert_icommand(['istream', 'write', src], input='foo')

        # Show that msiRenameCollection fails when the source references a data object.
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        cmd = ['irule', '-r', rep_name, 'msiRenameCollection("{0}", "{1}")'.format(src, dst), 'null', 'ruleExecOut']
        self.admin.assert_icommand(cmd, 'STDERR', ['-170000 NOT_A_COLLECTION'])

        # Show that the 'src' data object still exists and that the 'dst' data object does not
        # because the rename failed.
        self.admin.assert_icommand(['ils', src], 'STDOUT', [src])
        self.admin.assert_icommand(['ils', dst], 'STDERR', [dst + ' does not exist'])

        # Create the destination data object.
        self.admin.assert_icommand(['istream', 'write', dst], input='bar')

        # Show that msiRenameCollection fails when the destination references a data object.
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        cmd = ['irule', '-r', rep_name, 'msiRenameCollection("{0}", "{1}")'.format(self.admin.session_collection, dst), 'null', 'ruleExecOut']
        self.admin.assert_icommand(cmd, 'STDERR', ['-170000 NOT_A_COLLECTION'])

        # Show that the source and destination still exist because the rename failed.
        self.admin.assert_icommand(['ils', src], 'STDOUT', [src])
        self.admin.assert_icommand(['ils', dst], 'STDOUT', [dst])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    def test_msiDataObjPhymv_to_resource_hierarchy__3234(self):
        source_resource = self.admin.default_resource
        passthrough_resource = 'phymv_pt'
        path_to_leaf_resource_vault = tempfile.mkdtemp()
        leaf_resource = 'leafresc'
        resource_hierarchy = ';'.join([passthrough_resource, leaf_resource])
        data_object_name = 'phymv_obj'
        logical_path = os.path.join(self.admin.session_collection, data_object_name)

        rule_file = os.path.join(self.admin.local_session_dir, 'test_msiDataObjPhymv_to_resource_hierarchy__3234.r')
        rule_string = '''
test_msiDataObjPhymv_to_resource_hierarchy__3234 {{
    *logical_path = "{0}"
    *destination_resource = "{1}";
    *source_resource = "{2}";
    msiDataObjPhymv(*logical_path, *destination_resource, *source_resource, "null", "null", *status);
    writeLine("stdout", "msiDataObjPhymv status:[*status]");
}}
OUTPUT ruleExecOut
'''

        try:
            self.admin.assert_icommand(['iadmin', 'mkresc', passthrough_resource, 'passthru'], 'STDOUT', passthrough_resource)
            self.admin.assert_icommand(['iadmin', 'mkresc', leaf_resource, 'unixfilesystem',
                socket.gethostname() + ':' + path_to_leaf_resource_vault], 'STDOUT', 'unixfilesystem')
            self.admin.assert_icommand(['iadmin', 'addchildtoresc', passthrough_resource, leaf_resource])

            with open(rule_file, 'wt') as f:
                print(rule_string.format(logical_path, leaf_resource, source_resource), file=f, end='')
            self.admin.assert_icommand(['iput', rule_file, logical_path])
            self.admin.assert_icommand(['iadmin', 'ls', 'logical_path', logical_path, 'replica_number', '0'],
                'STDOUT', 'DATA_RESC_HIER: {}'.format(source_resource))

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDOUT', 'msiDataObjPhymv status')
            self.admin.assert_icommand(['iadmin', 'ls', 'logical_path', logical_path, 'replica_number', '0'],
                'STDOUT', 'DATA_RESC_HIER: {}'.format(resource_hierarchy))

        finally:
            if os.path.exists(rule_file):
                os.unlink(rule_file)
            self.admin.assert_icommand(['irm', '-f', logical_path])
            self.admin.run_icommand(['iadmin', 'rmchildfromresc', passthrough_resource, leaf_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', passthrough_resource])
            self.admin.run_icommand(['iadmin', 'rmresc', leaf_resource])

    @unittest.skip(("Fails against databases with transaction isolation level set to REPEATABLE-READ (e.g. MySQL). "
                    "For more details, see https://github.com/irods/irods/issues/4917"))
    def test_msi_atomic_apply_metadata_operations__issue_4484(self):
        def do_test(entity_name, entity_type, operation, expected_output=None):
            json_input = json.dumps({
                'entity_name': entity_name,
                'entity_type': entity_type,
                'operations': [
                    {
                        'operation': operation,
                        'attribute': 'issue_4484_name',
                        'value': 'issue_4484_value',
                        'units': 'issue_4484_units'
                    }
                ]
            })

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            rule = "msi_atomic_apply_metadata_operations('{0}', *ignored)".format(json_input)
            self.admin.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'null'])

            # Convert entity type to correct type for imeta.
            imeta_type = '-u'
            if   entity_type == 'resource':    imeta_type = '-R'
            elif entity_type == 'collection':  imeta_type = '-C'
            elif entity_type == 'data_object': imeta_type = '-d'

            if   expected_output is not None: imeta_output = expected_output
            elif operation == 'add':          imeta_output = ['issue_4484_name', 'issue_4484_value', 'issue_4484_units']
            elif operation == 'remove':       imeta_output = ['None']

            self.admin.assert_icommand(['imeta', 'ls', imeta_type, entity_name], 'STDOUT', imeta_output)

        do_test(self.admin.username, 'user', 'add')
        do_test(self.admin.username, 'user', 'remove')

        do_test(self.admin.default_resource, 'resource', 'add')
        do_test(self.admin.default_resource, 'resource', 'remove')

        do_test(self.admin.session_collection, 'collection', 'add')
        do_test(self.admin.session_collection, 'collection', 'remove')

        filename = os.path.join(self.admin.local_session_dir, 'issue_4484.txt')
        lib.make_file(filename, 1, 'arbitrary')
        self.admin.assert_icommand(['iput', filename])
        data_object = os.path.join(self.admin.session_collection, os.path.basename(filename))
        do_test(data_object, 'data_object', 'add')
        do_test(data_object, 'data_object', 'remove')

        # Verify that the PEPs for the API plugin are firing.
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            # Add PEPs to the core.re file.
            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    pep_api_atomic_apply_metadata_operations_pre(*INSTANCE, *COMM, *JSON_INPUT, *JSON_OUTPUT) {{ 
                        *kvp.'atomic_pre_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}

                    pep_api_atomic_apply_metadata_operations_post(*INSTANCE, *COMM, *JSON_INPUT, *JSON_OUTPUT) {{ 
                        *kvp.'atomic_post_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}
                '''.format(data_object))

            do_test(data_object, 'data_object', 'add', ['atomic_pre_fired', 'atomic_post_fired'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "database_instance_name cannot be determined from catalog consumer")
    @unittest.skipIf(database_instance_name == 'mysql',
                    ("Fails against databases with transaction isolation level set to REPEATABLE-READ (e.g. MySQL). "
                     "For more details, see https://github.com/irods/irods/issues/4917"))
    def test_msi_atomic_apply_metadata_operations_considers_group_permissions__issue_6190(self):
        def do_test(_entity_type):
            entity_name = os.path.join(self.admin.session_collection, 'issue_6190')

            json_input = json.dumps({
                'entity_name': entity_name,
                'entity_type': _entity_type,
                'operations': [
                    {
                        'operation': 'add',
                        'attribute': 'issue_6190_name',
                        'value': 'issue_6190_value',
                        'units': 'issue_6190_units'
                    }
                ]
            })

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            rule = "msi_atomic_apply_metadata_operations('{0}', *ignored)".format(json_input)

            imeta_type = 'INVALID_ENTITY_TYPE'
            if _entity_type == 'data_object':
                imeta_type = '-d'
                self.admin.assert_icommand(['itouch', entity_name])
            elif _entity_type == 'collection':
                imeta_type = '-C'
                self.admin.assert_icommand(['imkdir', entity_name])

            try:
                # Give the public group READ permission on the parent collection so that "self.user0"
                # can see the object.
                self.admin.assert_icommand(['ichmod', 'read', 'public', os.path.dirname(entity_name)])

                # Show that the non-admin user is not allowed to add metadata because they don't
                # have the necessary permissions. In this particular case, the atomic API plugin will
                # not be able to use GenQuery to retrieve information about the target object
                # (i.e. DATA_ID / COLL_ID).
                self.user0.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'], 'STDERR', ['SYS_INVALID_INPUT_PARAM'])
                self.admin.assert_icommand(['imeta', 'ls', imeta_type, entity_name], 'STDOUT', ['None'])

                # Give the public group WRITE permissions on the object owned by the administrator.
                # This allows "self.user0" to modify metadata on the object because all users are members
                # of the public group.
                self.admin.assert_icommand(['ichmod', 'write', 'public', entity_name])

                # Show that the non-admin user can now manipulate metadata because they have permission
                # to do so via the public group.
                self.user0.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'])
                self.admin.assert_icommand(['imeta', 'ls', imeta_type, entity_name],
                                           'STDOUT', ['issue_6190_name', 'issue_6190_value', 'issue_6190_units'])

            finally:
                self.admin.run_icommand(['irm', '-rf', entity_name])

        do_test('data_object')
        do_test('collection')

    @unittest.skip(("Fails against databases with transaction isolation level set to REPEATABLE-READ (e.g. MySQL). "
                    "For more details, see https://github.com/irods/irods/issues/4917"))
    def test_msi_atomic_apply_acl_operations__issue_5001(self):
        def do_test(logical_path, acl):
            json_input = json.dumps({
                'logical_path': logical_path,
                'operations': [
                    {
                        'entity_name': self.user0.username,
                        'acl': acl
                    }
                ]
            })

            # Convert ACL string to the format expected by "ils".
            if   'read'  == acl: ils_acl = 'read_object'
            elif 'write' == acl: ils_acl = 'modify_object'
            elif 'own'   == acl: ils_acl = 'own'

            # Atomically set the ACL for "user0".
            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            rule = "msi_atomic_apply_acl_operations('{0}', *ignored)".format(json_input)
            self.admin.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'null'])

            # Verify that the new ACL is set for "user0".
            expected_output = ' {0}#{1}:{2}'.format(self.user0.username, self.user0.zone_name, ils_acl)
            self.admin.assert_icommand(['ils', '-A', logical_path], 'STDOUT', [expected_output])

        sandbox = os.path.join(self.admin.session_collection, 'sandbox_5001')
        self.admin.assert_icommand(['imkdir', sandbox])

        # Test support for collections.
        do_test(sandbox, 'read')
        do_test(sandbox, 'write')
        do_test(sandbox, 'own')

        # Test support for data objects.
        data_object = os.path.join(sandbox, 'issue_5001')
        self.admin.assert_icommand(['istream', 'write', data_object], input='Hello, iRODS!')
        do_test(data_object, 'read')
        do_test(data_object, 'write')

        # Verify that the PEPs for the API plugin are firing.
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            # Add PEPs to the core.re file.
            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    pep_api_atomic_apply_acl_operations_pre(*INSTANCE, *COMM, *JSON_INPUT, *JSON_OUTPUT) {{ 
                        *kvp.'atomic_pre_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}

                    pep_api_atomic_apply_acl_operations_post(*INSTANCE, *COMM, *JSON_INPUT, *JSON_OUTPUT) {{ 
                        *kvp.'atomic_post_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}
                '''.format(data_object))

            do_test(data_object, 'own')
            self.admin.assert_icommand(['imeta', 'ls', '-d', data_object], 'STDOUT', ['atomic_pre_fired', 'atomic_post_fired'])

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP')
    @unittest.skipIf(test.settings.TOPOLOGY_FROM_RESOURCE_SERVER, "database_instance_name cannot be determined from catalog consumer")
    @unittest.skipIf(database_instance_name == 'mysql',
                    ("Fails against databases with transaction isolation level set to REPEATABLE-READ (e.g. MySQL). "
                     "For more details, see https://github.com/irods/irods/issues/4917"))
    def test_msi_atomic_apply_acl_operations_considers_group_permissions__issue_6191(self):
        def do_test(_entity_type):
            logical_path = os.path.join(self.admin.session_collection, 'issue_6191')

            json_input = json.dumps({
                'logical_path': logical_path,
                'operations': [
                    {
                        'entity_name': 'public',
                        'acl': 'read'
                    }
                ]
            })

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            rule = "msi_atomic_apply_acl_operations('{0}', *ignored)".format(json_input)

            if   _entity_type == 'data_object': self.admin.assert_icommand(['itouch', logical_path])
            elif _entity_type == 'collection' : self.admin.assert_icommand(['imkdir', logical_path])

            try:
                # Show that the non-admin user is not allowed to modify ACLs because they don't
                # have the necessary permissions to do so.
                self.user0.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'], 'STDERR', ['OBJ_PATH_DOES_NOT_EXIST'])
                self.user0.assert_icommand(['ils', '-A', logical_path], 'STDERR', ['does not exist or user lacks access permission'])

                # Give the public group OWN permissions on the object owned by the administrator.
                # This allows "self.user0" to modify ACLs on the object because all users are members
                # of the public group.
                self.admin.assert_icommand(['ichmod', 'own', 'public', logical_path])

                # Give the public group READ permission on the parent collection so that "self.user0"
                # can see the object.
                self.admin.assert_icommand(['ichmod', 'read', 'public', os.path.dirname(logical_path)])

                # Show that the non-admin user can now manipulate ACLs because they have permission
                # to do so via the public group.
                self.user0.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'])
                self.admin.assert_icommand(['ils', '-A', logical_path], 'STDOUT', [' g:public#{0}:read_object'.format(self.admin.zone_name)])

            finally:
                self.admin.run_icommand(['irm', '-rf', logical_path])

        do_test('data_object')
        do_test('collection')

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only run for native rule language')
    def test_msi_touch__issue_4669(self):
        data_object = os.path.join(self.admin.session_collection, 'issue_4669')

        # Show that the data object was created by the microservice.
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        rule = "msi_touch('{0}')".format(json.dumps({'logical_path': data_object}))
        self.admin.assert_icommand(['irule', '-r', rep_name, rule, 'null', 'ruleExecOut'])
        self.admin.assert_icommand(['ils', '-l', data_object], 'STDOUT', [os.path.basename(data_object)])

        # Verify that the PEPs for the API plugin are firing.
        config = IrodsConfig()
        core_re_path = os.path.join(config.core_re_directory, 'core.re')

        with lib.file_backed_up(core_re_path):
            # Add PEPs to the core.re file.
            with open(core_re_path, 'a') as core_re:
                core_re.write('''
                    pep_api_touch_pre(*INSTANCE, *COMM, *JSON_INPUT) {{ 
                        *kvp.'touch_pre_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}

                    pep_api_touch_post(*INSTANCE, *COMM, *JSON_INPUT) {{ 
                        *kvp.'touch_post_fired' = 'YES';
                        msiSetKeyValuePairsToObj(*kvp, '{0}', '-d');
                    }}
                '''.format(data_object))

            # Trigger the PEPs.
            self.admin.assert_icommand(['itouch', '-c', 'bar'])

            # Show that even though no data object was created, the PEPs fired correctly.
            self.admin.assert_icommand(['imeta', 'ls', '-d', data_object], 'STDOUT', ['touch_pre_fired', 'touch_post_fired'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'Skip for PREP and Topology Testing')
    def test_msiExit_prints_user_provided_error_information_on_client_side__issue_4463(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        error_code = '-808000'
        error_msg = '#4463: This message should appear on the client-side!'
        rule_text = 'msiExit("{0}", "{1}")'.format(error_code, error_msg)
        stdout, stderr, ec = self.admin.run_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'])
        self.assertNotEqual(ec, 0)
        self.assertTrue(error_msg in stdout)
        self.assertTrue(error_code + ' CAT_NO_ROWS_FOUND' in stderr)

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_non_admins_are_not_allowed_to_rename_zone_collection__issue_5445(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        cmd = ['irule', '-r', rep_name, 'msiRenameLocalZoneCollection("otherZone")', 'null', 'ruleExecOut']
        self.user0.assert_icommand(cmd, 'STDERR', ['-818000 CAT_NO_ACCESS_PERMISSION'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_rename_to_current_zone_collection_is_a_no_op__issue_5445(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, 'msiRenameLocalZoneCollection("{0}")'.format(self.admin.zone_name), 'null', 'ruleExecOut'])
        self.admin.assert_icommand(['iquest', 'select COLL_NAME'], 'STDOUT', [os.path.join('/', self.admin.zone_name, 'home')])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_rename_to_existing_collection_with_different_name_is_an_error__issue_5445(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        existing_collection = os.path.join(self.admin.zone_name, 'home', self.admin.username)
        cmd = ['irule', '-r', rep_name, 'msiRenameLocalZoneCollection("{0}")'.format(existing_collection), 'null', 'ruleExecOut']
        self.admin.assert_icommand(cmd, 'STDERR', ['-809000 CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_rename_local_zone__issue_5693(self):
        old_zone_name = self.rods_session.zone_name
        new_zone_name = 'issue_5693_zone'

        # Rename the local zone.
        self.rods_session.assert_icommand(['iadmin', 'modzone', old_zone_name, 'name', new_zone_name], 'STDOUT', input='y')

        try:
            config = IrodsConfig()

            with lib.file_backed_up(config.client_environment_path):
                # Update all references to the zone name in the client's irods_environment.json file.
                new_home_collection = os.path.join('/', new_zone_name, 'home', self.rods_session.username)
                lib.update_json_file_from_dict(config.client_environment_path, {
                    'irods_cwd': os.path.join(new_home_collection, os.path.basename(self.rods_session.session_collection)),
                    'irods_home': new_home_collection,
                    'irods_zone_name': new_zone_name
                })

                with lib.file_backed_up(config.server_config_path):
                    # Update the zone name in the server_config.json file.
                    lib.update_json_file_from_dict(config.server_config_path, {'zone_name': new_zone_name})

                    # Restart the server. This is done so that the delay server does not fill
                    # the log with errors due having incorrect zone information.
                    IrodsController().restart()

                    # Update the rods session with the new zone information so that icommands succeed.
                    with open(config.client_environment_path, 'r') as f:
                        self.rods_session.environment_file_contents = json.load(f)

                    # Show that the zone has been updated to reflect the new name.
                    # These assertions prove that msiRenameLocalZoneCollection updated the catalog.
                    self.rods_session.assert_icommand(['iquest', 'select ZONE_NAME'], 'STDOUT', ['ZONE_NAME = ' + new_zone_name])
                    self.rods_session.assert_icommand(['iquest', 'select COLL_NAME'], 'STDOUT', [
                        os.path.join('/', new_zone_name),
                        os.path.join('/', new_zone_name, 'home'),
                        os.path.join('/', new_zone_name, 'trash'),
                        new_home_collection
                    ])

                    # Restore the zone name to its original value.
                    self.rods_session.assert_icommand(['iadmin', 'modzone', new_zone_name, 'name', old_zone_name], 'STDOUT', input='y')
        finally:
            # Restart the server. This is done so that the delay server does not fill
            # the log file with errors due having incorrect zone information.
            IrodsController().restart()

            # Update the rods session with the original zone information so that icommands succeed.
            with open(config.client_environment_path, 'r') as f:
                self.rods_session.environment_file_contents = json.load(f)

            #
            # Show that everything has been restored and is operating in a good state.
            #

            # Create a new data object.
            data_object = 'foo_issue_5693'
            self.rods_session.assert_icommand(['istream', 'write', data_object], input='data')

            # Create a new collection.
            collection = os.path.join(self.rods_session.home_collection, 'issue_5693.d')
            self.rods_session.assert_icommand(['imkdir', collection])

            # Verify the existence of the data object and collection and then remove them.
            self.rods_session.assert_icommand(['ils'], 'STDOUT', [data_object, 'C- ' + collection])
            self.rods_session.assert_icommand(['irm', '-r', '-f', data_object, collection])
            self.rods_session.assert_icommand(['ils'], 'STDOUT', [self.rods_session.home_collection])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_use_lowercase_select_in_genquery_conditions__issue_4697(self):
        thecoll = os.path.join(self.admin.session_collection, 'selected')
        thefile = os.path.join(thecoll, 'foo')
        self.admin.assert_icommand(['imkdir', thecoll])
        self.admin.assert_icommand(['istream', 'write', thefile], input='foo')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        cmd = ['irule', '-r', rep_name, '-F', '/var/lib/irods/clients/icommands/test/rules/rulemsiMakeGenQuery.r', "*Coll='{0}%'".format(thecoll)]
        self.admin.assert_icommand(cmd, 'STDOUT', ['Number of files in {0}% is 1 and total size is 3'.format(thecoll)])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiGetValByKey_does_not_crash_the_agent_on_bad_input_arguments__issue_5420(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        for msi_cmd in ['msiGetValByKey(1, *ignored, *out)', '*kvp."name" = "issue_5420"; msiGetValByKey(*kvp, 1, *out)']:
            self.admin.assert_icommand(['irule', '-r', rep_name, msi_cmd, 'null', 'ruleExecOut'], 'STDERR', ['-130000 SYS_INVALID_INPUT_PARAM'])

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_data_obj_read_for_2100MB_file__5709(self):
        rule_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                msiDataObjOpen("objPath={0}", *fd);
                *len = {1};
                msiDataObjRead(*fd, *len, *buf);   # test of read from  data object >= 2GB in size, irods/irods#5709
                writeLine("stdout", *buf);
            ''')
        }
        rule_string = rule_map[plugin_name]
        f = None
        try:
            N = 2100
            read_len = 2048
            s_1M = b'_'*1024*1024
            with open('file_5709.dat','wb') as f:
                for _ in range(N): f.write(s_1M)
                f.flush()
                self.admin.assert_icommand(['icd'])
                self.admin.assert_icommand(['iput',f.name])
                rule_arg = rule_string.format(self.admin.home_collection + "/" + f.name, read_len)
                self.admin.assert_icommand( ['irule', rule_arg, 'null' , 'ruleExecOut'],
                                             'STDOUT_SINGLELINE', "^"+str(read_len)+"$", use_regex = True)
        finally:
            if f and f.name: os.unlink(f.name)
            self.admin.assert_icommand( ['irm','-f', f.name] )

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiDataObjRead_does_not_generate_a_stacktrace_on_bad_input_arguments__issue_4550(self):
        data_object = 'foo'
        self.admin.assert_icommand(['istream', 'write', data_object], input='foo')

        # Capture the current size of the log file. This will be used as the starting
        # point for searching the log file for a particular string.
        log_offset = lib.get_file_size_by_path(paths.server_log_path())

        rule_text = '''
            msiDataObjOpen("{0}", *fd);
            msiDataObjRead(*fd, *len, *bytes_read);
            msiDataObjClose(*fd, *ec);
        '''.format(os.path.join(self.admin.session_collection, data_object))

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDERR', ['-323000 USER_PARAM_TYPE_ERR'])

        # Show that the log file does not contain a stacktrace.
        for msg in ['Dumping stack trace', '<0>     Offset:']:
            lib.delayAssert(lambda: lib.log_message_occurrences_equals_count(msg=msg, count=0, start_index=log_offset))

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_msiGetStderrInExecCmdOut_does_not_segfault_when_using_failed_out_parameter_as_input__issue_5791(self):
        # The following rule causes PLUGIN_ERROR_MISSING_SHARED_OBJECT due to "ipc_RE_HOST" being
        # undefined. This is okay now that the pointers in msiGetStderrInExecCmdOut() are checked
        # before being dereferenced.
        rule_text = '''
            *ec = errorcode(msiExecCmd('non_existent_file.py', *arg_str, ipc_RE_HOST, "null", "null", *out));
            msiGetStderrInExecCmdOut(*out, *output_string);
        '''
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'])


    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only implemented for NREP.')
    def test_msiDataObjChksum_with_admin_keyword__issue_6118(self):
        pep_map = {
            'irods_rule_engine_plugin-irods_rule_language': textwrap.dedent('''
                pep_api_data_obj_repl_post(*INSTANCE_NAME, *COMM, *DATAOBJINP, *TRANSSTAT) {
                    msiDataObjChksum(*DATAOBJINP.obj_path, "irodsAdmin=++++ChksumAll=", *checksum);
                }
            ''')
        }

        resource = 'test_msiDataObjChksum_with_admin_keyword_resc'

        filename = 'test_msiDataObjChksum_with_admin_keyword__issue_6118'
        logical_path = os.path.join(self.user0.session_collection, filename)
        contents = 'please let this be a normal field trip'

        config = IrodsConfig()
        with lib.file_backed_up(config.server_config_path):
            core_re_path = os.path.join(config.core_re_directory, 'core.re')

            with lib.file_backed_up(core_re_path):
                # Inject a PEP to perform a checksum on the data object after replication.
                with open(core_re_path, 'a') as core_re:
                    core_re.write(pep_map[plugin_name])

                try:
                    # Create a resource to which we can replicate to trigger the policy.
                    lib.create_ufs_resource(resource, self.admin, hostname=test.settings.HOSTNAME_2)

                    # Create a data object which can be replicated.
                    self.user0.assert_icommand(['istream', 'write', logical_path], input=contents)

                    # Replicate as the rodsuser and confirm that an error occurs due to the presence of the irodsAdmin
                    # flag. The replication still created a new replica, so be sure to trim it before trying it again
                    # as the rodsadmin.
                    self.user0.assert_icommand(
                        ['irepl', '-R', resource, logical_path], 'STDERR', 'CAT_INSUFFICIENT_PRIVILEGE_LEVEL')
                    destination_checksum = lib.get_replica_checksum(self.user0, os.path.basename(logical_path), 1)
                    source_checksum = lib.get_replica_checksum(self.user0, os.path.basename(logical_path), 0)
                    self.assertEqual(0, len(destination_checksum))
                    self.assertEqual(0, len(source_checksum))
                    self.user0.assert_icommand(
                        ['itrim', '-N1', '-S', resource, logical_path], 'STDOUT', 'Number of files trimmed = 1.')

                    # Replicate the data object and ensure that a checksum was calculated.
                    self.admin.assert_icommand(['irepl', '-M', '-R', resource, logical_path])
                    destination_checksum = lib.get_replica_checksum(self.user0, os.path.basename(logical_path), 1)
                    source_checksum = lib.get_replica_checksum(self.user0, os.path.basename(logical_path), 0)
                    self.assertNotEqual(0, len(destination_checksum))
                    self.assertNotEqual(0, len(source_checksum))

                    # Ensure that the checksum for the source and the destination are equal.
                    self.assertEqual(source_checksum, destination_checksum)

                finally:
                    self.user0.assert_icommand(['ils', '-AL', os.path.dirname(logical_path)], 'STDOUT') # debugging

                    self.user0.run_icommand(['irm', '-f', logical_path])

                    lib.remove_resource(resource, self.admin)


class Test_msiDataObjRepl_checksum_keywords(session.make_sessions_mixin([('otherrods', 'rods')], [('alice', 'apass')]), unittest.TestCase):
    global plugin_name
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_msiDataObjRepl_checksum_keywords, self).setUp()

        self.admin = self.admin_sessions[0]
        self.user0 = self.user_sessions[0]
        self.test_user = self.admin if 'python' in plugin_name else self.user0

        self.filename = 'Test_msiDataObjRepl_checksum_keywords'
        self.logical_path = os.path.join(self.test_user.session_collection, self.filename)

        self.target_resc = 'msiDataObjRepl_test_resource'
        lib.create_ufs_resource(self.target_resc, self.admin, hostname=test.settings.HOSTNAME_2)


    def tearDown(self):
        self.test_user.assert_icommand(['irm', '-f', self.logical_path])
        lib.remove_resource(self.target_resc, self.admin)

        super(Test_msiDataObjRepl_checksum_keywords, self).tearDown()


    def get_rule_text(self, _keywords=None):
        keywords = '++++'.join(['destRescName={}'.format(self.target_resc)] +
                               (_keywords or list()))

        # TODO: add to rule texts or come up with some better way to do this?
        if plugin_name == 'irods_rule_engine_plugin-python':
            return '''def main(rule_args, callback, rei):
    return int(callback.msiDataObjRepl('{}', '{}', 0)['code'])

OUTPUT ruleExecOut
'''.format(self.logical_path, keywords)

        else:
            return '''
test_msiDataObjRepl_checksum_keywords {{
    *logical_path = "{}";
    msiDataObjRepl(*logical_path, "{}", *status);
}}
OUTPUT ruleExecOut
'''.format(self.logical_path, keywords)


    def do_checksum_keyword_test(self, checksum_source=False, expected_status=0, keywords=None):
        rep_instance = '-'.join([plugin_name, 'instance'])
        rule_file = os.path.join(self.test_user.local_session_dir, 'test_msiDataObjRepl_checksum_keywords.r')
        rule_text = self.get_rule_text(keywords)
        with open(rule_file, 'wt') as f:
            print(rule_text, file=f, end='')

        try:
            if checksum_source:
                self.test_user.assert_icommand(['iput', '-K', rule_file, self.logical_path])
                self.assertNotEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
            else:
                self.test_user.assert_icommand(['iput', rule_file, self.logical_path])
                self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))

            return self.test_user.run_icommand(['irule', '-r', rep_instance, '-F', rule_file]) # out, err, rc
        finally:
            os.unlink(rule_file)


    def test_chksum_false_forceChksum_none_verifyChksum_none__issue_5927(self):
        out, err, ec = self.do_checksum_keyword_test(checksum_source=False)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 1)))


    def test_chksum_true_forceChksum_none_verifyChksum_none__issue_5927(self):
        out, err, ec = self.do_checksum_keyword_test(checksum_source=True)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        source_checksum = lib.get_replica_checksum(self.test_user, self.filename, 0)
        self.assertNotEqual(0, len(source_checksum))
        self.assertEqual(source_checksum, lib.get_replica_checksum(self.test_user, self.filename, 1))


    def test_chksum_false_forceChksum_none_verifyChksum_0__issue_5927(self):
        keywords = ['verifyChksum=0']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=False, keywords=keywords)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 1)))


    def test_chksum_true_forceChksum_none_verifyChksum_0__issue_5927(self):
        keywords = ['verifyChksum=0']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=True, keywords=keywords)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        source_checksum = lib.get_replica_checksum(self.test_user, self.filename, 0)
        self.assertNotEqual(0, len(source_checksum))
        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 1)))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_false_forceChksum_none_verifyChksum_1__issue_5927(self):
        keywords = ['verifyChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=False, keywords=keywords)

        self.assertIn('error when executing microservice', out)
        self.assertIn('CAT_NO_CHECKSUM_FOR_REPLICA', err)
        self.assertNotEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


    def test_chksum_true_forceChksum_none_verifyChksum_1__issue_5927(self):

        keywords = ['verifyChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=True, keywords=keywords)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        source_checksum = lib.get_replica_checksum(self.test_user, self.filename, 0)
        self.assertNotEqual(0, len(source_checksum))
        self.assertEqual(source_checksum, lib.get_replica_checksum(self.test_user, self.filename, 1))


    def test_chksum_false_forceChksum_1_verifyChksum_none__issue_5927(self):
        keywords = ['forceChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=False, keywords=keywords)

        self.assertEqual(0, len(out))
        self.assertEqual(0, len(err))
        self.assertEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertNotEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 1)))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_true_forceChksum_1_verifyChksum_none__issue_5927(self):
        keywords = ['forceChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=True, keywords=keywords)

        self.assertIn('error when executing microservice', out)
        self.assertIn('SYS_NOT_ALLOWED', err)
        self.assertNotEqual(0, ec)

        self.assertNotEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_false_forceChksum_1_verifyChksum_0__issue_5927(self):
        keywords = ['forceChksum=', 'verifyChksum=0']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=False, keywords=keywords)

        self.test_user.assert_icommand(['ils', '-l', self.logical_path], 'STDOUT', self.filename) # debug

        self.assertIn('error when executing microservice', out)
        self.assertIn('USER_INCOMPATIBLE_PARAMS', err)
        self.assertNotEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_true_forceChksum_1_verifyChksum_0__issue_5927(self):
        keywords = ['forceChksum=', 'verifyChksum=0']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=True, keywords=keywords)

        self.assertIn('error when executing microservice', out)
        self.assertIn('USER_INCOMPATIBLE_PARAMS', err)
        self.assertNotEqual(0, ec)

        self.assertNotEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_false_forceChksum_1_verifyChksum_1__issue_5927(self):
        keywords = ['forceChksum=', 'verifyChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=False, keywords=keywords)

        self.assertIn('error when executing microservice', out)
        self.assertIn('USER_INCOMPATIBLE_PARAMS', err)
        self.assertNotEqual(0, ec)

        self.assertEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'checksum related errors in iRODS 4.3 under python RE plugin')
    def test_chksum_true_forceChksum_1_verifyChksum_1__issue_5927(self):
        keywords = ['forceChksum=', 'verifyChksum=']

        out, err, ec = self.do_checksum_keyword_test(checksum_source=True, keywords=keywords)

        self.assertIn('error when executing microservice', out)
        self.assertIn('USER_INCOMPATIBLE_PARAMS', err)
        self.assertNotEqual(0, ec)

        self.assertNotEqual(0, len(lib.get_replica_checksum(self.test_user, self.filename, 0)))
        self.assertFalse(lib.replica_exists(self.test_user, self.logical_path, 1))


class Test_JSON_microservices(session.make_sessions_mixin([], [('alice', 'apass')]), unittest.TestCase):

    global plugin_name
    plugin_name = IrodsConfig().default_rule_engine_plugin

    def setUp(self):
        super(Test_JSON_microservices, self).setUp()

        self.user = self.user_sessions[0]

    def tearDown(self):
        super(Test_JSON_microservices, self).tearDown()

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_json_microservices_detect_invalid_handles__issue_6968(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'

        msi_calls = [
            "msi_json_compare('a', '', 'b', '', *ignored);",
            "msi_json_contains('a', '', *ignored);",
            "msi_json_dump('a', '', *ignored);",
            "msi_json_names('a', '', *ignored);",
            "msi_json_size('a', '', *ignored);",
            "msi_json_type('a', '', *ignored);",
            "msi_json_value('a', '', *ignored);"
        ]

        for rule_text in msi_calls:
            self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDERR', ['INVALID_HANDLE'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_json_microservices_detect_invalid_json_pointers__issue_6968(self):
        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'

        msi_calls = [
            "msi_json_compare(*h, '/', *h, '', *ignored);",
            "msi_json_compare(*h, '', *h, '/', *ignored);",
            "msi_json_dump(*h, '/', *ignored);",
            "msi_json_names(*h, '/', *ignored);",
            "msi_json_size(*h, '/', *ignored);",
            "msi_json_type(*h, '/', *ignored);",
            "msi_json_value(*h, '/', *ignored);"
        ]

        for msi_call in msi_calls:
            rule_text = textwrap.dedent('''
                *json_string = '{{"x": 100}}';
                msiStrlen(*json_string, *size);

                msi_json_parse(*json_string, int(*size), *h);
                {0}
            '''.format(msi_call))

            self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDERR', ['DOES_NOT_EXIST'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_compare__issue_6968(self):
        json_object = json.dumps({
            'p1': 'alice',
            'p2': 'bob',
            'data': {'values': [1, 2, 3]}
        })

        def do_test(json_ptr_1, json_ptr_2, cmp_result):
            rule_text = textwrap.dedent('''
                msiStrlen('{0}', *size);

                msi_json_parse('{0}', int(*size), *h);
                msi_json_compare(*h, '{1}', *h, '{2}', *cmp_result);
                msi_json_free(*h);

                writeLine('stdout', 'cmp_result=[*cmp_result]');
            '''.format(json_object, json_ptr_1, json_ptr_2))

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', ['cmp_result=[{0}]'.format(cmp_result)])

        do_test(   '',    '', 'equivalent')
        do_test('/p1', '/p2', 'less')
        do_test('/p2', '/p1', 'greater')
        #do_test(   '', '/p1', 'unordered') # TODO Requires the ability to generate NaN.

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_contains__issue_6968(self):
        rule_text = textwrap.dedent('''
            *json = '{"name": "john"}';
            msiStrlen(*json, *size);

            msi_json_parse(*json, int(*size), *h);
            msi_json_contains(*h, '/name', *result_1);
            msi_json_contains(*h, '/nope', *result_2);
            msi_json_free(*h);

            writeLine('stdout', 'result_1=[*result_1]');
            writeLine('stdout', 'result_2=[*result_2]');
        ''')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        expected_output = ['result_1=[1]', 'result_2=[0]']
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', expected_output)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_dump__issue_6968(self):
        json_string = json.dumps({
            'fname': 'john',
            'lname': 'doe'
        }, separators=(',', ':'))

        rule_text = textwrap.dedent('''
            *json = '{0}';
            msiStrlen(*json, *size);

            msi_json_parse(*json, int(*size), *h);
            msi_json_dump(*h, '', *out);
            msi_json_free(*h);

            writeLine('stdout', 'json_string=[*out]');
        '''.format(json_string))

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        expected_output = ['json_string=[{0}]'.format(json_string)]
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', expected_output)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_handles__issue_6968(self):
        json_string = json.dumps({
            'fname': 'john',
            'lname': 'doe'
        })

        rule_text = textwrap.dedent('''
            *json = '{0}';
            msiStrlen(*json, *size);

            msi_json_parse(*json, int(*size), *h1);
            msi_json_parse(*json, int(*size), *h2);
            msi_json_parse(*json, int(*size), *h3);
            msi_json_handles(*handles);
            msi_json_free(*h1);
            msi_json_free(*h2);
            msi_json_free(*h3);

            *count = 0;
            foreach (*handles) {{
                *count = *count + 1;
            }}

            writeLine('stdout', 'count=[*count]');
        '''.format(json_string))

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', ['count=[3]'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_names__issue_6968(self):
        json_object = json.dumps({
            'p1': 'alice',
            'p2': 'bob',
            'data': {'values': [1, 2, 3]}
        })

        rule_text = textwrap.dedent('''
            msiStrlen('{0}', *size);

            msi_json_parse('{0}', int(*size), *h);
            msi_json_names(*h, '', *root_names);
            msi_json_names(*h, '/data', *data_names);
            msi_json_free(*h);

            foreach (*n in *root_names) {{
                writeLine('stdout', 'name=[*n]');
            }}

            foreach (*n in *data_names) {{
                writeLine('stdout', 'name=[*n]');
            }}
        '''.format(json_object))

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        expected_output = ['name=[p1]', 'name=[p2]', 'name=[data]', 'name=[values]']
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', expected_output)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_parse_with_list_of_integers__issue_6968(self):
        # In the rule text, "*bytes" includes four trailing identical bytes. This creates a situation
        # where the last four bytes need to be ignored in order for msi_json_parse() to succeed. We
        # show that the "buffer_size" parameter of msi_json_parse() can be used to achieve this.
        rule_text = textwrap.dedent('''
            # characters:   {   "    x   "   :   1    }   "   "   "   "
            *bytes = list(123, 34, 120, 34, 58, 49, 125, 34, 34, 34, 34);

            msi_json_parse(*bytes, size(*bytes) - 4, *h);
            msi_json_size(*h, '', *size);
            msi_json_free(*h);

            writeLine('stdout', 'size=[*size]');
        ''')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', ['size=[1]'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_parse_with_string__issue_6968(self):
        rule_text = textwrap.dedent('''
            *json_string = '{"x": 1}';
            msiStrlen(*json_string, *size);

            msi_json_parse(*json_string, int(*size), *h);
            msi_json_size(*h, '', *size);
            msi_json_free(*h);

            writeLine('stdout', 'size=[*size]');
        ''')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', ['size=[1]'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_size__issue_6968(self):
        rule_text = textwrap.dedent('''
            *json_string = '{"x": 1, "y": [1, 2, 3, 4]}';
            msiStrlen(*json_string, *size);

            msi_json_parse(*json_string, int(*size), *h);
            msi_json_size(*h, '/y', *size);
            msi_json_free(*h);

            writeLine('stdout', 'size=[*size]');
        ''')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', ['size=[4]'])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_type__issue_6968(self):
        json_string = json.dumps({
            'string': 'john doe',
            'integer': 100,
            'array': [1, 2, 3],
            'object': {'x': 3, 'y': 4},
            'null': None,
            'boolean': True
        })

        def do_test(json_ptr, expected_type_string):
            rule_text = textwrap.dedent('''
                *json = '{0}';
                msiStrlen(*json, *size);

                msi_json_parse(*json, int(*size), *h);
                msi_json_type(*h, '{1}', *result);
                msi_json_free(*h);

                writeLine('stdout', 'type=[*result]');
            '''.format(json_string, json_ptr))

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            expected_output = ['type=[{0}]'.format(expected_type_string)]
            self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', expected_output)

        do_test(        '',  'object')
        do_test( '/string',  'string')
        do_test('/integer',  'number')
        do_test(  '/array',   'array')
        do_test('/array/0',  'number')
        do_test( '/object',  'object')
        do_test(   '/null',    'null')
        do_test('/boolean', 'boolean')

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'Only applicable to the NREP')
    def test_msi_json_value__issue_6968(self):
        json_object = json.dumps({
            'p1': 'alice',
            'p2': 'bob',
            'data': {'values': [1, 2, 3]},
            'other': None,
            'bool1': True,
            'bool2': False
        })

        def do_test(json_ptr, expected_value):
            rule_text = textwrap.dedent('''
                msiStrlen('{0}', *size);

                msi_json_parse('{0}', int(*size), *h);
                msi_json_value(*h, '{1}', *v);
                msi_json_free(*h);

                writeLine('stdout', 'value=[*v]');
            '''.format(json_object, json_ptr))

            rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
            expected_output = ['value=[{0}]'.format(expected_value)]
            self.user.assert_icommand(['irule', '-r', rep_name, rule_text, 'null', 'ruleExecOut'], 'STDOUT', expected_output)

        do_test(              '', 'object')
        do_test(           '/p1',  'alice')
        do_test(           '/p2',    'bob')
        do_test(         '/data', 'object')
        do_test(  '/data/values',  'array')
        do_test('/data/values/0',      '1')
        do_test(        '/other',   'null')
        do_test(        '/bool1',   'true')
        do_test(        '/bool2',  'false')
