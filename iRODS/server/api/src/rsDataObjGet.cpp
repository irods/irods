/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */
/* See dataObjGet.h for a description of this API call.*/

#include "reFuncDefs.hpp"
#include "dataObjGet.hpp"
#include "rodsLog.hpp"
#include "dataGet.hpp"
#include "fileGet.hpp"
#include "dataObjOpen.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "specColl.hpp"
#include "subStructFileGet.hpp"
#include "getRemoteZoneResc.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"

int
rsDataObjGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
              portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf ) {
    int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    if ( dataObjOutBBuf == NULL ) {
        rodsLog( LOG_ERROR, "dataObjOutBBuf was null in call to rsDataObjGet." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput );
    remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_OPEN );

    if ( remoteFlag < 0 ) {
        return remoteFlag;
    }
    else if ( remoteFlag == LOCAL_HOST ) {
        // =-=-=-=-=-=-=-
        // working on the "home zone", determine if we need to redirect to a different
        // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
        // we know that the redirection decision has already been made
        if ( getValByKey( &dataObjInp->condInput, RESC_HIER_STR_KW ) == NULL ) {
            std::string       hier;
            irods::error ret = irods::resolve_resource_hierarchy( irods::OPEN_OPERATION, rsComm,
                               dataObjInp, hier );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << "rsDataObjGet :: failed in irods::resolve_resource_hierarchy for [";
                msg << dataObjInp->objPath << "]";
                irods::log( PASSMSG( msg.str(), ret ) );
                return ret.code();
            }

            // =-=-=-=-=-=-=-
            // we resolved the redirect and have a host, set the hier str for subsequent
            // api calls, etc.
            addKeyVal( &dataObjInp->condInput, RESC_HIER_STR_KW, hier.c_str() );

        } // if keyword

        status = _rsDataObjGet( rsComm, dataObjInp, portalOprOut, dataObjOutBBuf, BRANCH_MSG );

    }
    else {
        int l1descInx;
        status = _rcDataObjGet( rodsServerHost->conn, dataObjInp, portalOprOut,
                                dataObjOutBBuf );

        if ( status < 0 ) {
            return status;
        }
        if ( status == 0 || dataObjOutBBuf->len > 0 ) {
            /* data included in buf */
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

int
_rsDataObjGet( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
               portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf, int handlerFlag ) {
    int status;
    dataObjInfo_t *dataObjInfo;
    int l1descInx;
    char *chksumStr = NULL;
    int retval;
    openedDataObjInp_t dataObjCloseInp;

    /* PHYOPEN_BY_SIZE ask it to check whether "dataInclude" should be done */
    addKeyVal( &dataObjInp->condInput, PHYOPEN_BY_SIZE_KW, "" );
    l1descInx = _rsDataObjOpen( rsComm, dataObjInp );

    if ( l1descInx < 0 ) {
        return l1descInx;
    }

    L1desc[l1descInx].oprType = GET_OPR;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 && // JMC - backport 4682
            L1desc[l1descInx].l3descInx > 0 ) {
        /* l3descInx == 0 if included */
        *portalOprOut = ( portalOprOut_t * ) malloc( sizeof( portalOprOut_t ) );
        bzero( *portalOprOut,  sizeof( portalOprOut_t ) );
        ( *portalOprOut )->l1descInx = l1descInx;
        return l1descInx;
    }

    if ( getValByKey( &dataObjInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
        if ( strlen( dataObjInfo->chksum ) > 0 ) {
            /* a chksum already exists */
            chksumStr = strdup( dataObjInfo->chksum );
        }
        else {
            status = dataObjChksumAndReg( rsComm, dataObjInfo, &chksumStr );
            if ( status < 0 ) {
                return status;
            }
            rstrcpy( dataObjInfo->chksum, chksumStr, NAME_LEN );
        }
    }

    if ( L1desc[l1descInx].l3descInx <= 2 ) {
        /* no physical file was opened */
        status = l3DataGetSingleBuf( rsComm, l1descInx, dataObjOutBBuf,
                                     portalOprOut );
        if ( status >= 0 ) {
            int status2;
            status2 = applyRuleForPostProcForRead( rsComm, dataObjOutBBuf,
                                                   dataObjInp->objPath );
            if ( status2 >= 0 ) {
                status = 0;
            }
            else {
                status = status2;
            }
            if ( chksumStr != NULL ) {
                rstrcpy( ( *portalOprOut )->chksum, chksumStr, NAME_LEN );
            }
        }
        free( chksumStr );
        return status;
    }


    status = preProcParaGet( rsComm, l1descInx, portalOprOut );

    if ( status < 0 ) {
        memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
        dataObjCloseInp.l1descInx = l1descInx;
        rsDataObjClose( rsComm, &dataObjCloseInp );
        free( chksumStr );
        return status;
    }

    status = l1descInx;         /* means file not included */
    if ( chksumStr != NULL ) {
        rstrcpy( ( *portalOprOut )->chksum, chksumStr, NAME_LEN );
    }
    free( chksumStr );

    /* return portalOprOut to the client and wait for the rcOprComplete
     * call. That is when the parallel I/O is done */
    retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
                                   ( void * ) * portalOprOut, dataObjOutBBuf );

    if ( retval < 0 ) {
        memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
        dataObjCloseInp.l1descInx = l1descInx;
        rsDataObjClose( rsComm, &dataObjCloseInp );
    }

    if ( handlerFlag & INTERNAL_SVR_CALL ) {
        /* internal call. want to know the real status */
        return retval;
    }
    else {
        /* already send the client the status */
        return SYS_NO_HANDLER_REPLY_MSG;
    }

}

/* preProcParaGet - preprocessing for parallel get. Basically it calls
 * rsDataGet to setup portalOprOut with the resource server.
 */
int
preProcParaGet( rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut ) {
    int status;
    dataOprInp_t dataOprInp;

    initDataOprInp( &dataOprInp, l1descInx, GET_OPR );
    /* add RESC_HIER_STR_KW for getNumThreads */
    if ( L1desc[l1descInx].dataObjInfo != NULL ) {
        //addKeyVal (&dataOprInp.condInput, RESC_NAME_KW,
        //           L1desc[l1descInx].dataObjInfo->rescInfo->rescName);
        addKeyVal( &dataOprInp.condInput, RESC_HIER_STR_KW,
                   L1desc[l1descInx].dataObjInfo->rescHier );
    }
    if ( L1desc[l1descInx].remoteZoneHost != NULL ) {
        status =  remoteDataGet( rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost );
    }
    else {
        status =  rsDataGet( rsComm, &dataOprInp, portalOprOut );
    }

    if ( status >= 0 ) {
        ( *portalOprOut )->l1descInx = l1descInx;
    }
    clearKeyVal( &dataOprInp.condInput );
    return status;
}

int
l3DataGetSingleBuf( rsComm_t *rsComm, int l1descInx,
                    bytesBuf_t *dataObjOutBBuf, portalOprOut_t **portalOprOut ) {
    int status = 0;
    int bytesRead;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *dataObjInfo;

    /* just malloc an empty portalOprOut */

    *portalOprOut = ( portalOprOut_t* )malloc( sizeof( portalOprOut_t ) );
    memset( *portalOprOut, 0, sizeof( portalOprOut_t ) );

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if ( dataObjInfo->dataSize > 0 ) {
        dataObjOutBBuf->buf = malloc( dataObjInfo->dataSize );
        bytesRead = l3FileGetSingleBuf( rsComm, l1descInx, dataObjOutBBuf );
    }
    else {
        bytesRead = 0;
    }

    memset( &dataObjCloseInp, 0, sizeof( dataObjCloseInp ) );
    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose( rsComm, &dataObjCloseInp );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "l3DataGetSingleBuf: rsDataObjClose of %d error, status = %d",
                 l1descInx, status );
    }

    if ( bytesRead < 0 ) {
        return bytesRead;
    }
    else {
        return status;
    }
}

/* l3FileGetSingleBuf - Get the content of a small file into a single buffer
 * in dataObjOutBBuf->buf for an opened data obj in l1descInx.
 * Return value - int - number of bytes read.
 */

int
l3FileGetSingleBuf( rsComm_t *rsComm, int l1descInx,
                    bytesBuf_t *dataObjOutBBuf ) {
    dataObjInfo_t *dataObjInfo;
    fileOpenInp_t fileGetInp;
    int bytesRead;
    dataObjInp_t *dataObjInp;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "l3FileGetSingleBuf - failed in get_loc_for_hier_string", ret ) );
        return -1;
    }

    if ( getStructFileType( dataObjInfo->specColl ) >= 0 ) {



        subFile_t subFile;
        memset( &subFile, 0, sizeof( subFile ) );
        rstrcpy( subFile.subFilePath, dataObjInfo->subPath,
                 MAX_NAME_LEN );
        //rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,NAME_LEN);
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        subFile.flags = O_RDONLY;
        subFile.offset = dataObjInfo->dataSize;
        bytesRead = rsSubStructFileGet( rsComm, &subFile, dataObjOutBBuf );
        return bytesRead;
    }

    memset( &fileGetInp, 0, sizeof( fileGetInp ) );
    dataObjInp = L1desc[l1descInx].dataObjInp;
    rstrcpy( fileGetInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    rstrcpy( fileGetInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( fileGetInp.resc_name_, dataObjInfo->rescName, MAX_NAME_LEN );
    rstrcpy( fileGetInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileGetInp.objPath,    dataObjInfo->objPath,  MAX_NAME_LEN );
    fileGetInp.mode = getFileMode( dataObjInp );
    fileGetInp.flags = O_RDONLY;
    fileGetInp.dataSize = dataObjInfo->dataSize;
    /* XXXXX need to be able to handle structured file */
    bytesRead = rsFileGet( rsComm, &fileGetInp, dataObjOutBBuf );
    return bytesRead;
}

