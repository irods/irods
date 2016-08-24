/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileReaddir.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileReaddir.hpp"

#include "irods_structured_object.hpp"

int
rsSubStructFileReaddir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp,
                        rodsDirent_t **rodsDirent ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileReaddirInp->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileReaddir( rsComm, subStructFileReaddirInp, rodsDirent );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileReaddir( rsComm, subStructFileReaddirInp, rodsDirent,
                                             rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileReaddir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileReaddir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileReaddirInp,
                            rodsDirent_t **rodsDirent, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileReaddir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileReaddir( rodsServerHost->conn, subStructFileReaddirInp,
                                     rodsDirent );

    if ( status < 0 && status != -1 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileReaddir: rcFileReaddir failed for fd %d",
                 subStructFileReaddirInp->fd );
    }

    return status;

}

int _rsSubStructFileReaddir( rsComm_t*                 _comm,
                             subStructFileFdOprInp_t*  _read_inp,
                             rodsDirent_t **           _dirent ) {
    if ( !_read_inp ) {
        irods::log( LOG_NOTICE, "XXXX _rsSubStructFileReaddir - null _read_inp" );
        return -1;
    }

    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_read_inp ) );

    struct_obj->comm( _comm );
    struct_obj->resc_hier( _read_inp->resc_hier );
    struct_obj->file_descriptor( _read_inp->fd );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to read a file
    irods::error readdir_err = fileReaddir( _comm, struct_obj, _dirent );
    if ( !readdir_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileReaddir for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), readdir_err ) );
        return readdir_err.code();

    }
    else {
        return readdir_err.code();

    }

}
