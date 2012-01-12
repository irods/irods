/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsyncUtil.h - Header for for rsyncUtil.c */

#ifndef RSYNC_UTIL_H
#define RSYNC_UTIL_H

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif

int
rsyncUtil (rcComm_t *conn, rodsEnv *myEnv, rodsArguments_t *myRodsArgs, 
rodsPathInp_t *rodsPathInp);

int
initCondForRsync (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
dataObjInp_t *dataObjOprInp);
int
initCondForIrodsToIrodsRsync (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
dataObjCopyInp_t *dataObjCopyInp);

int
rsyncDataToFileUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjInp_t *dataObjOprInp);
int
rsyncFileToDataUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjInp_t *dataObjOprInp);
int
rsyncDataToDataUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjCopyInp_t *dataObjCopyInp);
int
rsyncCollToDirUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjInp_t *dataObjOprInp);
int
rsyncDirToCollUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjInp_t *dataObjOprInp);
int
rsyncCollToCollUtil (rcComm_t *conn, rodsPath_t *srcPath,
rodsPath_t *targPath, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
dataObjCopyInp_t *dataObjCopyInp);

#ifdef  __cplusplus
}
#endif

#endif	/* RSYNC_UTIL_H */
