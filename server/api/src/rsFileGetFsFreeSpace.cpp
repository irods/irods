/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileGetFsFreeSpace.h for a description of this API call.*/

#include "fileGetFsFreeSpace.h"
#include "miscServerFunct.hpp"
#include "rsFileGetFsFreeSpace.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"

int
rsFileGetFsFreeSpace( rsComm_t *rsComm,
                      fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
                      fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut ) {
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    *fileGetFsFreeSpaceOut = NULL;

    //remoteFlag = resolveHost (&fileGetFsFreeSpaceInp->addr, &rodsServerHost);
    irods::error ret = irods::get_host_for_hier_string( fileGetFsFreeSpaceInp->rescHier, remoteFlag, rodsServerHost );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "failed in call to irods::get_host_for_hier_string", ret ) );
        return -1;
    }
    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileGetFsFreeSpace( rsComm, fileGetFsFreeSpaceInp,
                                        fileGetFsFreeSpaceOut );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileGetFsFreeSpace( rsComm, fileGetFsFreeSpaceInp,
                                           fileGetFsFreeSpaceOut, rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileGetFsFreeSpace: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileGetFsFreeSpace( rsComm_t *rsComm,
                          fileGetFsFreeSpaceInp_t *fileGetFsFreeSpaceInp,
                          fileGetFsFreeSpaceOut_t **fileGetFsFreeSpaceOut,
                          rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileGetFsFreeSpace: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileGetFsFreeSpace( rodsServerHost->conn, fileGetFsFreeSpaceInp,
                                   fileGetFsFreeSpaceOut );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileGetFsFreeSpace: rcFileGetFsFreeSpace failed for %s, status = %d",
                 fileGetFsFreeSpaceInp->fileName, status );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to make call to freespace via resource plugin
int _rsFileGetFsFreeSpace(
    rsComm_t*                 _comm,
    fileGetFsFreeSpaceInp_t*  _freespace_inp,
    fileGetFsFreeSpaceOut_t** _freespace_out ) {
    // =-=-=-=-=-=-=-
    //
    if ( _freespace_inp->objPath[0] == '\0' ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        irods::log( LOG_ERROR, msg.str() );
        return SYS_INVALID_INPUT_PARAM;
    }

    // =-=-=-=-=-=-=-
    // make call to freespace via resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _freespace_inp->objPath,
            _freespace_inp->fileName,
            _freespace_inp->rescHier,
            0, 0,
            _freespace_inp->flag ) );

    irods::error free_err = fileGetFsFreeSpace( _comm, file_obj );
    // =-=-=-=-=-=-=-
    // handle errors if any
    if ( !free_err.ok() ) {
        std::stringstream msg;
        msg << "fileGetFsFreeSpace failed for [";
        msg << _freespace_inp->fileName;
        msg << "]";
        irods::error err = PASSMSG( msg.str(), free_err );
        irods::log( err );
        return ( int ) free_err.code();
    }

    // =-=-=-=-=-=-=-
    // otherwise its a success, set size appropriately
    *_freespace_out  = ( fileGetFsFreeSpaceOut_t* )malloc( sizeof( fileGetFsFreeSpaceOut_t ) );
    ( *_freespace_out )->size = free_err.code();

    return 0;

} // _rsFileGetFsFreeSpace
