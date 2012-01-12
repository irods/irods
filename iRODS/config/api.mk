# defines the client api request and server api handling routines 

SVR_API_OBJS += $(svrApiObjDir)/rsGetMiscSvrInfo.o
LIB_API_OBJS += $(libApiObjDir)/rcGetMiscSvrInfo.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileCreate.o
LIB_API_OBJS += $(libApiObjDir)/rcFileCreate.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileOpen.o
LIB_API_OBJS += $(libApiObjDir)/rcFileOpen.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileWrite.o
LIB_API_OBJS += $(libApiObjDir)/rcFileWrite.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileClose.o
LIB_API_OBJS += $(libApiObjDir)/rcFileClose.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileLseek.o
LIB_API_OBJS += $(libApiObjDir)/rcFileLseek.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileRead.o
LIB_API_OBJS += $(libApiObjDir)/rcFileRead.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileUnlink.o
LIB_API_OBJS += $(libApiObjDir)/rcFileUnlink.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileMkdir.o
LIB_API_OBJS += $(libApiObjDir)/rcFileMkdir.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileChmod.o
LIB_API_OBJS += $(libApiObjDir)/rcFileChmod.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileRmdir.o
LIB_API_OBJS += $(libApiObjDir)/rcFileRmdir.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileStat.o
LIB_API_OBJS += $(libApiObjDir)/rcFileStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileFstat.o
LIB_API_OBJS += $(libApiObjDir)/rcFileFstat.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileFsync.o
LIB_API_OBJS += $(libApiObjDir)/rcFileFsync.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileStage.o
LIB_API_OBJS += $(libApiObjDir)/rcFileStage.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileGetFsFreeSpace.o
LIB_API_OBJS += $(libApiObjDir)/rcFileGetFsFreeSpace.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileOpendir.o
LIB_API_OBJS += $(libApiObjDir)/rcFileOpendir.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileClosedir.o
LIB_API_OBJS += $(libApiObjDir)/rcFileClosedir.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileReaddir.o
LIB_API_OBJS += $(libApiObjDir)/rcFileReaddir.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjCreate.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjCreate.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjOpen.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjOpen.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjRead.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjRead.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjWrite.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjWrite.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjClose.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjClose.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjPut.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjPut.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataPut.o
LIB_API_OBJS += $(libApiObjDir)/rcDataPut.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjGet.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjGet.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataGet.o
LIB_API_OBJS += $(libApiObjDir)/rcDataGet.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjRepl.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjRepl.o

SVR_API_OBJS += $(svrApiObjDir)/rsFilePut.o
LIB_API_OBJS += $(libApiObjDir)/rcFilePut.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileGet.o
LIB_API_OBJS += $(libApiObjDir)/rcFileGet.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataCopy.o
LIB_API_OBJS += $(libApiObjDir)/rcDataCopy.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjLseek.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjLseek.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjCopy.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjCopy.o

SVR_API_OBJS += $(svrApiObjDir)/rsSimpleQuery.o
LIB_API_OBJS += $(libApiObjDir)/rcSimpleQuery.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjUnlink.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjUnlink.o

SVR_API_OBJS += $(svrApiObjDir)/rsCollCreate.o
LIB_API_OBJS += $(libApiObjDir)/rcCollCreate.o

SVR_API_OBJS += $(svrApiObjDir)/rsGeneralAdmin.o
LIB_API_OBJS += $(libApiObjDir)/rcGeneralAdmin.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileChksum.o
LIB_API_OBJS += $(libApiObjDir)/rcFileChksum.o

SVR_API_OBJS += $(svrApiObjDir)/rsChkNVPathPerm.o
LIB_API_OBJS += $(libApiObjDir)/rcChkNVPathPerm.o

SVR_API_OBJS += $(svrApiObjDir)/rsGenQuery.o
LIB_API_OBJS += $(libApiObjDir)/rcGenQuery.o

SVR_API_OBJS += $(svrApiObjDir)/rsAuthRequest.o
LIB_API_OBJS += $(libApiObjDir)/rcAuthRequest.o

SVR_API_OBJS += $(svrApiObjDir)/rsAuthResponse.o
LIB_API_OBJS += $(libApiObjDir)/rcAuthResponse.o

SVR_API_OBJS += $(svrApiObjDir)/rsAuthCheck.o
LIB_API_OBJS += $(libApiObjDir)/rcAuthCheck.o

SVR_API_OBJS += $(svrApiObjDir)/rsRmCollOld.o
LIB_API_OBJS += $(libApiObjDir)/rcRmCollOld.o

SVR_API_OBJS += $(svrApiObjDir)/rsRegColl.o
LIB_API_OBJS += $(libApiObjDir)/rcRegColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsRegDataObj.o
LIB_API_OBJS += $(libApiObjDir)/rcRegDataObj.o

SVR_API_OBJS += $(svrApiObjDir)/rsUnregDataObj.o
LIB_API_OBJS += $(libApiObjDir)/rcUnregDataObj.o

SVR_API_OBJS += $(svrApiObjDir)/rsRegReplica.o
LIB_API_OBJS += $(libApiObjDir)/rcRegReplica.o

SVR_API_OBJS += $(svrApiObjDir)/rsModDataObjMeta.o
LIB_API_OBJS += $(libApiObjDir)/rcModDataObjMeta.o

SVR_API_OBJS += $(svrApiObjDir)/rsModAVUMetadata.o
LIB_API_OBJS += $(libApiObjDir)/rcModAVUMetadata.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileRename.o
LIB_API_OBJS += $(libApiObjDir)/rcFileRename.o

SVR_API_OBJS += $(svrApiObjDir)/rsModAccessControl.o
LIB_API_OBJS += $(libApiObjDir)/rcModAccessControl.o

SVR_API_OBJS += $(svrApiObjDir)/rsRuleExecSubmit.o
LIB_API_OBJS += $(libApiObjDir)/rcRuleExecSubmit.o

SVR_API_OBJS += $(svrApiObjDir)/rsRuleExecDel.o
LIB_API_OBJS += $(libApiObjDir)/rcRuleExecDel.o

SVR_API_OBJS += $(svrApiObjDir)/rsRuleExecMod.o
LIB_API_OBJS += $(libApiObjDir)/rcRuleExecMod.o

SVR_API_OBJS += $(svrApiObjDir)/rsExecMyRule.o
LIB_API_OBJS += $(libApiObjDir)/rcExecMyRule.o

SVR_API_OBJS += $(svrApiObjDir)/rsOprComplete.o
LIB_API_OBJS += $(libApiObjDir)/rcOprComplete.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjRename.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjRename.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjRsync.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjRsync.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjChksum.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjChksum.o

SVR_API_OBJS += $(svrApiObjDir)/rsPhyPathReg.o
LIB_API_OBJS += $(libApiObjDir)/rcPhyPathReg.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjPhymv.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjPhymv.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjTrim.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjTrim.o

SVR_API_OBJS += $(svrApiObjDir)/rsObjStat.o
LIB_API_OBJS += $(libApiObjDir)/rcObjStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsExecCmd.o
LIB_API_OBJS += $(libApiObjDir)/rcExecCmd.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileCreate.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileCreate.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileOpen.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileOpen.o


SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileRead.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileRead.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileWrite.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileWrite.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileClose.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileClose.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileUnlink.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileUnlink.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileStat.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileFstat.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileFstat.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileLseek.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileLseek.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileRename.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileRename.o

SVR_API_OBJS += $(svrApiObjDir)/rsQuerySpecColl.o
LIB_API_OBJS += $(libApiObjDir)/rcQuerySpecColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetTempPassword.o
LIB_API_OBJS += $(libApiObjDir)/rcGetTempPassword.o

SVR_API_OBJS += $(svrApiObjDir)/rsModColl.o
LIB_API_OBJS += $(libApiObjDir)/rcModColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileMkdir.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileMkdir.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileRmdir.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileRmdir.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileOpendir.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileOpendir.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileReaddir.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileReaddir.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileClosedir.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileClosedir.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjTruncate.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjTruncate.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileTruncate.o
LIB_API_OBJS += $(libApiObjDir)/rcFileTruncate.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileTruncate.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileTruncate.o

SVR_API_OBJS += $(svrApiObjDir)/rsGeneralUpdate.o
LIB_API_OBJS += $(libApiObjDir)/rcGeneralUpdate.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetXmsgTicket.o
LIB_API_OBJS += $(libApiObjDir)/rcGetXmsgTicket.o

SVR_API_OBJS += $(svrApiObjDir)/rsSendXmsg.o
LIB_API_OBJS += $(libApiObjDir)/rcSendXmsg.o

SVR_API_OBJS += $(svrApiObjDir)/rsRcvXmsg.o
LIB_API_OBJS += $(libApiObjDir)/rcRcvXmsg.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFileGet.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFileGet.o

SVR_API_OBJS += $(svrApiObjDir)/rsSubStructFilePut.o
LIB_API_OBJS += $(libApiObjDir)/rcSubStructFilePut.o

SVR_API_OBJS += $(svrApiObjDir)/rsSyncMountedColl.o
LIB_API_OBJS += $(libApiObjDir)/rcSyncMountedColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsStructFileSync.o
LIB_API_OBJS += $(libApiObjDir)/rcStructFileSync.o

SVR_API_OBJS += $(svrApiObjDir)/rsGsiAuthRequest.o
LIB_API_OBJS += $(libApiObjDir)/rcGsiAuthRequest.o


SVR_API_OBJS += $(svrApiObjDir)/rsOpenCollection.o
LIB_API_OBJS += $(libApiObjDir)/rcOpenCollection.o

SVR_API_OBJS += $(svrApiObjDir)/rsReadCollection.o
LIB_API_OBJS += $(libApiObjDir)/rcReadCollection.o

SVR_API_OBJS += $(svrApiObjDir)/rsCloseCollection.o
LIB_API_OBJS += $(libApiObjDir)/rcCloseCollection.o

SVR_API_OBJS += $(svrApiObjDir)/rsCollRepl.o
LIB_API_OBJS += $(libApiObjDir)/rcCollRepl.o

SVR_API_OBJS += $(svrApiObjDir)/rsRmColl.o
LIB_API_OBJS += $(libApiObjDir)/rcRmColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsStructFileExtract.o
LIB_API_OBJS += $(libApiObjDir)/rcStructFileExtract.o

SVR_API_OBJS += $(svrApiObjDir)/rsStructFileExtAndReg.o
LIB_API_OBJS += $(libApiObjDir)/rcStructFileExtAndReg.o

SVR_API_OBJS += $(svrApiObjDir)/rsStructFileBundle.o
LIB_API_OBJS += $(libApiObjDir)/rcStructFileBundle.o

SVR_API_OBJS += $(svrApiObjDir)/rsChkObjPermAndStat.o
LIB_API_OBJS += $(libApiObjDir)/rcChkObjPermAndStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsUserAdmin.o
LIB_API_OBJS += $(libApiObjDir)/rcUserAdmin.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetRemoteZoneResc.o
LIB_API_OBJS += $(libApiObjDir)/rcGetRemoteZoneResc.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjOpenAndStat.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjOpenAndStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsL3FileGetSingleBuf.o
LIB_API_OBJS += $(libApiObjDir)/rcL3FileGetSingleBuf.o

SVR_API_OBJS += $(svrApiObjDir)/rsL3FilePutSingleBuf.o
LIB_API_OBJS += $(libApiObjDir)/rcL3FilePutSingleBuf.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjCreateAndStat.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjCreateAndStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileStageToCache.o
LIB_API_OBJS += $(libApiObjDir)/rcFileStageToCache.o

SVR_API_OBJS += $(svrApiObjDir)/rsFileSyncToArch.o
LIB_API_OBJS += $(libApiObjDir)/rcFileSyncToArch.o

SVR_API_OBJS += $(svrApiObjDir)/rsGeneralRowInsert.o
LIB_API_OBJS += $(libApiObjDir)/rcGeneralRowInsert.o

SVR_API_OBJS += $(svrApiObjDir)/rsGeneralRowPurge.o
LIB_API_OBJS += $(libApiObjDir)/rcGeneralRowPurge.o

SVR_API_OBJS += $(svrApiObjDir)/rsKrbAuthRequest.o
LIB_API_OBJS += $(libApiObjDir)/rcKrbAuthRequest.o

SVR_API_OBJS += $(svrApiObjDir)/rsPhyBundleColl.o
LIB_API_OBJS += $(libApiObjDir)/rcPhyBundleColl.o

SVR_API_OBJS += $(svrApiObjDir)/rsUnbunAndRegPhyBunfile.o
LIB_API_OBJS += $(libApiObjDir)/rcUnbunAndRegPhyBunfile.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetHostForPut.o
LIB_API_OBJS += $(libApiObjDir)/rcGetHostForPut.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetRescQuota.o
LIB_API_OBJS += $(libApiObjDir)/rcGetRescQuota.o

SVR_API_OBJS += $(svrApiObjDir)/rsBulkDataObjReg.o
LIB_API_OBJS += $(libApiObjDir)/rcBulkDataObjReg.o

SVR_API_OBJS += $(svrApiObjDir)/rsBulkDataObjPut.o
LIB_API_OBJS += $(libApiObjDir)/rcBulkDataObjPut.o

SVR_API_OBJS += $(svrApiObjDir)/rsEndTransaction.o
LIB_API_OBJS += $(libApiObjDir)/rcEndTransaction.o

SVR_API_OBJS += $(svrApiObjDir)/rsProcStat.o
LIB_API_OBJS += $(libApiObjDir)/rcProcStat.o

SVR_API_OBJS += $(svrApiObjDir)/rsStreamRead.o
LIB_API_OBJS += $(libApiObjDir)/rcStreamRead.o

SVR_API_OBJS += $(svrApiObjDir)/rsDatabaseRescOpen.o
LIB_API_OBJS += $(libApiObjDir)/rcDatabaseRescOpen.o

SVR_API_OBJS += $(svrApiObjDir)/rsDatabaseRescClose.o
LIB_API_OBJS += $(libApiObjDir)/rcDatabaseRescClose.o

SVR_API_OBJS += $(svrApiObjDir)/rsDatabaseObjControl.o
LIB_API_OBJS += $(libApiObjDir)/rcDatabaseObjControl.o

SVR_API_OBJS += $(svrApiObjDir)/rsSpecificQuery.o
LIB_API_OBJS += $(libApiObjDir)/rcSpecificQuery.o

SVR_API_OBJS += $(svrApiObjDir)/rsStreamClose.o
LIB_API_OBJS += $(libApiObjDir)/rcStreamClose.o

SVR_API_OBJS += $(svrApiObjDir)/rsGetHostForGet.o
LIB_API_OBJS += $(libApiObjDir)/rcGetHostForGet.o

SVR_API_OBJS += $(svrApiObjDir)/rsDataObjFsync.o
LIB_API_OBJS += $(libApiObjDir)/rcDataObjFsync.o

