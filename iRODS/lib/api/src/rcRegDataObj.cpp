/* This is script-generated code.  */
/* See regDataObj.h for a description of this API call.*/

#include "regDataObj.hpp"

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
