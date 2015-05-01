/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regUtil.h - Header for for regUtil.c */

#ifndef REG_UTIL_H__
#define REG_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif
int
regUtil( rcComm_t *conn, rodsEnv *myEnv, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp );
int
initCondForReg( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                dataObjInp_t *dataObjOprInp );

#ifdef __cplusplus
}
#endif

#endif	// REG_UTIL_H__
