/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* scanUtil.h - Header for scanUtil.c */

#ifndef SCAN_UTIL_H
#define SCAN_UTIL_H

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef  __cplusplus
extern "C" {
#endif

int
scanObj (rcComm_t *conn, rodsArguments_t *myRodsArgs, rodsPathInp_t *rodsPathInp, char hostname[LONG_NAME_LEN]);
int 
scanObjDir (rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath, char *hostname);
int 
scanObjCol (rcComm_t *conn, rodsArguments_t *myRodsArgs, char *inpPath);
int
statPhysFile (rcComm_t *conn, genQueryOut_t *genQueryOut);
int
chkObjExist (rcComm_t *conn, char *inpPath, char *hostname);
int 
checkIsMount (rcComm_t *conn, char *inpPath);

#ifdef  __cplusplus
}
#endif

#endif  /* SCAN_UTIL_H */
