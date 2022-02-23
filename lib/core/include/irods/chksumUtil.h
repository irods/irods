#ifndef CHKSUM_UTIL_H__
#define CHKSUM_UTIL_H__

#include "irods/rodsClient.h"
#include "irods/parseCommandLine.h"
#include "irods/rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif
int
chksumUtil( rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
            rodsPathInp_t *rodsPathInp );
int
chksumDataObjUtil( rcComm_t *conn, char *srcPath,
                   rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );
int
initCondForChksum( rodsArguments_t *rodsArgs,
                   dataObjInp_t *dataObjInp, collInp_t *collInp );
int
chksumCollUtil( rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
                rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp, collInp_t *collInp );

#ifdef __cplusplus
}
#endif

#endif	// CHKSUM_UTIL_H__
