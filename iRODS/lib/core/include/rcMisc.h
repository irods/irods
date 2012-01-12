/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rcMisc.h - header file for rcMisc.c
 */



#ifndef RC_MISC_H
#define RC_MISC_H

#include "rods.h"
#include "rodsError.h"
#include "objInfo.h"
#include "dataObjCopy.h"
#include "execCmd.h"
#include "rodsPath.h"
#include "bulkDataObjPut.h"

#ifdef  __cplusplus
extern "C" {
#endif

int isPath (char *path);
rodsLong_t
getFileSize (char *path);
int
addRErrorMsg (rError_t *myError, int status, char *msg);
int freeBBuf (bytesBuf_t *myBBuf);
int
clearBBuf (bytesBuf_t *myBBuf);
int
freeRError (rError_t *myError);
int
replErrorStack (rError_t *srcRError, rError_t *destRError);
int
freeRErrorContent (rError_t *myError);
int
parseUserName(char *fullUserNameIn, char *userName, char *userZone);
int
apiTableLookup (int apiNumber);
int
myHtonll (rodsLong_t inlonglong, rodsLong_t *outlonglong);
int
myNtohll (rodsULong_t inlonglong,  rodsLong_t *outlonglong);
int
statToRodsStat (rodsStat_t *rodsStat, struct stat *myFileStat);
int
rodsStatToStat (struct stat *myFileStat, rodsStat_t *rodsStat);
int
direntToRodsDirent (rodsDirent_t *rodsDirent, struct dirent *fileDirent);
int
getLine(FILE *fp, char *buf, int bufsz);
int
getStrInBuf (char **inbuf, char *outbuf, int *inbufLen, int outbufLen);
int
getZoneNameFromHint (char *rcatZoneHint, char *zoneName, int len);
int
freeDataObjInfo (dataObjInfo_t *dataObjInfo);
int
freeAllDataObjInfo (dataObjInfo_t *dataObjInfoHead);
char *
getValByKey (keyValPair_t *condInput, char *keyWord);
int
getIvalByInx (inxIvalPair_t *inxIvalPair, int inx, int *outValue);
char *
getValByInx (inxValPair_t *inxValPair, int inx);
int
replKeyVal (keyValPair_t *srcCondInput, keyValPair_t *destCondInput);
int
replDataObjInp (dataObjInp_t *srcDataObjInp, dataObjInp_t *destDataObjInp);
int
replSpecColl (specColl_t *inSpecColl, specColl_t **outSpecColl);
int
addKeyVal (keyValPair_t *condInput, char *keyWord, char *value);
int
addInxIval (inxIvalPair_t *inxIvalPair, int inx, int value);
int
addInxVal (inxValPair_t *inxValPair, int inx, char *value);
int
addStrArray (strArray_t *strArray, char *value);
int
resizeStrArray (strArray_t *strArray, int newSize);
int
queDataObjInfo (dataObjInfo_t **dataObjInfoHead, dataObjInfo_t *dataObjInfo,
int singleInfoFlag, int topFlag);
int
clearKeyVal (keyValPair_t *condInput);
int
clearInxIval (inxIvalPair_t *inxIvalPair);
int
clearInxVal (inxValPair_t *inxValPair);
int
moveKeyVal (keyValPair_t *destKeyVal, keyValPair_t *srcKeyVal);
int
rmKeyVal (keyValPair_t *condInput, char *keyWord);
int
sendTranHeader (int sock, int oprType, int flags, rodsLong_t offset,
rodsLong_t length);
int 
freeGenQueryOut (genQueryOut_t **genQueryOut);
int
clearGenQueryInp (genQueryInp_t *genQueryInp);
sqlResult_t *
getSqlResultByInx (genQueryOut_t *genQueryOut, int attriInx);
int 
clearGenQueryOut (genQueryOut_t *genQueryOut);
int
catGenQueryOut (genQueryOut_t *targGenQueryOut, genQueryOut_t *genQueryOut,
int maxRowCnt);
int
clearBulkOprInp (bulkOprInp_t *bulkOprInp);
int
getUnixUid (char *userName);
int 
freeAllRescGrpInfo (rescGrpInfo_t *rescGrpInfoHead);
int
parseMultiStr (char *strInput, strArray_t *strArray);
void
getNowStr(char *timeStr);
int
getLocalTimeFromRodsTime(char *timeStrIn, char *timeStrOut);
int
getLocalTimeStr (struct tm *mytm, char *timeStr);
void
getOffsetTimeStr(char *timeStr, char *offSet);
void
updateOffsetTimeStr(char *timeStr, int offset);
int
checkDateFormat(char *s);
int
localToUnixTime (char *localTime, char *unixTime);
int
printErrorStack (rError_t *rError);
int
closeQueryOut (rcComm_t *conn, genQueryOut_t *genQueryOut);
int 
getDataObjInfoCnt (dataObjInfo_t *dataObjInfoHead);
int
appendRandomToPath (char *trashPath);
int
isTrashPath (char *myPath);
orphanPathType_t
isOrphanPath (char *myPath);
int
isHomeColl (char *myPath);
int
isTrashHome (char *myPath);
int
openRestartFile (char *restartFile, rodsRestart_t *rodsRestart,
rodsArguments_t *rodsArgs);
int
setStateForResume (rcComm_t *conn, rodsRestart_t *rodsRestart, 
char *restartPath, objType_t objType, keyValPair_t *condInput,
int deleteFlag);
int
writeRestartFile (rodsRestart_t *rodsRestart, char *lastDonePath);
int
procAndWrriteRestartFile (rodsRestart_t *rodsRestart, char *donePath);
int
setStateForRestart (rcComm_t *conn, rodsRestart_t *rodsRestart,
rodsPath_t *targPath, rodsArguments_t *rodsArgs);
int
chkStateForResume (rcComm_t *conn, rodsRestart_t *rodsRestart,
char *targPath, rodsArguments_t *rodsArgs, objType_t objType,
keyValPair_t *condInput, int deleteFlag);
int
addTagStruct (tagStruct_t *condInput, char *preTag, char *postTag, 
char *keyWord);
int
clearDataObjInp (dataObjInp_t *dataObjInp);
int
clearCollInp (collInp_t *collInp);
int
clearAuthResponseInp (void * myInStruct);
int
isInteger (char *inStr);
int
addIntArray (intArray_t *intArray, int value);
int
getMountedSubPhyPath (char *logMountPoint, char *phyMountPoint, 
char *logSubPath, char *phySubPathOut);
int
resolveSpecCollType (char *type, char *collection, char *collInfo1,
char *collInfo2, specColl_t *specColl);
int
getSpecCollTypeStr (specColl_t *specColl, char *outStr);
int
getErrno (int errCode);
int 
getIrodsErrno (int irodError);
structFileOprType_t
getSpecCollOpr (keyValPair_t *condInput, specColl_t *specColl);
void
resolveStatForStructFileOpr (keyValPair_t *condInput,
rodsObjStat_t *rodsObjStatOut);
int keyValToString( keyValPair_t* list, char** string );
int keyValFromString( char* string, keyValPair_t** list );
int
convertDateFormat(char *s, char *currTime);
int
getNextRepeatTime(char *currTime, char *delayStr, char *nextTime);
int
printError(rcComm_t *Conn, int status, char *routineName);
int
fillGenQueryInpFromStrCond(char *str, genQueryInp_t *genQueryInp);
int
printGenQueryOut(FILE *fd, char *format, char *hint, 
genQueryOut_t *genQueryOut);

int appendToByteBuf(bytesBuf_t *bytesBuf, char *str);

char * getAttrNameFromAttrId(int cid);
int getAttrIdFromAttrName(char *cname);
int showAttrNames();

int
separateSelFuncFromAttr(char *t, char **aggOp, char **colNm);

int
getSelVal(char *c);

int
clearSendXmsgInfo (sendXmsgInfo_t *sendXmsgInfo);

int
parseCachedStructFileStr (char *collInfo2, specColl_t *specColl);
int
makeCachedStructFileStr (char *collInfo2, specColl_t *specColl);
int
getLineInBuf (char **inbuf, char *outbuf, int bufLen);
int
freeRodsObjStat (rodsObjStat_t *rodsObjStat);
int
parseHostAddrStr (char *hostAddr, rodsHostAddr_t *addr);
#ifdef COMPAT_201
int
collInp201ToCollInp (collInp201_t *collInp201, collInp_t *collInp);
#endif
void
printReleaseInfo (char *cmdName);
unsigned int
seedRandom ();
int
initBulkDataObjRegInp (genQueryOut_t *bulkDataObjRegInp);
int
initBulkDataObjRegOut (genQueryOut_t **bulkDataObjRegOut);
int
fillBulkDataObjRegInp (char *rescName, char *rescGroupName, char *objPath,
char *filePath, char *dataType, rodsLong_t dataSize, int dataMode,
int modFlag, int replNum, char *chksum, genQueryOut_t *bulkDataObjRegInp);
int
untarBuf (char *phyBunDir, bytesBuf_t *tarBBuf);
int
tarToBuf (char *phyBunDir, bytesBuf_t *tarBBuf);
int
readToByteBuf (int fd, bytesBuf_t *bytesBuf);
int
writeFromByteBuf (int fd, bytesBuf_t *bytesBuf);
int
initAttriArrayOfBulkOprInp (bulkOprInp_t *bulkOprInp);
int
fillAttriArrayOfBulkOprInp (char *objPath, int dataMode, char *inpChksum,
int offset, bulkOprInp_t *bulkOprInp);
int
getPhyBunPath (char *collection, char *objPath, char *phyBunDir,
char *outPhyBunPath);
int
unbunBulkBuf (char *phyBunDir, bulkOprInp_t *bulkOprInp, bytesBuf_t *bulkBBuf);
int
mySetenvStr (char *envname, char *envval);
int
mySetenvInt (char *envname, int envval);
int
getNumFilesInDir (char *mydir);
int
getRandomArray (int **randomArray, int size);
int
isPathSymlink (rodsArguments_t *rodsArgs, char *path);
int
getAttriInAttriArray (char *objPath, genQueryOut_t *attriArray,
int *outDataMode, char **outChksum);
#ifdef USE_BOOST_FS
int
getPathStMode (path & p);
#endif
#ifdef  __cplusplus
}
#endif

#endif	/* RC_MISC_H */
