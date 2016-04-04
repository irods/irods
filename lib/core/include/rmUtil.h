/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* getUtil.h - Header for for getUtil.c */

#ifndef RM_UTIL_H__
#define RM_UTIL_H__

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"

#ifdef __cplusplus
extern "C" {
#endif

int
rmUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp );
int
rmDataObjUtil( rcComm_t *conn, char *srcPath,
               rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp );
int
initCondForRm( rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp,
               collInp_t *collInp );
int
rmCollUtil( rcComm_t *conn, char *srcColl,
            rodsArguments_t *rodsArgs, collInp_t *collInp );
int
mvDataObjToTrash( rcComm_t *conn, dataObjInp_t *dataObjInp );
int
mkTrashPath( rcComm_t *conn, dataObjInp_t *dataObjInp, char *trashPath );
int
mvCollToTrash( rcComm_t *conn, dataObjInp_t *dataObjInp );

#ifdef __cplusplus
}
#endif

#endif	// RM_UTIL_H__
