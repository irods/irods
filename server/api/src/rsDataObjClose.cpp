/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjClose.h for a description of this API call.*/


#include "dataObjClose.h"
#include "key_value_proxy.hpp"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "regReplica.h"
#include "modDataObjMeta.h"
#include "modAVUMetadata.h"
#include "dataObjOpr.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "resource.hpp"
#include "dataObjUnlink.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "ruleExecSubmit.h"
#include "subStructFileRead.h"
#include "subStructFileStat.h"
#include "subStructFileClose.h"
#include "regDataObj.h"
#include "dataObjRepl.h"
#include "dataObjTrim.h"
#include "dataObjLock.h"
#include "fileClose.h"
#include "fileStat.h"
#include "getRescQuota.h"
#include "miscServerFunct.hpp"
#include "rsDataObjClose.hpp"
#include "apiNumber.h"
#include "rsModDataObjMeta.hpp"
#include "rsDataObjTrim.hpp"
#include "rsDataObjUnlink.hpp"
#include "rsRegReplica.hpp"
#include "rsGetRescQuota.hpp"
#include "rsSubStructFileClose.hpp"
#include "rsFileClose.hpp"
#include "rsRegDataObj.hpp"
#include "rsSubStructFileStat.hpp"
#include "rsFileStat.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_file_object.hpp"
#include "irods_exception.hpp"
#include "irods_serialization.hpp"
#include "irods_server_api_call.hpp"
#include "irods_server_properties.hpp"
#include "irods_at_scope_exit.hpp"
#include "replica_access_table.hpp"

#include <memory>
#include <functional>

#include <sys/types.h>
#include <unistd.h>

namespace{
    int trimDataObjInfo(
        rsComm_t*      rsComm,
        dataObjInfo_t* dataObjInfo ) {
        // =-=-=-=-=-=-=-
        // add the hier to a parser to get the leaf
        //std::string cache_resc = irods::hierarchy_parser{dataObjInfo->rescHier}.last_resc();

        dataObjInp_t dataObjInp{};
        rstrcpy( dataObjInp.objPath,  dataObjInfo->objPath, MAX_NAME_LEN );
        char tmpStr[NAME_LEN]{};
        snprintf( tmpStr, NAME_LEN, "1" );
        addKeyVal( &dataObjInp.condInput, COPIES_KW, tmpStr );

        // =-=-=-=-=-=-=-
        // specify the cache repl num to trim just the cache
        addKeyVal( &dataObjInp.condInput, REPL_NUM_KW, std::to_string(dataObjInfo->replNum).c_str() );
        addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, dataObjInfo->rescHier );

        int status = rsDataObjTrim( rsComm, &dataObjInp );
        clearKeyVal( &dataObjInp.condInput );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "%s: error trimming obj info for [%s]", __FUNCTION__, dataObjInfo->objPath );
        }
        return status;
    }
}

int rsDataObjClose(rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp)
{
    if (dataObjCloseInp->l1descInx <= 0) {
        return SYS_BAD_FILE_DESCRIPTOR;
    }

    const auto& l1desc = L1desc[dataObjCloseInp->l1descInx];
    std::unique_ptr<irods::at_scope_exit<std::function<void()>>> restore_entry;
    int ec = 0;

    // Replica access tokens only apply to write operations.
    if ((l1desc.dataObjInp->openFlags & O_ACCMODE) != O_RDONLY) {
        namespace ix = irods::experimental;

        if (!l1desc.replica_token.empty()) {
            // Capture the replica token and erase the PID from the replica access table.
            // This must always happen before calling "irsDataObjClose" because other operations
            // may attempt to open this replica, but will fail because those operations do not
            // have the replica token.
            if (auto entry = ix::replica_access_table::instance().erase_pid(l1desc.replica_token, getpid()); entry) {
                // "entry" should always be populated in normal situations.
                // Because closing a data object triggers a file modified notification, it is
                // important to be able to restore the previously removed replica access table entry.
                // This is required so that the iRODS state is maintained in relation to the client.
                restore_entry.reset(new irods::at_scope_exit<std::function<void()>>{
                    [&ec, e = std::move(*entry)] {
                        if (ec != 0) {
                            ix::replica_access_table::instance().restore(e);
                        }
                    }
                });
            }
        }
        else {
            rodsLog(LOG_WARNING, "No replica access token in L1 descriptor. Ignoring replica access table. "
                                 "[path=%s, resource_hierarchy=%s]",
                                 l1desc.dataObjInfo->objPath, l1desc.dataObjInfo->rescHier);
        }
    }

    // Capture the error code so that the at_scope_exit handler can respond to it.
    return ec = irsDataObjClose(rsComm, dataObjCloseInp, nullptr);
}

int irsDataObjClose(rsComm_t* rsComm,
                    openedDataObjInp_t* dataObjCloseInp,
                    dataObjInfo_t** outDataObjInfo)
{
    int status;
    ruleExecInfo_t rei;
    int l1descInx = dataObjCloseInp->l1descInx;
    if ( l1descInx <= 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjClose: l1descInx %d out of range",
                 l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
    }

    // ensure that l1 descriptor is in use before closing
    if ( L1desc[l1descInx].inuseFlag != FD_INUSE ) {
        rodsLog(
            LOG_ERROR,
            "rsDataObjClose: l1descInx %d out of range",
            l1descInx );
        return BAD_INPUT_DESC_INDEX;
    }

    // sanity check for in-flight l1 descriptor
    if( !L1desc[l1descInx].dataObjInp ) {
        rodsLog(
            LOG_ERROR,
            "rsDataObjClose: invalid dataObjInp for index %d",
            l1descInx );
        return SYS_INVALID_INPUT_PARAM;
    }

    if ( outDataObjInfo != NULL ) {
        *outDataObjInfo = NULL;
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        /* cross zone operation */
        dataObjCloseInp->l1descInx = L1desc[l1descInx].remoteL1descInx;
        /* XXXXX outDataObjInfo will not be returned in this call.
         * The only call that requires outDataObjInfo to be returned is
         * _rsDataObjReplS which does a remote repl early for cross zone */
        status = rcDataObjClose( L1desc[l1descInx].remoteZoneHost->conn, dataObjCloseInp );
        dataObjCloseInp->l1descInx = l1descInx;
    }
    else {
        status = _rsDataObjClose( rsComm, dataObjCloseInp );
        // =-=-=-=-=-=-=-
        // JMC - backport 4640
        if ( L1desc[l1descInx].lockFd > 0 ) {
            char fd_string[NAME_LEN];
            snprintf( fd_string, sizeof( fd_string ), "%-d", L1desc[l1descInx].lockFd );
            addKeyVal( &L1desc[l1descInx].dataObjInp->condInput, LOCK_FD_KW, fd_string );
            irods::server_api_call(
                DATA_OBJ_UNLOCK_AN,
                rsComm,
                L1desc[l1descInx].dataObjInp,
                NULL,
                ( void** ) NULL,
                NULL );

            L1desc[l1descInx].lockFd = -1;
        }
        // =-=-=-=-=-=-=-

        if ( status >= 0 && L1desc[l1descInx].oprStatus >= 0 ) {
            /* note : this may overlap with acPostProcForPut or
             * acPostProcForCopy */
            if ( L1desc[l1descInx].openType == CREATE_TYPE ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;

                // make resource properties available as rule session variables
                irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

                rei.status = applyRule( "acPostProcForCreate", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);
            }
            else if ( L1desc[l1descInx].openType == OPEN_FOR_READ_TYPE ||
                      L1desc[l1descInx].openType == OPEN_FOR_WRITE_TYPE ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;

                // make resource properties available as rule session variables
                irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

                rei.status = applyRule( "acPostProcForOpen", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                // =-=-=-=-=-=-=-
                // JMC - bacport 4623
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);
            }
            else if ( L1desc[l1descInx].oprType == REPLICATE_DEST ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;
                rei.status = applyRule( "acPostProcForRepl", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                // =-=-=-=-=-=-=-
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);
           }

            if ( L1desc[l1descInx].oprType == COPY_DEST ) {
                /* have to put copy first because the next test could
                 * trigger put rule for copy operation */
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;

                // make resource properties available as rule session variables
                irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

                rei.status = applyRule( "acPostProcForCopy", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);
            }
            else if ( L1desc[l1descInx].oprType == PUT_OPR ||
                      L1desc[l1descInx].openType == CREATE_TYPE ||
                      ( L1desc[l1descInx].openType == OPEN_FOR_WRITE_TYPE &&
                        ( L1desc[l1descInx].bytesWritten > 0 ||
                          dataObjCloseInp->bytesWritten > 0 ) ) ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;

                // make resource properties available as rule session variables
                irods::get_resc_properties_as_kvp(rei.doi->rescHier, rei.condInputData);

                rei.status = applyRule( "acPostProcForPut", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);
            }
            else if ( L1desc[l1descInx].dataObjInp != NULL &&
                      L1desc[l1descInx].dataObjInp->oprType == PHYMV_OPR ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;
                rei.status = applyRule( "acPostProcForPhymv", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                clearKeyVal(rei.condInputData);
                free(rei.condInputData);

            }
        }
    }

    if ( outDataObjInfo ) {
        *outDataObjInfo = L1desc[l1descInx].dataObjInfo;
        L1desc[l1descInx].dataObjInfo = NULL;
    }

    freeL1desc( l1descInx );

    return status;
}

int _modDataObjSize(
    rsComm_t*      _comm,
    int            _l1descInx,
    dataObjInfo_t* _info ) {

    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    memset( &regParam, 0, sizeof( regParam ) );
    char tmpStr[MAX_NAME_LEN];
    snprintf( tmpStr, sizeof( tmpStr ), "%ji", ( intmax_t ) _info->dataSize );
    addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
    addKeyVal( &regParam, IN_PDMO_KW, _info->rescHier ); // to stop resource hierarchy recursion
    if ( getValByKey(
            &L1desc[_l1descInx].dataObjInp->condInput,
            ADMIN_KW ) != NULL ) {
        addKeyVal( &regParam, ADMIN_KW, "" );
    }
    char* repl_status = getValByKey(&L1desc[_l1descInx].dataObjInp->condInput, REPL_STATUS_KW);
    if (repl_status) {
        addKeyVal(&regParam, REPL_STATUS_KW, repl_status);
    }

    if (getValByKey(&L1desc[_l1descInx].dataObjInp->condInput, STALE_ALL_INTERMEDIATE_REPLICAS_KW)) {
        addKeyVal(&regParam, STALE_ALL_INTERMEDIATE_REPLICAS_KW, "");
    }

    modDataObjMetaInp.dataObjInfo = _info;
    modDataObjMetaInp.regParam = &regParam;
    int status = rsModDataObjMeta( _comm, &modDataObjMetaInp );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s: rsModDataObjMeta failed, dataSize [%d] status = %d",
                 __FUNCTION__, _info->dataSize, status );
    }
    return status;
}

int
_rsDataObjClose(
    rsComm_t *rsComm,
    openedDataObjInp_t *dataObjCloseInp ) {
    int status = 0;
    char tmpStr[MAX_NAME_LEN];
    modDataObjMeta_t modDataObjMetaInp;
    int updateChksumFlag = 0;

    int l1descInx = dataObjCloseInp->l1descInx;
    int l3descInx = L1desc[l1descInx].l3descInx;

    // Store openType in key/val in case a hop occurs
    keyValPair_t regParam{};
    addKeyVal(&regParam, OPEN_TYPE_KW, std::to_string(L1desc[l1descInx].openType).c_str());

    if ( l3descInx > 2 ) {
        /* it could be -ive for parallel I/O */
        status = l3Close( rsComm, l1descInx );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "%s: l3Close of %d failed, status = %d",
                     __FUNCTION__, l3descInx, status );
            return status;
        }
    }

    if ( L1desc[l1descInx].oprStatus < 0 ) {
        // #3674 - elide any additional errors for catalog update if this is an intermediate replica
        if(INTERMEDIATE_REPLICA == L1desc[l1descInx].replStatus) {
            if( L1desc[l1descInx].oprType == REPLICATE_OPR ||
                L1desc[l1descInx].oprType == REPLICATE_DEST ||
                L1desc[l1descInx].oprType == REPLICATE_SRC ) {
                // Make change here if we want to stop replication
                return L1desc[l1descInx].oprStatus;
            }
        }

        const rodsLong_t vault_size = getSizeInVault(
                                          rsComm,
                                          L1desc[l1descInx].dataObjInfo );
        if ( vault_size < 0 ) {
            rodsLog( LOG_ERROR,
                     "%s - getSizeInVault failed [%ld]",
                     __FUNCTION__, vault_size );
            return vault_size;
        }

        //if ( L1desc[l1descInx].dataObjInfo->dataSize != vault_size ) {
            addKeyVal(&L1desc[l1descInx].dataObjInp->condInput, REPL_STATUS_KW,
                std::to_string(STALE_REPLICA).c_str());
            addKeyVal(&L1desc[l1descInx].dataObjInp->condInput, STALE_ALL_INTERMEDIATE_REPLICAS_KW, "");
            L1desc[l1descInx].dataObjInfo->dataSize = vault_size;
            int status = _modDataObjSize(
                             rsComm,
                             l1descInx,
                             L1desc[l1descInx].dataObjInfo );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "%s - _modDataObjSize failed [%d]",
                         __FUNCTION__, status );
                return status;
            }
        //}

        /* an error has occurred */
        return L1desc[l1descInx].oprStatus;
    }

    if ( dataObjCloseInp->bytesWritten > 0 &&
            L1desc[l1descInx].bytesWritten <= 0 ) {
        /* dataObjCloseInp->bytesWritten is used to specify bytesWritten
         * for cross zone operation */
        L1desc[l1descInx].bytesWritten = dataObjCloseInp->bytesWritten;
    }

    /* note that bytesWritten only indicates whether the file has been written
     * to. Not necessarily the size of the file */
    if ( L1desc[l1descInx].bytesWritten < 0 &&
            L1desc[l1descInx].oprType != REPLICATE_DEST &&
            L1desc[l1descInx].oprType != PHYMV_DEST &&
            L1desc[l1descInx].oprType != COPY_DEST ) {
        /* no write */
        // =-=-=-=-=-=-=-
        // JMC - backport 4537
        if ( L1desc[l1descInx].purgeCacheFlag > 0 ) {
            int trim_status = trimDataObjInfo(rsComm, L1desc[l1descInx].dataObjInfo);
            if (trim_status < 0) {
                rodsLogError(LOG_ERROR, trim_status,
                             "%s: trimDataObjInfo error for %s",
                             __FUNCTION__, L1desc[l1descInx].dataObjInfo->objPath);
            }
        }

        try {
            applyMetadataFromKVP(rsComm, L1desc[l1descInx].dataObjInp);
            applyACLFromKVP(rsComm, L1desc[l1descInx].dataObjInp);
        }
        catch ( const irods::exception& e ) {
            rodsLog( LOG_ERROR, "%s", e.what() );
            if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
            }
            return e.code();
        }

        if ( L1desc[l1descInx].chksumFlag != 0 && L1desc[l1descInx].oprType == PUT_OPR ) {
            char *chksumStr = NULL;
            status = procChksumForClose( rsComm, l1descInx, &chksumStr );
            if (status >= 0) {
                std::string checksum = std::string( chksumStr );
                free( chksumStr );

                if ( !checksum.empty() ) {
                    addKeyVal( &regParam, CHKSUM_KW, checksum.c_str() );
                }

                modDataObjMetaInp.dataObjInfo = L1desc[l1descInx].dataObjInfo;
                modDataObjMetaInp.regParam = &regParam;
                status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
            }
        }

        return status;
    }

    int noChkCopyLenFlag = 0;
    if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                      NO_CHK_COPY_LEN_KW ) != NULL ) {
        noChkCopyLenFlag = 1;
    }

    rodsLong_t newSize = getSizeInVault( rsComm, L1desc[l1descInx].dataObjInfo );

    // since we are not filtering out writes to archive resources, the
    // archive plugins report UNKNOWN_FILE_SZ as their size since they may
    // not be able to stat the file.  filter that out and trust the plugin
    // in this instance
    if ( newSize == UNKNOWN_FILE_SZ && L1desc[l1descInx].dataSize >= 0 ) {
        newSize = L1desc[l1descInx].dataSize;
    }
    /* check for consistency of the write operation */
    else if ( newSize < 0 && newSize != UNKNOWN_FILE_SZ ) {
        status = ( int ) newSize;
        rodsLog( LOG_ERROR,
                 "_rsDataObjClose: getSizeInVault error for %s, status = %d",
                 L1desc[l1descInx].dataObjInfo->objPath, status );
        return status;
    }
    else if ( L1desc[l1descInx].dataSize > 0 ) {
        if ( newSize != L1desc[l1descInx].dataSize && noChkCopyLenFlag == 0 ) {
            rodsLog( LOG_NOTICE,
                     "_rsDataObjClose: size in vault %lld != target size %lld",
                     newSize, L1desc[l1descInx].dataSize );
            return SYS_COPY_LEN_ERR;
        }
    }

    // If an object with a checksum was written to, checksum needs updating
    if ( ( OPEN_FOR_WRITE_TYPE == L1desc[l1descInx].openType ||
            CREATE_TYPE == L1desc[l1descInx].openType ) &&
            strlen( L1desc[l1descInx].dataObjInfo->chksum ) > 0 ) {

        L1desc[l1descInx].chksumFlag = REG_CHKSUM;
        updateChksumFlag = 1;
    }

    // need a checksum check
    std::string checksum{};
    if ( !noChkCopyLenFlag || updateChksumFlag ) {
        char *chksumStr = NULL;
        status = procChksumForClose( rsComm, l1descInx, &chksumStr );
        if ( status < 0 ) {
            return status;
        }
        if ( chksumStr != NULL ) {
            checksum = std::string( chksumStr );
            free( chksumStr );
        }
    }

    if (L1desc[l1descInx].oprType == REPLICATE_DEST ||
             L1desc[l1descInx].oprType == PHYMV_DEST ) {
        dataObjInfo_t *destDataObjInfo = L1desc[l1descInx].dataObjInfo;
        int srcL1descInx = L1desc[l1descInx].srcL1descInx;
        if ( srcL1descInx <= 2 ) {
            rodsLog( LOG_NOTICE,
                     "%s: srcL1descInx %d out of range",
                     __FUNCTION__, srcL1descInx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }
        dataObjInfo_t *srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;

        snprintf( tmpStr, MAX_NAME_LEN, "%d", srcDataObjInfo->replStatus );
        addKeyVal( &regParam, REPL_STATUS_KW, tmpStr );
        snprintf( tmpStr, MAX_NAME_LEN, "%lld", srcDataObjInfo->dataSize );
        addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
        snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
        addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );
        if ( !checksum.empty() ) {
            addKeyVal( &regParam, CHKSUM_KW, checksum.c_str() );
        }

        if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                          ADMIN_KW ) != NULL ) {
            addKeyVal( &regParam, ADMIN_KW, "" );
        }
        addKeyVal( &regParam, FILE_PATH_KW, destDataObjInfo->filePath );
        char* pdmo_kw = getValByKey( &dataObjCloseInp->condInput, IN_PDMO_KW );
        if ( pdmo_kw != NULL ) {
            addKeyVal( &regParam, IN_PDMO_KW, pdmo_kw );
        }
        char* sync = getValByKey( &L1desc[l1descInx].dataObjInp->condInput, SYNC_OBJ_KW );
        if (sync) {
            addKeyVal( &regParam, SYNC_OBJ_KW, "" );
        }
        if (L1desc[l1descInx].oprType == PHYMV_DEST) {
            addKeyVal(&regParam, REPL_NUM_KW, std::to_string(srcDataObjInfo->replNum).c_str());
        }
        modDataObjMetaInp.dataObjInfo = destDataObjInfo;
        modDataObjMetaInp.regParam = &regParam;
        status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );

        if (CREATE_TYPE == L1desc[l1descInx].openType) {
            /* update quota overrun */
            updatequotaOverrun( destDataObjInfo->rescHier, destDataObjInfo->dataSize, ALL_QUOTA );
        }

        clearKeyVal( &regParam );

        if ( status < 0 ) {
            L1desc[l1descInx].oprStatus = status;
            // =-=-=-=-=-=-=-
            // JMC - backport 4608
            /* don't delete replica with the same filePath */
            if ( status != CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
                l3Unlink( rsComm, L1desc[l1descInx].dataObjInfo );
            }
            // =-=-=-=-=-=-=-
            rodsLog( LOG_NOTICE,
                     "%s: RegReplica/ModDataObjMeta %s err. stat = %d",
                     __FUNCTION__, destDataObjInfo->objPath, status );
            return status;
        }
    }
    else if ( L1desc[l1descInx].dataObjInfo->specColl == NULL ) {
        /* put or copy */
        if ( l3descInx < 2 &&
                getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                             CROSS_ZONE_CREATE_KW ) != NULL &&
                INTERMEDIATE_REPLICA == L1desc[l1descInx].replStatus ) {
            /* the comes from a cross zone copy. have not been
             * registered yet */
            status = svrRegDataObj( rsComm, L1desc[l1descInx].dataObjInfo );
            if ( status < 0 ) {
                L1desc[l1descInx].oprStatus = status;
                rodsLog( LOG_NOTICE,
                         "%s: svrRegDataObj for %s failed, status = %d",
                         __FUNCTION__, L1desc[l1descInx].dataObjInfo->objPath, status );
            }
        }

        if ( L1desc[l1descInx].dataObjInfo->dataSize != newSize ) {
            snprintf( tmpStr, MAX_NAME_LEN, "%lld", newSize );
            addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
            /* update this in case we need to replicate it */
            L1desc[l1descInx].dataObjInfo->dataSize = newSize;
        }

        if ( !checksum.empty() ) {
            addKeyVal( &regParam, CHKSUM_KW, checksum.c_str() );
        }

        if (OPEN_FOR_WRITE_TYPE == L1desc[l1descInx].openType) {
            addKeyVal( &regParam, ALL_REPL_STATUS_KW, "TRUE" );
            snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
            addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );
        }
        else {
            snprintf( tmpStr, MAX_NAME_LEN, "%d", GOOD_REPLICA );
            addKeyVal( &regParam, REPL_STATUS_KW, tmpStr );
        }
        modDataObjMetaInp.dataObjInfo = L1desc[l1descInx].dataObjInfo;
        modDataObjMetaInp.regParam = &regParam;
        status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        clearKeyVal( &regParam );

        if ( status < 0 ) {
            return status;
        }

        try {
            applyACLFromKVP(rsComm, L1desc[l1descInx].dataObjInp);
            applyMetadataFromKVP(rsComm, L1desc[l1descInx].dataObjInp);
        }
        catch ( const irods::exception& e ) {
            rodsLog( LOG_ERROR, "%s", e.what() );
            if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
            }
            return e.code();
        }

        if ( L1desc[l1descInx].replStatus == GOOD_REPLICA ) {
            /* update quota overrun */
            updatequotaOverrun( L1desc[l1descInx].dataObjInfo->rescHier,
                                newSize, ALL_QUOTA );
        }
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4537
    /* purge the cache copy */
    if ( L1desc[l1descInx].purgeCacheFlag > 0 ) {
        int trim_status = trimDataObjInfo(rsComm, L1desc[l1descInx].dataObjInfo);
        if (trim_status < 0) {
            rodsLogError(LOG_ERROR, trim_status,
                    "%s: trimDataObjInfo error for %s",
                    __FUNCTION__, L1desc[l1descInx].dataObjInfo->objPath);
        }
    }

    /* for post processing */
    L1desc[l1descInx].bytesWritten =
        L1desc[l1descInx].dataObjInfo->dataSize = newSize;

    return status;
}

int
l3Close( rsComm_t *rsComm, int l1descInx ) {
    fileCloseInp_t fileCloseInp;
    int status = 0;
    dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Close - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }


    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subStructFileFdOprInp_t subStructFileCloseInp;
        memset( &subStructFileCloseInp, 0, sizeof( subStructFileCloseInp ) );
        subStructFileCloseInp.type = dataObjInfo->specColl->type;
        subStructFileCloseInp.fd = L1desc[l1descInx].l3descInx;
        rstrcpy( subStructFileCloseInp.addr.hostAddr, location.c_str(), NAME_LEN );
        rstrcpy( subStructFileCloseInp.resc_hier, dataObjInfo->rescHier, MAX_NAME_LEN );
        status = rsSubStructFileClose( rsComm, &subStructFileCloseInp );
    }
    else {
        memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
        fileCloseInp.fileInx = L1desc[l1descInx].l3descInx;
        rstrcpy( fileCloseInp.in_pdmo, L1desc[l1descInx].in_pdmo, MAX_NAME_LEN );
        status = rsFileClose( rsComm, &fileCloseInp );

    }
    return status;
}

int
_l3Close( rsComm_t *rsComm, int l3descInx ) {
    fileCloseInp_t fileCloseInp;
    int status;

    memset( &fileCloseInp, 0, sizeof( fileCloseInp ) );
    fileCloseInp.fileInx = l3descInx;
    status = rsFileClose( rsComm, &fileCloseInp );

    return status;
}

int
l3Stat( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, rodsStat_t **myStat ) {
    fileStatInp_t fileStatInp;
    int status;

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {

        std::string location;
        irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
        if ( !ret.ok() ) {
            irods::log( PASSMSG( "l3Stat - failed in get_loc_for_hier_string", ret ) );
            return -1;
        }

        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        subFile.specColl = dataObjInfo->specColl;
        status = rsSubStructFileStat( rsComm, &subFile, myStat );
    }
    else {
        memset( &fileStatInp, 0, sizeof( fileStatInp ) );
        rstrcpy( fileStatInp.fileName, dataObjInfo->filePath,
                 MAX_NAME_LEN );
        //rstrcpy( fileStatInp.addr.hostAddr, dataObjInfo->rescInfo->rescLoc, NAME_LEN );
        rstrcpy( fileStatInp.rescHier, dataObjInfo->rescHier, MAX_NAME_LEN );
        rstrcpy( fileStatInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
        status = rsFileStat( rsComm, &fileStatInp, myStat );
    }
    return status;
}

/* procChksumForClose - handle checksum issues on close. Returns a non-null
 * chksumStr if it needs to be registered.
 */
int
procChksumForClose(
    rsComm_t *rsComm,
    int l1descInx,
    char **chksumStr ) {
    int status = 0;
    dataObjInfo_t *dataObjInfo = L1desc[l1descInx].dataObjInfo;
    int oprType = L1desc[l1descInx].oprType;
    int srcL1descInx = 0;
    dataObjInfo_t *srcDataObjInfo;

    *chksumStr = NULL;
    if ( oprType == REPLICATE_DEST || oprType == PHYMV_DEST ) {
        srcL1descInx = L1desc[l1descInx].srcL1descInx;
        if ( srcL1descInx <= 2 ) {
            rodsLog( LOG_NOTICE,
                     "procChksumForClose: srcL1descInx %d out of range",
                     srcL1descInx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }

        srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
        if ( strlen( srcDataObjInfo->chksum ) > 0 ) {
            addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, srcDataObjInfo->chksum );
        }

        if ( strlen( srcDataObjInfo->chksum ) > 0 &&
                srcDataObjInfo->replStatus > 0 ) {
            /* the source has chksum. Must verify chksum */
            status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );
            if ( status < 0 ) {
                dataObjInfo->chksum[0] = '\0';
                if ( status == DIRECT_ARCHIVE_ACCESS ) {
                    *chksumStr = strdup( srcDataObjInfo->chksum );
                    rstrcpy( dataObjInfo->chksum, *chksumStr, NAME_LEN );
                    return 0;
                }
                else {
                    rodsLog( LOG_NOTICE,
                             "procChksumForClose: _dataObjChksum error for %s, status = %d",
                             dataObjInfo->objPath, status );
                    return status;
                }
            }
            else if ( *chksumStr == NULL ) {
                rodsLog( LOG_ERROR, "chksumStr is NULL" );
                return SYS_INTERNAL_NULL_INPUT_ERR;
            }
            else {
                rstrcpy( dataObjInfo->chksum, *chksumStr, NAME_LEN );
                if ( strcmp( srcDataObjInfo->chksum, *chksumStr ) != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "procChksumForClose: chksum mismatch for %s src [%s] new [%s]",
                             dataObjInfo->objPath, srcDataObjInfo->chksum, *chksumStr );
                    free( *chksumStr );
                    *chksumStr = NULL;
                    return USER_CHKSUM_MISMATCH;
                }
                else {
                    return 0;
                }
            }
        }
    }

    /* overwriting an old copy. need to verify the chksum again */
    if ( strlen( L1desc[l1descInx].dataObjInfo->chksum ) > 0 && !L1desc[l1descInx].chksumFlag ) {
        L1desc[l1descInx].chksumFlag = VERIFY_CHKSUM;
    }

    if ( L1desc[l1descInx].chksumFlag == 0 ) {
        return 0;
    }
    else if ( L1desc[l1descInx].chksumFlag == VERIFY_CHKSUM ) {
        if ( strlen( L1desc[l1descInx].chksum ) > 0 ) {
            if ( strlen( L1desc[l1descInx].chksum ) > 0 ) {
                addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, L1desc[l1descInx].chksum );
            }

            status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );

            rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );
            if ( status < 0 ) {
                return status;
            }
            /* from a put type operation */
            /* verify against the input value. */
            if ( strcmp( L1desc[l1descInx].chksum, *chksumStr ) != 0 ) {
                rodsLog( LOG_NOTICE,
                         "procChksumForClose: mismatch chksum for %s.inp=%s,compute %s",
                         dataObjInfo->objPath,
                         L1desc[l1descInx].chksum, *chksumStr );
                free( *chksumStr );
                *chksumStr = NULL;
                return USER_CHKSUM_MISMATCH;
            }

            if ( strcmp( dataObjInfo->chksum, *chksumStr ) == 0 ) {
                /* the same as in rcat */
                free( *chksumStr );
                *chksumStr = NULL;
            }
        }
        else if ( oprType == REPLICATE_DEST ) {
            if ( strlen( dataObjInfo->chksum ) > 0 ) {
                addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, dataObjInfo->chksum );

            }
            status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );
            if ( status < 0 ) {
                return status;
            }
            if ( *chksumStr == NULL ) {
                rodsLog( LOG_ERROR, "chksumStr is NULL" );
                return SYS_INTERNAL_NULL_INPUT_ERR;
            }

            if ( strlen( dataObjInfo->chksum ) > 0 ) {
                rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );

                /* for replication, the chksum in dataObjInfo was duplicated */
                if ( strcmp( dataObjInfo->chksum, *chksumStr ) != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "procChksumForClose:mismach chksum for %s.Rcat=%s,comp %s",
                             dataObjInfo->objPath, dataObjInfo->chksum, *chksumStr );
                    status = USER_CHKSUM_MISMATCH;
                }
                else {
                    /* not need to register because reg repl will do it */
                    free( *chksumStr );
                    *chksumStr = NULL;
                    status = 0;
                }
            }
            return status;
        }
        else if ( oprType == COPY_DEST ) {
            /* created through copy */
            srcL1descInx = L1desc[l1descInx].srcL1descInx;
            if ( srcL1descInx <= 2 ) {
                /* not a valid srcL1descInx */
                rodsLog( LOG_DEBUG,
                         "procChksumForClose: invalid srcL1descInx %d for copy",
                         srcL1descInx );
                /* just register it for now */
                return 0;
            }
            srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
            if ( strlen( srcDataObjInfo->chksum ) > 0 ) {
                addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, srcDataObjInfo->chksum );

            }
            status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );
            if ( status < 0 ) {
                return status;
            }
            if ( *chksumStr == NULL ) {
                rodsLog( LOG_ERROR, "chkSumStr is null." );
                return SYS_INTERNAL_NULL_INPUT_ERR;
            }
            if ( strlen( srcDataObjInfo->chksum ) > 0 ) {
                rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );
                if ( strcmp( srcDataObjInfo->chksum, *chksumStr ) != 0 ) {
                    rodsLog( LOG_NOTICE,
                             "procChksumForClose:mismach chksum for %s.Rcat=%s,comp %s",
                             dataObjInfo->objPath, srcDataObjInfo->chksum, *chksumStr );
                    free( *chksumStr );
                    *chksumStr = NULL;
                    return USER_CHKSUM_MISMATCH;
                }
            }
            /* just register it */
            return 0;
        }
    }
    else {	/* REG_CHKSUM */
        if ( strlen( L1desc[l1descInx].chksum ) > 0 ) {
            addKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW, L1desc[l1descInx].chksum );
        }

        status = _dataObjChksum( rsComm, dataObjInfo, chksumStr );
        if ( status < 0 ) {
            return status;
        }

        if ( strlen( L1desc[l1descInx].chksum ) > 0 ) {
            rmKeyVal( &dataObjInfo->condInput, ORIG_CHKSUM_KW );
            /* from a put type operation */

            if ( strcmp( dataObjInfo->chksum, L1desc[l1descInx].chksum ) == 0 ) {
                /* same as in icat */
                free( *chksumStr );
                *chksumStr = NULL;
            }
            return 0;
        }
        else if ( oprType == COPY_DEST ) {
            /* created through copy */
            srcL1descInx = L1desc[l1descInx].srcL1descInx;
            if ( srcL1descInx <= 2 ) {
                /* not a valid srcL1descInx */
                rodsLog( LOG_DEBUG,
                         "procChksumForClose: invalid srcL1descInx %d for copy",
                         srcL1descInx );
                return 0;
            }
            srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
            if ( strlen( srcDataObjInfo->chksum ) == 0 ) {
                free( *chksumStr );
                *chksumStr = NULL;
            }
        }
        return 0;
    }
    return status;
}
