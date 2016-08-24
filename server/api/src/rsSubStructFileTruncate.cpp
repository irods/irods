/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileTruncate.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileTruncate.hpp"

#include "irods_structured_object.hpp"


int
rsSubStructFileTruncate( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileTruncate( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileTruncate( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileTruncate: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileTruncate( rsComm_t *rsComm, subFile_t *subFile,
                             rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileTruncate: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileTruncate( rodsServerHost->conn, subFile );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileTruncate: rcSubStructFileTruncate failed for %s, status = %d",
                 subFile->subFilePath, status );
    }

    return status;
}

int _rsSubStructFileTruncate( rsComm_t*   _comm,
                              subFile_t * _sub_file ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to truncate
    irods::error trunc_err = fileTruncate( _comm, struct_obj );
    if ( !trunc_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileTruncate for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), trunc_err ) );
        return trunc_err.code();

    }
    else {
        return trunc_err.code();

    }

}
