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
from ..configuration import IrodsConfig
from . import metaclass_unittest_test_case_generator
from . import resource_suite
from . import session


class Test_AllRules(resource_suite.ResourceBase, unittest.TestCase):
    __metaclass__ = metaclass_unittest_test_case_generator.MetaclassUnittestTestCaseGenerator

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

            # XMSG
            names_to_skip = [
                "rulemsiCreateXmsgInp",
                "rulemsiRcvXmsg",
                "rulemsiSendXmsg",
                "rulemsiXmsgCreateStream",
                "rulemsiXmsgServerConnect",
                "rulemsiXmsgServerDisConnect",
                "rulereadXMsg",
                "rulewriteXMsg",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print("skipping " + rulefile + " ----- XMSG")
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
        self.user0.assert_icommand("irule -F " + os.path.join(rulesdir, rulefile), 'STDERR_SINGLELINE', "SYS_NO_API_PRIV")

        # cleanup
        self.rods_session.run_icommand(['irm', '-rf', bundle_path])

    @unittest.skipUnless(plugin_name == 'irods_rule_engine_plugin-irods_rule_language', 'only applicable for irods_rule_language REP')
    def test_str_2528(self):
        self.rods_session.assert_icommand('''irule "*a.a = 'A'; *a.b = 'B'; writeLine('stdout', str(*a))" null ruleExecOut''', 'STDOUT_SINGLELINE', "a=A++++b=B")

    @unittest.skipIf(plugin_name == 'irods_rule_engine_plugin-python', 'msiGetStderrInExecCmdOut not available in Python yet - RTS')
    def test_return_data_structure_non_null_2604(self):
        self.rods_session.assert_icommand(
            '''irule "*Err = errorcode(msiExecCmd('cmd', '', '', '', '', *Out)); msiGetStderrInExecCmdOut(*Out, *Stderr); writeLine('stdout', 'stderr: *Err*Stderr')" null ruleExecOut''', 'STDOUT_SINGLELINE', "stderr")
