/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileClosedir.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileClosedir.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"

int
rsSubStructFileClosedir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileClosedirInp->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileClosedir( rsComm, subStructFileClosedirInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileClosedir( rsComm, subStructFileClosedirInp,
                                              rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileClosedir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileClosedir( rsComm_t *rsComm, subStructFileFdOprInp_t *subStructFileClosedirInp,
                             rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileClosedir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileClosedir( rodsServerHost->conn, subStructFileClosedirInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileClosedir: rcFileClosedir failed for fd %d",
                 subStructFileClosedirInp->fd );
    }

    return status;
}

int _rsSubStructFileClosedir( rsComm_t*                _comm,
                              subStructFileFdOprInp_t* _close_inp ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object( *_close_inp ) );

    struct_obj->comm( _comm );
    struct_obj->resc_hier( _close_inp->resc_hier );
    struct_obj->file_descriptor( _close_inp->fd );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to close a file
    irods::error closedir_err = fileClosedir( _comm, struct_obj );
    if ( !closedir_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileClosedir for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), closedir_err ) );
        return closedir_err.code();

    }
    else {
        return closedir_err.code();

    }

}
