#include "modDataObjMeta.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcModDataObjMeta( rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp )
 *
 * \brief Modify a data object's metadata.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] modDataObjMetaInp - input data structure for modDataObj
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcModDataObjMeta( rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp ) {
    int status;
    dataObjInfo_t *srcNext;

    srcNext = modDataObjMetaInp->dataObjInfo->next;
    modDataObjMetaInp->dataObjInfo->next = NULL;

    status = procApiRequest( conn, MOD_DATA_OBJ_META_AN, modDataObjMetaInp, NULL,
                             ( void ** ) NULL, NULL );

    modDataObjMetaInp->dataObjInfo->next = srcNext;


    return status;
}
