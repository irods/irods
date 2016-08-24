/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileUnlink.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileUnlink.hpp"

#include "irods_structured_object.hpp"


int
rsSubStructFileUnlink( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileUnlink( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileUnlink( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileUnlink: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileUnlink( rsComm_t *rsComm, subFile_t *subFile,
                           rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileUnlink: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileUnlink( rodsServerHost->conn, subFile );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileUnlink: rcSubStructFileUnlink failed for %s, status = %d",
                 subFile->subFilePath, status );
    }

    return status;
}

int _rsSubStructFileUnlink( rsComm_t*  _comm,
                            subFile_t* _sub_file ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to unlink
    irods::error unlink_err = fileUnlink( _comm, struct_obj );
    if ( !unlink_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileUnlink for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), unlink_err ) );
        return unlink_err.code();

    }
    else {
        return unlink_err.code();

    }

}
