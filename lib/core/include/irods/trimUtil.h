/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* trimUtil.h - Header for for trimUtil.c */

#ifndef TRIMUTIL_H__
#define TRIMUTIL_H__

#include "irods/rodsClient.h"
#include "irods/parseCommandLine.h"
#include "irods/rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif

int
trimUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
          rodsPathInp_t *rodsPathInp );
int
trimDataObjUtil( rcComm_t *conn, char *srcPath,
                 rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );
int
initCondForTrim( rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );
int
trimCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
              rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );

#ifdef __cplusplus
}
#endif

#endif	// TRIMUTIL_H__
