import os
import socket
import shutil
import sys
import getpass

if (sys.version_info >= (2, 7)):
    import unittest
else:
    import unittest2 as unittest

import metaclass_unittest_test_case_generator
import pydevtest_sessions as s
from pydevtest_common import assertiCmd, assertiCmdFail, getiCmdOutput, get_irods_config_dir
from resource_suite import ResourceBase


class Test_AllRules(unittest.TestCase, ResourceBase):

    __metaclass__ = metaclass_unittest_test_case_generator.MetaclassUnittestTestCaseGenerator

    global rules30dir
    currentdir = os.path.dirname(os.path.realpath(__file__))
    rules30dir = currentdir + "/../../iRODS/clients/icommands/test/rules3.0/"
    conf_dir = get_irods_config_dir()

    my_test_resource = {
        "setup": [
        ],
        "teardown": [
        ],
    }

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()

        # testallrules setup
        global rules30dir
        hostname = socket.gethostname()
        hostuser = getpass.getuser()
        progname = __file__
        dir_w = rules30dir + ".."
        s.adminsession.runCmd('icd')  # to get into the home directory (for testallrules assumption)
        s.adminsession.runAdminCmd('iadmin', ["mkuser", "devtestuser", "rodsuser"])
        s.adminsession.runAdminCmd(
            'iadmin', ["mkresc", "testallrulesResc", "unix file system", hostname + ":/tmp/" + hostuser + "/pydevtest_testallrulesResc"])
        s.adminsession.runCmd('imkdir', ["sub1"])
        s.adminsession.runCmd('imkdir', ["sub3"])
        s.adminsession.runCmd('imkdir', ["forphymv"])
        s.adminsession.runCmd('imkdir', ["ruletest"])
        s.adminsession.runCmd('imkdir', ["test"])
        s.adminsession.runCmd('imkdir', ["test/phypathreg"])
        s.adminsession.runCmd('imkdir', ["ruletest/subforrmcoll"])
        s.adminsession.runCmd('iput', [progname, "test/foo1"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/dcmetadatatarget"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/mdcopysource"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/mdcopydest"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/foo1"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/foo2"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/foo3"])
        s.adminsession.runCmd('icp', ["test/foo1", "forphymv/phymvfile"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/objunlink1"])
        s.adminsession.runCmd('irm', ["sub1/objunlink1"])  # put it in the trash
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/objunlink2"])
        s.adminsession.runCmd('irepl', ["-R", "testallrulesResc", "sub1/objunlink2"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/freebuffer"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/automove"])
        s.adminsession.runCmd('icp', ["test/foo1", "test/versiontest.txt"])
        s.adminsession.runCmd('icp', ["test/foo1", "test/metadata-target.txt"])
        s.adminsession.runCmd('icp', ["test/foo1", "test/ERAtestfile.txt"])
        s.adminsession.runCmd('ichmod', ["read devtestuser", "test/ERAtestfile.txt"])
        s.adminsession.runCmd('imeta', ["add", "-d", "test/ERAtestfile.txt", "Fun", "99", "Balloons"])
        s.adminsession.runCmd('icp', ["test/foo1", "sub1/for_versioning.txt"])
        s.adminsession.runCmd('imkdir', ["sub1/SaveVersions"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/devtestuser-account-ACL.txt", "test"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/load-metadata.txt", "test"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/load-usermods.txt", "test"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/sample.email", "test"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/email.tag", "test"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/sample.email", "test/sample2.email"])
        s.adminsession.runCmd('iput', [dir_w + "/misc/email.tag", "test/email2.tag"])

        # setup for rulemsiAdmChangeCoreRE and the likes
        empty_core_file_name = 'empty.test.re'
        new_core_file_name = 'new.test.re'
        open(self.conf_dir + "/" + empty_core_file_name, 'w').close()                       # create empty file
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/core.re.bckp")           # back up core.re
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/" + new_core_file_name)   # copy core.re

    def tearDown(self):
        # testallrules teardown
        s.adminsession.runCmd('icd')  # for home directory assumption
        s.adminsession.runCmd('ichmod', ["-r", "own", "rods", "."])
        s.adminsession.runCmd('imcoll', ["-U", "/" + s.adminsession.getZoneName() + "/home/rods/test/phypathreg"])
        s.adminsession.runCmd('irm', ["-rf", "test", "ruletest", "forphymv", "sub1", "sub2", "sub3",
                                      "bagit", "rules", "bagit.tar", "/" + s.adminsession.getZoneName() + "/bundle/home/rods"])
        s.adminsession.runAdminCmd('iadmin', ["rmresc", "testallrulesResc"])
        s.adminsession.runAdminCmd('iadmin', ["rmuser", "devtestuser"])
        s.adminsession.runCmd('iqdel', ["-a"])  # remove all/any queued rules

        # cleanup mods in iRODS config dir
        os.system('mv -f %s/core.re.bckp %s/core.re' % (self.conf_dir, self.conf_dir))
        os.system('rm -f %s/*.test.re' % self.conf_dir)

        self.run_resource_teardown()
        s.twousers_down()

    def generate_tests_allrules():
        global rules30dir

        def filter_rulefiles(rulefile):

            # skip rules that handle .irb files
            names_to_skip = [
                "rulemsiAdmAppendToTopOfCoreIRB",
                "rulemsiAdmChangeCoreIRB",
                "rulemsiGetRulesFromDBIntoStruct",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- RE"
                    return False

            # skip rules that fail by design
            names_to_skip = [
                "GoodFailure"
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- failbydesign"
                    return False

            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- failbydesign"
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
                    # print "skipping " + rulefile + " ----- input/output"
                    return False

            # skip rules we are not yet supporting
            names_to_skip = [
                "rulemsiobj",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- msiobj"
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
                    # print "skipping " + rulefile + " ----- ERA"
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
                    # print "skipping " + rulefile + " ----- XMSG"
                    return False

            # FTP
            names_to_skip = [
                "rulemsiFtpGet",
                "rulemsiTwitterPost",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- FTP"
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
                    # print "skipping " + rulefile + " ----- webservices"
                    return False

            # XML
            names_to_skip = [
                "rulemsiLoadMetadataFromXml",
                "rulemsiXmlDocSchemaValidate",
                "rulemsiXsltApply",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- XML"
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
                    # print "skipping " + rulefile + " ----- transition to core"
                    return False

            # skipping rules requiring additional .re files in community code
            names_to_skip = [
                "rulemsiAdmAddAppRuleStruct.r",
                "rulemsiAdmClearAppRuleStruct.r",
                "rulemsiAdmInsertRulesFromStructIntoDB.r",
                "rulemsiAdmReadRulesFromFileIntoStruct.r",
                "rulemsiAdmRetrieveRulesFromDBIntoStruct.r",
                "rulemsiAdmWriteRulesFromStructIntoFile.r",
            ]
            for n in names_to_skip:
                if n in rulefile:
                    # print "skipping " + rulefile + " ----- community"
                    return False

            # skipping for now, not sure why it's throwing a stacktrace at the moment
            if "rulemsiPropertiesToString" in rulefile:
                # print "skipping " + rulefile + " ----- b/c of stacktrace"
                return False

            # misc / other
            if "ruleintegrity" in rulefile:
                # print "skipping " + rulefile + " ----- integrityChecks"
                return False
            if "z3950" in rulefile:
                # print "skipping " + rulefile + " ----- z3950"
                return False
            if "rulemsiImage" in rulefile:
                # print "skipping " + rulefile + " ----- image"
                return False
            if "rulemsiRda" in rulefile:
                # print "skipping " + rulefile + " ----- RDA"
                return False
            if "rulemsiCollRepl" in rulefile:
                # print "skipping " + rulefile + " ----- deprecated"
                return False
            if "rulemsiDataObjGetWithOptions" in rulefile:
                # print "skipping " + rulefile + " ----- deprecated"
                return False
            if "rulemsiDataObjReplWithOptions" in rulefile:
                # print "skipping " + rulefile + " ----- deprecated"
                return False
            if "rulemsiExecStrCondQueryWithOptions" in rulefile:
                # print "skipping " + rulefile + " ----- SYS_HEADER_READ_LEN_ERR, Operation now in progress"
                return False
            if "rulemsiTarFileExtract" in rulefile:
                # print "skipping " + rulefile + " ----- CAT_NO_ROWS_FOUND - failed in
                # call to getDataObjInfoIncSpecColl"
                return False
            if "rulemsiDataObjRsync" in rulefile:
                # print "skipping " + rulefile + " ----- tested separately"
                return False

            return True

        for rulefile in filter(filter_rulefiles, sorted(os.listdir(rules30dir))):
            def make_test(rulefile):
                def test(self):
                    global rules30dir
                    assertiCmd(s.adminsession, "icd")
                    assertiCmd(s.adminsession, "irule -vF " + rules30dir + rulefile,
                               "STDOUT", "completed successfully")
                return test

            yield 'test_' + rulefile.replace('.', '_'), make_test(rulefile)

    def test_rulemsiDataObjRsync(self):
        rulefile = 'rulemsiDataObjRsync.r'
        src_filename = 'source.txt'
        dest_filename = 'dest.txt'
        test_dir = '/tmp'
        test_coll = '/tempZone/home/rods/synctest'
        src_file = os.path.join(test_dir, src_filename)
        src_obj = test_coll + '/' + src_filename
        dest_obj = test_coll + '/' + dest_filename

        # create test collection
        s.adminsession.runCmd('imkdir', [test_coll])

        # create source test file
        f = open(src_file, 'a')
        f.write('blah\n')
        f.close()

        # upload source test file
        s.adminsession.runCmd('iput', [src_file, test_coll])

        # first rsync rule test
        assertiCmd(s.adminsession, "irule -F " + rules30dir + rulefile, "LIST", "status = 99999992")

        # modify the source and try again
        for i in range(1, 5):
            f = open(src_file, 'a')
            f.write('blah_' + str(i) + '\n')
            f.close()

            # force upload source
            s.adminsession.runCmd('iput', ['-f', src_file, test_coll])

            # sync test
            assertiCmd(s.adminsession, "irule -F " + rules30dir + rulefile, "LIST", "status = 99999992")

        # cleanup
        s.adminsession.runCmd('irm', ['-rf', test_coll])
        os.remove(src_file)

    def test_rulemsiPhyBundleColl(self):
        rulefile = 'rulemsiPhyBundleColl.r'

        # rule test
        assertiCmd(s.adminsession, "irule -F " + rules30dir + rulefile, "LIST",
                   "Create tar file of collection /tempZone/home/rods/test on resource testallrulesResc")

        # look for the bundle
        output = getiCmdOutput(s.adminsession, "ils -L /tempZone/bundle/home/rods")

        # last token in stdout should be the bundle file's full physical path
        bundlefile = output[0].split()[-1]

        # check on the bundle file's name
        assert bundlefile.find('test.') >= 0

        # check physical path on resource
        assert os.path.isfile(bundlefile)

        # now try as a normal user (expect err msg)
        assertiCmd(s.sessions[1], "irule -F " + rules30dir + rulefile, "ERROR", "SYS_NO_API_PRIV")

        # cleanup
        s.adminsession.runCmd('irm', ['-rf', '/tempZone/bundle/home/rods'])
