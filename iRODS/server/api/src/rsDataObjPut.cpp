/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjPut.hpp for a description of this API call.*/

#include "dataObjPut.hpp"
#include "rodsLog.hpp"
#include "dataPut.hpp"
#include "reFuncDefs.hpp"
#include "filePut.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "dataObjOpen.hpp"
#include "dataObjCreate.hpp"
#include "regDataObj.hpp"
#include "dataObjUnlink.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "subStructFilePut.hpp"
#include "dataObjRepl.hpp"
#include "getRemoteZoneResc.hpp"
#include "icatHighLevelRoutines.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"

int
rsDataObjPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
              bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut ) {
    int status;
    int status2;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == LOCAL_HOST ) {
        // =-=-=-=-=-=-=-
        // working on the "home zone", determine if we need to redirect to a different
        // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
        // we know that the redirection decision has already been made
        std::string       hier;
        if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
            irods::error ret = irods::resolve_resource_hierarchy(
                                   irods::CREATE_OPERATION, rsComm,
                                   dataObjInp, hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " :: failed in irods::irods::resolve_resource_hierarchy for [";
                msg << dataObjInp->objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }
            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

        } // if keyword

        status2 = applyRuleForPostProcForWrite( rsComm, dataObjInpBBuf,
                                                dataObjInp->objPath );
        if ( status2 < 0 ) {
            return ( status2 );
        }

        dataObjInp->openFlags = O_RDWR;
        status = _rsDataObjPut( rsComm, dataObjInp, dataObjInpBBuf,
                                portalOprOut );
    }
    else {
        int l1descInx;
        status = _rcDataObjPut( rodsServerHost->conn, dataObjInp,
                                dataObjInpBBuf, portalOprOut );
        if ( status < 0 ||
                getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
            return status;
        }
        else {
            /* have to allocate a local l1descInx to keep track of things
             * since the file is in remote zone. It sets remoteL1descInx,
             * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that
             * rsComplete knows what to do */
            l1descInx = allocAndSetL1descForZoneOpr(
                            ( *portalOprOut )->l1descInx, dataObjInp, rodsServerHost, NULL );
            if ( l1descInx < 0 ) {
                return l1descInx;
            }
            ( *portalOprOut )->l1descInx = l1descInx;
            return status;
        }
    }

    return status;
}

/* _rsDataObjPut - process put request
 * The reply to this API can go off the main part of the API's
 * request/reply protocol and uses the sendAndRecvOffMainMsg call
 * to handle a sequence of request/reply until a return value of
 * SYS_HANDLER_NO_ERROR.
 */

int
_rsDataObjPut( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
               bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut ) {
    int status;
    int l1descInx;
    int retval;
    openedDataObjInp_t dataObjCloseInp;
    int allFlag;
    transferStat_t *transStat = NULL;
    dataObjInp_t replDataObjInp;

    if ( getValByKey( &dataObjInp->condInput, ALL_KW ) != NULL ) {
        allFlag = 1;
    }
    else {
        allFlag = 0;
    }

    if ( getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
        /* single buffer put */
        status = l3DataPutSingleBuf( rsComm, dataObjInp, dataObjInpBBuf );
        if ( status >= 0 && allFlag == 1 ) {
            /* update the rest of copies */
            addKeyVal( &dataObjInp->condInput, UPDATE_REPL_KW, "" );
            status = rsDataObjRepl( rsComm, dataObjInp, &transStat );
            if ( transStat != NULL ) {
                free( transStat );
            }
        }
        if ( status >= 0 ) {
            int status2;
            status2 = applyRuleForPostProcForWrite( rsComm, dataObjInpBBuf,
                                                    dataObjInp->objPath );
            if ( status2 >= 0 ) {
                status = 0;
            }
            else {
                status = status2;
            }
        }
        return status;
    }

    /* get down here. will do parallel I/O */
    /* so that mmap will work */
    dataObjInp->openFlags |= O_RDWR;
    l1descInx = rsDataObjCreate( rsComm, dataObjInp );

    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    L1desc[l1descInx].oprType = PUT_OPR;
    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    if ( getStructFileType( L1desc[l1descInx].dataObjInfo->specColl ) >= 0 ) { // JMC - backport 4682
        *portalOprOut = ( portalOprOut_t * ) malloc( sizeof( portalOprOut_t ) );
        bzero( *portalOprOut,  sizeof( portalOprOut_t ) );
        ( *portalOprOut )->l1descInx = l1descInx;
        return l1descInx;
    }


    status = preProcParaPut( rsComm, l1descInx, portalOprOut );

    if ( status < 0 ) {
        memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = status;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        return status;
    }

    if ( allFlag == 1 ) {
        /* need to save dataObjInp. get freed in sendAndRecvBranchMsg */
        memset( &replDataObjInp, 0, sizeof( replDataObjInp ) );
        rstrcpy( replDataObjInp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
        addKeyVal( &replDataObjInp.condInput, UPDATE_REPL_KW, "" );
        addKeyVal( &replDataObjInp.condInput, ALL_KW, "" );
    }
    /* return portalOprOut to the client and wait for the rcOprComplete
     * call. That is when the parallel I/O is done */
    retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
                                   ( void * ) * portalOprOut, NULL );

    if ( retval < 0 ) {
        memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = retval;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        if ( allFlag == 1 ) {
            clearKeyVal( &replDataObjInp.condInput );
        }
    }
    else if ( allFlag == 1 ) {
        status = rsDataObjRepl( rsComm, &replDataObjInp, &transStat );
        if ( transStat != NULL ) {
            free( transStat );
        }
        clearKeyVal( &replDataObjInp.condInput );
    }

    /* already send the client the status */
    return SYS_NO_HANDLER_REPLY_MSG;

}

/* preProcParaPut - preprocessing for parallel put. Basically it calls
 * rsDataPut to setup portalOprOut with the resource server.
 */

int
preProcParaPut( rsComm_t *rsComm, int l1descInx,
                portalOprOut_t **portalOprOut ) {
    int status;
    dataOprInp_t dataOprInp;

    initDataOprInp( &dataOprInp, l1descInx, PUT_OPR );
    /* add RESC_HIER_STR_KW for getNumThreads */
    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        addKeyVal( &dataOprInp.condInput, RESC_HIER_STR_KW,
                   L1desc[l1descInx].dataObjInfo->rescHier );
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        status =  remoteDataPut( rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost );
    }
    else {
        status =  rsDataPut( rsComm, &dataOprInp, portalOprOut );
    }

    if ( status >= 0 ) {
        ( *portalOprOut )->l1descInx = l1descInx;
        L1desc[l1descInx].bytesWritten = dataOprInp.dataSize;
    }
    clearKeyVal( &dataOprInp.condInput );
    return status;
}

int
l3DataPutSingleBuf( rsComm_t*     rsComm,
                    dataObjInp_t* dataObjInp,
                    bytesBuf_t*   dataObjInpBBuf ) {
    int bytesWritten;
    int l1descInx;
    int status;
    openedDataObjInp_t dataObjCloseInp;
    std::string resc_name;

    /* don't actually physically open the file */
    addKeyVal( &dataObjInp->condInput, NO_OPEN_FLAG_KW, "" );
    l1descInx = rsDataObjCreate( rsComm, dataObjInp );
    if ( l1descInx <= 2 ) {
        if ( l1descInx >= 0 ) {
            rodsLog( LOG_ERROR,
                     "l3DataPutSingleBuf: rsDataObjCreate of %s error, status = %d",
                     dataObjInp->objPath,
                     l1descInx );
            return SYS_FILE_DESC_OUT_OF_RANGE;
        }
        else {
            return l1descInx;

        }
    }

    bytesWritten = _l3DataPutSingleBuf( rsComm, l1descInx, dataObjInp, dataObjInpBBuf );

    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx;
    L1desc[l1descInx].oprStatus = bytesWritten;
    status = rsDataObjClose( rsComm, &dataObjCloseInp );
    if ( status < 0 ) {
        rodsLog( LOG_DEBUG,
                 "l3DataPutSingleBuf: rsDataObjClose of %d error, status = %d",
                 l1descInx, status );
    }

    if ( bytesWritten >= 0 ) {
        return status;
    }

    return bytesWritten;
}


int
_l3DataPutSingleBuf( rsComm_t *rsComm, int l1descInx, dataObjInp_t *dataObjInp,
                     bytesBuf_t *dataObjInpBBuf ) {
    int status = 0;
    int bytesWritten;
    dataObjInfo_t *myDataObjInfo;

    myDataObjInfo = L1desc[l1descInx].dataObjInfo;

    bytesWritten = l3FilePutSingleBuf( rsComm, l1descInx, dataObjInpBBuf );
    if ( bytesWritten >= 0 ) {
        if ( L1desc[l1descInx].replStatus == NEWLY_CREATED_COPY &&
                myDataObjInfo->specColl == NULL &&
                L1desc[l1descInx].remoteZoneHost == NULL ) {
            /* the check for remoteZoneHost host is not needed because
             * the put would have done in the remote zone. But it make
             * the code easier to read (similar ro copy).
             */
            status = svrRegDataObj( rsComm, myDataObjInfo );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "l3DataPutSingleBuf: rsRegDataObj for %s failed, status = %d",
                         myDataObjInfo->objPath, status );
                if ( status != CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME ) {
                    l3Unlink( rsComm, myDataObjInfo );
                }
                return status;
            }
            else {
                myDataObjInfo->replNum = status;
            }
        }
        /* myDataObjInfo->dataSize = bytesWritten; update size problem */
        if ( bytesWritten == 0 && myDataObjInfo->dataSize > 0 ) {
            /* overwrite with 0 len file */
            L1desc[l1descInx].bytesWritten = 1;
        }
        else {
            L1desc[l1descInx].bytesWritten = bytesWritten;
        }
    }
    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    return bytesWritten;
}

int
l3FilePutSingleBuf( rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjInpBBuf ) {
    dataObjInfo_t *dataObjInfo;
    fileOpenInp_t filePutInp;
    int bytesWritten;
    dataObjInp_t *dataObjInp;
    int retryCnt = 0;
    int chkType = 0; // JMC - backport 4774

    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    dataObjInp = L1desc[l1descInx].dataObjInp;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3FilePutSingleBuf - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {
        subFile_t subFile;

        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath, MAX_NAME_LEN );
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );
        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( dataObjInp );
        subFile.flags = O_WRONLY | dataObjInp->openFlags;

        if ( ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) != 0 ) {
            subFile.flags |= FORCE_FLAG;
        }

        bytesWritten = rsSubStructFilePut( rsComm, &subFile, dataObjInpBBuf );
        return bytesWritten;


    } // struct file type >= 0

    std::string prev_resc_hier;
    memset( &filePutInp, 0, sizeof( filePutInp ) );
    rstrcpy( filePutInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( filePutInp.objPath, dataObjInp->objPath, MAX_NAME_LEN );
    if ( ( L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY ) != 0 ) {
        filePutInp.otherFlags |= FORCE_FLAG;
    }

    rstrcpy( filePutInp.addr.hostAddr, location.c_str(), NAME_LEN );
    rstrcpy( filePutInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    filePutInp.mode = getFileMode( dataObjInp );

    filePutInp.flags = O_WRONLY | dataObjInp->openFlags;
    rstrcpy( filePutInp.in_pdmo, L1desc[l1descInx].in_pdmo, MAX_NAME_LEN );

    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    chkType = getchkPathPerm( rsComm, L1desc[l1descInx].dataObjInp, L1desc[l1descInx].dataObjInfo );

    if ( chkType == DISALLOW_PATH_REG ) {
        clearKeyVal( &filePutInp.condInput );
        return PATH_REG_NOT_ALLOWED;
    }
    else if ( chkType == NO_CHK_PATH_PERM ) {
        // =-=-=-=-=-=-=-
        filePutInp.otherFlags |= NO_CHK_PERM_FLAG; // JMC - backport 4758
    }

    filePutOut_t* put_out = 0;
    prev_resc_hier = filePutInp.resc_hier_;
    bytesWritten = rsFilePut( rsComm, &filePutInp, dataObjInpBBuf, &put_out );

    // update the dataObjInfo with the potential changes made by the resource - hcj
    rstrcpy( dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN );
    if ( put_out ) {
        rstrcpy( dataObjInfo->filePath, put_out->file_name, MAX_NAME_LEN );
        free( put_out );
    }

    /* file already exists ? */
    while ( bytesWritten < 0 && retryCnt < 10 &&
            ( filePutInp.otherFlags & FORCE_FLAG ) == 0 &&
            getErrno( bytesWritten ) == EEXIST ) {

        if ( resolveDupFilePath( rsComm, dataObjInfo, dataObjInp ) < 0 ) {
            break;
        }
        rstrcpy( filePutInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );


        filePutOut_t* put_out = 0;
        bytesWritten = rsFilePut( rsComm, &filePutInp, dataObjInpBBuf, &put_out );
        // update the dataObjInfo with the potential changes made by the resource - hcj
        rstrcpy( dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN );
        if ( put_out ) {
            rstrcpy( dataObjInfo->filePath, put_out->file_name, MAX_NAME_LEN );
            free( put_out );
        }
        retryCnt ++;
    } // while
    clearKeyVal( &filePutInp.condInput );
    return bytesWritten;

} // l3FilePutSingleBuf

