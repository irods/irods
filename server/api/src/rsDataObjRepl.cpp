/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjRepl.h for a description of this API call.*/


#include "dataObjRepl.h"
#include "dataObjOpr.hpp"
#include "dataObjCreate.h"
#include "dataObjOpen.h"
#include "dataObjClose.h"
#include "dataObjPut.h"
#include "dataObjGet.h"
#include "rodsLog.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "resource.hpp"
#include "icatDefines.h"
#include "getRemoteZoneResc.h"
#include "l3FileGetSingleBuf.h"
#include "l3FilePutSingleBuf.h"
#include "fileSyncToArch.h"
#include "fileStageToCache.h"
#include "unbunAndRegPhyBunfile.h"
#include "dataObjTrim.h"
#include "dataObjLock.h"
#include "miscServerFunct.hpp"
#include "rsDataObjRepl.hpp"
#include "apiNumber.h"
#include "rsDataCopy.hpp"
#include "rsDataObjCreate.hpp"
#include "rsDataObjOpen.hpp"
#include "rsDataObjClose.hpp"
#include "rsL3FileGetSingleBuf.hpp"
#include "rsDataObjGet.hpp"
#include "rsDataObjPut.hpp"
#include "rsL3FilePutSingleBuf.hpp"
#include "rsFileStageToCache.hpp"
#include "rsFileSyncToArch.hpp"
#include "rsUnbunAndRegPhyBunfile.hpp"

#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_log.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_server_api_call.hpp"
#include "irods_random.hpp"
#include <boost/lexical_cast.hpp>

/* rsDataObjRepl - The Api handler of the rcDataObjRepl call - Replicate
 * a data object.
 * Input -
 *    rsComm_t *rsComm
 *    dataObjInp_t *dataObjInp - The replication input
 *    transferStat_t **transStat - transfer stat output
 */

int
rsDataObjRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
               transferStat_t **transStat ) {
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    dataObjInfo_t *dataObjInfo = NULL;
    char* lockType = NULL; // JMC - backport 4609
    int   lockFd   = -1;   // JMC - backport 4609

    if ( getValByKey( &dataObjInp->condInput, SU_CLIENT_USER_KW ) != NULL ) {
        /* To SU, cannot be called by normal user directly */
        if ( rsComm->proxyUser.authInfo.authFlag < REMOTE_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
    }

    status = resolvePathInSpecColl( rsComm, dataObjInp->objPath,
                                    READ_COLL_PERM, 0, &dataObjInfo );

    if ( status == DATA_OBJ_T ) {
        if ( dataObjInfo != NULL && dataObjInfo->specColl != NULL ) {
            if ( dataObjInfo->specColl->collClass == LINKED_COLL ) {
                rstrcpy( dataObjInp->objPath, dataObjInfo->objPath,
                         MAX_NAME_LEN );
                freeAllDataObjInfo( dataObjInfo );
            }
            else {
                freeAllDataObjInfo( dataObjInfo );
                return SYS_REG_OBJ_IN_SPEC_COLL;
            }
        }
    }

    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        status = _rcDataObjRepl( rodsServerHost->conn, dataObjInp,
                                 transStat );
        return status;
    }

    // =-=-=-=-=-=-=-
    // if the dest resc name key is set then we will resolve that key as our hier
    // if a resc name key word is not also set in which case we inadvertently resolve
    // the DESTINATION resource, not a valid source.  unset it before resolve hier
    // and replace it afterwards to get intended behavior
    char* dest_resc_ptr = getValByKey( &dataObjInp->condInput, DEST_RESC_NAME_KW );
    std::string tmp_dest_resc;
    if ( dest_resc_ptr ) {
        tmp_dest_resc = dest_resc_ptr;
        rmKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW );
    }

    // =-=-=-=-=-=-=-
    // make sure tmp_dest_resc exists and is available
    if ( !tmp_dest_resc.empty() ) {
        irods::error resc_err = irods::is_hier_live( tmp_dest_resc );
        if ( !resc_err.ok() ) {
            irods::log( resc_err );
            return resc_err.code();
        }
    }

    // =-=-=-=-=-=-=-
    // call redirect for our operation of choice to request the hier string appropriately
    std::string hier;
    char*       tmp_hier  = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );

    if ( 0 == tmp_hier ) {
        // set a repl keyword here so resources can respond accordingly
        addKeyVal( &dataObjInp->condInput, IN_REPL_KW, "" );
        irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION,
                           rsComm, dataObjInp, hier );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed in irods::resolve_resource_hierarchy for [";
            msg << dataObjInp->objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );
            return ret.code();
        }

        addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

    }
    else {
        hier = tmp_hier;
    }

    // =-=-=-=-=-=-=-
    // reset dest resc name if it was set to begin with
    if ( !tmp_dest_resc.empty() ) {
        addKeyVal( &dataObjInp->condInput, DEST_RESC_NAME_KW, tmp_dest_resc.c_str() );
    }

    // =-=-=-=-=-=-=-
    // performing a local replication
    *transStat = ( transferStat_t* )malloc( sizeof( transferStat_t ) );
    memset( *transStat, 0, sizeof( transferStat_t ) );

    // =-=-=-=-=-=-=-
    // JMC - backport 4609
    lockType = getValByKey( &dataObjInp->condInput, LOCK_TYPE_KW );
    if ( lockType != NULL ) {
        lockFd = irods::server_api_call(
                     DATA_OBJ_LOCK_AN,
                     rsComm,
                     dataObjInp,
                     NULL,
                     ( void** ) NULL,
                     NULL );
        if ( lockFd >= 0 ) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW );
        }
        else {
            rodsLogError( LOG_ERROR, lockFd,
                          "rsDataObjRepl: lock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType );
            return lockFd;
        }
    }
    // =-=-=-=-=-=-=-

    status = _rsDataObjRepl( rsComm, dataObjInp, *transStat, NULL );
    if ( status < 0 && status != DIRECT_ARCHIVE_ACCESS ) {
        rodsLog( LOG_NOTICE, "%s - Failed to replicate data object.", __FUNCTION__ );
    }

    if ( lockFd > 0 ) {
        char fd_string[NAME_LEN];
        snprintf( fd_string, sizeof( fd_string ), "%-d", lockFd );
        addKeyVal( &dataObjInp->condInput, LOCK_FD_KW, fd_string );
        irods::server_api_call(
            DATA_OBJ_UNLOCK_AN,
            rsComm,
            dataObjInp,
            NULL,
            ( void** ) NULL,
            NULL );
    }

    // =-=-=-=-=-=-=-
    // specifically ignore this error as it should not cause
    // any issues with replication.
    if ( status == DIRECT_ARCHIVE_ACCESS ) {
        return 0;
    }
    else {
        return status;
    }
}

int
_rsDataObjRepl(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    transferStat_t *transStat,
    dataObjInfo_t *outDataObjInfo ) {
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *oldDataObjInfoHead = NULL;
    dataObjInfo_t *destDataObjInfo = NULL;
    std::string root_resc_name;
    ruleExecInfo_t rei;
    int multiCopyFlag;
    char *accessPerm;
    int allFlag;
    int savedStatus = 0;
    if ( getValByKey( &dataObjInp->condInput, SU_CLIENT_USER_KW ) != NULL ) {
        accessPerm = NULL;
    }
    else if ( getValByKey( &dataObjInp->condInput, ADMIN_KW ) != NULL ) {
        if ( rsComm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            return CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        }
        accessPerm = NULL;
    }
    else {
        accessPerm = ACCESS_READ_OBJECT;
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
    if ( multiCopyFlag ) {
        status = getDataObjInfo( rsComm, dataObjInp, &dataObjInfoHead,
                                 accessPerm, 0 );
    }
    else {
        /* No multiCopy allowed. ignoreCondInput - need to find all copies
         * to make sure no multiCopy in the same resource */
        status = getDataObjInfo( rsComm, dataObjInp, &dataObjInfoHead, accessPerm, 1 );
    }

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s: getDataObjInfo for [%s] failed", __FUNCTION__, dataObjInp->objPath );
        return status;
    }

    char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
    char* dest_hier = getValByKey( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW );

    status = sortObjInfoForRepl( &dataObjInfoHead, &oldDataObjInfoHead, 0, resc_hier, dest_hier );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "%s - Failed to sort objects for replication.", __FUNCTION__ );
        return status;
    }

    // =-=-=-=-=-=-=-
    // if a resc is specified and it has a stale copy then we should just treat this as an update
    // also consider the 'update' keyword as that might also have some bearing on updates
    if ( ( !multiCopyFlag && oldDataObjInfoHead ) || getValByKey( &dataObjInp->condInput, UPDATE_REPL_KW ) != NULL ) {

        /* update old repl to new repl */
        status = _rsDataObjReplUpdate( rsComm, dataObjInp, dataObjInfoHead, oldDataObjInfoHead, transStat );

        if ( status >= 0 && outDataObjInfo != NULL ) {
            *outDataObjInfo = *oldDataObjInfoHead; // JMC - possible double free situation
            outDataObjInfo->next = NULL;
        }
        else {
            if ( status < 0 && status != DIRECT_ARCHIVE_ACCESS ) {
                rodsLog( LOG_NOTICE, "%s - Failed to update replica.", __FUNCTION__ );
            }
        }

        freeAllDataObjInfo( dataObjInfoHead );
        freeAllDataObjInfo( oldDataObjInfoHead );

        return status;

    } // repl update

    /* if multiCopy allowed, remove old so they won't be overwritten */
    status = sortObjInfoForRepl( &dataObjInfoHead, &oldDataObjInfoHead, multiCopyFlag, resc_hier, dest_hier );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "%s - Failed to sort objects for replication.", __FUNCTION__ );
        return status;
    }

    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
        allFlag = 1;
    }
    else {
        allFlag = 0;
    }

    /* query rcat for resource info and sort it */
    dataObjInp->oprType = REPLICATE_OPR; // JMC - backport 4660
    status = getRescForCreate( rsComm, dataObjInp, root_resc_name );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE, "%s - Failed to get a resource group for create.", __FUNCTION__ );
        return status;
    }

    if ( multiCopyFlag == 0 ) { // JMC - backport 4594

        /* if one copy per resource, see if a good copy already exist,
         * If it does, the copy is returned in destDataObjInfo.
         * Otherwise, Resources in &myRescGrpInfo are trimmed. Only those
         ( target resources remained are left in &myRescGrpInfo.
         * Also, the copies need to be overwritten is returned
         * in destDataObjInfo. */
        status = resolveSingleReplCopy( &dataObjInfoHead, &oldDataObjInfoHead,
                                        root_resc_name, &destDataObjInfo,
                                        &dataObjInp->condInput );

        if ( status == HAVE_GOOD_COPY ) {
            // =-=-=-=-=-=-=-
            // JMC - backport 4450
            // =-=-=-=-=-=-=-
            if ( outDataObjInfo != NULL && destDataObjInfo != NULL ) {
                /* pass back the GOOD_COPY */
                *outDataObjInfo = *destDataObjInfo;
                outDataObjInfo->next = NULL;
            }

            freeAllDataObjInfo( dataObjInfoHead );
            freeAllDataObjInfo( oldDataObjInfoHead );
            freeAllDataObjInfo( destDataObjInfo ); // JMC - backport 4494

            return 0;
        }
        else if ( status < 0 ) {
            freeAllDataObjInfo( dataObjInfoHead );
            freeAllDataObjInfo( oldDataObjInfoHead );
            freeAllDataObjInfo( destDataObjInfo ); // JMC - backport 4494
            rodsLog( LOG_NOTICE, "%s - Failed to resolve a single replication copy.", __FUNCTION__ );

            return status;
        }
        /* NO_GOOD_COPY drop through here */

    } // if multicopy flg

    status = applyPreprocRuleForOpen( rsComm, dataObjInp, &dataObjInfoHead );
    if ( status < 0 ) {
        freeAllDataObjInfo( dataObjInfoHead );
        freeAllDataObjInfo( oldDataObjInfoHead );
        freeAllDataObjInfo( destDataObjInfo ); // JMC - backport 4494

        return status;
    }

    /* If destDataObjInfo is not NULL, we will overwrite it. Otherwise
     * replicate to myRescGrpInfo */
    if ( destDataObjInfo != NULL ) {

        status = _rsDataObjReplUpdate( rsComm, dataObjInp, dataObjInfoHead,
                                       destDataObjInfo, transStat );
        if ( status >= 0 ) {
            if ( outDataObjInfo != NULL ) {
                *outDataObjInfo = *destDataObjInfo;
                outDataObjInfo->next = NULL;
            }
            if ( allFlag == 0 ) {
                freeAllDataObjInfo( dataObjInfoHead );
                freeAllDataObjInfo( oldDataObjInfoHead );
                freeAllDataObjInfo( destDataObjInfo ); // JMC - backport 4494

                return 0;
            }
            else {   // JMC - backport 4494
                /* queue destDataObjInfo in &dataObjInfoHead so that stage to cache
                 * can evaluate it */
                queDataObjInfo( &dataObjInfoHead, destDataObjInfo, 0, 1 );
                destDataObjInfo = NULL;
            }
        }
        else {
            savedStatus = status;
        }
    }

    if ( !root_resc_name.empty() ) {
        /* new replication to the resource group */
        status = _rsDataObjReplNewCopy( rsComm, dataObjInp, dataObjInfoHead,
                                        root_resc_name.c_str(), transStat,
                                        outDataObjInfo );
        if ( status < 0 ) {
            savedStatus = status;
        }
    }

    freeAllDataObjInfo( dataObjInfoHead );
    freeAllDataObjInfo( oldDataObjInfoHead );

    return savedStatus;
}

/* _rsDataObjRepl - Update existing copies from srcDataObjInfoHead to
 *      destDataObjInfoHead.
 * Additional input -
 *   dataObjInfo_t *srcDataObjInfoHead _ a link list of the src to be

 *   dataObjInfo_t *destDataObjInfoHead -  a link of copies to be updated.
 *     The dataSize in this struct will also be updated.
 *   dataObjInfo_t *oldDataObjInfo - this is for destDataObjInfo is a
 *       COMPOUND_CL resource. If it is, need to find an old copy of
 *       the resource in the same group so that it can be updated first.
 */
int _rsDataObjReplUpdate(
    rsComm_t*       rsComm,
    dataObjInp_t*   dataObjInp,
    dataObjInfo_t*  srcDataObjInfoHead,
    dataObjInfo_t*  destDataObjInfoHead,
    transferStat_t* transStat ) {

    // =-=-=-=-=-=-=-
    //
    dataObjInfo_t *destDataObjInfo = 0;
    dataObjInfo_t *srcDataObjInfo = 0;
    int status = 0;
    int allFlag = 0;
    int savedStatus = 0;
    int replCnt = 0;

    // =-=-=-=-=-=-=-
    // set all or single flag
    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
        allFlag = 1;
    }
    else {
        allFlag = 0;
    }

    // =-=-=-=-=-=-=-
    // cache a copy of the dest resc hier if there is one
    std::string dst_resc_hier;
    char* dst_resc_hier_ptr = getValByKey( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW );
    if ( dst_resc_hier_ptr ) {
        dst_resc_hier = dst_resc_hier_ptr;

    }

    // =-=-=-=-=-=-=-
    // loop over all the dest data obj info structs
    transStat->bytesWritten = srcDataObjInfoHead->dataSize;
    destDataObjInfo = destDataObjInfoHead;
    while ( destDataObjInfo != NULL ) {
        // =-=-=-=-=-=-=-
        // skip invalid data ids
        if ( destDataObjInfo->dataId == 0 ) {
            destDataObjInfo = destDataObjInfo->next;
            continue;
        }

        // =-=-=-=-=-=-=-
        // overwrite a specific destDataObjInfo
        srcDataObjInfo = srcDataObjInfoHead;
        while ( srcDataObjInfo != NULL ) {
            // =-=-=-=-=-=-=-
            // if the dst hier kw is not set, then set the dest resc hier kw from the dest obj info
            // as it is already known and we do not want the resc hier making this decision again
            addKeyVal( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW, destDataObjInfo->rescHier );
            status = _rsDataObjReplS( rsComm, dataObjInp, srcDataObjInfo, "", destDataObjInfo, 1 );

            if ( status >= 0 ) {
                break;
            }
            srcDataObjInfo = srcDataObjInfo->next;
        }

        if ( status >= 0 ) {
            transStat->numThreads = dataObjInp->numThreads;
            if ( allFlag == 0 ) {
                return 0;
            }
        }
        else {
            savedStatus = status;
            replCnt ++;
        }
        destDataObjInfo = destDataObjInfo->next;
    }

    // =-=-=-=-=-=-=-
    // repave the key word if it was set before
    if ( !dst_resc_hier.empty() ) {
        addKeyVal( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW, dst_resc_hier.c_str() );

    }

    return savedStatus;

} // _rsDataObjReplUpdate

/* _rsDataObjReplNewCopy - Replicate new copies to destRescGrpInfo.
 * Additional input -
 *   dataObjInfo_t *srcDataObjInfoHead _ a link list of the src to be
 *     replicated. Only one will be picked.
 *   rescGrpInfo_t *destRescGrpInfo - The dest resource info
 *   dataObjInfo_t *oldDataObjInfo - this is for destDataObjInfo is a
 *       COMPOUND_CL resource. If it is, need to find an old copy of
 *       the resource in the same group so that it can be updated first.
 *   dataObjInfo_t *outDataObjInfo - If it is not NULL, output the last
 *       dataObjInfo_t of the new copy.
 */

int
_rsDataObjReplNewCopy(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    dataObjInfo_t *srcDataObjInfoHead,
    const char* _root_resc_name,
    transferStat_t *transStat,
    dataObjInfo_t *outDataObjInfo ) {
    // =-=-=-=-=-=-=-

    dataObjInfo_t *srcDataObjInfo;
    int status;
    int allFlag;
    int savedStatus = 0;

    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
        allFlag = 1;
    }
    else {
        allFlag = 0;
    }

    // =-=-=-=-=-=-=-
    transStat->bytesWritten = srcDataObjInfoHead->dataSize;

    srcDataObjInfo = srcDataObjInfoHead;
    while ( srcDataObjInfo != NULL ) {
        status = _rsDataObjReplS( rsComm, dataObjInp, srcDataObjInfo, _root_resc_name, outDataObjInfo, 0 );
        if ( status >= 0 ) {
            break;
        }
        else {
            savedStatus = status;
        }
        srcDataObjInfo = srcDataObjInfo->next;
    }

    if ( status >= 0 ) {
        transStat->numThreads = dataObjInp->numThreads;
        if ( allFlag == 0 ) {
            return 0;
        }
    }
    else {
        savedStatus = status;
    }

    return savedStatus;
}


/* _rsDataObjReplS - replicate a single obj
 *   dataObjInfo_t *srcDataObjInfo - the src to be replicated.
 *   _resc_name - only meaningful if the destDataObj does not exist.
 *   dataObjInfo_t *destDataObjInfo - This can be both input and output.
 *      If destDataObjInfo == NULL, dest is new and no output is required.
 *      If destDataObjInfo != NULL:
 *          If updateFlag == 0, Output only.  Output the dataObjInfo
 *          of the replicated copy. This is needed by msiSysReplDataObj and
 *          msiStageDataObj which need a copy of destDataObjInfo.
 *          If updateFlag > 0, the dest repl exists. Need to
 *          update it.
 */
int
_rsDataObjReplS(
    rsComm_t * rsComm,
    dataObjInp_t * dataObjInp,
    dataObjInfo_t * srcDataObjInfo,
    const char * _root_resc_name,
    dataObjInfo_t * destDataObjInfo,
    int updateFlag ) {
    // =-=-=-=-=-=-=-

    int status, status1;
    int l1descInx;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *myDestDataObjInfo = NULL;

    l1descInx = dataObjOpenForRepl( rsComm, dataObjInp, srcDataObjInfo,
                                    _root_resc_name, destDataObjInfo, updateFlag );
    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    int single_buff_sz;
    try {
        single_buff_sz = irods::get_advanced_setting<const int>(irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER) * 1024 * 1024;
    } catch ( const irods::exception& e ) {
        irods::log(e);
        return e.code();
    }

    if ( L1desc[l1descInx].stageFlag != NO_STAGING ) {
        status = l3DataStageSync( rsComm, l1descInx );
    }
    else if ( L1desc[l1descInx].dataObjInp->numThreads == 0 &&
              L1desc[L1desc[l1descInx].srcL1descInx].dataObjInfo->dataSize  <= single_buff_sz ) {
        status = l3DataCopySingleBuf( rsComm, l1descInx );
    }
    else {
        status = dataObjCopy( rsComm, l1descInx );
    }

    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );

    dataObjCloseInp.l1descInx = l1descInx;
    /* myDestDataObjInfo = L1desc[l1descInx].dataObjInfo; */
    L1desc[l1descInx].oprStatus = status;
    if ( status >= 0 ) {
        L1desc[l1descInx].bytesWritten =  L1desc[l1descInx].dataObjInfo->dataSize;
    }

    // Need to propagate the in pdmo flag
    char* pdmo_kw = getValByKey( &dataObjInp->condInput, IN_PDMO_KW );
    if ( pdmo_kw != NULL ) {
        addKeyVal( &dataObjCloseInp.condInput, IN_PDMO_KW, pdmo_kw );
    }

    status1 = irsDataObjClose( rsComm, &dataObjCloseInp, &myDestDataObjInfo );

    if ( destDataObjInfo != NULL ) {
        if ( destDataObjInfo->dataId <= 0 && myDestDataObjInfo != NULL ) {
            destDataObjInfo->dataId = myDestDataObjInfo->dataId;
            destDataObjInfo->replNum = myDestDataObjInfo->replNum;
        }
        else if ( myDestDataObjInfo ) {
            /* the size could change */
            destDataObjInfo->dataSize = myDestDataObjInfo->dataSize;
        }
    }

    freeDataObjInfo( myDestDataObjInfo );
    clearKeyVal( &dataObjCloseInp.condInput );

    if ( status < 0 ) {
        return status;
    }
    else if ( status1 < 0 ) {
        return status1;
    }
    else {
        return status;
    }
}

/* dataObjOpenForRepl - Create/open the dest and open the src
 */

int
dataObjOpenForRepl(
    rsComm_t * rsComm,
    dataObjInp_t * dataObjInp,
    dataObjInfo_t * inpSrcDataObjInfo,
    const char* _root_resc_name,
    dataObjInfo_t * inpDestDataObjInfo,
    int updateFlag ) {

    irods::error resc_err;
    const char *my_resc_name; // replaces myDestRescInfo
    if ( _root_resc_name && strlen( _root_resc_name ) ) {
        my_resc_name = _root_resc_name;
    }
    else {
        my_resc_name = inpDestDataObjInfo->rescName;
    }

    resc_err = irods::is_hier_live( my_resc_name );
    if ( !resc_err.ok() ) {
        return resc_err.code();
    }

    resc_err = irods::is_hier_live( inpSrcDataObjInfo->rescName );
    if ( !resc_err.ok() ) {
        return resc_err.code();
    }

    dataObjInfo_t * srcDataObjInfo = ( dataObjInfo_t* )calloc( 1, sizeof( dataObjInfo_t ) );
    if ( NULL == srcDataObjInfo ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR, "dataObjOpenForRepl - srcDataObjInfo is NULL" );
        return -1;
    }
    *srcDataObjInfo = *inpSrcDataObjInfo;

    memset( &srcDataObjInfo->condInput, 0, sizeof( srcDataObjInfo->condInput ) );
    replKeyVal( &inpSrcDataObjInfo->condInput, &srcDataObjInfo->condInput );

    /* open the dest */
    dataObjInp_t myDataObjInp = *dataObjInp;
    replKeyVal( &dataObjInp->condInput, &myDataObjInp.condInput );
    myDataObjInp.dataSize = inpSrcDataObjInfo->dataSize;

    int destL1descInx = allocL1desc();

    if ( destL1descInx < 0 ) {
        freeDataObjInfo( srcDataObjInfo );
        return destL1descInx;
    }

    // =-=-=-=-=-=-=-=-
    // use for redirect
    std::string op_name;
    dataObjInfo_t * myDestDataObjInfo = ( dataObjInfo_t* )calloc( 1, sizeof( dataObjInfo_t ) );

    int replStatus;
    if ( updateFlag > 0 ) {
        // =-=-=-=-=-=-=-
        // set a open operation
        op_name = irods::WRITE_OPERATION;

        /* update an existing copy */
        if ( inpDestDataObjInfo == NULL || inpDestDataObjInfo->dataId <= 0 ) {
            rodsLog( LOG_ERROR, "dataObjOpenForRepl: dataId of %s copy to be updated not defined",
                     srcDataObjInfo->objPath );
            freeDataObjInfo( myDestDataObjInfo );
            freeDataObjInfo( srcDataObjInfo );
            return SYS_UPDATE_REPL_INFO_ERR;
        }

        /* inherit the replStatus of the src */
        inpDestDataObjInfo->replStatus = srcDataObjInfo->replStatus;
        *myDestDataObjInfo = *inpDestDataObjInfo;
        // =-=-=-=-=-=-=-
        // JMC :: deep copy of condInput - necessary for preventing a double-free
        //     :: after an irsDataObjClose is called and then a freeDataObjInfo
        //     :: on the copied outgoing dataObjInfo.  see _rsDataObjReplS()
        memset( &myDestDataObjInfo->condInput, 0, sizeof( keyValPair_t ) );
        replKeyVal( &inpDestDataObjInfo->condInput, &myDestDataObjInfo->condInput );

        replStatus = srcDataObjInfo->replStatus | OPEN_EXISTING_COPY;
        addKeyVal( &myDataObjInp.condInput, FORCE_FLAG_KW, "" );
        myDataObjInp.openFlags |= ( O_TRUNC | O_WRONLY );

    }
    else {      /* a new copy */
        // =-=-=-=-=-=-=-
        // set a creation operation
        op_name = irods::CREATE_OPERATION;

        initDataObjInfoForRepl( myDestDataObjInfo, srcDataObjInfo, _root_resc_name );
        replStatus = srcDataObjInfo->replStatus;
    }

    // =-=-=-=-=-=-=-
    // call redirect for our operation of choice to request the hier string appropriately
    std::string hier;
    char*       dst_hier_str = getValByKey( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW );
    if ( 0 == dst_hier_str ) {
        // set a repl keyword here so resources can respond accordingly
        addKeyVal( &dataObjInp->condInput, IN_REPL_KW, "" );

        dataObjInp_t dest_inp;
        memset( &dest_inp, 0, sizeof( dest_inp ) );
        memset( &dest_inp.condInput, 0, sizeof( dest_inp.condInput ) );
        strncpy( dest_inp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
        addKeyVal( &( dest_inp.condInput ), RESC_NAME_KW, my_resc_name );

        irods::error ret = irods::resolve_resource_hierarchy( op_name, rsComm, &dest_inp, hier );
        clearKeyVal( &dest_inp.condInput );

        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "failed in irods::resolve_resource_hierarchy for [";
            msg << dest_inp.objPath << "]";
            irods::log( PASSMSG( msg.str(), ret ) );

            freeDataObjInfo( srcDataObjInfo );
            freeDataObjInfo( myDestDataObjInfo );
            return ret.code();
        }

        addKeyVal( &dataObjInp->condInput, DEST_RESC_HIER_STR_KW, hier.c_str() );

    }
    else {
        hier = dst_hier_str;

    }

    // =-=-=-=-=-=-=-
    // expected by fillL1desc
    // repave the rescName with the leaf name as it was initialized to the root name
    irods::hierarchy_parser parser;
    parser.set_string( hier );
    std::string leaf_resc;
    parser.last_resc(leaf_resc);
    rstrcpy( myDestDataObjInfo->rescHier, hier.c_str(), MAX_NAME_LEN );
    rstrcpy( myDestDataObjInfo->rescName, leaf_resc.c_str(), NAME_LEN );

    rodsLong_t dst_resc_id;
    resc_mgr.hier_to_leaf_id( hier, dst_resc_id );
    myDestDataObjInfo->rescId = dst_resc_id;

    // =-=-=-=-=-=-=-
    // JMC :: [ ticket 1746 ] this should always be set - this was overwriting the KW
    //     :: in the incoming dataObjInp leaving this here for future consideration if issues arise
    // addKeyVal( &(myDataObjInp.condInput), RESC_HIER_STR_KW, hier.c_str() ); // <===============
    fillL1desc( destL1descInx, &myDataObjInp, myDestDataObjInfo, replStatus, srcDataObjInfo->dataSize );

    dataObjInp_t * l1DataObjInp = L1desc[destL1descInx].dataObjInp;
    if ( l1DataObjInp->oprType == PHYMV_OPR ) {
        L1desc[destL1descInx].oprType = PHYMV_DEST;
        myDestDataObjInfo->replNum = srcDataObjInfo->replNum;
        myDestDataObjInfo->dataId = srcDataObjInfo->dataId;
    }
    else {
        L1desc[destL1descInx].oprType = REPLICATE_DEST;
    }

    // =-=-=-=-=-=-=-
    // reproduce the stage / sync behavior using keywords rather
    // than the resource class for use in the compound resource plugin
    char* stage_kw = getValByKey( &dataObjInp->condInput, STAGE_OBJ_KW );
    char* sync_kw  = getValByKey( &dataObjInp->condInput, SYNC_OBJ_KW );
    if ( stage_kw ) {
        L1desc[destL1descInx].stageFlag = STAGE_SRC;
    }
    else if ( sync_kw ) {
        L1desc[destL1descInx].stageFlag = SYNC_DEST;
    }

    l1DataObjInp->numThreads = dataObjInp->numThreads =
                                   getNumThreads( rsComm, l1DataObjInp->dataSize, l1DataObjInp->numThreads,
                                           &dataObjInp->condInput, dst_hier_str, srcDataObjInfo->rescHier, dataObjInp->oprType );

    int single_buff_sz;
    try {
        single_buff_sz = irods::get_advanced_setting<const int>(irods::CFG_MAX_SIZE_FOR_SINGLE_BUFFER) * 1024 * 1024;
    } catch ( const irods::exception& e ) {
        irods::log(e);
        freeDataObjInfo( srcDataObjInfo );
        return e.code();
    }

    if ( ( l1DataObjInp->numThreads > 0 ||
            l1DataObjInp->dataSize > single_buff_sz ) &&
            L1desc[destL1descInx].stageFlag == NO_STAGING ) {
        int status = 0;
        if ( updateFlag > 0 ) {
            status = dataOpen( rsComm, destL1descInx );
        }
        else {
            status = getFilePathName( rsComm, myDestDataObjInfo, L1desc[destL1descInx].dataObjInp );
            if ( status >= 0 ) {
                status = dataCreate( rsComm, destL1descInx );
            }
        }

        if ( status < 0 ) {
            freeL1desc( destL1descInx );
            freeDataObjInfo( srcDataObjInfo );
            return status;
        }
    }
    else {
        if ( updateFlag == 0 ) {
            int status = getFilePathName( rsComm, myDestDataObjInfo, L1desc[destL1descInx].dataObjInp );
            if ( status < 0 ) {
                freeL1desc( destL1descInx );
                freeDataObjInfo( srcDataObjInfo );
                return status;
            }
        }
    }

    if ( inpDestDataObjInfo != NULL && updateFlag == 0 ) {
        /* a new replica */
        *inpDestDataObjInfo = *myDestDataObjInfo;

        // =-=-=-=-=-=-=-
        // JMC :: deep copy of condInput - necessary for preventing a double-free
        //     :: after an irsDataObjClose is called and then a freeDataObjInfo
        //     :: on the copied outgoing dataObjInfo.  see _rsDataObjReplS()
        memset( &inpDestDataObjInfo->condInput, 0, sizeof( keyValPair_t ) );
        replKeyVal( &myDestDataObjInfo->condInput, &inpDestDataObjInfo->condInput );

        inpDestDataObjInfo->next = NULL;
    }

    // =-=-=-=-=-=-=-
    // notify the dest resource hierarchy that something is afoot
    irods::file_object_ptr file_obj(
        new irods::file_object(
            rsComm,
            myDestDataObjInfo ) );
    irods::error ret = fileNotify(
              rsComm,
              file_obj,
              irods::WRITE_OPERATION );
    if ( !ret.ok() ) {
        std::stringstream msg;
        msg << "Failed to signal the resource that the data object \"";
        msg << myDestDataObjInfo->objPath;
        msg << "\" was modified.";
        ret = PASSMSG( msg.str(), ret );
        irods::log( ret );
        freeDataObjInfo( srcDataObjInfo );
        return ret.code();
    }

    /* open the src */
    rstrcpy( srcDataObjInfo->rescHier, inpSrcDataObjInfo->rescHier, MAX_NAME_LEN );
    ret = resc_mgr.hier_to_leaf_id(inpSrcDataObjInfo->rescHier,srcDataObjInfo->rescId);
    if( !ret.ok() ) {
        irods::log(PASS(ret));
    }

    int srcL1descInx = allocL1desc();
    if ( srcL1descInx < 0 ) {
        freeDataObjInfo( srcDataObjInfo );
        return srcL1descInx;
    }
    fillL1desc( srcL1descInx, &myDataObjInp, srcDataObjInfo, srcDataObjInfo->replStatus, srcDataObjInfo->dataSize );
    l1DataObjInp = L1desc[srcL1descInx].dataObjInp;
    l1DataObjInp->numThreads = dataObjInp->numThreads;
    if ( l1DataObjInp->oprType == PHYMV_OPR ) {
        L1desc[srcL1descInx].oprType = PHYMV_SRC;
    }
    else {
        L1desc[srcL1descInx].oprType = REPLICATE_SRC;
    }

    if ( getValByKey( &dataObjInp->condInput, PURGE_CACHE_KW ) != NULL ) {
        L1desc[srcL1descInx].purgeCacheFlag = 1;
    }

    if ( ( l1DataObjInp->numThreads > 0 ||
            l1DataObjInp->dataSize > single_buff_sz ) &&
            L1desc[destL1descInx].stageFlag == NO_STAGING ) {
        openedDataObjInp_t dataObjCloseInp;

        l1DataObjInp->openFlags = O_RDONLY;
        int status = dataOpen( rsComm, srcL1descInx );
        if ( status < 0 ) {
            freeL1desc( srcL1descInx );
            memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
            dataObjCloseInp.l1descInx = destL1descInx;
            rsDataObjClose( rsComm, &dataObjCloseInp );
            return status;
        }
    }

    L1desc[destL1descInx].srcL1descInx = srcL1descInx;

    clearKeyVal( &myDataObjInp.condInput );

    return destL1descInx;
}

int
dataObjCopy( rsComm_t * rsComm, int l1descInx ) {

    dataCopyInp_t dataCopyInp;
    bzero( &dataCopyInp, sizeof( dataCopyInp ) );
    dataOprInp_t *dataOprInp = &dataCopyInp.dataOprInp;
    int srcL1descInx = L1desc[l1descInx].srcL1descInx;
    int destL1descInx = l1descInx;

    int srcL3descInx = L1desc[srcL1descInx].l3descInx;
    int destL3descInx = L1desc[destL1descInx].l3descInx;

    int srcRemoteFlag;
    if ( L1desc[srcL1descInx].remoteZoneHost != NULL ) {
        srcRemoteFlag = REMOTE_ZONE_HOST;
    }
    else {
        srcRemoteFlag = FileDesc[srcL3descInx].rodsServerHost->localFlag;
    }
    int destRemoteFlag;
    if ( L1desc[destL1descInx].remoteZoneHost != NULL ) {
        destRemoteFlag = REMOTE_ZONE_HOST;
    }
    else {
        destRemoteFlag = FileDesc[destL3descInx].rodsServerHost->localFlag;
    }

    portalOprOut_t * portalOprOut = NULL;
    if ( srcRemoteFlag == REMOTE_ZONE_HOST &&
            destRemoteFlag == REMOTE_ZONE_HOST ) {
        /* remote zone to remote zone copy. Have to do L1 level copy */
        initDataOprInp( &dataCopyInp.dataOprInp, l1descInx, COPY_TO_REM_OPR );
        L1desc[l1descInx].dataObjInp->numThreads = 0;
        dataCopyInp.portalOprOut.l1descInx = l1descInx;
        int status = singleL1Copy( rsComm, &dataCopyInp );
        clearKeyVal( &dataOprInp->condInput );
        return status;
    }
    else if ( srcRemoteFlag != REMOTE_ZONE_HOST &&
              destRemoteFlag != REMOTE_ZONE_HOST &&
              FileDesc[srcL3descInx].rodsServerHost ==
              FileDesc[destL3descInx].rodsServerHost ) {
        /* local zone same host copy */
        initDataOprInp( &dataCopyInp.dataOprInp, l1descInx, SAME_HOST_COPY_OPR );
        /* dataCopyInp.portalOprOut.numThreads is needed by rsDataCopy */
        dataCopyInp.portalOprOut.numThreads =
            dataCopyInp.dataOprInp.numThreads;
        if ( srcRemoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
        }

    }
    else if ( ( srcRemoteFlag == LOCAL_HOST && destRemoteFlag != LOCAL_HOST ) ||
              destRemoteFlag == REMOTE_ZONE_HOST ) {
        initDataOprInp( &dataCopyInp.dataOprInp, l1descInx, COPY_TO_REM_OPR );
        /* do it locally if numThreads == 0 */
        if ( L1desc[l1descInx].dataObjInp->numThreads > 0 ) {
            /* copy from local to remote */
            /* preProcParaPut to establish portalOprOut without data transfer */
            int status = preProcParaPut( rsComm, destL1descInx, &portalOprOut );
            if ( status < 0 || NULL == portalOprOut ) { // JMC cppcheck - nullptr
                rodsLog( LOG_NOTICE,
                         "dataObjCopy: preProcParaPut error for %s",
                         L1desc[srcL1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = destL1descInx;
        }
        if ( srcRemoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
        }
    }
    else if ( ( srcRemoteFlag != LOCAL_HOST && destRemoteFlag == LOCAL_HOST ) ||
              srcRemoteFlag == REMOTE_ZONE_HOST ) {
        /* copy from remote to local */
        initDataOprInp( &dataCopyInp.dataOprInp, l1descInx, COPY_TO_LOCAL_OPR );
        /* do it locally if numThreads == 0 */
        if ( L1desc[l1descInx].dataObjInp->numThreads > 0 ) {
            /* preProcParaGet to establish portalOprOut without data transfer */
            int status = preProcParaGet( rsComm, srcL1descInx, &portalOprOut );
            if ( status < 0 || NULL == portalOprOut ) { // JMC cppcheck - null ptr
                rodsLog( LOG_NOTICE,
                         "dataObjCopy: preProcParaGet error for %s",
                         L1desc[srcL1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = srcL1descInx;
        }
        if ( destRemoteFlag == LOCAL_HOST ) {
            addKeyVal( &dataOprInp->condInput, EXEC_LOCALLY_KW, "" );
        }
    }
    else {
        /* remote to remote */
        initDataOprInp( &dataCopyInp.dataOprInp, l1descInx, COPY_TO_LOCAL_OPR );
        /* preProcParaGet only establish &portalOprOut without data transfer */
        /* do it locally if numThreads == 0 */
        if ( L1desc[l1descInx].dataObjInp->numThreads > 0 ) {
            int status = preProcParaGet( rsComm, srcL1descInx, &portalOprOut );
            if ( status < 0 || NULL == portalOprOut ) { // JMC cppcheck - null ptr
                rodsLog( LOG_NOTICE,
                         "dataObjCopy: preProcParaGet error for %s",
                         L1desc[srcL1descInx].dataObjInfo->objPath );
                free( portalOprOut );
                return status;
            }
            dataCopyInp.portalOprOut = *portalOprOut;
        }
        else {
            dataCopyInp.portalOprOut.l1descInx = srcL1descInx;
        }
    }
    /* rsDataCopy - does the physical data transfer */
    if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                      NO_CHK_COPY_LEN_KW ) != NULL ) {
        /* don't check the transfer len */
        addKeyVal( &dataOprInp->condInput, NO_CHK_COPY_LEN_KW, "" );
        if ( L1desc[l1descInx].dataObjInp->numThreads > 1 ) {
            L1desc[l1descInx].dataObjInp->numThreads =
                L1desc[srcL1descInx].dataObjInp->numThreads =
                    dataCopyInp.portalOprOut.numThreads = 1;
        }
    }
    int status =  rsDataCopy( rsComm, &dataCopyInp );

    if ( status >= 0 && portalOprOut != NULL &&
            L1desc[l1descInx].dataObjInp != NULL ) {
        /* update numThreads since it could be changed by remote server */
        L1desc[l1descInx].dataObjInp->numThreads = portalOprOut->numThreads;
    }
    clearKeyVal( &dataOprInp->condInput );
    free( portalOprOut );

    return status;
}

int
l3DataCopySingleBuf( rsComm_t * rsComm, int l1descInx ) {
    bytesBuf_t dataBBuf;
    int bytesRead, bytesWritten;
    int srcL1descInx;

    memset( &dataBBuf, 0, sizeof( bytesBuf_t ) );
    srcL1descInx = L1desc[l1descInx].srcL1descInx;

    if ( L1desc[srcL1descInx].dataSize < 0 ) {
        rodsLog( LOG_ERROR,
                 "l3DataCopySingleBuf: dataSize %lld for %s is negative",
                 L1desc[srcL1descInx].dataSize,
                 L1desc[srcL1descInx].dataObjInfo->objPath );
        return SYS_COPY_LEN_ERR;
    }
    else if ( L1desc[srcL1descInx].dataSize == 0 ) {
        bytesRead = 0;
    }
    else {
        dataBBuf.buf = malloc( L1desc[srcL1descInx].dataSize );
        bytesRead = rsL3FileGetSingleBuf( rsComm, &srcL1descInx, &dataBBuf );
    }

    if ( bytesRead < 0 ) {
        free( dataBBuf.buf ); // JMC cppcheck - leak
        return bytesRead;
    }
    else if ( getValByKey( &L1desc[l1descInx].dataObjInp->condInput,
                           NO_CHK_COPY_LEN_KW ) != NULL ) {
        /* need to update size */
        L1desc[l1descInx].dataSize = L1desc[l1descInx].dataObjInp->dataSize
                                     = bytesRead;
    }

    bytesWritten = rsL3FilePutSingleBuf( rsComm, &l1descInx, &dataBBuf );

    L1desc[l1descInx].bytesWritten = bytesWritten;

    if ( dataBBuf.buf != NULL ) {
        free( dataBBuf.buf );
        memset( &dataBBuf, 0, sizeof( bytesBuf_t ) );
    }

    if ( bytesWritten != bytesRead ) {
        if ( bytesWritten >= 0 ) {
            rodsLog( LOG_NOTICE,
                     "l3DataCopySingleBuf: l3FilePut error, towrite %d, written %d",
                     bytesRead, bytesWritten );
            return SYS_COPY_LEN_ERR;
        }
        else {
            return bytesWritten;
        }
    }


    return 0;
}

int
l3DataStageSync( rsComm_t * rsComm, int l1descInx ) {
    bytesBuf_t dataBBuf;
    int srcL1descInx;
    int status = 0;

    memset( &dataBBuf, 0, sizeof( bytesBuf_t ) );

    srcL1descInx = L1desc[l1descInx].srcL1descInx;
    if ( L1desc[srcL1descInx].dataSize < 0 &&
            L1desc[srcL1descInx].dataSize != UNKNOWN_FILE_SZ ) {
        rodsLog( LOG_ERROR,
                 "l3DataStageSync: dataSize %lld for %s is negative",
                 L1desc[srcL1descInx].dataSize,
                 L1desc[srcL1descInx].dataObjInfo->objPath );
        return SYS_COPY_LEN_ERR;
    }
    else if ( L1desc[srcL1descInx].dataSize >= 0 ||
              L1desc[srcL1descInx].dataSize == UNKNOWN_FILE_SZ ) {
        if ( L1desc[l1descInx].stageFlag == SYNC_DEST ) {
            /* dest a DO_STAGE type, sync */
            status = l3FileSync( rsComm, srcL1descInx, l1descInx );
        }
        else {
            /* src a DO_STAGE type, stage */
            status = l3FileStage( rsComm, srcL1descInx, l1descInx );
        }
    }
    else {

    }

    if ( status < 0 ) {
        L1desc[l1descInx].bytesWritten = -1;
    }
    else {
        L1desc[l1descInx].bytesWritten = L1desc[l1descInx].dataSize =
                                             L1desc[srcL1descInx].dataSize;
    }

    return status;
}

int
l3FileSync( rsComm_t * rsComm, int srcL1descInx, int destL1descInx ) {
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    // int rescTypeInx, cacheRescTypeInx;
    fileStageSyncInp_t fileSyncToArchInp;
    dataObjInp_t *dataObjInp;
    int status;
    dataObjInfo_t tmpDataObjInfo;
    std::string object_id;

    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;

    int dst_create_path = 0;
    irods::error err = irods::get_resource_property< int >(
                           destDataObjInfo->rescId,
                           irods::RESOURCE_CREATE_PATH,
                           dst_create_path );
    if ( !err.ok() ) {
        irods::log( PASS( err ) );
    }

    if ( CREATE_PATH == dst_create_path ) {

        status = chkOrphanFile( rsComm, destDataObjInfo->filePath, destDataObjInfo->rescName, &tmpDataObjInfo );
        if ( status == 0 && tmpDataObjInfo.dataId != destDataObjInfo->dataId ) {
            /* someone is using it */
            char tmp_str[ MAX_NAME_LEN ];
            snprintf( tmp_str, MAX_NAME_LEN, "%s.%-u", destDataObjInfo->filePath, irods::getRandom<unsigned int>() );
            strncpy( destDataObjInfo->filePath, tmp_str, MAX_NAME_LEN );
        }
    }

    memset( &fileSyncToArchInp, 0, sizeof( fileSyncToArchInp ) );
    dataObjInp                      = L1desc[destL1descInx].dataObjInp;
    fileSyncToArchInp.dataSize      = srcDataObjInfo->dataSize;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( srcDataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3FileSync - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    rstrcpy( fileSyncToArchInp.addr.hostAddr, location.c_str(), NAME_LEN );

    rstrcpy( fileSyncToArchInp.filename,      destDataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( fileSyncToArchInp.rescHier,      destDataObjInfo->rescHier,  MAX_NAME_LEN );
    rstrcpy( fileSyncToArchInp.objPath,       srcDataObjInfo->objPath,   MAX_NAME_LEN );
    rstrcpy( fileSyncToArchInp.cacheFilename, srcDataObjInfo->filePath,  MAX_NAME_LEN );

    // add object id keyword to pass down to resource plugins
    object_id = boost::lexical_cast<std::string>(srcDataObjInfo->dataId);
    addKeyVal(&fileSyncToArchInp.condInput, DATA_ID_KW, object_id.c_str());

    fileSyncToArchInp.mode = getFileMode( dataObjInp );
    fileSyncOut_t* sync_out = 0;
    status = rsFileSyncToArch( rsComm, &fileSyncToArchInp, &sync_out );

    if ( status >= 0 &&
            CREATE_PATH == dst_create_path &&
            NULL != sync_out ) {

        /* path name is created by the resource */
        rstrcpy( destDataObjInfo->filePath, sync_out->file_name, MAX_NAME_LEN );
        L1desc[destL1descInx].replStatus |= FILE_PATH_HAS_CHG;
    }
    free( sync_out );
    return status;
}

int
l3FileStage( rsComm_t * rsComm, int srcL1descInx, int destL1descInx ) {
    dataObjInfo_t *srcDataObjInfo, *destDataObjInfo;
    int mode, status; // JMC - backport 4527

    srcDataObjInfo = L1desc[srcL1descInx].dataObjInfo;
    destDataObjInfo = L1desc[destL1descInx].dataObjInfo;

    mode = getFileMode( L1desc[destL1descInx].dataObjInp );

    status = _l3FileStage( rsComm, srcDataObjInfo, destDataObjInfo, mode );

    return status;
}

int
_l3FileStage( rsComm_t * rsComm, dataObjInfo_t * srcDataObjInfo, // JMC - backport 4527
              dataObjInfo_t * destDataObjInfo, int mode ) {
    // int rescTypeInx, cacheRescTypeInx;
    fileStageSyncInp_t file_stage;
    int status;

    std::string resc_loc;
    std::string resc_hier( destDataObjInfo->rescHier );

    memset( &file_stage, 0, sizeof( file_stage ) );
    file_stage.dataSize      = srcDataObjInfo->dataSize;


    irods::error ret = irods::get_loc_for_hier_string( resc_hier, resc_loc );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "irods::get_loc_for_hier_string() failed", ret ) );
        return ret.code();
    }
    rstrcpy( file_stage.addr.hostAddr, resc_loc.c_str(), NAME_LEN );


    rstrcpy( file_stage.cacheFilename, destDataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( file_stage.filename,      srcDataObjInfo->filePath,  MAX_NAME_LEN );
    rstrcpy( file_stage.rescHier,      destDataObjInfo->rescHier,  MAX_NAME_LEN );
    rstrcpy( file_stage.objPath,       srcDataObjInfo->objPath,   MAX_NAME_LEN );

    file_stage.mode = mode;
    status = rsFileStageToCache( rsComm, &file_stage );
    return status;
}

/* rsReplAndRequeDataObjInfo - This routine handle the msiSysReplDataObj
 * micro-service. It replicates from srcDataObjInfoHead to destRescName
 * and support options flags given in flagStr.
 * Flags supported are: ALL_KW, RBUDP_TRANSFER_KW, SU_CLIENT_USER_KW
 * and UPDATE_REPL_KW. Multiple flags can be input with % as separator.
 * The replicated DataObjInfoHead will be put on top of the queue.
 */

int
rsReplAndRequeDataObjInfo( rsComm_t * rsComm,
                           dataObjInfo_t **srcDataObjInfoHead, char * destRescName, char * flagStr ) {
    dataObjInfo_t *dataObjInfoHead, *myDataObjInfo;
    transferStat_t transStat;
    dataObjInp_t dataObjInp;
    char tmpStr[NAME_LEN];
    int status;

    dataObjInfoHead = *srcDataObjInfoHead;
    myDataObjInfo = ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    memset( myDataObjInfo, 0, sizeof( dataObjInfo_t ) );
    memset( &dataObjInp, 0, sizeof( dataObjInp_t ) );
    memset( &transStat, 0, sizeof( transStat ) );

    if ( flagStr != NULL ) {
        if ( strstr( flagStr, ALL_KW ) != NULL ) {
            addKeyVal( &dataObjInp.condInput, ALL_KW, "" );
        }
        if ( strstr( flagStr, RBUDP_TRANSFER_KW ) != NULL ) {
            addKeyVal( &dataObjInp.condInput, RBUDP_TRANSFER_KW, "" );
        }
        if ( strstr( flagStr, SU_CLIENT_USER_KW ) != NULL ) {
            addKeyVal( &dataObjInp.condInput, SU_CLIENT_USER_KW, "" );
        }
        if ( strstr( flagStr, UPDATE_REPL_KW ) != NULL ) {
            addKeyVal( &dataObjInp.condInput, UPDATE_REPL_KW, "" );
        }
    }

    rstrcpy( dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN );
    snprintf( tmpStr, NAME_LEN, "%d", dataObjInfoHead->replNum );
    addKeyVal( &dataObjInp.condInput, REPL_NUM_KW, tmpStr );
    addKeyVal( &dataObjInp.condInput, DEST_RESC_NAME_KW, destRescName );

    status = _rsDataObjRepl( rsComm, &dataObjInp, &transStat,
                             myDataObjInfo );

    /* fix mem leak */
    clearKeyVal( &dataObjInp.condInput );
    if ( status >= 0 ) {
        status = 1;
        /* que the cache copy at the top */
        queDataObjInfo( srcDataObjInfoHead, myDataObjInfo, 0, 1 );
    }
    else {
        freeAllDataObjInfo( myDataObjInfo );
    }

    return status;
}

int
stageBundledData( rsComm_t * rsComm, dataObjInfo_t **subfileObjInfoHead ) {
    int status;
    dataObjInfo_t *dataObjInfoHead = *subfileObjInfoHead;
    char *cacheRescName;
    dataObjInp_t dataObjInp;
    dataObjInfo_t *cacheObjInfo;

    status = unbunAndStageBunfileObj( rsComm, dataObjInfoHead->filePath,
                                      &cacheRescName );

    if ( status < 0 ) {
        return status;
    }

    /* query the bundle dataObj */
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, dataObjInfoHead->objPath, MAX_NAME_LEN );
    addKeyVal( &dataObjInp.condInput, RESC_NAME_KW, cacheRescName );
    status = getDataObjInfo( rsComm, &dataObjInp, &cacheObjInfo, NULL, 0 );
    clearKeyVal( &dataObjInp.condInput );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unbunAndStageBunfileObj: getDataObjInfo of subfile %s failed.stat=%d",
                 dataObjInp.objPath, status );
        return status;
    }
    /* que the cache copy at the top */
    queDataObjInfo( subfileObjInfoHead, cacheObjInfo, 0, 1 );


    return status;
}

int
unbunAndStageBunfileObj( rsComm_t * rsComm, char * bunfileObjPath, char **outCacheRescName ) {
    dataObjInfo_t *bunfileObjInfoHead;
    dataObjInp_t dataObjInp;
    int status;

    /* query the bundle dataObj */
    bzero( &dataObjInp, sizeof( dataObjInp ) );
    rstrcpy( dataObjInp.objPath, bunfileObjPath, MAX_NAME_LEN );

    status = getDataObjInfo( rsComm, &dataObjInp, &bunfileObjInfoHead, NULL, 1 );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "unbunAndStageBunfileObj: getDataObjInfo of bunfile %s failed.stat=%d",
                 dataObjInp.objPath, status );
        return status;
    }
    status = _unbunAndStageBunfileObj( rsComm, &bunfileObjInfoHead, &dataObjInp.condInput,
                                       outCacheRescName, 0 );

    freeAllDataObjInfo( bunfileObjInfoHead );

    return status;
}

int
_unbunAndStageBunfileObj( rsComm_t * rsComm, dataObjInfo_t **bunfileObjInfoHead, keyValPair_t * condInput,
                          char **outCacheRescName, int rmBunCopyFlag ) {
    int status;
    dataObjInp_t dataObjInp;

    bzero( &dataObjInp, sizeof( dataObjInp ) );
    bzero( &dataObjInp.condInput, sizeof( dataObjInp.condInput ) );
    rstrcpy( dataObjInp.objPath, ( *bunfileObjInfoHead )->objPath, MAX_NAME_LEN );
    status = sortObjInfoForOpen( bunfileObjInfoHead, condInput, 0 );

    addKeyVal( &dataObjInp.condInput, RESC_HIER_STR_KW, ( *bunfileObjInfoHead )->rescHier );
    if ( status < 0 ) {
        return status;
    }

    if ( outCacheRescName != NULL ) {
        *outCacheRescName = ( *bunfileObjInfoHead )->rescName;
    }

    addKeyVal( &dataObjInp.condInput, BUN_FILE_PATH_KW,  // JMC - backport 4768
               ( *bunfileObjInfoHead )->filePath );
    if ( rmBunCopyFlag > 0 ) {
        addKeyVal( &dataObjInp.condInput, RM_BUN_COPY_KW, "" );
    }
    if ( strlen( ( *bunfileObjInfoHead )->dataType ) > 0 ) { // JMC - backport 4664
        addKeyVal( &dataObjInp.condInput, DATA_TYPE_KW,
                   ( *bunfileObjInfoHead )->dataType );
    }
    status = _rsUnbunAndRegPhyBunfile( rsComm, &dataObjInp,
                                       ( *bunfileObjInfoHead )->rescName );

    return status;
}
