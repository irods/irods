/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* trimUtil.h - Header for for trimUtil.c */

#ifndef TRIMUTIL_H
#define TRIMUTIL_H

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif

int
trimUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp);
int
trimDataObjUtil (rcComm_t *conn, char *srcPath,
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
dataObjInp_t *dataObjInp);
int
initCondForTrim (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
dataObjInp_t *dataObjInp);
int
trimCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp);

#ifdef  __cplusplus
}
#endif

#endif	/* TRIMUTIL_H */
