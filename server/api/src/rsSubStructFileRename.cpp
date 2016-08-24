/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to subStructFiles in the COPYRIGHT directory ***/
#include "subStructFileRename.h"
#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "rsSubStructFileRename.hpp"

#include "irods_structured_object.hpp"


int
rsSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    remoteFlag = resolveHost( &subStructFileRenameInp->subFile.addr, &rodsServerHost );

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsSubStructFileRename( rsComm, subStructFileRenameInp );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteSubStructFileRename( rsComm, subStructFileRenameInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsSubStructFileRename: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    return status;
}

int
remoteSubStructFileRename( rsComm_t *rsComm, subStructFileRenameInp_t *subStructFileRenameInp,
                           rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRename: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    status = rcSubStructFileRename( rodsServerHost->conn, subStructFileRenameInp );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteSubStructFileRename: rcSubStructFileRename failed for %s, status = %d",
                 subStructFileRenameInp->subFile.subFilePath, status );
    }

    return status;
}

int
_rsSubStructFileRename( rsComm_t*                 _comm,
                        subStructFileRenameInp_t* _rename_inp ) {
    // =-=-=-=-=-=-=-
    // create first class structured object
    irods::structured_object_ptr struct_obj(
        new irods::structured_object(
            _rename_inp->subFile ) );
    struct_obj->comm( _comm );
    struct_obj->resc_hier( _rename_inp->resc_hier );

    // =-=-=-=-=-=-=-
    // call abstrcated interface to rename
    irods::error rename_err = fileRename( _comm, struct_obj, _rename_inp->newSubFilePath );
    if ( !rename_err.ok() ) {
        std::stringstream msg;
        msg << "failed on call to fileRename for [";
        msg << struct_obj->physical_path();
        msg << "]";
        irods::log( PASSMSG( msg.str(), rename_err ) );
        return rename_err.code();

    }
    else {
        return rename_err.code();

    }

}
