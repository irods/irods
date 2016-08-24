/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See fileStat.h for a description of this API call.*/

#include "fileStat.h"
#include "miscServerFunct.hpp"
#include "rsFileStat.hpp"

// =-=-=-=-=-=-=-
#include "irods_log.hpp"
#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_resource_backport.hpp"


int
rsFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp,
            rodsStat_t **fileStatOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    *fileStatOut = NULL;

    if( 0 == fileStatInp->rescId ) {
        if( 0 == strlen(fileStatInp->rescHier) ) {
            rodsLog(
                LOG_ERROR,
                "rsFileStat - rescId and rescHier are invalid");
            return SYS_INVALID_INPUT_PARAM;
        }
        irods::error ret = resc_mgr.hier_to_leaf_id(
                               fileStatInp->rescHier,
                               fileStatInp->rescId);
        if( !ret.ok() ) {
            irods::log(PASS(ret));
            return ret.code();
        }
    }

    irods::resource_ptr resc;
    irods::error ret = resc_mgr.resolve(fileStatInp->rescId,resc);
    if ( !ret.ok() ) {
        irods::log(PASS(ret));
        return ret.code();
    }

    ret = resc->get_property<rodsServerHost_t*>(
              irods::RESOURCE_HOST,
              rodsServerHost );
    if ( !ret.ok() ) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if ( rodsServerHost->localFlag < 0 ) {
        return rodsServerHost->localFlag;
    }
    else {
        status = rsFileStatByHost( rsComm, fileStatInp, fileStatOut,
                                   rodsServerHost );
        return status;
    }
}

int
rsFileStatByHost( rsComm_t *rsComm, fileStatInp_t *fileStatInp,
                  rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost ) {
    int remoteFlag;
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsFileStatByHost: Input NULL rodsServerHost" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remoteFlag = rodsServerHost->localFlag;

    if ( remoteFlag == LOCAL_HOST ) {
        status = _rsFileStat( rsComm, fileStatInp, fileStatOut );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = remoteFileStat( rsComm, fileStatInp, fileStatOut,
                                 rodsServerHost );
    }
    else {
        if ( remoteFlag < 0 ) {
            return remoteFlag;
        }
        else {
            rodsLog( LOG_NOTICE,
                     "rsFileStat: resolveHost returned unrecognized value %d",
                     remoteFlag );
            return SYS_UNRECOGNIZED_REMOTE_FLAG;
        }
    }

    /* Manually insert call-specific code here */

    return status;
}

int
remoteFileStat( rsComm_t *rsComm, fileStatInp_t *fileStatInp,
                rodsStat_t **fileStatOut, rodsServerHost_t *rodsServerHost ) {
    int status;

    if ( rodsServerHost == NULL ) {
        rodsLog( LOG_NOTICE,
                 "remoteFileStat: Invalid rodsServerHost" );
        return SYS_INVALID_SERVER_HOST;
    }

    if ( ( status = svrToSvrConnect( rsComm, rodsServerHost ) ) < 0 ) {
        return status;
    }


    status = rcFileStat( rodsServerHost->conn, fileStatInp, fileStatOut );

    if ( status < 0 ) {
        rodsLog( LOG_DEBUG,
                 "remoteFileStat: rcFileStat failed for %s",
                 fileStatInp->fileName );
    }

    return status;
}

// =-=-=-=-=-=-=-
// local function to handle call to stat via resource plugin
int _rsFileStat(
    rsComm_t*      _comm,
    fileStatInp_t* _stat_inp,
    rodsStat_t**   _stat_out ) {
    struct stat myFileStat;
    memset( &myFileStat, 0, sizeof( myFileStat ) );

    // =-=-=-=-=-=-=-
    // make call to stat via resource plugin
    irods::file_object_ptr file_obj(
        new irods::file_object(
            _comm,
            _stat_inp->objPath,
            _stat_inp->fileName,
            _stat_inp->rescId,
            0, 0, 0 ) );
    irods::error stat_err = fileStat( _comm, file_obj, &myFileStat );

    // =-=-=-=-=-=-=-
    // log error if necessary
    if ( !stat_err.ok() ) {
//        irods::log(LOG_ERROR, stat_err.result());
        return stat_err.code();
    }

    // =-=-=-=-=-=-=-
    // convert unix stat struct to an irods stat struct
    *_stat_out = ( rodsStat_t* )malloc( sizeof( rodsStat_t ) );
    int status = statToRodsStat( *_stat_out, &myFileStat );

    // =-=-=-=-=-=-=-
    // manage error if necessary
    if ( status < 0 ) {
        free( *_stat_out );
        *_stat_out = NULL;
    }

    return status;

} // _rsFileStat
