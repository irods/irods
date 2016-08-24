/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileClose.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileClose.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"

int
rsSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileCloseInp->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileClose( rsComm, subStructFileCloseInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileClose( rsComm, subStructFileCloseInp,
                                           rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileClose: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileClose( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileCloseInp,
                          rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileClose: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileClose( rodsServerHost->conn, subStructFileCloseInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileClose: rcFileClose failed for fd %d",
                 subStructFileCloseInp->fd );
    }

    return status;
}

int _rsSubStructFileClose(
    rsComm_t*                _comm,
    subStructFileFdOprInp_t* _close_inp ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_close_inp ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _close_inp->resc_hier );
    struct_obj->file_descriptor( _close_inp->fd );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to open a file
    irods::error close_err = fileClose( _comm, struct_obj );
    if ( !close_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileClose for fd [ ";
        msg << struct_obj->file_descriptor();
        msg << " ]";
        irods::log( PASSMSG( msg.str(), close_err ) );
        return close_err.code();

    }
    else {
        return close_err.code();

    }

}
