/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

#include "dataObjPhymv.hpp"
#include "reFuncDefs.hpp"
#include "dataObjRepl.hpp"
#include "dataObjOpr.hpp"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "reSysDataObjOpr.hpp"
#include "dataObjCreate.hpp"
#include "getRemoteZoneResc.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"

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
    std::string resc_name;

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
        irods::resource_ptr resc;
        irods::error ret = resc_mgr.resolve( dest_resc, resc );
        if ( !ret.ok() ) {
            return SYS_RESC_DOES_NOT_EXIST;
        }
    }

    // =-=-=-=-=-=-=-
    // determine hierarchy string
    if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
        std::string       hier;
        irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION, rsComm,
                           dataObjInp, hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " :: failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        // =-=-=-=-=-=-=-
        // we resolved the redirect and have a host, set the hier str for subsequent
        // api calls, etc.
        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    } // if keyword

    *transStat = ( transferStat_t* )malloc( sizeof( transferStat_t ) );
    memset( *transStat, 0, sizeof( transferStat_t ) );

    if ( getValByKey( &dataObjInp->condInput, ADMIN_KW ) != NULL ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        accessPerm = NULL;
    }
    else {
        accessPerm = ACCESS_DELETE_OBJECT;
    }

    /* query rcat for resource info and sort it */
    status = getRescForCreate( rsComm, dataObjInp, resc_name );
    if ( status < 0 ) {
        return status;
    }

    initReiWithDataObjInp( &rei, rsComm, dataObjInp );
    status = applyRule( "acSetMultiReplPerResc", NULL, &rei, NO_SAVE_REI );
    if ( strcmp( rei.statusStr, MULTI_COPIES_PER_RESC ) == 0 ) {
        multiCopyFlag = 1;
    }
    else {
        multiCopyFlag = 0;
    }

    /* query rcat for dataObjInfo and sort it */
    status = getDataObjInfo( rsComm, dataObjInp, &dataObjInfoHead,
                             accessPerm, 1 );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjPhymv: getDataObjInfo for %s", dataObjInp->objPath );
        return status;
    }

    status = resolveInfoForPhymv( &dataObjInfoHead, &oldDataObjInfoHead, resc_name, &dataObjInp->condInput, multiCopyFlag );

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

    status = _rsDataObjPhymv( rsComm, dataObjInp, dataObjInfoHead, resc_name.c_str(),
                              *transStat, multiCopyFlag );

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


    if ( srcDataObjInfo != NULL ) {
        /* not everything got moved */
        if ( savedStatus == 0 ) {
            status = SYS_COPY_ALREADY_IN_RESC;
        }
    }
    else {
        savedStatus = 0;
    }

    return savedStatus;
}

