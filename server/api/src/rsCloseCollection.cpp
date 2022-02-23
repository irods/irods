/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsCloseCollection.c
 */

#include "irods/openCollection.h"
#include "irods/closeCollection.h"
#include "irods/objMetaOpr.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rsCloseCollection.hpp"

int
rsCloseCollection( rsComm_t*, int *handleInxInp ) {
    int status;
    int handleInx = *handleInxInp;

    if ( handleInx < 0 || static_cast<std::size_t>(handleInx) >= CollHandle.size() ||
            CollHandle[handleInx].inuseFlag != FD_INUSE ) {
        rodsLog( LOG_NOTICE,
                 "rsCloseCollection: handleInx %d out of range",
                 handleInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    status = freeCollHandle( handleInx );

    return status;
}
