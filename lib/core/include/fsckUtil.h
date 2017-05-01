/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* fsckUtil.h - Header for fsckUtil.c */

#ifndef FSCK_UTIL_H__
#define FSCK_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*SetGenQueryInpFromPhysicalPath)(genQueryInp_t* out, const char* physical_path, const char* generic_function_argument);

int
fsckObj( rcComm_t *conn, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp, SetGenQueryInpFromPhysicalPath, const char* argument_for_SetGenQueryInpFromPhysicalPath);
int
fsckObjDir( rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, SetGenQueryInpFromPhysicalPath, const char* argument_for_SetGenQueryInpFromPhysicalPath);
int
chkObjConsistency( rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, SetGenQueryInpFromPhysicalPath, const char* argument_for_SetGenQueryInpFromPhysicalPath);

#ifdef __cplusplus
}
#endif
#endif  // FSCK_UTIL_H__
