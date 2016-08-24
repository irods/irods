/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileRmdir.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileRmdir.hpp"

#include "irods_structured_object.hpp"


int
rsSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        fd = _rsSubStructFileRmdir( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        fd = remoteSubStructFileRmdir( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileRmdir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return fd;
}

int
remoteSubStructFileRmdir( rsComm_t *rsComm, subFile_t *subFile,
                          rodsServerHost_t *rodsServerHost ) {
    int fd;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRmdir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fd = rcSubStructFileRmdir( rodsServerHost->conn, subFile );

    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRmdir: rcSubStructFileRmdir failed for %s, status = %d",
                 subFile->subFilePath, fd );
    }

    return fd;
}

int _rsSubStructFileRmdir( rsComm_t*  _comm,
                           subFile_t* _sub_file ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to rmdir
    irods::error rmdir_err = fileRmdir( _comm, struct_obj );
    if ( !rmdir_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileRmdir for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), rmdir_err ) );
        return rmdir_err.code();

    }
    else {
        return rmdir_err.code();

    }

}
