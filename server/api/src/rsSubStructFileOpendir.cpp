/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileOpendir.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileOpendir.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"

int
rsSubStructFileOpendir( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        fd = _rsSubStructFileOpendir( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        fd = remoteSubStructFileOpendir( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileOpendir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return fd;
}

int
remoteSubStructFileOpendir( rsComm_t *rsComm, subFile_t *subFile,
                            rodsServerHost_t *rodsServerHost ) {
    int fd;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileOpendir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fd = rcSubStructFileOpendir( rodsServerHost->conn, subFile );

    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileOpendir: rcSubStructFileOpendir failed for %s, status = %d",
                 subFile->subFilePath, fd );
    }

    return fd;
}

int _rsSubStructFileOpendir( rsComm_t*  _comm,
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
    irods::error opendir_err = fileOpendir( _comm, struct_obj );
    if ( !opendir_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileOpendir for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), opendir_err ) );
        return opendir_err.code();

    }
    else {
        return opendir_err.code();

    }
}
