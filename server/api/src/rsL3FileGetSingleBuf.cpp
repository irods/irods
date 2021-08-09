#include "l3FileGetSingleBuf.h"
#include "dataObjGet.h"
#include "rodsLog.h"
#include "dataGet.h"
#include "fileGet.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "rsApiHandler.hpp"
#include "objMetaOpr.hpp"
#include "getRemoteZoneResc.h"
#include "rsL3FileGetSingleBuf.hpp"
#include "physPath.hpp"
#include "rsFileGet.hpp"
#include "rsSubStructFileGet.hpp"

#include "irods_resource_backport.hpp"

#include <cstring>

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

int
rsL3FileGetSingleBuf( rsComm_t *rsComm, int *l1descInx,
                      bytesBuf_t *dataObjOutBBuf ) {
    int bytesRead;

    if ( L1desc[*l1descInx].dataObjInfo->dataSize > 0 ) {
        if ( L1desc[*l1descInx].remoteZoneHost != NULL ) {
            bytesRead = rcL3FileGetSingleBuf(
                            L1desc[*l1descInx].remoteZoneHost->conn,
                            L1desc[*l1descInx].remoteL1descInx, dataObjOutBBuf );
        }
        else {
            bytesRead = l3FileGetSingleBuf( rsComm, *l1descInx, dataObjOutBBuf );
        }
    }
    else {
        bytesRead = 0;
        std::memset(dataObjOutBBuf, 0, sizeof(bytesBuf_t));
    }
    return bytesRead;
}
