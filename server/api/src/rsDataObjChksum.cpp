#include "boost/lexical_cast.hpp"
#include "dataObjChksum.h"
#include "dataObjOpr.hpp"
#include "getRemoteZoneResc.h"
#include "irods_log.hpp"
#include "modDataObjMeta.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "resource.hpp"
#include "rsApiHandler.hpp"
#include "rsDataObjChksum.hpp"
#include "rsFileStat.hpp"
#include "rsGenQuery.hpp"
#include "rsModDataObjMeta.hpp"
#include "specColl.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods_query.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "key_value_proxy.hpp"

#include "fmt/format.h"

#include <optional>

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
        std::optional<rodsLong_t> size_in_database {};
        std::string select_and_condition {
            (boost::format("select DATA_SIZE where DATA_PATH = '%s'") % dataObjInfo->filePath).str()
        };
        for (auto&& row : irods::query{rsComm, select_and_condition}) {
            try {
                size_in_database = boost::lexical_cast<rodsLong_t>(row[0]);
            } catch (const boost::bad_lexical_cast&) {
                rodsLog(LOG_ERROR, "_rsFileChksum: lexical_cast of [%s] for [%s] [%s] [%s] failed", row[0].c_str(), dataObjInfo->filePath, dataObjInfo->rescHier, dataObjInfo->objPath);
                return INVALID_LEXICAL_CAST;
            }
        }
        if (!size_in_database.has_value()) { return CAT_NO_ROWS_FOUND; }
        if (size_in_database.value() != size_in_vault) {
            rodsLog(LOG_ERROR, "_rsFileChksum: file size mismatch. resource hierarchy [%s] vault path [%s] size [%ji] object path [%s] size [%ji]", dataObjInfo->rescHier, dataObjInfo->filePath,
                    static_cast<intmax_t>(size_in_vault), dataObjInfo->objPath, static_cast<intmax_t>(size_in_database.value()));
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

    // If the client specified a leaf resource, then discover the hierarchy and
    // store it in the keyValPair_t. This instructs the iRODS server to create
    // the replica at the specified resource if it does not exist.
    auto kvp = irods::experimental::make_key_value_proxy(dataObjChksumInp->condInput);
    if (kvp.contains(LEAF_RESOURCE_NAME_KW)) {
        std::string hier;
        auto leaf = kvp[LEAF_RESOURCE_NAME_KW].value();
        bool is_coord_resc = false;

        if (const auto err = resc_mgr.is_coordinating_resource(leaf.data(), is_coord_resc); !err.ok()) {
            irods::log(LOG_ERROR, err.result());
            return err.code();
        }

        // Leaf resources cannot be coordinating resources. This essentially checks
        // if the resource has any child resources which is exactly what we're interested in.
        if (is_coord_resc) {
            irods::log(LOG_ERROR, fmt::format("[{}] is not a leaf resource.", leaf));
            return USER_INVALID_RESC_INPUT;
        }

        if (const auto err = resc_mgr.get_hier_to_root_for_resc(leaf.data(), hier); !err.ok()) {
            irods::log(LOG_ERROR, err.result());
            return err.code();
        }

        kvp[RESC_HIER_STR_KW] = hier;
    }

    // =-=-=-=-=-=-=-
    // determine the resource hierarchy if one is not provided
    if (!kvp.contains(RESC_HIER_STR_KW)) {
        try {
            auto result = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, rsComm, *dataObjChksumInp);
            kvp[RESC_HIER_STR_KW] = std::get<std::string>(result);
        }
        catch (const irods::exception& e ) {
            irods::log(e);
            return e.code();
        }
    } // if keyword
    status = _rsDataObjChksum(rsComm, dataObjChksumInp, outChksum, &dataObjInfoHead);

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

    int status = _dataObjChksum( rsComm, dataObjInfo, outChksumStr );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "%s: _dataObjChksum error for %s, status = %d",
                 __FUNCTION__, dataObjInfo->objPath, status );
        return status;
    }

    if ( dataObjInfo->specColl != NULL ) {
        return status;
    }

    keyValPair_t regParam{};
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
    modDataObjMeta_t modDataObjMetaInp{};
    modDataObjMetaInp.dataObjInfo = dataObjInfo;
    modDataObjMetaInp.regParam = &regParam;
    status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
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
