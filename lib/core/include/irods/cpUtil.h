/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef CP_UTIL_H__
#define CP_UTIL_H__

#include <stdio.h>
#include "irods/rodsClient.h"
#include "irods/parseCommandLine.h"
#include "irods/rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif

int
cpUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp );
int
cpFileUtil( rcComm_t *conn, char *srcPath, char *targPath, rodsLong_t srcSize,
            rodsArguments_t *rodsArgs, dataObjCopyInp_t *dataObjCopyInp );
int
initCondForCp( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
               dataObjCopyInp_t *dataObjCopyInp, rodsRestart_t *rodsRestart );
int
cpCollUtil( rcComm_t *conn, char *srcColl, char *targColl,
            rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
            dataObjCopyInp_t *dataObjCopyInp, rodsRestart_t *rodsRestart );

#ifdef __cplusplus
}
#endif
#endif	// CP_UTIL_H__
