#include "dataGet.h"
#include "dataObjClose.h"
#include "dataObjGet.h"
#include "dataObjOpen.h"
#include "fileGet.h"
#include "getRemoteZoneResc.h"
#include "objMetaOpr.hpp"
#include "physPath.hpp"
#include "rcGlobalExtern.h"
#include "rodsLog.h"
#include "rsApiHandler.hpp"
#include "rsDataGet.hpp"
#include "rsDataObjClose.hpp"
#include "rsDataObjGet.hpp"
#include "rsDataObjOpen.hpp"
#include "rsFileGet.hpp"
#include "rsFileLseek.hpp"
#include "rsGlobalExtern.hpp"
#include "rsSubStructFileGet.hpp"
#include "specColl.hpp"
#include "subStructFileGet.h"

// =-=-=-=-=-=-=-
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"
#include "key_value_proxy.hpp"

namespace
{
    int _rsDataObjGet(
        rsComm_t *rsComm,
        dataObjInp_t *dataObjInp,
        portalOprOut_t **portalOprOut,
        bytesBuf_t *dataObjOutBBuf)
    {
        char *chksumStr = NULL;

        auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);

        /* PHYOPEN_BY_SIZE ask it to check whether "dataInclude" should be done */
        cond_input[PHYOPEN_BY_SIZE_KW] = "";

        dataObjInp->openFlags = O_RDONLY;
        int fd = rsDataObjOpen(rsComm, dataObjInp);
        if (fd < 0) {
            return fd;
        }

        L1desc[fd].oprType = GET_OPR;

        dataObjInfo_t* dataObjInfo = L1desc[fd].dataObjInfo;
        copyKeyVal( &dataObjInp->condInput, &dataObjInfo->condInput );

        if ( getStructFileType( dataObjInfo->specColl ) >= 0 && // JMC - backport 4682
                L1desc[fd].l3descInx > 0 ) {
            /* l3descInx == 0 if included */
            *portalOprOut = ( portalOprOut_t * ) malloc( sizeof( portalOprOut_t ) );
            bzero( *portalOprOut,  sizeof( portalOprOut_t ) );
            ( *portalOprOut )->l1descInx = fd;
            return fd;
        }

        if ( getValByKey( &dataObjInp->condInput, VERIFY_CHKSUM_KW ) != NULL ) {
            if ( strlen( dataObjInfo->chksum ) > 0 ) {
                /* a chksum already exists */
                chksumStr = strdup( dataObjInfo->chksum );
            }
            else {
                int status = dataObjChksumAndReg( rsComm, dataObjInfo, &chksumStr );
                if ( status < 0 ) {
                    return status;
                }
                rstrcpy( dataObjInfo->chksum, chksumStr, NAME_LEN );
            }
        }

        if ( L1desc[fd].l3descInx <= 2 ) {
            /* no physical file was opened */
            int status = l3DataGetSingleBuf( rsComm, fd, dataObjOutBBuf, portalOprOut );
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


        int status = preProcParaGet( rsComm, fd, portalOprOut );

        if ( status < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = fd;
            rsDataObjClose( rsComm, &dataObjCloseInp );
            free( chksumStr );
            return status;
        }

        status = fd;         /* means file not included */
        if ( chksumStr != NULL ) {
            rstrcpy( ( *portalOprOut )->chksum, chksumStr, NAME_LEN );
        }
        free( chksumStr );

        /* return portalOprOut to the client and wait for the rcOprComplete
         * call. That is when the parallel I/O is done */
        int retval = sendAndRecvBranchMsg( rsComm, rsComm->apiInx, status,
                                       ( void * ) * portalOprOut, dataObjOutBBuf );

        if ( retval < 0 ) {
            openedDataObjInp_t dataObjCloseInp{};
            dataObjCloseInp.l1descInx = fd;
            rsDataObjClose( rsComm, &dataObjCloseInp );
        }

        /* already send the client the status */
        return SYS_NO_HANDLER_REPLY_MSG;
    } // _rsDataObjGet
} // anonymous namespace

int rsDataObjGet(
    rsComm_t *rsComm,
    dataObjInp_t *dataObjInp,
    portalOprOut_t **portalOprOut,
    bytesBuf_t *dataObjOutBBuf)
{
    if (!dataObjOutBBuf) {
        rodsLog(LOG_ERROR, "dataObjOutBBuf was null in call to rsDataObjGet.");
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    remove_trailing_path_separators(dataObjInp->objPath);

    auto cond_input = irods::experimental::make_key_value_proxy(dataObjInp->condInput);

    specCollCache_t *specCollCache = NULL;
    resolveLinkedPath( rsComm, dataObjInp->objPath, &specCollCache, cond_input.get());

    rodsServerHost_t *rodsServerHost;
    int remoteFlag = getAndConnRemoteZone( rsComm, dataObjInp, &rodsServerHost, REMOTE_OPEN);

    if (remoteFlag < 0) {
        return remoteFlag;
    }
    else if (LOCAL_HOST != remoteFlag) {
        int status = _rcDataObjGet(rodsServerHost->conn, dataObjInp, portalOprOut, dataObjOutBBuf);

        if (status < 0) {
            return status;
        }

        if ( status == 0 || dataObjOutBBuf->len > 0 ) {
            /* data included in buf */
            return status;
        }
        else if ( !( *portalOprOut ) ) {
            rodsLog( LOG_ERROR, "_rcDataObjGet returned a %d status code, but left portalOprOut null.", status );
            return SYS_INVALID_PORTAL_OPR;
        }
        else {
            /* have to allocate a local l1descInx to keep track of things
             * since the file is in remote zone. It sets remoteL1descInx,
             * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that
             * rsComplete knows what to do */
            int l1descInx = allocAndSetL1descForZoneOpr(
                            ( *portalOprOut )->l1descInx, dataObjInp, rodsServerHost, NULL );
            if ( l1descInx < 0 ) {
                return l1descInx;
            }
            ( *portalOprOut )->l1descInx = l1descInx;
            return status;
        }
    }

    // =-=-=-=-=-=-=-
    // working on the "home zone", determine if we need to redirect to a different
    // server in this zone for this operation.  if there is a RESC_HIER_STR_KW then
    // we know that the redirection decision has already been made
    if (!cond_input.contains(RESC_HIER_STR_KW)) {
        try {
            auto result = irods::resolve_resource_hierarchy(irods::OPEN_OPERATION, rsComm, *dataObjInp);
            cond_input[RESC_HIER_STR_KW] = std::get<std::string>(result);
        }
        catch (const irods::exception& e) {
            irods::log(e);
            return e.code();
        }
    }

    return _rsDataObjGet(rsComm, dataObjInp, portalOprOut, dataObjOutBBuf);
} // rsDataObjGet


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
    /* just malloc an empty portalOprOut */

    *portalOprOut = ( portalOprOut_t* )malloc( sizeof( portalOprOut_t ) );
    memset( *portalOprOut, 0, sizeof( portalOprOut_t ) );

    dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;

    dataObjOutBBuf->buf = malloc( dataObjInfo->dataSize );
    int bytesRead = l3FileGetSingleBuf( rsComm, l1descInx, dataObjOutBBuf );

    openedDataObjInp_t dataObjCloseInp{};
    dataObjCloseInp.l1descInx = l1descInx;
    int status = rsDataObjClose( rsComm, &dataObjCloseInp );
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "%s: rsDataObjClose of %d error, status = %d",
                 __FUNCTION__, l1descInx, status );
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
    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    dataObjInfo_t* dataObjInfo = L1desc[l1descInx].dataObjInfo;
    std::string location{};
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
        rstrcpy( subFile.addr.hostAddr, location.c_str(), NAME_LEN );

        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode( L1desc[l1descInx].dataObjInp );
        subFile.flags = O_RDONLY;
        subFile.offset = dataObjInfo->dataSize;
        return rsSubStructFileGet( rsComm, &subFile, dataObjOutBBuf );
    }

    fileOpenInp_t fileGetInp{};
    dataObjInp_t* dataObjInp = L1desc[l1descInx].dataObjInp;
    rstrcpy( fileGetInp.addr.hostAddr,  location.c_str(), NAME_LEN );
    rstrcpy( fileGetInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN );
    rstrcpy( fileGetInp.resc_name_, dataObjInfo->rescName, MAX_NAME_LEN );
    rstrcpy( fileGetInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    rstrcpy( fileGetInp.objPath,    dataObjInfo->objPath,  MAX_NAME_LEN );
    fileGetInp.mode = getFileMode( dataObjInp );
    fileGetInp.flags = O_RDONLY;
    fileGetInp.dataSize = dataObjInfo->dataSize;

    copyKeyVal(
        &dataObjInfo->condInput,
        &fileGetInp.condInput );

    /* XXXXX need to be able to handle structured file */
    int bytesRead = rsFileGet( rsComm, &fileGetInp, dataObjOutBBuf );

    clearKeyVal( &fileGetInp.condInput );

    return bytesRead;
}
