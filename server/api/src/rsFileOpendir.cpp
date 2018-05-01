/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* rsFileOpendir.c - server routine that handles the fileOpendir
 * API
 */

/* script generated code */
#include "fileOpendir.h"
#include "miscServerFunct.hpp"
#include "rsGlobalExtern.hpp"
#include "rsFileOpendir.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_collection_object.hpp"
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"

int
rsFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int fileInx;
    int status;
    void *dirPtr = nullptr;

    //remoteFlag = resolveHost (&fileOpendirInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileOpendirInp->resc_hier_, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileOpendir( rsComm, fileOpendirInp, &dirPtr );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileOpendir( rsComm, fileOpendirInp, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileOpendir: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    if ( status < 0 ) {
        return status;
    }

    fileInx = allocAndFillFileDesc( rodsServerHost, fileOpendirInp->objPath, fileOpendirInp->dirName, fileOpendirInp->resc_hier_,
                                    status, 0 );
    if ( fileInx < 0 ) {
        rodsLog( LOG_NOTICE, "call to allocAndFileDesc failed with status %ji", ( intmax_t ) fileInx );
        return fileInx;
    }
    FileDesc[fileInx].driverDep = dirPtr;

    return fileInx;
}

int
remoteFileOpendir( rsComm_t *rsComm, fileOpendirInp_t *fileOpendirInp,
                   rodsServerHost_t *rodsServerHost ) {
    int fileInx;
    int status;

    if ( rodsServerHost == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpendir: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }

    fileInx = rcFileOpendir( rodsServerHost->conn, fileOpendirInp );

    if ( fileInx < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileOpendir: rcFileOpendir failed for %s",
                 fileOpendirInp->dirName );
    }

    return fileInx;
}

// =-=-=-=-=-=-=-
// _rsFileOpendir - this the local version of rsFileOpendir.
int _rsFileOpendir(
    rsComm_t*         _comm,
    fileOpendirInp_t* _opendir_inp,
    void**            _dir_ptr ) {
    // =-=-=-=-=-=-=-
    // FIXME:: XXXX need to check resource permission and vault permission
    // when RCAT is available

    // =-=-=-=-=-=-=-
    // make the call to opendir via resource plugin
    irods::collection_object_ptr coll_obj(
        new irods::collection_object(
            _opendir_inp->dirName,
            _opendir_inp->resc_hier_,
            0, 0 ) );
    irods::error opendir_err = fileOpendir( _comm, coll_obj );

    // =-=-=-=-=-=-=-
    // log an error, if any
    if ( !opendir_err.ok() ) {
        std::stringstream msg;
        msg << "fileOpendir failed for [";
        msg << _opendir_inp->dirName;
        msg << "]";
        irods::error err = PASSMSG( msg.str(), opendir_err );
        irods::log( err );
    }

    ( *_dir_ptr ) = coll_obj->directory_pointer(); // JMC -- TEMPORARY

    return opendir_err.code();

} // _rsFileOpendir
