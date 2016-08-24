/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* rsOpenCollection.c - server handling routine for rcOpenCollection
 */

#include "rsOpenCollection.hpp"
#include "rcMisc.h"
#include "openCollection.h"
#include "closeCollection.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsObjStat.hpp"
#include "rsCloseCollection.hpp"

int
rsOpenCollection( rsComm_t *rsComm, collInp_t *openCollInp ) {
    int status;
    int handleInx;
    collHandle_t *collHandle;

    handleInx = allocCollHandle();

    if ( handleInx < 0 ) {
        return handleInx;
    }

    collHandle = &CollHandle[handleInx];

    status = rsInitQueryHandle( &collHandle->queryHandle, rsComm );

    if ( status < 0 ) {
        return status;
    }

    rstrcpy( collHandle->dataObjInp.objPath, openCollInp->collName, MAX_NAME_LEN );

    if ( ( openCollInp->flags & INCLUDE_CONDINPUT_IN_QUERY ) != 0 ) {
        replKeyVal( &openCollInp->condInput, &collHandle->dataObjInp.condInput );
    }

    rodsObjStat_t *rodsObjStatOut = NULL;
    status = rsObjStat( rsComm, &collHandle->dataObjInp, &rodsObjStatOut );
    if ( status < 0 ) {
        freeRodsObjStat( rodsObjStatOut );
        rsCloseCollection( rsComm, &handleInx );
        return status;
    }

    if ( rodsObjStatOut->objType != COLL_OBJ_T ) {
        freeRodsObjStat( rodsObjStatOut );
        rsCloseCollection( rsComm, &handleInx );
        return CAT_NAME_EXISTS_AS_DATAOBJ;
    }

    replSpecColl( rodsObjStatOut->specColl, &collHandle->dataObjInp.specColl );
    if ( rodsObjStatOut->specColl != NULL &&
            rodsObjStatOut->specColl->collClass == LINKED_COLL ) {
        /* save the linked path */
        rstrcpy( collHandle->linkedObjPath, rodsObjStatOut->specColl->objPath, MAX_NAME_LEN );
    };

    collHandle->rodsObjStat = rodsObjStatOut;

    collHandle->state = COLL_OPENED;
    collHandle->flags = openCollInp->flags;
    /* the collection exist. now query the data in it */
    return handleInx;
}
