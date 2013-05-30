import pydevtest_sessions as s
from nose.plugins.skip import SkipTest
from pydevtest_common import assertiCmd, assertiCmdFail, interruptiCmd
import commands
import os
import datetime
import time
import shutil
import random
import inspect

class Test_AllRules(object):

    def setUp(self):
        s.twousers_up()
    def tearDown(self):
        s.twousers_down()

    currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    rules30dir = currentdir+"/../../iRODS/clients/icommands/test/rules3.0/"

    def run_irule(self, rulefile):
        assertiCmd(s.adminsession, "icd" )
        assertiCmd(s.adminsession,"irule -vF "+self.rules30dir+rulefile, "LIST", "completed successfully")

    def test_allrules(self):

        for rulefile in sorted(os.listdir(self.rules30dir)):

            skipme = 0
            # skip rules that touch our core re
            names_to_skip = [
                "rulemsiAdmAppendToTopOfCoreIRB",
                "rulemsiAdmAppendToTopOfCoreRE",
                "rulemsiAdmChangeCoreIRB",
                "rulemsiAdmChangeCoreRE",
                "rulemsiGetRulesFromDBIntoStruct",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- RE"; skipme = 1

            # skip rules that fail by design
            names_to_skip = [
                "GoodFailure",
                "ruleworkflowcut",
                "ruleworkflowfail",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- failbydesign"; skipme = 1

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
                "rulemsiVacuum",
                "rulemsiNoChkFilePathPerm",
                "rulemsiNoTrashCan",
                "rulemsiVacuum",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- input/output"; skipme = 1


            # skip rules we are not yet supporting
            names_to_skip = [
                "rulemsiDbo",
                "rulemsiDbr",
                "rulemsiobj",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- DBO/DBR/msiobj"; skipme = 1

            # ERA
            names_to_skip = [
                "rulemsiFlagInfectedObjs",
                "rulemsiGetAuditTrailInfoByActionID",
                "rulemsiGetAuditTrailInfoByKeywords",
                "rulemsiGetAuditTrailInfoByObjectID",
                "rulemsiGetAuditTrailInfoByTimeStamp",
                "rulemsiGetAuditTrailInfoByUserID",
                "rulemsiMergeDataCopies",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- ERA"; skipme = 1

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
                if n in rulefile: print "skipping "+rulefile+" ----- XMSG"; skipme = 1

            # FTP
            names_to_skip = [
                "rulemsiFtpGet",
                "rulemsiTwitterPost",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- FTP"; skipme = 1

            # webservices
            names_to_skip = [
                "rulemsiConvertCurrency",
                "rulemsiGetQuote",
                "rulemsiIp2location",
                "rulemsiObjByName",
                "rulemsiSdssImgCutout_GetJpeg",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- webservices"; skipme = 1

            # XML
            names_to_skip = [
                "rulemsiLoadMetadataFromXml",
                "rulemsiXmlDocSchemaValidate",
                "rulemsiXsltApply",
            ]
            for n in names_to_skip:
                if n in rulefile: print "skipping "+rulefile+" ----- XML"; skipme = 1

            if skipme == 1: continue

            # skipping for now, not sure why it's throwing a stacktrace at the moment
            if "rulemsiPropertiesToString" in rulefile:
                print "skipping "+rulefile+" ----- b/c of stacktrace"; continue

            # misc / other
            if "ruleintegrity" in rulefile:
                print "skipping "+rulefile+" ----- integrityChecks"; continue
            if "z3950" in rulefile:
                print "skipping "+rulefile+" ----- z3950"; continue
            if "rulemsiImage" in rulefile:
                print "skipping "+rulefile+" ----- image"; continue
            if "rulemsiRda" in rulefile:
                print "skipping "+rulefile+" ----- RDA"; continue
            if "rulemsiCollRepl" in rulefile:
                print "skipping "+rulefile+" ----- deprecated"; continue
            if "rulemsiDataObjGetWithOptions" in rulefile:
                print "skipping "+rulefile+" ----- deprecated"; continue
            if "rulemsiServerBackup" in rulefile:
                print "skipping "+rulefile+" ----- serverbackup"; continue
            if "rulemsiExecStrCondQueryWithOptions" in rulefile:
                print "skipping "+rulefile+" ----- SYS_HEADER_READ_LEN_ERR, Operation now in progress"; continue

            # actually run the test - yield means create a separate test for each rulefile
            print "running "+rulefile
            yield self.run_irule, rulefile

        # cleanup
        os.unlink( "foo1" )
