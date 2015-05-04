/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjClose.h for a description of this API call.*/


#include "dataObjClose.h"
#include "rodsLog.h"
#include "reFuncDefs.hpp"
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
#include "reGlobalsExtern.hpp"
#include "reDefines.h"
#include "ruleExecSubmit.h"
#include "subStructFileRead.h"
#include "subStructFileStat.h"
#include "subStructFileClose.h"
#include "regDataObj.h"
#include "dataObjRepl.h"
#include "dataObjTrim.h" // JMC - backport 4537
#include "dataObjLock.h" // JMC - backport 4604
#include "fileClose.h"
#include "fileStat.h"
#include "getRescQuota.h"


// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_stacktrace.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_file_object.hpp"
#include "irods_serialization.hpp"
#include "irods_exception.hpp"
#include "irods_server_api_call.hpp"


int
rsDataObjClose( rsComm_t *rsComm, openedDataObjInp_t *dataObjCloseInp ) {
    int status = irsDataObjClose( rsComm, dataObjCloseInp, NULL );

    return status;
}

int
irsDataObjClose(
    rsComm_t *rsComm,
    openedDataObjInp_t *dataObjCloseInp,
    dataObjInfo_t **outDataObjInfo ) {
    int status;
    int srcL1descInx;
    openedDataObjInp_t myDataObjCloseInp;
    int l1descInx;
    ruleExecInfo_t rei;
    l1descInx = dataObjCloseInp->l1descInx;
    if ( l1descInx <= 2 || l1descInx >= NUM_L1_DESC ) {
        rodsLog( LOG_NOTICE,
                 "rsDataObjClose: l1descInx %d out of range",
                 l1descInx );
        return SYS_FILE_DESC_OUT_OF_RANGE;
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
                rei.status = applyRule( "acPostProcForCreate", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
            }
            else if ( L1desc[l1descInx].openType == OPEN_FOR_READ_TYPE ||
                      L1desc[l1descInx].openType == OPEN_FOR_WRITE_TYPE ) {
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;
                rei.status = applyRule( "acPostProcForOpen", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
                // =-=-=-=-=-=-=-
                // JMC - bacport 4623
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
            }

            if ( L1desc[l1descInx].oprType == COPY_DEST ) {
                /* have to put copy first because the next test could
                 * trigger put rule for copy operation */
                initReiWithDataObjInp( &rei, rsComm,
                                       L1desc[l1descInx].dataObjInp );
                rei.doi = L1desc[l1descInx].dataObjInfo;
                rei.status = status;
                rei.status = applyRule( "acPostProcForCopy", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
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
                rei.status = applyRule( "acPostProcForPut", NULL, &rei,
                                        NO_SAVE_REI );
                /* doi might have changed */
                L1desc[l1descInx].dataObjInfo = rei.doi;
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

            }
        }
    }

    srcL1descInx = L1desc[l1descInx].srcL1descInx;
    if ( ( L1desc[l1descInx].oprType == REPLICATE_DEST ||
            L1desc[l1descInx].oprType == COPY_DEST ) &&
            srcL1descInx > 2 ) {
        memset( &myDataObjCloseInp, 0, sizeof( myDataObjCloseInp ) );
        myDataObjCloseInp.l1descInx = srcL1descInx;
        rsDataObjClose( rsComm, &myDataObjCloseInp );
    }

    if ( outDataObjInfo != NULL ) {
        *outDataObjInfo = L1desc[l1descInx].dataObjInfo;
        L1desc[l1descInx].dataObjInfo = NULL;
    }

    freeL1desc( l1descInx );

    return status;
}

int _modDataObjSize(
    rsComm_t* _comm,
    dataObjInfo_t* _info ) {

    keyValPair_t regParam;
    modDataObjMeta_t modDataObjMetaInp;
    memset( &regParam, 0, sizeof( regParam ) );
    char tmpStr[MAX_NAME_LEN];
    snprintf( tmpStr, sizeof( tmpStr ), "%ji", ( intmax_t ) _info->dataSize );
    addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
    addKeyVal( &regParam, IN_PDMO_KW, _info->rescHier ); // to stop resource hierarchy recursion
    modDataObjMetaInp.dataObjInfo = _info;
    modDataObjMetaInp.regParam = &regParam;
    int status = rsModDataObjMeta( _comm, &modDataObjMetaInp );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "_modDataObjSize: rsModDataObjMeta failed, dataSize [%d] status = %d",
                 _info->dataSize, status );
    }
    return status;
}

int
_rsDataObjClose(
    rsComm_t *rsComm,
    openedDataObjInp_t *dataObjCloseInp ) {
    int status = 0;
    int l1descInx, l3descInx;
    keyValPair_t regParam;
    rodsLong_t newSize;
    char tmpStr[MAX_NAME_LEN];
    modDataObjMeta_t modDataObjMetaInp;
    char *chksumStr = NULL;
    dataObjInfo_t *destDataObjInfo, *srcDataObjInfo;
    int srcL1descInx;
    regReplica_t regReplicaInp;
    int noChkCopyLenFlag = 0;
    int updateChksumFlag = 0;

    l1descInx = dataObjCloseInp->l1descInx;
    l3descInx = L1desc[l1descInx].l3descInx;

    if ( l3descInx > 2 ) {
        /* it could be -ive for parallel I/O */
        status = l3Close( rsComm, l1descInx );

        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "_rsDataObjClose: l3Close of %d failed, status = %d",
                     l3descInx, status );
            return status;
        }
    }

    if ( L1desc[l1descInx].oprStatus < 0 ) {
        const rodsLong_t vault_size = getSizeInVault(
                                          rsComm,
                                          L1desc[l1descInx].dataObjInfo );
        if ( L1desc[l1descInx].dataObjInfo->dataSize != vault_size ) {
            L1desc[l1descInx].dataObjInfo->dataSize = vault_size;
            int status = _modDataObjSize(
                             rsComm,
                             L1desc[l1descInx].dataObjInfo );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsDataObjClose - _modDataObjSize failed [%d]",
                         status );
                return status;
            }
        }

        /* an error has occurred */
        return L1desc[l1descInx].oprStatus;
    }

    if ( dataObjCloseInp->bytesWritten > 0 &&
            L1desc[l1descInx].bytesWritten <= 0 ) {
        /* dataObjCloseInp->bytesWritten is used to specify bytesWritten
         * for cross zone operation */
        L1desc[l1descInx].bytesWritten =
            dataObjCloseInp->bytesWritten;
    }

    /* note that bytesWritten only indicates whether the file has been written
     * to. Not necessarily the size of the file */
    if ( L1desc[l1descInx].bytesWritten <= 0 &&
            L1desc[l1descInx].oprType != REPLICATE_DEST &&
            L1desc[l1descInx].oprType != PHYMV_DEST &&
            L1desc[l1descInx].oprType != COPY_DEST ) {
        /* no write */
        // =-=-=-=-=-=-=-
        // JMC - backport 4537
        if ( L1desc[l1descInx].purgeCacheFlag > 0 ) {
            int status1 = trimDataObjInfo( rsComm,
                                           L1desc[l1descInx].dataObjInfo );
            if ( status1 < 0 ) {
                rodsLogError( LOG_ERROR, status1,
                              "_rsDataObjClose: trimDataObjInfo error for %s",
                              L1desc[l1descInx].dataObjInfo->objPath );
            }
        }

        return status;
    }

    if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                      NO_CHK_COPY_LEN_KW ) != NULL ) {
        noChkCopyLenFlag = 1;
    }

    newSize = getSizeInVault( rsComm, L1desc[l1descInx].dataObjInfo );

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
    if ( !noChkCopyLenFlag || updateChksumFlag ) {
        status = procChksumForClose( rsComm, l1descInx, &chksumStr );
        if ( status < 0 ) {
            return status;
        }
    }

    memset( &regParam, 0, sizeof( regParam ) );
    if ( L1desc[l1descInx].oprType == PHYMV_DEST ) {
        /* a phymv */
        destDataObjInfo = L1desc[l1descInx].dataObjInfo;
        srcL1descInx = L1desc[l1descInx].srcL1descInx;
        if ( srcL1descInx <= 2 ) {
            rodsLog( LOG_NOTICE,
                     "_rsDataObjClose: srcL1descInx %d out of range",
                     srcL1descInx );
            free( chksumStr );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }
        srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;

        if ( chksumStr != NULL ) {
            addKeyVal( &regParam, CHKSUM_KW, chksumStr );
        }
        addKeyVal( &regParam, FILE_PATH_KW,     destDataObjInfo->filePath );
        addKeyVal( &regParam, RESC_NAME_KW,     destDataObjInfo->rescName );
        addKeyVal( &regParam, RESC_HIER_STR_KW, destDataObjInfo->rescHier );
        if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                          ADMIN_KW ) != NULL ) {
            addKeyVal( &regParam, ADMIN_KW, "" );
        }
        char* pdmo_kw = getValByKey( &dataObjCloseInp->condInput, IN_PDMO_KW );
        if ( pdmo_kw != NULL ) {
            addKeyVal( &regParam, IN_PDMO_KW, pdmo_kw );
        }
        modDataObjMetaInp.dataObjInfo = destDataObjInfo;
        modDataObjMetaInp.regParam = &regParam;
        status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        clearKeyVal( &regParam );
        /* have to handle the l3Close here because the need to
         * unlink the srcDataObjInfo */
        if ( status >= 0 ) {
            if ( L1desc[srcL1descInx].l3descInx > 2 ) {
                int status1;
                status1 = l3Close( rsComm, srcL1descInx );
                if ( status1 < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "_rsDataObjClose: l3Close of %s error. status = %d",
                             srcDataObjInfo->objPath, status1 );
                }
            }
            l3Unlink( rsComm, srcDataObjInfo );
            updatequotaOverrun( destDataObjInfo->rescHier,
                                destDataObjInfo->dataSize, RESC_QUOTA );
        }
        else {
            if ( L1desc[srcL1descInx].l3descInx > 2 ) {
                l3Close( rsComm, srcL1descInx );
            }
        }
        freeL1desc( srcL1descInx );
        L1desc[l1descInx].srcL1descInx = 0;
    }
    else if ( L1desc[l1descInx].oprType == REPLICATE_DEST ) {
        destDataObjInfo = L1desc[l1descInx].dataObjInfo;
        srcL1descInx = L1desc[l1descInx].srcL1descInx;
        if ( srcL1descInx <= 2 ) {
            rodsLog( LOG_NOTICE,
                     "_rsDataObjClose: srcL1descInx %d out of range",
                     srcL1descInx );
            free( chksumStr );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }
        srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;

        if ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) {
            /* repl to an existing copy */
            snprintf( tmpStr, MAX_NAME_LEN, "%d", srcDataObjInfo->replStatus );
            addKeyVal( &regParam, REPL_STATUS_KW, tmpStr );
            snprintf( tmpStr, MAX_NAME_LEN, "%lld", srcDataObjInfo->dataSize );
            addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
            snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
            addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );
            if ( chksumStr != NULL ) {
                addKeyVal( &regParam, CHKSUM_KW, chksumStr );
            }

            if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                              ADMIN_KW ) != NULL ) {
                addKeyVal( &regParam, ADMIN_KW, "" );
            }
            if ( ( L1desc[l1descInx].replStatus & FILE_PATH_HAS_CHG ) != 0 ) {
                /* path has changed */
                addKeyVal( &regParam, FILE_PATH_KW, destDataObjInfo->filePath );
            }
            char* pdmo_kw = getValByKey( &dataObjCloseInp->condInput, IN_PDMO_KW );
            if ( pdmo_kw != NULL ) {
                addKeyVal( &regParam, IN_PDMO_KW, pdmo_kw );
            }
            modDataObjMetaInp.dataObjInfo = destDataObjInfo;
            modDataObjMetaInp.regParam = &regParam;
            status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        }
        else {
            /* repl to a new copy */
            memset( &regReplicaInp, 0, sizeof( regReplicaInp ) );
            if ( destDataObjInfo->dataId <= 0 ) {
                destDataObjInfo->dataId = srcDataObjInfo->dataId;
            }
            regReplicaInp.srcDataObjInfo = srcDataObjInfo;
            regReplicaInp.destDataObjInfo = destDataObjInfo;
            if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                              SU_CLIENT_USER_KW ) != NULL ) {
                addKeyVal( &regReplicaInp.condInput, SU_CLIENT_USER_KW, "" );
                addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );
            }
            else if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                                   ADMIN_KW ) != NULL ) {
                addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );
            }

            char* pdmo_kw = getValByKey( &dataObjCloseInp->condInput, IN_PDMO_KW );
            if ( pdmo_kw ) {
                addKeyVal( &regReplicaInp.condInput, IN_PDMO_KW, pdmo_kw );
            }

            status = rsRegReplica( rsComm, &regReplicaInp );
            clearKeyVal( &regReplicaInp.condInput );

            // update datasize in catalog if there is a mismatch between
            // what is expected and what was actually written to the resource
            if ( srcDataObjInfo->dataSize != newSize ) {
                srcDataObjInfo->dataSize = newSize;
                status = _modDataObjSize( rsComm, srcDataObjInfo );
                if ( status < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "_rsDataObjClose: _modDataObjSize srcDataObjInfo failed, status = [%d]", status );
                    free( chksumStr );
                    return status;
                }
            }
            if ( destDataObjInfo->dataSize != newSize ) {
                destDataObjInfo->dataSize = newSize;
                status = _modDataObjSize( rsComm, destDataObjInfo );
                if ( status < 0 ) {
                    rodsLog( LOG_NOTICE,
                             "_rsDataObjClose: _modDataObjSize destDataObjInfo failed, status = [%d]", status );
                    free( chksumStr );
                    return status;
                }
            }

            /* update quota overrun */
            updatequotaOverrun( destDataObjInfo->rescHier,
                                destDataObjInfo->dataSize, ALL_QUOTA );

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
                     "_rsDataObjClose: RegReplica/ModDataObjMeta %s err. stat = %d",
                     destDataObjInfo->objPath, status );
            free( chksumStr );
            return status;
        }
    }
    else if ( L1desc[l1descInx].dataObjInfo->specColl == NULL ) {
        /* put or copy */
        if ( l3descInx < 2 &&
                getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                             CROSS_ZONE_CREATE_KW ) != NULL &&
                L1desc[l1descInx].replStatus == NEWLY_CREATED_COPY ) {
            /* the comes from a cross zone copy. have not been
             * registered yet */
            status = svrRegDataObj( rsComm, L1desc[l1descInx].dataObjInfo );
            if ( status < 0 ) {
                L1desc[l1descInx].oprStatus = status;
                rodsLog( LOG_NOTICE,
                         "_rsDataObjClose: svrRegDataObj for %s failed, status = %d",
                         L1desc[l1descInx].dataObjInfo->objPath, status );
            }
        }

        if ( L1desc[l1descInx].dataObjInfo->dataSize != newSize ) {
            snprintf( tmpStr, MAX_NAME_LEN, "%lld", newSize );
            addKeyVal( &regParam, DATA_SIZE_KW, tmpStr );
            /* update this in case we need to replicate it */
            L1desc[l1descInx].dataObjInfo->dataSize = newSize;
        }

        if ( chksumStr != NULL ) {
            addKeyVal( &regParam, CHKSUM_KW, chksumStr );
        }

        if ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) {
            addKeyVal( &regParam, ALL_REPL_STATUS_KW, tmpStr );
            snprintf( tmpStr, MAX_NAME_LEN, "%d", ( int ) time( NULL ) );
            addKeyVal( &regParam, DATA_MODIFY_KW, tmpStr );
        }
        else {
            snprintf( tmpStr, MAX_NAME_LEN, "%d", NEWLY_CREATED_COPY );
            addKeyVal( &regParam, REPL_STATUS_KW, tmpStr );
        }
        modDataObjMetaInp.dataObjInfo = L1desc[l1descInx].dataObjInfo;
        modDataObjMetaInp.regParam = &regParam;

        status = rsModDataObjMeta( rsComm, &modDataObjMetaInp );
        clearKeyVal( &regParam );

        if ( status < 0 ) {
            free( chksumStr );
            return status;
        }

        if ( const char* serialized_acl = getValByKey( &L1desc[l1descInx].dataObjInp->condInput, ACL_INCLUDED_KW ) ) {
            std::vector<std::vector<std::string> > deserialized_acl;
            try {
                deserialized_acl = irods::deserialize_acl( serialized_acl );
            }
            catch ( const irods::exception& e ) {
                rodsLog( LOG_ERROR, "%s", e.what() );
                if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                    rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
                }
                return e.code();
            }
            for ( std::vector<std::vector<std::string> >::const_iterator iter = deserialized_acl.begin(); iter != deserialized_acl.end(); ++iter ) {
                modAccessControlInp_t modAccessControlInp;
                modAccessControlInp.recursiveFlag = 0;
                modAccessControlInp.accessLevel = strdup ( ( *iter )[0].c_str() );
                modAccessControlInp.userName = ( char * )malloc( sizeof( char ) * NAME_LEN );
                modAccessControlInp.zone = ( char * )malloc( sizeof( char ) * NAME_LEN );
                parseUserName( ( *iter )[1].c_str(), modAccessControlInp.userName, modAccessControlInp.zone );
                modAccessControlInp.path = strdup( L1desc[l1descInx].dataObjInfo->objPath );
                int status = rsModAccessControl( rsComm, &modAccessControlInp );
                clearModAccessControlInp( &modAccessControlInp );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "rsModAccessControl failed in _rsDataObjClose with status %d", status );
                    if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                        rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
                    }
                    return status;
                }
            }
        }

        if ( const char* serialized_metadata = getValByKey( &L1desc[l1descInx].dataObjInp->condInput, METADATA_INCLUDED_KW ) ) {
            std::vector<std::string> deserialized_metadata;
            try {
                deserialized_metadata = irods::deserialize_metadata( serialized_metadata );
            }
            catch ( const irods::exception& e ) {
                rodsLog( LOG_ERROR, "%s", e.what() );
                if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                    rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
                }
                return e.code();
            }
            for ( size_t i = 0; i + 2 < deserialized_metadata.size(); i += 3 ) {
                modAVUMetadataInp_t modAVUMetadataInp;
                memset( &modAVUMetadataInp, 0, sizeof( modAVUMetadataInp ) );

                modAVUMetadataInp.arg0 = strdup( "add" );
                modAVUMetadataInp.arg1 = strdup( "-d" );
                modAVUMetadataInp.arg2 = strdup( L1desc[l1descInx].dataObjInfo->objPath );
                modAVUMetadataInp.arg3 = strdup( deserialized_metadata[i].c_str() );
                modAVUMetadataInp.arg4 = strdup( deserialized_metadata[i + 1].c_str() );
                modAVUMetadataInp.arg5 = strdup( deserialized_metadata[i + 2].c_str() );

                int status = rsModAVUMetadata( rsComm, &modAVUMetadataInp );
                clearModAVUMetadataInp( &modAVUMetadataInp );
                if ( status < 0 ) {
                    rodsLog( LOG_ERROR, "rsModAVUMetadata failed in _rsDataObjClose with status %d", status );
                    if ( L1desc[l1descInx].dataObjInp->oprType == PUT_OPR ) {
                        rsDataObjUnlink( rsComm, L1desc[l1descInx].dataObjInp );
                    }
                    return status;
                }
            }
        }


        if ( L1desc[l1descInx].replStatus == NEWLY_CREATED_COPY ) {
            /* update quota overrun */
            updatequotaOverrun( L1desc[l1descInx].dataObjInfo->rescHier,
                                newSize, ALL_QUOTA );
        }
    }

    free( chksumStr );

    // =-=-=-=-=-=-=-
    // JMC - backport 4537
    /* purge the cache copy */
    if ( L1desc[l1descInx].purgeCacheFlag > 0 ) {
        int status1 = trimDataObjInfo( rsComm,
                                       L1desc[l1descInx].dataObjInfo );
        if ( status1 < 0 ) {
            rodsLogError( LOG_ERROR, status1,
                          "_rsDataObjClose: trimDataObjInfo error for %s",
                          L1desc[l1descInx].dataObjInfo->objPath );
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
                             "procChksumForClose: chksum mismatch for for %s src [%s] new [%s]",
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
