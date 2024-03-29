/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsReadCollection.c
 */

#include "irods/openCollection.h"
#include "irods/readCollection.h"
#include "irods/objMetaOpr.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/rsReadCollection.hpp"

int
rsReadCollection( rsComm_t*, int *handleInxInp,
                  collEnt_t **collEnt ) {
    int status;
    collHandle_t *collHandle;

    int handleInx = *handleInxInp;

    if ( handleInx < 0 || static_cast<std::size_t>(handleInx) >= CollHandle.size() ||
            CollHandle[handleInx].inuseFlag != FD_INUSE ) {
        rodsLog( LOG_NOTICE,
                 "rsReadCollection: handleInx %d out of range",
                 handleInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    collHandle = &CollHandle[handleInx];
    *collEnt = ( collEnt_t * ) malloc( sizeof( collEnt_t ) );

    status = readCollection( collHandle, *collEnt );

    if ( status < 0 ) {
        free( *collEnt );
        *collEnt = NULL;
    }

    return status;
}
