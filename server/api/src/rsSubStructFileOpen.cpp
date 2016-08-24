/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileOpen.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileOpen.hpp"

#include "irods_structured_object.hpp"



int
rsSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        fd = _rsSubStructFileOpen( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        fd = remoteSubStructFileOpen( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileOpen: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return fd;
}

int
remoteSubStructFileOpen( rsComm_t *rsComm, subFile_t *subFile,
                         rodsServerHost_t *rodsServerHost ) {
    int fd;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileOpen: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fd = rcSubStructFileOpen( rodsServerHost->conn, subFile );

    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileOpen: rcSubStructFileOpen failed for %s, status = %d",
                 subFile->subFilePath, fd );
    }

    return fd;
}

int
_rsSubStructFileOpen(
    rsComm_t*  _comm,
    subFile_t* _sub_file ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to open a file
    irods::error open_err = fileOpen( _comm, struct_obj );
    if ( !open_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileOpen for [";
        msg << struct_obj->sub_file_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), open_err ) );
        return open_err.code();

    }
    else {
        return open_err.code();

    }

} // _rsSubStructFileOpen
