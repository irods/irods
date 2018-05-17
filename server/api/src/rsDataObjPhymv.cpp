/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "dataObjPhymv.h"
#include "dataObjRepl.h"
#include "dataObjOpr.hpp"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "icatDefines.h"
#include "dataObjCreate.h"
#include "getRemoteZoneResc.h"
#include "physPath.hpp"
#include "fileClose.h"
#include "rsDataObjPhymv.hpp"
#include "rsFileOpen.hpp"
#include "rsFileClose.hpp"
#include "rsDataObjRepl.hpp"
#include "rsDataObjPhymv.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_resource_backport.hpp"
#include "irods_hierarchy_parser.hpp"


irods::error test_source_replica_for_write_permissions(
    rsComm_t*      _comm,
    std::string    _resc_hier,
    dataObjInfo_t* _data_obj_info ) {
    
    while(_data_obj_info != NULL && _data_obj_info->rescHier != _resc_hier) {
        _data_obj_info = _data_obj_info->next;
    }
	if( !_comm || !_data_obj_info ) {
        return ERROR(
		           SYS_INTERNAL_NULL_INPUT_ERR,
				   "null _data_obj_info or _comm" );
	}

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string(
	                       _data_obj_info->rescHier,
						   location );
    if ( !ret.ok() ) {
        return PASS( ret );
    }

	// test the source hier to determine if we have write access to the data
	// stored.  if not then we cannot unlink that replica and should throw an
	// error.
	fileOpenInp_t open_inp;
	memset(
	    &open_inp, 0,
		sizeof( open_inp ) );
    open_inp.mode = getDefFileMode();
    open_inp.flags = O_WRONLY;
    rstrcpy(
	    open_inp.resc_name_,
		_data_obj_info->rescName,
		MAX_NAME_LEN );
    rstrcpy(
	    open_inp.resc_hier_,
		_data_obj_info->rescHier,
		MAX_NAME_LEN );
    rstrcpy(
	    open_inp.objPath,
		_data_obj_info->objPath,
		MAX_NAME_LEN );
    rstrcpy(
	    open_inp.addr.hostAddr,
		location.c_str(),
		NAME_LEN );
    rstrcpy(
	    open_inp.fileName,
		_data_obj_info->filePath,
		MAX_NAME_LEN );
    rstrcpy(
	    open_inp.in_pdmo,
		_data_obj_info->in_pdmo,
		MAX_NAME_LEN );

    // kv passthru
    copyKeyVal(
        &_data_obj_info->condInput,
        &open_inp.condInput );

    int l3_idx = rsFileOpen( _comm, &open_inp );
	clearKeyVal( &open_inp.condInput );
    if( l3_idx < 0 ) {
		std::string msg = "unable to open ";
		msg += _data_obj_info->objPath;
		msg += " for unlink";
		addRErrorMsg(
		    &_comm->rError,
			SYS_USER_NO_PERMISSION,
			msg.c_str() );
		return ERROR(
		           SYS_USER_NO_PERMISSION,
				   msg );
	}


    fileCloseInp_t close_inp;
	memset( &close_inp, 0, sizeof( close_inp ) );
	close_inp.fileInx = l3_idx;
	int status = rsFileClose( _comm, &close_inp );
    if( status < 0 ) {
		std::string msg = "failed to close ";
		msg += _data_obj_info->objPath;
		return ERROR(
		           status,
				   msg );
    }

	return SUCCESS();

} // test_source_replica_for_write_permissions


/* rsDataObjPhymv - The Api handler of the rcDataObjPhymv call - phymove
 * a data object from one resource to another.
 * Input -
 *    rsComm_t *rsComm
 *    dataObjInp_t *dataObjInp - The replication input
 *    transferStat_t **transStat - transfer stat output
 */

int
rsDataObjPhymv( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                transferStat_t **transStat ) {
    int status = 0;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *oldDataObjInfoHead = NULL;
    ruleExecInfo_t rei;
    int multiCopyFlag = 0;
    char *accessPerm = NULL;
    int remoteFlag = 0;
    rodsServerHost_t *rodsServerHost = NULL;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = _rcDataObjPhymv( rodsServerHost->conn, dataObjInp,
                                  transStat );
        return status;
    }

    char* dest_resc = getValByKey( &dataObjInp->condInput, DEST_RESC_NAME_KW );
    if ( dest_resc ) {
        std::string dest_hier;
        irods::error ret = resolve_hierarchy_for_resc_from_cond_input(
                               rsComm,
                               dest_resc,
                               dest_hier );
        if( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        addKeyVal(
            &dataObjInp->condInput,
            DEST_RESC_HIER_STR_KW,
            dest_hier.c_str() );
    }

    // =-=-=-=-=-=-=-
    // determine hierarchy string
	char* dest_hier_kw = getValByKey( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW );
    std::string dest_hier;
    if ( NULL == dest_hier_kw || 0 == strlen(dest_hier_kw) ) {
        irods::error ret = irods::resolve_resource_hierarchy(
			irods::CREATE_OPERATION,
			rsComm,
			dataObjInp,
			dest_hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the dest_hier str for subsequent
        // api calls, etc.
        addKeyVal(
		    &dataObjInp->condInput,
			DEST_RESC_HIER_STR_KW,
			dest_hier.c_str() );
    } // if keyword
    else {
        dest_hier = dest_hier_kw;
    }

    char* src_resc = getValByKey( &dataObjInp->condInput, RESC_NAME_KW );
    if ( src_resc ) {
        std::string src_hier;
        irods::error ret = resolve_hierarchy_for_resc_from_cond_input(
                               rsComm,
                               src_resc,
                               src_hier );
        if( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }
        addKeyVal(
            &dataObjInp->condInput,
            RESC_HIER_STR_KW,
            src_hier.c_str() );
    }

    // =-=-=-=-=-=-=-
    // determine hierarchy string
	char* resc_hier_kw = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    std::string src_hier;
    if ( NULL == resc_hier_kw || 0 == strlen(resc_hier_kw) ) {
        irods::error ret = irods::resolve_resource_hierarchy(
		                       irods::OPEN_OPERATION,
							   rsComm,
                               dataObjInp,
							   src_hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the src_hier str for subsequent
        // api calls, etc.
        addKeyVal(
		    &dataObjInp->condInput,
			RESC_HIER_STR_KW,
			src_hier.c_str() );
    } // if keyword
    else {
        src_hier = resc_hier_kw;
    }

    *transStat = ( transferStat_t* )malloc( sizeof( transferStat_t ) );
    memset( *transStat, 0, sizeof( transferStat_t ) );

    if( getValByKey( &dataObjInp->condInput, ADMIN_KW ) != NULL ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        accessPerm = NULL;
    }
    else {
        accessPerm = ACCESS_DELETE_OBJECT;
    }

    // get root of the destination hierarchy as the 'resc for create'
    irods::hierarchy_parser h_parse;
    h_parse.set_string( dest_hier );

    std::string dest_root;
    h_parse.first_resc( dest_root );
    if( status < 0 ) {
        return status;
    }

    initReiWithDataObjInp(&rei, rsComm, dataObjInp);
    status = applyRule("acSetMultiReplPerResc", NULL, &rei, NO_SAVE_REI);
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);
    if (status < 0) {
        if (rei.status < 0) {
            status = rei.status;
        }
        const auto err{ERROR(status, "acSetMultiReplPerResc failed")};
        irods::log(err);
        return status;
    }

    if ( strcmp( rei.statusStr, MULTI_COPIES_PER_RESC ) == 0 ) {
        multiCopyFlag = 1;
    }
    else {
        multiCopyFlag = 0;
    }

    // query rcat for dataObjInfo and sort it
    status = getDataObjInfo(
	             rsComm,
				 dataObjInp,
				 &dataObjInfoHead,
                 accessPerm, 1 );

    if ( status < 0 ) {
        rodsLog(
		    LOG_NOTICE,
            "rsDataObjPhymv: getDataObjInfo for %s",
			dataObjInp->objPath );
        return status;
    }

    irods::error ret = test_source_replica_for_write_permissions(
	                       rsComm,
                               src_hier,
	                       dataObjInfoHead );
	if( !ret.ok() ) {
        irods::log( PASS( ret ) );
		return ret.code();
	}

    status = resolveInfoForPhymv(
	             &dataObjInfoHead,
				 &oldDataObjInfoHead,
				 dest_root.c_str(),
				 &dataObjInp->condInput,
				 multiCopyFlag );
    if ( status < 0 ) {
        freeAllDataObjInfo( dataObjInfoHead );
        freeAllDataObjInfo( oldDataObjInfoHead );
        if ( status == CAT_NO_ROWS_FOUND ) {
            return 0;
        }
        else {
            return status;
        }
    }
    status = _rsDataObjPhymv(
	             rsComm,
				 dataObjInp,
				 dataObjInfoHead,
				 dest_root.c_str(),
                 *transStat,
				 multiCopyFlag );

    freeAllDataObjInfo( dataObjInfoHead );
    freeAllDataObjInfo( oldDataObjInfoHead );

    return status;
}

int
_rsDataObjPhymv( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 dataObjInfo_t *srcDataObjInfoHead, const char *_resc_name,
                 transferStat_t *transStat, int multiCopyFlag ) {
    dataObjInfo_t *srcDataObjInfo;
    int status = 0;
    int savedStatus = 0;


    srcDataObjInfo = srcDataObjInfoHead;

    while ( srcDataObjInfo ) {
        /* use _rsDataObjReplS for the phymv */
        dataObjInp->oprType = PHYMV_OPR;    /* should be set already */
        status = _rsDataObjReplS( rsComm, dataObjInp, srcDataObjInfo,
                                  _resc_name, NULL, 0 );

        if ( multiCopyFlag == 0 ) {
            if ( status >= 0 ) {
                srcDataObjInfo = srcDataObjInfo->next;
            }
            else {
                savedStatus = status;
            }
            /* use another resc */
            break;
        }
        else {
            if ( status < 0 ) {
                savedStatus = status;
                /* use another resc */
                break;
            }
        }
        srcDataObjInfo = srcDataObjInfo->next;
    }
    if ( status >= 0 ) {
        transStat->numThreads = dataObjInp->numThreads;
    }

    if ( NULL == srcDataObjInfo ) {
        savedStatus = 0;
    }

    return savedStatus;
}
