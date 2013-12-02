/* This is script-generated code.  */ 
/* See syncMountedColl.h for a description of this API call.*/

#include "syncMountedColl.hpp"

int
rcSyncMountedColl (rcComm_t *conn, dataObjInp_t *syncMountedCollInp)
{
    int status;
    status = procApiRequest (conn, SYNC_MOUNTED_COLL_AN, syncMountedCollInp, 
      NULL, (void **) NULL, NULL);

    return (status);
}
