/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "subStructFileCreate.h"
#include "rsSubStructFileCreate.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"
#include "irods_log.hpp"

int
rsSubStructFileCreate( rsComm_t *rsComm, subFile_t *subFile ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fd;

    remoteFlag = resolveHost( &subFile->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        fd = _rsSubStructFileCreate( rsComm, subFile );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        fd = remoteSubStructFileCreate( rsComm, subFile, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileCreate: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return fd;
}

int
remoteSubStructFileCreate( rsComm_t *rsComm, subFile_t *subFile,
                           rodsServerHost_t *rodsServerHost ) {
    int fd;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileCreate: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fd = rcSubStructFileCreate( rodsServerHost->conn, subFile );

    if ( fd < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileCreate: rcSubStructFileCreate failed for %s, status = %d",
                 subFile->subFilePath, fd );
    }

    return fd;
}

// =-=-=-=-=-=-=-
// local function to handle sub file creation
int _rsSubStructFileCreate(
    rsComm_t*  _comm,
    subFile_t* _sub_file ) {

    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            *_sub_file ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _sub_file->specColl->rescHier );

    irods::error err = fileCreate( _comm, struct_obj );
    if ( !err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileCreate for [";
        msg << struct_obj->sub_file_path();
        irods::log( PASSMSG( msg.str(), err ) );
        return 0;

    }
    else {
        return err.code();

    }

} // _rsSubStructFileCreate
