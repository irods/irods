/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileLseek.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileLseek.hpp"

// =-=-=-=-=-=-=-
#include "irods_structured_object.hpp"


int
rsSubStructFileLseek( rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
                      fileLseekOut_t **subStructFileLseekOut ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileLseekInp->addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileLseek( rsComm, subStructFileLseekInp, subStructFileLseekOut );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileLseek( rsComm, subStructFileLseekInp, subStructFileLseekOut,
                                           rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileLseek: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileLseek( rsComm_t *rsComm, subStructFileLseekInp_t *subStructFileLseekInp,
                          fileLseekOut_t **subStructFileLseekOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileLseek: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileLseek( rodsServerHost->conn, subStructFileLseekInp,
                                   subStructFileLseekOut );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileLseek: rcFileLseek failed for fd %d",
                 subStructFileLseekInp->fd );
    }

    return status;
}

int _rsSubStructFileLseek( rsComm_t*                _comm,
                           subStructFileLseekInp_t* _lseek_inp,
                           fileLseekOut_t**         _lseek_out ) {
    // =-=-=-=-=-=-=-
    // create a structured object fco
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
        ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _lseek_inp->resc_hier );
    struct_obj->file_descriptor( _lseek_inp->fd );

    // =-=-=-=-=-=-=-
    // call lseek interface
    irods::error lseek_err = fileLseek( _comm,
                                        struct_obj,
                                        _lseek_inp->offset,
                                        _lseek_inp->whence );
    if ( !lseek_err.ok() ) {
        std::stringstream msg;
        msg << "fileLseek failed for fd [";
        msg << struct_obj->file_descriptor();
        msg << "]";
        irods::log( PASSMSG( msg.str(), lseek_err ) );
        return lseek_err.code();

    }
    else {
        // =-=-=-=-=-=-=-
        // allocate outgoing lseek buffer
        if ( ! *_lseek_out ) {
            *_lseek_out = new fileLseekOut_t;
            memset( *_lseek_out, 0, sizeof( fileLseekOut_t ) );
        }

        ( *_lseek_out )->offset = lseek_err.code();
        return 0;

    }

}
