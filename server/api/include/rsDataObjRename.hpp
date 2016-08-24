#ifndef RS_DATA_OBJ_RENAME_HPP
#define RS_DATA_OBJ_RENAME_HPP

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "dataObjCopy.h"
#include "objInfo.h"

int rsDataObjRename( rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp );
int _rsDataObjRename( rsComm_t *rsComm, dataObjCopyInp_t *dataObjRenameInp );
int specCollObjRename( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, dataObjInfo_t *destDataObjInfo );
int l3Rename( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, char *newFileName );
int moveMountedCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, int srcType, dataObjInp_t *destDataObjInp );
int moveMountedCollDataObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, dataObjInp_t *destDataObjInp );
int moveMountedCollCollObj( rsComm_t *rsComm, dataObjInfo_t *srcDataObjInfo, dataObjInp_t *destDataObjInp );

#endif
