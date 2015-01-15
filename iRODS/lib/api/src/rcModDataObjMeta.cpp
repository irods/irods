/* This is script-generated code.  */
/* See modDataObjMeta.h for a description of this API call.*/

#include "modDataObjMeta.hpp"

int
rcModDataObjMeta( rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp ) {
    int status;
    dataObjInfo_t *srcNext;

    srcNext = modDataObjMetaInp->dataObjInfo->next;
    modDataObjMetaInp->dataObjInfo->rescInfo = NULL;
    modDataObjMetaInp->dataObjInfo->next = NULL;

    status = procApiRequest( conn, MOD_DATA_OBJ_META_AN, modDataObjMetaInp, NULL,
                             ( void ** ) NULL, NULL );

    modDataObjMetaInp->dataObjInfo->rescInfo = NULL; // srcRescInfo;
    modDataObjMetaInp->dataObjInfo->next = srcNext;


    return status;
}
