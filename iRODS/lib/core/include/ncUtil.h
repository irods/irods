/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncUtil.h - Header for for ncUtil.c */

#ifndef NCUTIL_H
#define NCUTIL_H

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif


int
ncUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp);
int
ncOperDataObjUtil (rcComm_t *conn, char *srcPath,
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset);
int
initCondForNcOper (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset);
int
ncOperCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
rodsArguments_t *rodsArgs, ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset);
int
catAggInfo (rcComm_t *conn, char *srcColl);
int
setAggInfo (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
rodsArguments_t *rodsArgs, ncOpenInp_t *ncOpenInp);
#ifdef  __cplusplus
}
#endif

#endif	/* NCUTIL_H */
