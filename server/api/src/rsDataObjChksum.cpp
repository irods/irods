#include "dataObjChksum.h"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "specColl.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "rsApiHandler.hpp"
#include "modDataObjMeta.h"
#include "getRemoteZoneResc.h"
#include "rsDataObjChksum.hpp"
#include "rsModDataObjMeta.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"

int
rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjChksumInp,
                 char **outChksum ) {
    int status;
    dataObjInfo_t *dataObjInfoHead;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, dataObjChksumInp->objPath, &specCollCache, &dataObjChksumInp->condInput );

    remoteFlag = getAndConnRemoteZone( rsComm, dataObjChksumInp, &rodsServerHost, REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = rcDataObjChksum( rodsServerHost->conn, dataObjChksumInp,
                                  outChksum );
        return status;
    }
    else {
        // =-=-=-=-=-=-=-
        // determine the resource hierarchy if one is not provided
        if ( getValByKey( &dataObjChksumInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
            std::string       hier;
            irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION,
                               rsComm, dataObjChksumInp, hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjChksumInp->objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjChksumInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

        } // if keyword

        status = _rsDataObjChksum( rsComm, dataObjChksumInp, outChksum,
                                   &dataObjInfoHead );
    }
    freeAllDataObjInfo( dataObjInfoHead );
    rodsLog( LOG_DEBUG, "rsDataObjChksum - returning status %d", status );
    return status;
}

int
_rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                  char **outChksumStr, dataObjInfo_t **dataObjInfoHead ) {

    *dataObjInfoHead = NULL;
    *outChksumStr = NULL;

    bool allFlag = getValByKey( &dataObjInp->condInput, CHKSUM_ALL_KW ) != NULL;
    bool verifyFlag = getValByKey( &dataObjInp->condInput, VERIFY_CHKSUM_KW ) != NULL;
    bool forceFlag = getValByKey( &dataObjInp->condInput, FORCE_CHKSUM_KW ) != NULL;

    int status_getDataObjInfoIncSpecColl = getDataObjInfoIncSpecColl( rsComm, dataObjInp, dataObjInfoHead );
    if ( status_getDataObjInfoIncSpecColl < 0 ) {
        return status_getDataObjInfoIncSpecColl;
    }

    int status = 0;
    if ( !allFlag ) {
        /* screen out any stale copies */
        int status_sortObjInfoForOpen = sortObjInfoForOpen( dataObjInfoHead, &dataObjInp->condInput, 0 );
        if ( status_sortObjInfoForOpen < 0 ) {
            return status_sortObjInfoForOpen;
        }

        dataObjInfo_t *tmpDataObjInfo = *dataObjInfoHead;
        do {
            if ( ( tmpDataObjInfo->replStatus > 0 || !(*dataObjInfoHead)->next ) && strlen( tmpDataObjInfo->chksum ) > 0 ) {
                if ( !verifyFlag && !forceFlag ) {
                    *outChksumStr = strdup( tmpDataObjInfo->chksum );
                    return 0;
                }
                else {
                    break;
                }
            }

        } while ( (tmpDataObjInfo = tmpDataObjInfo->next) );

        if ( !tmpDataObjInfo ) {
            tmpDataObjInfo = *dataObjInfoHead;
        }

        /* need to compute the chksum */
        if ( verifyFlag && strlen( tmpDataObjInfo->chksum ) > 0 ) {
            status = verifyDatObjChksum( rsComm, tmpDataObjInfo,
                                         outChksumStr );
        }
        else {
            addKeyVal( &tmpDataObjInfo->condInput, ORIG_CHKSUM_KW, getValByKey( &dataObjInp->condInput, ORIG_CHKSUM_KW ) );
            status = dataObjChksumAndRegInfo( rsComm, tmpDataObjInfo, outChksumStr );
        }
    } else {
        dataObjInfo_t *tmpDataObjInfo = *dataObjInfoHead;
        int verifyStatus = 0;
        do {
            //JMC - legacy resource :: int rescClass = getRescClass (tmpDataObjInfo->rescInfo);
            std::string resc_class;
            irods::error err = irods::get_resource_property< std::string >(
                                tmpDataObjInfo->rescId,
                                irods::RESOURCE_CLASS,
                                resc_class );
            if ( !err.ok() ) {
                irods::log( PASSMSG( "failed in get_resource_property [class]", err ) );
            }
            if ( resc_class == irods::RESOURCE_CLASS_BUNDLE ) { // (rescClass == BUNDLE_CL) {
                /* don't do BUNDLE_CL. should be done on the bundle file */
                tmpDataObjInfo = tmpDataObjInfo->next;
                status = 0;
                continue;
            }

            char *tmpChksumStr = 0;
            if ( strlen( tmpDataObjInfo->chksum ) == 0 ) {
                /* need to chksum no matter what */
                status = dataObjChksumAndRegInfo( rsComm, tmpDataObjInfo,
                                                &tmpChksumStr );
            }
            else if ( verifyFlag ) {
                if ( int code = verifyDatObjChksum( rsComm, tmpDataObjInfo, &tmpChksumStr ) ) {
                    verifyStatus = code;
                }
            }
            else if ( forceFlag ) {
                status = dataObjChksumAndRegInfo( rsComm, tmpDataObjInfo,
                                                &tmpChksumStr );
            }
            else {
                tmpChksumStr = strdup( tmpDataObjInfo->chksum );
                status = 0;
            }

            if ( status < 0 ) {
                return status;
            }
            if ( tmpDataObjInfo->replStatus > 0 && *outChksumStr == NULL ) {
                *outChksumStr = tmpChksumStr;
            }
            else {
                /* save it */
                if ( strlen( tmpDataObjInfo->chksum ) == 0 ) {
                    rstrcpy( tmpDataObjInfo->chksum, tmpChksumStr, NAME_LEN );
                }
                free( tmpChksumStr );
            }
        } while ( (tmpDataObjInfo = tmpDataObjInfo->next) );
        if ( *outChksumStr == NULL ) {
            *outChksumStr = strdup( ( *dataObjInfoHead )->chksum );
        }
        if (status >= 0 && verifyStatus < 0) {
            status = verifyStatus;
        }
    }

    return status;
}

int
dataObjChksumAndRegInfo( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                         char **outChksumStr ) {
    int              status;
    keyValPair_t     regParam;
    modDataObjMeta_t modDataObjMetaInp;

    status = _dataObjChksum( rsComm, dataObjInfo, outChksumStr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "dataObjChksumAndRegInfo: _dataObjChksum error for %s, status = %d",
                 dataObjInfo->objPath, status );
        return status;
    }

    if ( dataObjInfo->specColl != NULL ) {
        return status;
    }

    memset( &regParam, 0, sizeof( regParam ) );
    addKeyVal( &regParam, CHKSUM_KW, *outChksumStr );
    // set pdmo flag so that chksum doesn't trigger file operations
    addKeyVal( &regParam, IN_PDMO_KW, "" );
    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
    rodsLog( LOG_DEBUG, "dataObjChksumAndRegInfo - rsModDataObjMeta status %d", status );
    clearKeyVal( &regParam );

    return status;
}

int
verifyDatObjChksum( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    char **outChksumStr ) {
    int status;

    addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, dataObjInfo->chksum );
    status = _dataObjChksum( rsComm, dataObjInfo, outChksumStr );
    rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "verifyDatObjChksum:_dataObjChksum error for %s, stat=%d",
                 dataObjInfo->objPath, status );
        return status;
    }
    if ( *outChksumStr == NULL ) {
        rodsLog( LOG_ERROR, "verifyDatObjChksum: outChkSumStr is null." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strcmp( *outChksumStr, dataObjInfo->chksum ) != 0 ) {
        std::stringstream error_message;
        error_message << "verifyDatObjChksum: computed chksum [" << *outChksumStr << "] != icat value [" << dataObjInfo->chksum << "] for [" << dataObjInfo->objPath << "] hierarchy [" << dataObjInfo->rescHier << "] replNum [" << dataObjInfo->replNum << "]";
        rodsLog( LOG_ERROR, "%s", error_message.str().c_str());
        addRErrorMsg(&rsComm->rError, USER_CHKSUM_MISMATCH, error_message.str().c_str());
        return USER_CHKSUM_MISMATCH;
    }
    else {
        return status;
    }
}
