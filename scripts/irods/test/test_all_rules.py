from __future__ import print_function
import os
import socket
import shutil
import sys
import getpass

if sys.version_info >= (2, 7):
    import unittest
else:
    import unittest2 as unittest

from .. import lib
from .. import test
from ..configuration import IrodsConfig
from ..controller import IrodsController
from . import metaclass_unittest_test_case_generator
from . import resource_suite
from . import session
from .rule_texts_for_tests import rule_texts


class Test_AllRules(resource_suite.ResourceBase, unittest.TestCase):
    __metaclass__ = metaclass_unittest_test_case_generator.MetaclassUnittestTestCaseGenerator
    class_name = 'Test_AllRules'

    global plugin_name
    plugin_name = IrodsConfig().default_rule_engine_plugin

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

        # setup for test_msiCheckAccess_3309
        self.rods_session.assert_icommand('icd')
        lib.make_file('test_file_3309', 1)
        self.rods_session.assert_icommand('iput test_file_3309')
        self.rods_session.assert_icommand('imkdir test_collection_3309')
        self.rods_session.assert_icommand('iadmin mkgroup group1_3309')  # group rods does not belong to 
        self.rods_session.assert_icommand('iadmin mkgroup group2_3309')  # group rods belongs to 
        self.rods_session.assert_icommand('iadmin atg group2_3309 rods')  

    def tearDown(self):
        self.rods_session.assert_icommand('icd')  # for home directory assumption
        self.rods_session.assert_icommand(['ichmod', '-r', 'own', self.rods_session.username, '.'])
        self.rods_session.run_icommand(['imcoll', '-U', self.rods_session.home_collection + '/test/phypathreg'])
        self.rods_session.run_icommand('irm -rf test ruletest forphymv sub1 sub2 sub3 bagit rules bagit.tar /' +
                                       self.rods_session.zone_name + '/bundle/home/' + self.rods_session.username)
        self.rods_session.assert_icommand('iadmin rmresc testallrulesResc')
        self.rods_session.assert_icommand('iadmin rmuser devtestuser')
        self.rods_session.assert_icommand('iqdel -a')  # remove all/any queued rules

        # cleanup mods in iRODS config dir
        lib.execute_command('mv -f {0}/core.re.bckp {0}/core.re'.format(self.conf_dir))
        lib.execute_command('rm -f {0}/*.test.re'.format(self.conf_dir))

        # cleanup for test_msiCheckAccess_3309
        self.rods_session.assert_icommand('irm -f test_file_3309')
        self.rods_session.assert_icommand('irm -rf test_collection_3309')
        self.rods_session.assert_icommand('iadmin rmgroup group1_3309')
        self.rods_session.assert_icommand('iadmin rmgroup group2_3309')

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

            # skip rules that handle .irb files
            names_to_skip = [
                "rulemsiAdmAppendToTopOfCoreIRB",
                "rulemsiAdmChangeCoreIRB",
                "rulemsiGetRulesFromDBIntoStruct",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- RE")
                    return False

            # skip rules that fail by design
            names_to_skip = [
                "GoodFailure"
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- failbydesign")
                    return False

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
                "rulemsiRegisterData",
                "rulemsiRenameCollection",
                "rulemsiRenameLocalZone",
                "rulemsiRollback",
                "rulemsiSetBulkPutPostProcPolicy",
                "rulemsiSetDataObjAvoidResc",
                "rulemsiSetDataObjPreferredResc",
                "rulemsiSetDataTypeFromExt",
                "rulemsiSetDefaultResc",
                "rulemsiSetGraftPathScheme",
                "rulemsiSetMultiReplPerResc",
                "rulemsiSetNoDirectRescInp",
                "rulemsiSetNumThreads",
                "rulemsiSetPublicUserOpr",
                "rulemsiSetRandomScheme",
                "rulemsiSetRescQuotaPolicy",
                "rulemsiSetRescSortScheme",
                "rulemsiSetReServerNumProc",
                "rulemsiSetResource",
                "rulemsiSortDataObj",
                "rulemsiStageDataObj",
                "rulemsiSysChksumDataObj",
                "rulemsiSysMetaModify",
                "rulemsiSysReplDataObj",
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

            # ERA
            names_to_skip = [
                "rulemsiFlagInfectedObjs",
                "rulemsiGetAuditTrailInfoByActionID",
                "rulemsiGetAuditTrailInfoByKeywords",
                "rulemsiGetAuditTrailInfoByObjectID",
                "rulemsiGetAuditTrailInfoByTimeStamp",
                "rulemsiGetAuditTrailInfoByUserID",
                "rulemsiMergeDataCopies",
                "rulemsiGetCollectionPSmeta-null"  # marked for removal - iquest now handles this natively
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- ERA")
                    return False

            # FTP
            names_to_skip = [
                "rulemsiFtpGet",
                "rulemsiTwitterPost",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- FTP")
                    return False

            # webservices
            names_to_skip = [
                "rulemsiConvertCurrency",
                "rulemsiGetQuote",
                "rulemsiIp2location",
                "rulemsiObjByName",
                "rulemsiSdssImgCutout_GetJpeg",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- webservices")
                    return False

            # XML
            names_to_skip = [
                "rulemsiLoadMetadataFromXml",
                "rulemsiXmlDocSchemaValidate",
                "rulemsiXsltApply",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- XML")
                    return False

            # transition to core microservices only
            names_to_skip = [
                "rulemsiAddKeyVal.r",
                "rulemsiApplyDCMetadataTemplate.r",
                "rulemsiAssociateKeyValuePairsToObj.r",
                "rulemsiCollectionSpider.r",
                "rulemsiCopyAVUMetadata.r",
                "rulemsiExportRecursiveCollMeta.r",
                "rulemsiFlagDataObjwithAVU.r",
                "rulemsiGetCollectionACL.r",
                "rulemsiGetCollectionContentsReport.r",
                "rulemsiGetCollectionPSmeta.r",
                "rulemsiGetCollectionSize.r",
                "rulemsiGetDataObjACL.r",
                "rulemsiGetDataObjAIP.r",
                "rulemsiGetDataObjAVUs.r",
                "rulemsiGetDataObjPSmeta.r",
                "rulemsiGetObjectPath.r",
                "rulemsiGetUserACL.r",
                "rulemsiGetUserInfo.r",
                "rulemsiGuessDataType.r",
                "rulemsiIsColl.r",
                "rulemsiIsData.r",
                "rulemsiLoadACLFromDataObj.r",
                "rulemsiLoadMetadataFromDataObj.r",
                "rulemsiLoadUserModsFromDataObj.r",
                "rulemsiPropertiesAdd.r",
                "rulemsiPropertiesClear.r",
                "rulemsiPropertiesClone.r",
                "rulemsiPropertiesExists.r",
                "rulemsiPropertiesFromString.r",
                "rulemsiPropertiesGet.r",
                "rulemsiPropertiesNew.r",
                "rulemsiPropertiesRemove.r",
                "rulemsiPropertiesSet.r",
                "rulemsiRecursiveCollCopy.r",
                "rulemsiRemoveKeyValuePairsFromObj.r",
                "rulemsiSetDataType.r",
                "rulemsiString2KeyValPair.r",
                "rulemsiStripAVUs.r",
                "rulemsiStructFileBundle.r",
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
            if "ruleintegrity" in rulefile:
                # print("skipping " + rulefile + " ----- integrityChecks")
                return False
            if "z3950" in rulefile:
                # print("skipping " + rulefile + " ----- z3950")
                return False
            if "rulemsiImage" in rulefile:
                # print("skipping " + rulefile + " ----- image")
                return False
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
                    self.rods_session.assert_icommand("irule -vF " + os.path.join(rulesdir, rulefile),
                                                      'STDOUT_SINGLELINE', "completed successfully")
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
            self.rods_session.run_icommand(['iput', '-f', src_file, test_coll])

            # sync test
            self.rods_session.assert_icommand("irule -F " + os.path.join(rulesdir, rulefile), 'STDOUT_SINGLELINE', "status = 99999992")

        # cleanup
        self.rods_session.run_icommand(['irm', '-rf', test_coll])
        os.remove(src_file)

    def test_rulemsiPhyBundleColl(self):
        rulefile = 'rulemsiPhyBundleColl.r'

        # rule test
        self.rods_session.assert_icommand("irule -F " + os.path.join(rulesdir, rulefile), 'STDOUT_SINGLELINE',
                                          "Create tar file of collection /tempZone/home/rods/test on resource testallrulesResc")

        # look for the bundle
        bundle_path = '/tempZone/bundle/home/' + self.rods_session.username
        out, _, _ = self.rods_session.run_icommand(['ils', '-L', bundle_path])

        # last token in stdout should be the bundle file's full physical path
        bundlefile = out.split()[-1]

        # check on the bundle file's name
        assert bundlefile.find('test.') >= 0

        # check physical path on resource
        assert os.path.isfile(bundlefile)

        # now try as a normal user (expect err msg)
        self.user0.assert_icommand("irule -r irods_rule_engine_plugin-irods_rule_language-instance -F " + os.path.join(rulesdir, rulefile), 'STDERR_SINGLELINE', "SYS_NO_API_PRIV")

        # cleanup
        self.rods_session.run_icommand(['irm', '-rf', bundle_path])

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

        data_obj_rule_file="test_msiCheckAccess_data_obj_3309.r"
        rule_string= '''
testMsiCheckAccess {
     msiCheckAccess("/tempZone/home/rods/test_file_3309", "read object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_file_3309", "modify object", *result);
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
     msiCheckAccess("/tempZone/home/rods/test_collection_3309", "read object", *result);
     writeLine("stdout", "result=*result");
     msiCheckAccess("/tempZone/home/rods/test_collection_3309", "modify object", *result);
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

        self.rods_session.assert_icommand('ichmod read rods test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod write rods test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod own rods test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
            use_regex=True)


        # remove user permission and check permissions by group 

        self.rods_session.assert_icommand('ichmod null rods test_file_3309')

        self.rods_session.assert_icommand('ichmod read group2_3309 test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod write group2_3309 test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod own group2_3309 test_file_3309')
        self.rods_session.assert_icommand("irule -F " + data_obj_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
            use_regex=True)


        # --------- check permissions on collection -----------

        # check permissions by user 

        self.rods_session.assert_icommand('ichmod null rods test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=0$', 'result=0$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod read rods test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod write rods test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod own rods test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
            use_regex=True)


        # remove user permission and check permissions by group 

        self.rods_session.assert_icommand('ichmod null rods test_collection_3309')

        self.rods_session.assert_icommand('ichmod read group2_3309 test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=0$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod write group2_3309 test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=0$'],
            use_regex=True)

        self.rods_session.assert_icommand('ichmod own group2_3309 test_collection_3309')
        self.rods_session.assert_icommand("irule -F " + collection_rule_file, 'STDOUT_MULTILINE', ['result=1$', 'result=1$', 'result=1$'],
            use_regex=True)

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    @unittest.skip('Generation of large file causes I/O thrashing... skip for now')
    def test_msiTarFileExtract_big_file__issue_4118(self):
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
                rule_string = rule_texts[plugin_name][self.class_name][test_name].format(**locals())
                with open(rule_file, 'w') as f:
                    f.write(rule_string)
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

    @unittest.skipIf(test.settings.RUN_IN_TOPOLOGY, "Skip for Topology Testing")
    def test_msiRenameCollection_does_rename_collections__issue_4597(self):
        src = os.path.join(self.admin.session_collection, 'col.a')
        dst = os.path.join(self.admin.session_collection, 'col.a.renamed')
        self.admin.assert_icommand(['imkdir', src])

        rule_file = os.path.join(self.admin.local_session_dir, 'test_rule_file.r')
        rule_string = '''
main {{
    *src = "{0}";
    *dst = "{1}";
    msiRenameCollection(*src, *dst);
}}
INPUT null
OUTPUT ruleExecOut'''

        with open(rule_file, 'wt') as f:
            print(rule_string.format(src, dst), file=f, end='')

        rep_name = 'irods_rule_engine_plugin-irods_rule_language-instance'
        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file])
        self.admin.assert_icommand(['ils', '-l', dst], 'STDOUT', [dst])

        with open(rule_file, 'wt') as f:
            print(rule_string.format(dst, os.path.basename(src)), file=f, end='')

        self.admin.assert_icommand(['irule', '-r', rep_name, '-F', rule_file], 'STDERR', ['-358000 OBJ_PATH_DOES_NOT_EXIST'])
        self.admin.assert_icommand(['ils', '-l', dst], 'STDOUT', [dst])

