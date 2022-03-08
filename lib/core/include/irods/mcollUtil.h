/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* mcollUtil.h - Header for for mcollUtil.c */

#ifndef MCOLL_UTIL_H__
#define MCOLL_UTIL_H__

#include "irods/rodsClient.h"
#include "irods/parseCommandLine.h"
#include "irods/rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif
int
mcollUtil( rcComm_t *conn, rodsEnv *myEnv, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp );
int
initCondForMcoll( rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs,
                  dataObjInp_t *dataObjOprInp );

#ifdef __cplusplus
}
#endif

#endif	// MCOLL_UTIL_H__
