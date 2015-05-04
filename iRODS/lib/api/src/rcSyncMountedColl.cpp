/* This is script-generated code.  */
/* See syncMountedColl.h for a description of this API call.*/

#include "syncMountedColl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcSyncMountedColl( rcComm_t *conn, dataObjInp_t *syncMountedCollInp )
 *
 * \brief Sync the mounted structured file with the cache.
 *
 * \user client
 *
 * \ingroup collection_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] syncMountedCollInp - Elements of dataObjInp_t used :
 *    \li char \b objPath - the path of the Mounted collection
 *    \li char \b oprType
 *      \n PURGE_STRUCT_FILE_CACHE - purge the cache after the sync
 *      \n DELETE_STRUCT_FILE - delete the structured file and the cache
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/
int
rcSyncMountedColl( rcComm_t *conn, dataObjInp_t *syncMountedCollInp ) {
    int status;
    status = procApiRequest( conn, SYNC_MOUNTED_COLL_AN, syncMountedCollInp,
                             NULL, ( void ** ) NULL, NULL );

    return status;
}
