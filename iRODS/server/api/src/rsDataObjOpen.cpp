/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjOpen.h for a description of this API call.*/

#include "dataObjOpen.hpp"
#include "dataObjOpenAndStat.hpp"
#include "rodsLog.hpp"
#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "resource.hpp"
#include "dataObjOpr.hpp"
#include "physPath.hpp"
#include "dataObjCreate.hpp"
#include "dataObjLock.hpp"

#include "fileOpen.hpp"
#include "subStructFileOpen.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "reDefines.hpp"
#include "getRemoteZoneResc.hpp"
#include "regReplica.hpp"
#include "regDataObj.hpp"
#include "dataObjClose.hpp"
#include "dataObjRepl.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"

int
rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {
    int status, l1descInx;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );
    if ( remoteFlag < 0 ) {
        return ( remoteFlag );
    }
    else if ( remoteFlag == REMOTE_HOST ) {
        openStat_t *openStat = NULL;
        status = rcDataObjOpenAndStat( rodsServerHost->conn, dataObjInp,
                                       &openStat );
        if ( status < 0 ) {
            return status;
        }
        l1descInx = allocAndSetL1descForZoneOpr( status, dataObjInp,
                    rodsServerHost, openStat );
        if ( openStat != NULL ) {
            free( openStat );
        }
        return ( l1descInx );
    }
    else {
        // =-=-=-=-=-=-=-
        // determine the resource hierarchy if one is not provided
        if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
            std::string       hier;
            irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION,
                               rsComm, dataObjInp, hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjInp->objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

        } // if keyword

        l1descInx = _rsDataObjOpen( rsComm, dataObjInp );

    }

    return ( l1descInx );
}

/* _rsDataObjOpen - handle internal server dataObj open request.
 * valid phyOpenFlag are DO_PHYOPEN, DO_NOT_PHYOPEN and PHYOPEN_BY_SIZE
 */

int
_rsDataObjOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp ) {

    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *otherDataObjInfo = NULL;
    dataObjInfo_t *nextDataObjInfo = NULL;
    dataObjInfo_t *tmpDataObjInfo;
    dataObjInfo_t *compDataObjInfo = NULL;
    dataObjInfo_t *cacheDataObjInfo = NULL;
    rescInfo_t *compRescInfo = NULL;
    int l1descInx;
    int writeFlag;
    int phyOpenFlag = DO_PHYOPEN;
    char *lockType = NULL; // JMC - backport 4604
    int lockFd = -1; // JMC - backport 4604

    if ( getValByKey( &dataObjInp->condInput, NO_OPEN_FLAG_KW ) != NULL ) {
        phyOpenFlag = DO_NOT_PHYOPEN;
    }
    else if ( getValByKey( &dataObjInp->condInput, PHYOPEN_BY_SIZE_KW )
              != NULL ) {
        phyOpenFlag = PHYOPEN_BY_SIZE;
    }
    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    lockType = getValByKey( &dataObjInp->condInput, LOCK_TYPE_KW );
    if ( lockType != NULL ) {
        lockFd = rsDataObjLock( rsComm, dataObjInp );
        if ( lockFd > 0 ) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal( &dataObjInp->condInput, LOCK_TYPE_KW );
        }
        else {
            rodsLogError( LOG_ERROR, lockFd,
                          "_rsDataObjOpen: rsDataObjLock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType );
            return lockFd;
        }
    }

    // =-=-=-=-=-=-=-
    /* query rcat for dataObjInfo and sort it */
    status = getDataObjInfoIncSpecColl( rsComm, dataObjInp, &dataObjInfoHead );

    writeFlag = getWriteFlag( dataObjInp->openFlags );

    if ( status < 0 ) {
        if ( dataObjInp->openFlags & O_CREAT && writeFlag > 0 ) {
            l1descInx = rsDataObjCreate( rsComm, dataObjInp );
            status = l1descInx; // JMC - backport 4604
        }
        // =-=-=-=-=-=-=-
        // JMC - backport 4604
        if ( lockFd >= 0 ) {
            if ( status > 0 ) {
                L1desc[l1descInx].lockFd = lockFd;
            }
            else {
                rsDataObjUnlock( rsComm, dataObjInp, lockFd );
            }
        }
        return ( status );
        // =-=-=-=-=-=-=-
    }
    else {
        /* screen out any stale copies */
        status = sortObjInfoForOpen( rsComm, &dataObjInfoHead, &dataObjInp->condInput, writeFlag );
        if ( status < 0 ) { // JMC - backport 4604
            if ( lockFd > 0 ) {
                rsDataObjUnlock( rsComm, dataObjInp, lockFd );
            }
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Unable to select a data obj info matching the resource hierarchy from the keywords.";
            irods::log( ERROR( status, msg.str() ) );
            return status;
        }

        status = applyPreprocRuleForOpen( rsComm, dataObjInp, &dataObjInfoHead );
        if ( status < 0 ) { // JMC - backport 4604
            if ( lockFd > 0 ) {
                rsDataObjUnlock( rsComm, dataObjInp, lockFd );
            }
            return status;
        }
    }

    if ( getStructFileType( dataObjInfoHead->specColl ) >= 0 ) {
        /* special coll. Nothing to do */
    }
    else if ( writeFlag > 0 ) {
        // JMC :: had to reformat this code to find a missing {
        //     :: i seriously hope its in the right place...
        status = procDataObjOpenForWrite( rsComm, dataObjInp, &dataObjInfoHead, &cacheDataObjInfo, &compDataObjInfo, &compRescInfo );
    }

    if ( status < 0 ) {
        if ( lockFd > 0 ) {
            rsDataObjUnlock( rsComm, dataObjInp, lockFd );    // JMC - backport 4604
        }
        freeAllDataObjInfo( dataObjInfoHead );
        return status;
    }

    std::string resc_class;
    irods::error prop_err = irods::get_resource_property<std::string>(
                                dataObjInfoHead->rescInfo->rescName, "class", resc_class );
    if ( prop_err.ok() ) {
        if ( resc_class == "bundle" ) {
            status = stageBundledData( rsComm, &dataObjInfoHead );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsDataObjOpen: stageBundledData of %s failed stat=%d",
                         dataObjInfoHead->objPath, status );
                freeAllDataObjInfo( dataObjInfoHead );
                if ( lockFd >= 0 ) {
                    rsDataObjUnlock( rsComm, dataObjInp, lockFd );    // JMC - backport 4604
                }
                return status;
            }
        }
    }

    /* If cacheDataObjInfo != NULL, this is the staged copy of
     * the compound obj. This copy must be opened.
     * If compDataObjInfo != NULL, an existing COMPOUND_CL DataObjInfo exist.
     * Need to replicate to * this DataObjInfo in rsdataObjClose.
     * At this point, cacheDataObjInfo is left in the dataObjInfoHead queue
     * but not compDataObjInfo.
     * If compRescInfo != NULL, writing to a compound resource where there
     * is no existing copy in the resource. Need to replicate to this
     * resource in rsdataObjClose.
     * For read, both compDataObjInfo and compRescInfo should be NULL.
     */
    tmpDataObjInfo = dataObjInfoHead;

    while ( tmpDataObjInfo != NULL ) {
        nextDataObjInfo = tmpDataObjInfo->next;
        tmpDataObjInfo->next = NULL;
        if ( writeFlag > 0 && cacheDataObjInfo != NULL &&
                tmpDataObjInfo != cacheDataObjInfo ) {
            /* skip anything that does not match cacheDataObjInfo */
            queDataObjInfo( &otherDataObjInfo, tmpDataObjInfo, 1, 1 );
            tmpDataObjInfo = nextDataObjInfo;
            continue;
        }

        status = l1descInx = _rsDataObjOpenWithObjInfo( rsComm, dataObjInp, phyOpenFlag, tmpDataObjInfo, cacheDataObjInfo );

        if ( status >= 0 ) {
            if ( compDataObjInfo != NULL ) {
                L1desc[l1descInx].replDataObjInfo = compDataObjInfo;
            }
            else if ( compRescInfo != NULL ) {
                L1desc[l1descInx].replRescInfo = compRescInfo;
            }

            queDataObjInfo( &otherDataObjInfo, nextDataObjInfo, 0, 1 ); // JMC - backport 4542
            L1desc[l1descInx].otherDataObjInfo = otherDataObjInfo; // JMC - backport 4542

            if ( writeFlag > 0 ) {
                L1desc[l1descInx].openType = OPEN_FOR_WRITE_TYPE;
            }
            else {
                L1desc[l1descInx].openType = OPEN_FOR_READ_TYPE;
            }
            // =-=-=-=-=-=-=-
            // JMC - backport 4604
            if ( lockFd >= 0 ) {
                if ( l1descInx >= 0 ) {
                    L1desc[l1descInx].lockFd = lockFd;
                }
                else {
                    rsDataObjUnlock( rsComm, dataObjInp, lockFd );
                }
            }
            // =-=-=-=-=-=-=-
            return ( l1descInx );

        } // if status >= 0

        tmpDataObjInfo = nextDataObjInfo;
    } // while

    freeAllDataObjInfo( otherDataObjInfo );

    return ( status );
} // BAD

/* _rsDataObjOpenWithObjInfo - given a dataObjInfo, open a single copy
 * of the data object.
 *
 * return l1descInx
 */

int
_rsDataObjOpenWithObjInfo( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                           int phyOpenFlag, dataObjInfo_t *dataObjInfo, dataObjInfo_t *cacheDataObjInfo ) { // JMC - backport 4537
    int replStatus;
    int status;
    int l1descInx;
    l1descInx = allocL1desc();

    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    replStatus = dataObjInfo->replStatus | OPEN_EXISTING_COPY;

    /* the size was set to -1 because we don't know the target size.
     * For copy and replicate, the calling routine should modify this
     * dataSize */
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, replStatus, -1 );
    if ( getValByKey( &dataObjInp->condInput, PURGE_CACHE_KW ) != NULL ) {
        L1desc[l1descInx].purgeCacheFlag = 1;
    }

    if ( phyOpenFlag == DO_NOT_PHYOPEN ) {
        /* don't actually physically open the file */
        status = 0;
    }
    else if ( phyOpenFlag == PHYOPEN_BY_SIZE ) {
        /* open for put or get. May do "dataInclude" */
        if ( getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL
                && dataObjInfo->dataSize <= MAX_SZ_FOR_SINGLE_BUF ) {
            status = 0;
        }
        else if ( dataObjInfo->dataSize != UNKNOWN_FILE_SZ &&
                  dataObjInfo->dataSize < MAX_SZ_FOR_SINGLE_BUF ) {
            status = 0;
        }
        else {
            status = dataOpen( rsComm, l1descInx );
        }
    }
    else {
        status = dataOpen( rsComm, l1descInx );
    }

    if ( status < 0 ) {
        freeL1desc( l1descInx );
        return ( status );
    }
    else {
        return ( l1descInx );
    }
}

int
dataOpen( rsComm_t *rsComm, int l1descInx ) {
    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;


    status = l3Open( rsComm, l1descInx );

    if ( status <= 0 ) {
        rodsLog( LOG_NOTICE,
                 "dataOpen: l3Open of %s failed, status = %d",
                 myDataObjInfo->filePath, status );
        return ( status );
    }
    else {
        L1desc[l1descInx].l3descInx = status;
        return ( 0 );
    }
}

int
l3Open( rsComm_t *rsComm, int l1descInx ) {
    dataObjInfo_t *dataObjInfo;
    int l3descInx;
    int mode, flags;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3Open - failed in specColl open", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        subFile.flags = getFileFlags( l1descInx );
        l3descInx = rsSubStructFileOpen( rsComm, &subFile );
    }
    else {
        mode = getFileMode( L1desc[l1descInx].dataObjInp );
        flags = getFileFlags( l1descInx );
        l3descInx = _l3Open( rsComm, dataObjInfo, mode, flags );
    }
    return ( l3descInx );
}

int
_l3Open( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, int mode, int flags ) {
    int l3descInx;
    fileOpenInp_t fileOpenInp;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3FilePutSingleBuf - failed in get_loc_for_hier_String", ret ) );
        return -1;
    }

    memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );
    rstrcpy( fileOpenInp.resc_name_, dataObjInfo->rescInfo->rescName, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.objPath,    dataObjInfo->objPath, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    rstrcpy( fileOpenInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    fileOpenInp.mode = mode;
    fileOpenInp.flags = flags;
    rstrcpy( fileOpenInp.in_pdmo, dataObjInfo->in_pdmo, MAX_NAME_LEN );
    l3descInx = rsFileOpen( rsComm, &fileOpenInp );

    return ( l3descInx );
}

/* l3OpenByHost - call level3 open - this differs from l3Open in that
 * rsFileOpenByHost is called instead of rsFileOpen
 */

int
l3OpenByHost( rsComm_t *rsComm, int rescTypeInx, int l3descInx, int flags ) {
    fileOpenInp_t fileOpenInp;
    int newL3descInx;

    memset( &fileOpenInp, 0, sizeof( fileOpenInp ) );
    rstrcpy( fileOpenInp.resc_hier_, FileDesc[l3descInx].rescHier, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.fileName, FileDesc[l3descInx].fileName, MAX_NAME_LEN );
    rstrcpy( fileOpenInp.objPath, FileDesc[l3descInx].objPath, MAX_NAME_LEN );
    fileOpenInp.mode = FileDesc[l3descInx].mode;
    fileOpenInp.flags = flags;
    newL3descInx = rsFileOpenByHost( rsComm, &fileOpenInp,
                                     FileDesc[l3descInx].rodsServerHost );

    return ( newL3descInx );
}

int
applyPreprocRuleForOpen( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                         dataObjInfo_t **dataObjInfoHead ) {
    int status;
    ruleExecInfo_t rei;

    initReiWithDataObjInp( &rei, rsComm, dataObjInp );
    rei.doi = *dataObjInfoHead;

    status = applyRule( "acPreprocForDataObjOpen", NULL, &rei, NO_SAVE_REI );

    if ( status < 0 ) {
        if ( rei.status < 0 ) {
            status = rei.status;
        }
        rodsLog( LOG_ERROR,
                 "applyPreprocRuleForOpen:acPreprocForDataObjOpen error for %s,stat=%d",
                 dataObjInp->objPath, status );
    }
    else {
        *dataObjInfoHead = rei.doi;
    }
    return ( status );
}

/* createEmptyRepl - Physically create a zero length file and register
 * as a replica and queue the dataObjInfo on the top of dataObjInfoHead.
 */
int
createEmptyRepl( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                 dataObjInfo_t **dataObjInfoHead ) {
    int status;
    char *rescName;
    rescInfo_t *rescInfo;
    rescGrpInfo_t *tmpRescGrpInfo;
    regReplica_t regReplicaInp;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    keyValPair_t *condInput = &dataObjInp->condInput;
    dataObjInfo_t *myDataObjInfo;

    if ( ( rescName = getValByKey( condInput, DEST_RESC_NAME_KW ) ) == NULL &&
            ( rescName = getValByKey( condInput, BACKUP_RESC_NAME_KW ) ) == NULL &&
            ( rescName = getValByKey( condInput, DEF_RESC_NAME_KW ) ) == NULL ) {
        return USER_NO_RESC_INPUT_ERR;
    }

    status = getRescGrpForCreate( rsComm, dataObjInp, &myRescGrpInfo );
    if ( status < 0 || myRescGrpInfo == NULL ) {
        return status;    // JMC cppcheck
    }

    myDataObjInfo = ( dataObjInfo_t* )malloc( sizeof( dataObjInfo_t ) );
    *myDataObjInfo = *( *dataObjInfoHead );
    tmpRescGrpInfo = myRescGrpInfo;
    while ( tmpRescGrpInfo != NULL ) {
        rescInfo = ( *dataObjInfoHead )->rescInfo;

        myDataObjInfo->rescInfo = new rescInfo_t;
        memcpy( myDataObjInfo->rescInfo, rescInfo, sizeof( rescInfo_t ) );

        rstrcpy( myDataObjInfo->rescName, rescInfo->rescName, NAME_LEN );
        rstrcpy( myDataObjInfo->rescGroupName, ( *dataObjInfoHead )->rescGroupName, NAME_LEN );

        char* resc_hier = getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW );
        if ( resc_hier ) {
            rstrcpy( myDataObjInfo->rescHier, resc_hier, MAX_NAME_LEN ); // hier sent from upper level code
        }
        else {
            rodsLog( LOG_NOTICE, "createEmptyRepl :: using rescInfo->rescName for hier" );
            rstrcpy( myDataObjInfo->rescHier, rescInfo->rescName, MAX_NAME_LEN ); // in kw else

        }


        status = getFilePathName( rsComm, myDataObjInfo, dataObjInp );
        if ( status < 0 ) {
            tmpRescGrpInfo = tmpRescGrpInfo->next;
            continue;
        }
        status = l3CreateByObjInfo( rsComm, dataObjInp, myDataObjInfo );
        if ( status < 0 ) {
            tmpRescGrpInfo = tmpRescGrpInfo->next;
            continue;
        }
        /* close it */
        _l3Close( rsComm, rescInfo->rescTypeInx, status );

        /* register the replica */
        memset( &regReplicaInp, 0, sizeof( regReplicaInp ) );
        regReplicaInp.srcDataObjInfo = *dataObjInfoHead;
        regReplicaInp.destDataObjInfo = myDataObjInfo;
        if ( getValByKey( &dataObjInp->condInput, ADMIN_KW ) != NULL ) {
            addKeyVal( &regReplicaInp.condInput, ADMIN_KW, "" );
        }
        status = rsRegReplica( rsComm, &regReplicaInp );
        clearKeyVal( &regReplicaInp.condInput );

        break;
    }

    freeAllRescGrpInfo( myRescGrpInfo );

    if ( status < 0 ) {
        free( myDataObjInfo );
    }
    else {
        /* queue it on top */
        myDataObjInfo->next = *dataObjInfoHead;
        *dataObjInfoHead = myDataObjInfo;
    }
    return status;
}

// =-=-=-=-=-=-=-
// JMC - backport 4590
int
procDataObjOpenForWrite( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                         dataObjInfo_t **dataObjInfoHead, dataObjInfo_t **cacheDataObjInfo,
                         dataObjInfo_t **compDataObjInfo, rescInfo_t **compRescInfo ) {
    int status = 0;
    rescGrpInfo_t *myRescGrpInfo = NULL;

    /* put the copy with destResc on top */
    status = requeDataObjInfoByDestResc( dataObjInfoHead, &dataObjInp->condInput, 1, 1 );

    /* status < 0 means there is no copy in the DEST_RESC */
    if ( status < 0 && ( *dataObjInfoHead )->specColl == NULL &&
            getValByKey( &dataObjInp->condInput, DEST_RESC_NAME_KW ) != NULL ) {
        /* we don't have a copy in the DEST_RESC_NAME */
        status = getRescGrpForCreate( rsComm, dataObjInp, &myRescGrpInfo );
        if ( status < 0 ) {
            return status;
        }

        /* we don't have a copy, so create an empty dataObjInfo */
        status = createEmptyRepl( rsComm, dataObjInp, dataObjInfoHead );
        if ( status < 0 ) {
            rodsLogError( LOG_ERROR, status,
                          "procDataObjForOpenWrite: createEmptyRepl of %s failed",
                          ( *dataObjInfoHead )->objPath );
            freeAllRescGrpInfo( myRescGrpInfo );
            return status;
        }

    }
    else {     /*  The target data object exists */
        status = 0;
    }

    if ( *compDataObjInfo != NULL ) {
        dequeDataObjInfo( dataObjInfoHead, *compDataObjInfo );
    }
    freeAllRescGrpInfo( myRescGrpInfo );
    return status;
}


/// @brief Selects the dataObjInfo in the specified list whose resc hier matches that of the cond input
irods::error selectObjInfo(
    dataObjInfo_t * _dataObjInfoHead,
    keyValPair_t* _condInput,
    dataObjInfo_t** _rtn_dataObjInfo ) {
    irods::error result = SUCCESS();
    *_rtn_dataObjInfo = NULL;
    char* resc_hier = getValByKey( _condInput, RESC_HIER_STR_KW );
    if ( !resc_hier ) {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - No resource hierarchy specified in keywords.";
        result = ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
    }
    else {
        for ( dataObjInfo_t* dataObjInfo = _dataObjInfoHead;
                result.ok() && *_rtn_dataObjInfo == NULL && dataObjInfo != NULL;
                dataObjInfo = dataObjInfo->next ) {
            if ( strcmp( resc_hier, dataObjInfo->rescHier ) == 0 ) {
                *_rtn_dataObjInfo = dataObjInfo;
            }
        }
        if ( *_rtn_dataObjInfo == NULL ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to find a data obj matching resource hierarchy: \"";
            msg << resc_hier;
            msg << "\"";
            result = ERROR( HIERARCHY_ERROR, msg.str() );
        }
    }
    return result;
}
