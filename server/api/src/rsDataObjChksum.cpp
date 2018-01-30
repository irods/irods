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
#include "rsFileStat.hpp"
#include "boost/lexical_cast.hpp"
#include "rsGenQuery.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"


namespace {
    // assumes zone redirection has already occurred, and that the function is being called on a server in the zone the object belongs to
    int verifyVaultSizeEqualsDatabaseSize( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo) {
        fileStatInp_t fileStatInp;
        memset(&fileStatInp, 0, sizeof(fileStatInp));
        rstrcpy(fileStatInp.objPath, dataObjInfo->objPath, sizeof(fileStatInp.objPath));
        rstrcpy(fileStatInp.rescHier, dataObjInfo->rescHier, sizeof(fileStatInp.rescHier));
        rstrcpy(fileStatInp.fileName, dataObjInfo->filePath, sizeof(fileStatInp.fileName));
        rodsStat_t *fileStatOut = nullptr;
        const int status_rsFileStat = rsFileStat(rsComm, &fileStatInp, &fileStatOut);
        if (status_rsFileStat < 0) {
            rodsLog(LOG_ERROR, "verifyVaultSizeEqualsDatabaseSize: rsFileStat of objPath [%s] rescHier [%s] fileName [%s] failed with [%d]", fileStatInp.objPath, fileStatInp.rescHier, fileStatInp.fileName, status_rsFileStat);
            return status_rsFileStat;
        }
        const rodsLong_t size_in_vault = fileStatOut->st_size;
        free(fileStatOut);
        genQueryInp_t genQueryInp;
        memset(&genQueryInp, 0, sizeof(genQueryInp));
        addInxIval(&genQueryInp.selectInp, COL_DATA_SIZE, 1);
        addInxVal(&genQueryInp.sqlCondInp, COL_D_DATA_PATH, (boost::format(" = '%s'") % dataObjInfo->filePath).str().c_str());
        genQueryInp.maxRows = 1;
        genQueryOut_t *genQueryOut = nullptr;
        const int status_rsGenQuery = rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
        clearGenQueryInp(&genQueryInp);
        if (status_rsGenQuery < 0) {
            if (status_rsGenQuery == CAT_NO_ROWS_FOUND) {
                freeGenQueryOut(&genQueryOut);
            }
            rodsLog(LOG_ERROR, "_rsFileChksum: rsGenQuery of [%s] [%s] [%s] failed with [%d]", dataObjInfo->filePath, dataObjInfo->rescHier, dataObjInfo->objPath, status_rsGenQuery);
            return status_rsGenQuery;
        }

        rodsLong_t size_in_database;
        try {
            size_in_database = boost::lexical_cast<rodsLong_t>(genQueryOut->sqlResult[0].value);
        } catch (const boost::bad_lexical_cast&) {
            freeGenQueryOut(&genQueryOut);
            rodsLog(LOG_ERROR, "_rsFileChksum: lexical_cast of [%s] for [%s] [%s] [%s] failed", genQueryOut->sqlResult[0].value, dataObjInfo->filePath, dataObjInfo->rescHier, dataObjInfo->objPath);
            return INVALID_LEXICAL_CAST;
        }
        freeGenQueryOut(&genQueryOut);
        if (size_in_database != size_in_vault) {
            rodsLog(LOG_ERROR, "_rsFileChksum: file size mismatch. resource hierarchy [%s] vault path [%s] size [%ji] object path [%s] size [%ji]", dataObjInfo->rescHier, dataObjInfo->filePath,
                    static_cast<intmax_t>(size_in_vault), dataObjInfo->objPath, static_cast<intmax_t>(size_in_database));
            return USER_FILE_SIZE_MISMATCH;
        }
        return 0;
    }
}

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
    const bool shouldVerifyVaultSizeEqualsDatabaseSize = getValByKey( &dataObjInp->condInput, VERIFY_VAULT_SIZE_EQUALS_DATABASE_SIZE_KW ) != NULL;

    int status_getDataObjInfoIncSpecColl = getDataObjInfoIncSpecColl( rsComm, dataObjInp, dataObjInfoHead );
    if ( status_getDataObjInfoIncSpecColl < 0 ) {
        return status_getDataObjInfoIncSpecColl;
    }

    // If admin flag is in play, make sure the client user is allowed to use it
    const bool admin_flag = getValByKey( &dataObjInp->condInput, ADMIN_KW );
    if ( admin_flag && rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
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

        // Add the admin flag before running checksum
        if ( admin_flag ) {
            addKeyVal( &tmpDataObjInfo->condInput, ADMIN_KW, "" );
        }

        /* need to compute the chksum */
        if ( verifyFlag && strlen( tmpDataObjInfo->chksum ) > 0 ) {
            status = verifyDatObjChksum( rsComm, tmpDataObjInfo,
                                         outChksumStr );
            if (status == 0 && shouldVerifyVaultSizeEqualsDatabaseSize) {
                if (int code = verifyVaultSizeEqualsDatabaseSize(rsComm, tmpDataObjInfo)) {
                    status = code;
                }
            }
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

            // Add the admin flag before running checksum
            if ( admin_flag ) {
                addKeyVal( &tmpDataObjInfo->condInput, ADMIN_KW, "" );
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
                if (verifyStatus == 0 && shouldVerifyVaultSizeEqualsDatabaseSize) {
                    if (int code = verifyVaultSizeEqualsDatabaseSize(rsComm, tmpDataObjInfo)) {
                        verifyStatus = code;
                    }
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
    // Make sure admin flag is set as appropriate
    if ( NULL != getValByKey( &dataObjInfo->condInput, ADMIN_KW ) ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        addKeyVal( &regParam, ADMIN_KW, "" );
    }
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
