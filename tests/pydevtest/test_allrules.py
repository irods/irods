import pydevtest_sessions as s
from pydevtest_common import assertiCmd, assertiCmdFail, get_irods_config_dir
from resource_suite import ResourceBase
import os
import inspect
import socket
import shutil

class Test_AllRules(ResourceBase):

    global rules30dir
    currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    rules30dir = currentdir+"/../../iRODS/clients/icommands/test/rules3.0/"
    conf_dir = get_irods_config_dir()

    my_test_resource = {
        "setup"    : [
        ],
        "teardown" : [
        ],
    }

    def setUp(self):
        ResourceBase.__init__(self)
        s.twousers_up()
        self.run_resource_setup()
        
        # testallrules setup
        global rules30dir
        hostname = socket.gethostname()
        progname = __file__
        dir_w = rules30dir+".."
        s.adminsession.runCmd('icd') # to get into the home directory (for testallrules assumption)
        s.adminsession.runAdminCmd('iadmin',["mkuser","devtestuser","rodsuser"] )
        s.adminsession.runAdminCmd('iadmin',["mkresc","testallrulesResc","unix file system",hostname+":/tmp/pydevtest_testallrulesResc"] )
        s.adminsession.runCmd('imkdir', ["sub1"] )
        s.adminsession.runCmd('imkdir', ["sub3"] )
        s.adminsession.runCmd('imkdir', ["forphymv"] )
        s.adminsession.runCmd('imkdir', ["ruletest"] )
        s.adminsession.runCmd('imkdir', ["test"] )
        s.adminsession.runCmd('imkdir', ["test/phypathreg"] )
        s.adminsession.runCmd('imkdir', ["ruletest/subforrmcoll"] )
        s.adminsession.runCmd('iput', [progname,"test/foo1"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/dcmetadatatarget"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/mdcopysource"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/mdcopydest"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/foo1"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/foo2"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/foo3"] )
        s.adminsession.runCmd('icp', ["test/foo1","forphymv/phymvfile"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/objunlink1"] )
        s.adminsession.runCmd('irm', ["sub1/objunlink1"] ) # put it in the trash
        s.adminsession.runCmd('icp', ["test/foo1","sub1/objunlink2"] )
        s.adminsession.runCmd('irepl', ["-R","testallrulesResc","sub1/objunlink2"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/freebuffer"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/automove"] )
        s.adminsession.runCmd('icp', ["test/foo1","test/versiontest.txt"] )
        s.adminsession.runCmd('icp', ["test/foo1","test/metadata-target.txt"] )
        s.adminsession.runCmd('icp', ["test/foo1","test/ERAtestfile.txt"] )
        s.adminsession.runCmd('ichmod', ["read devtestuser","test/ERAtestfile.txt"] )
        s.adminsession.runCmd('imeta', ["add","-d","test/ERAtestfile.txt","Fun","99","Balloons"] )
        s.adminsession.runCmd('icp', ["test/foo1","sub1/for_versioning.txt"] )
        s.adminsession.runCmd('imkdir', ["sub1/SaveVersions"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/devtestuser-account-ACL.txt","test"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/load-metadata.txt","test"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/load-usermods.txt","test"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/sample.email","test"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/email.tag","test"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/sample.email","test/sample2.email"] )
        s.adminsession.runCmd('iput', [dir_w+"/misc/email.tag","test/email2.tag"] )
        
        # setup for rulemsiAdmChangeCoreRE and the likes
        empty_core_file_name = 'empty.test.re'
        new_core_file_name = 'new.test.re'
        with open(self.conf_dir + "/" + empty_core_file_name, 'w'): pass                    # create empty file
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/core.re.bckp" )           # back up core.re
        shutil.copy(self.conf_dir + "/core.re", self.conf_dir + "/" + new_core_file_name)   # copy core.re
        

    def tearDown(self):
        # testallrules teardown
        s.adminsession.runCmd('icd') # for home directory assumption
        s.adminsession.runCmd('ichmod',["-r","own","rods","."] )
        s.adminsession.runCmd('imcoll',["-U","/"+s.adminsession.getZoneName()+"/home/rods/test/phypathreg"] )
        s.adminsession.runCmd('irm',["-rf","test","ruletest","forphymv","sub1","sub2","sub3","bagit","rules","bagit.tar","/"+s.adminsession.getZoneName()+"/bundle/home/rods"] )
        s.adminsession.runAdminCmd('iadmin',["rmresc","testallrulesResc"] )
        s.adminsession.runAdminCmd('iadmin',["rmuser","devtestuser"] )
        s.adminsession.runCmd('iqdel',["-a"] ) # remove all/any queued rules
        
        # cleanup mods in iRODS config dir
        os.system('mv -f %s/core.re.bckp %s/core.re' % (self.conf_dir, self.conf_dir))
        os.system('rm -f %s/*.test.re' % self.conf_dir)
 
        self.run_resource_teardown()
        s.twousers_down()

 

    #def create_testallrules_test(self, rulefile):
        #def do_test_expected(self):
            #self.run_irule(rulefile)
        #return do_test_expected


    def run_irule(self, rulefile):
        global rules30dir
        assertiCmd(s.adminsession, "icd" )
        assertiCmd(s.adminsession,"irule -vF "+rules30dir+rulefile, "LIST", "completed successfully")

    def test_allrules(self):

        global rules30dir
        print rules30dir

        for rulefile in sorted(os.listdir(rules30dir)):

            skipme = 0
            # skip rules that touch our core re
            names_to_skip = [
                "rulemsiAdmAppendToTopOfCoreIRB",
                "rulemsiAdmAppendToTopOfCoreRE",
                "rulemsiAdmChangeCoreIRB",
                #"rulemsiAdmChangeCoreRE",
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

            # skip rules that are for workflows
            names_to_skip = [
                "ruleTestWSO.r",
                "ruleTestWSO1.r",
                "ruleTestWSO2.r",
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
                "rulemsiGetCollectionPSmeta-null" # marked for removal - iquest now handles this natively
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
                if n in rulefile: print "skipping "+rulefile+" ----- transition to core"; skipme = 1

            if skipme == 1: continue
            
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
                if n in rulefile: print "skipping "+rulefile+" ----- community"; skipme = 1

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
            if "rulemsiDataObjReplWithOptions" in rulefile:
                print "skipping "+rulefile+" ----- deprecated"; continue
            if "rulemsiServerBackup" in rulefile:
                print "skipping "+rulefile+" ----- serverbackup"; continue
            if "rulemsiExecStrCondQueryWithOptions" in rulefile:
                print "skipping "+rulefile+" ----- SYS_HEADER_READ_LEN_ERR, Operation now in progress"; continue
            if "rulemsiTarFileExtract" in rulefile:
                print "skipping "+rulefile+" ----- CAT_NO_ROWS_FOUND - failed in call to getDataObjInfoIncSpecColl"; continue
            if "rulemsiDataObjRsync" in rulefile:
                print "skipping "+rulefile+" ----- -130000 SYS_INVALID_INPUT_PARAM - no resource found for name [testResc]"; continue

            # actually run the test - yield means create a separate test for each rulefile
            print "-- running "+rulefile
            yield self.run_irule, rulefile

#            self.run_irule(rulefile)

