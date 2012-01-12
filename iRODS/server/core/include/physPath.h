/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* physPath.h - header file for physPath.c
 */



#ifndef PHYS_PATH_H
#define PHYS_PATH_H

#include "rods.h"
#include "initServer.h"
#include "objInfo.h"
#include "dataObjInpOut.h"
#include "fileRename.h"
#include "miscUtil.h"
#include "structFileSync.h"
#include "structFileExtAndReg.h"
#include "dataObjOpenAndStat.h"

#define ORPHAN_DIR	"orphan"
#define REPL_DIR	"replica"
#define CHK_ORPHAN_CNT_LIMIT  20  /* number of failed check before stopping */

#ifdef  __cplusplus
extern "C" {
#endif

int
getFileMode (dataObjInp_t *dataObjInp);

int
getFileFlags (int l1descInx);

int
getFilePathName (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
dataObjInp_t *dataObjInp);
int
getVaultPathPolicy (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
vaultPathPolicy_t *outVaultPathPolicy);
int
setPathForGraftPathScheme (char *objPath, char *vaultPath, int addUserName,
char *userName, int trimDirCnt, char *outPath);
int 
setPathForRandomScheme (char *objPath, char *vaultPath, char *userName,
char *outPath);
int
resolveDupFilePath (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
dataObjInp_t *dataObjInp);
int
getchkPathPerm (rsComm_t *rsComm, dataObjInp_t *dataObjInp, dataObjInfo_t *dataObjInfo);
int 
getCopiesFromCond (keyValPair_t *condInput);
int
getWriteFlag (int openFlag);
int
dataObjChksum (rsComm_t *rsComm, int l1descInx, keyValPair_t *regParam);
int
_dataObjChksum (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char **chksumStr);
rodsLong_t 
getSizeInVault (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo);
int
dataObjChksumAndReg (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
char **chksumStr);
int
chkAndHandleOrphanFile (rsComm_t *rsComm, char *filePath, 
rescInfo_t *rescInfo, int replStatus);
int
renameFilePathToNewDir (rsComm_t *rsComm, char *newDir,
fileRenameInp_t *fileRenameInp, rescInfo_t *rescInfo, int renameFlag);
int
syncDataObjPhyPath (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t *dataObjInfoHead, char *acLCollection);
int
syncDataObjPhyPathS (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t *dataObjInfo, char *acLCollection);
int
syncCollPhyPath (rsComm_t *rsComm, char *collection);
int
isInVault (dataObjInfo_t *dataObjInfo);
int
initStructFileOprInp (rsComm_t *rsComm, structFileOprInp_t *structFileOprInp,
structFileExtAndRegInp_t *structFileExtAndRegInp,
dataObjInfo_t *dataObjInfo);
int
getDefFileMode ();
int
getDefDirMode ();
int
getLogPathFromPhyPath (char *phyPath, rescInfo_t *rescInfo, char *outLogPath);
int
rsMkOrhpanPath (rsComm_t *rsComm, char *objPath, char *orphanPath);

#ifdef  __cplusplus
}
#endif

#endif	/* PHYS_PATH_H */

