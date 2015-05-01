/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* phymvUtil.h - Header for for phymvUtil.c */

#ifndef PHYMVUTIL_H__
#define PHYMVUTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif

int
phymvUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp );
int
phymvDataObjUtil( rcComm_t *conn, char *srcPath,
                  rodsArguments_t *rodsArgs,
                  dataObjInp_t *dataObjInp );
int
initCondForPhymv( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                  dataObjInp_t *dataObjInp );
int
phymvCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
               rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );

#ifdef __cplusplus
}
#endif

#endif	// PHYMVUTIL_H__
