/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef MKDIR_UTIL_H__
#define MKDIR_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif
int
mkdirUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
           rodsPathInp_t *rodsPathInp );

#ifdef __cplusplus
}
#endif

#endif	// MKDIR_UTIL_H__
