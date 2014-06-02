/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* See dataObjPhymv.h for a description of this API call.*/

#include "dataObjPhymv.hpp"
#include "reFuncDefs.hpp"
#include "dataObjRepl.hpp"
#include "dataObjOpr.hpp"
#include "rodsLog.hpp"
#include "objMetaOpr.hpp"
#include "specColl.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "reSysDataObjOpr.hpp"
#include "dataObjCreate.hpp"
#include "getRemoteZoneResc.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"

int
rsDataObjPhymv250( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   transStat_t **transStat ) {
    int status;
    transferStat_t *transferStat = NULL;

    status = rsDataObjPhymv( rsComm, dataObjInp, &transferStat );

    if ( transStat != NULL && status >= 0 && transferStat != NULL ) {
        *transStat = ( transStat_t * ) malloc( sizeof( transStat_t ) );
        ( *transStat )->numThreads = transferStat->numThreads;
        ( *transStat )->bytesWritten = transferStat->bytesWritten;
        free( transferStat );
    }
    return status;
}

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
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *oldDataObjInfoHead = NULL;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    ruleExecInfo_t rei;
    int multiCopyFlag;
    char *accessPerm;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = _rcDataObjPhymv( rodsServerHost->conn, dataObjInp,
                                  transStat );
        return status;
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
            return ( CAT_INSUFFICIENT_PRIVILEGE_LEVEL );
        }
        accessPerm = NULL;
    }
    else {
        accessPerm = ACCESS_DELETE_OBJECT;
    }

    /* query rcat for resource info and sort it */
    status = getRescGrpForCreate( rsComm, dataObjInp, &myRescGrpInfo );
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
        delete myRescGrpInfo->rescInfo;
        delete myRescGrpInfo;
        return ( status );
    }

    status = resolveInfoForPhymv( &dataObjInfoHead, &oldDataObjInfoHead, &myRescGrpInfo, &dataObjInp->condInput, multiCopyFlag );

    if ( status < 0 ) {
        freeAllDataObjInfo( dataObjInfoHead );
        freeAllDataObjInfo( oldDataObjInfoHead );
        delete myRescGrpInfo->rescInfo;
        delete myRescGrpInfo;
        if ( status == CAT_NO_ROWS_FOUND ) {
            return ( 0 );
        }
        else {
            return ( status );
        }
    }

    status = _rsDataObjPhymv( rsComm, dataObjInp, dataObjInfoHead,
                              myRescGrpInfo, *transStat, multiCopyFlag );

    freeAllDataObjInfo( dataObjInfoHead );
    freeAllDataObjInfo( oldDataObjInfoHead );
    delete myRescGrpInfo->rescInfo;
    delete myRescGrpInfo;

    return ( status );
}

int
_rsDataObjPhymv( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 dataObjInfo_t *srcDataObjInfoHead, rescGrpInfo_t *destRescGrpInfo,
                 transferStat_t *transStat, int multiCopyFlag ) {
    dataObjInfo_t *srcDataObjInfo;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int status = 0;
    int savedStatus = 0;

    tmpRescGrpInfo = destRescGrpInfo;
    srcDataObjInfo = srcDataObjInfoHead;
    while ( tmpRescGrpInfo != NULL ) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        while ( srcDataObjInfo != NULL ) {
            /* use _rsDataObjReplS for the phymv */
            dataObjInp->oprType = PHYMV_OPR;    /* should be set already */
            status = _rsDataObjReplS( rsComm, dataObjInp, srcDataObjInfo,
                                      tmpRescInfo, tmpRescGrpInfo->rescGroupName, NULL, 0 );

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
        tmpRescGrpInfo = tmpRescGrpInfo->next;
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

    return ( savedStatus );
}

