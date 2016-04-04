/*** Copyright (c), The Regents of the University of California            ***
 *** For more infophybunation please refer to files in the COPYRIGHT directory ***/
/* phybunUtil.h - Header for for phybunUtil.c */

#ifndef PHYBUN_UTIL_H__
#define PHYBUN_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif
int
phybunUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
            rodsPathInp_t *rodsPathInp );
int
initCondForPhybunOpr( rodsArguments_t *rodsArgs,
                      structFileExtAndRegInp_t *phyBundleCollInp );

#ifdef __cplusplus
}
#endif

#endif	// PHYBUN_UTIL_H__
