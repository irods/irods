#include "regDataObj.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcRegDataObj( rcComm_t *conn, dataObjInfo_t *dataObjInfo, dataObjInfo_t **outDataObjInfo )
 *
 * \brief Register a data object.
 *
 * \ingroup server_icat
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInfo - the dataObjInfo
 * \param[out] outDataObjInfo - the dataObjInfo output
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcRegDataObj( rcComm_t *conn, dataObjInfo_t *dataObjInfo,
              dataObjInfo_t **outDataObjInfo ) {
    int            status      = 0;
    dataObjInfo_t* srcNext     = 0;

    /* don't send next */
    srcNext     = dataObjInfo->next;
    dataObjInfo->next     = NULL;
    status = procApiRequest( conn, REG_DATA_OBJ_AN, dataObjInfo, NULL,
                             ( void ** ) outDataObjInfo, NULL );

    /* restore */
    dataObjInfo->next     = srcNext;
    /* cleanup fake pointers */
    if ( status >= 0 && *outDataObjInfo != NULL ) {
        if ( ( *outDataObjInfo )->next != NULL ) {
            free( ( *outDataObjInfo )->next );
            ( *outDataObjInfo )->next = NULL;
        }
    }
    return status;
}
