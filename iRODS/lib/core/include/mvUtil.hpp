/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef MV_UTIL_H
#define MV_UTIL_H

#include <stdio.h>
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif

int
mvUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp);
int
initCondForMv (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
dataObjCopyInp_t *dataObjCopyInp);
int
mvObjUtil (rcComm_t *conn, char *srcPath, char *targPath, objType_t objType,
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
dataObjCopyInp_t *dataObjCopyInp);

#ifdef  __cplusplus
}
#endif

#endif	/* MV_UTIL_H */
