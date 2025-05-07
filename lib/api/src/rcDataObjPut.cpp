#include "irods/dataObjPut.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"
#include "irods/rcPortalOpr.h"
#include "irods/oprComplete.h"
#include "irods/sockComm.h"
#include "irods/rcMisc.h"
#include "irods/irods_client_server_negotiation.hpp"

#include <cstring>

/**
 * \fn rcDataObjPut (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *   char *locFilePath)
 *
 * \brief Put (upload) a data object to iRODS.
 *
 * \user client
 *
 * \ingroup data_object
 *
 * \since 1.0
 *
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Put (upload) a data object /myZone/home/john/myfile in myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n char locFilePath[MAX_NAME_LEN];
 * \n memset(&dataObjInp, 0, sizeof(dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n rstrcpy (locFilePath, "./mylocalfile", MAX_NAME_LEN);
 * \n dataObjInp.createMode = 0750;
 * \n dataObjInp.dataSize = 12345;
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjPut (conn, &dataObjInp, locFilePath);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b createMode - the file mode of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommended setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FORCE_FLAG_KW - overwrite existing copy. This keyWd has no value
 *    \n REPL_NUM_KW - If the data object already exist, the replica number
 *            of the copy to overwrite.
 *    \n REG_CHKSUM_KW -  register the target checksum value after the copy.
 *            The value is the md5 checksum value of the local file.
 *    \n VERIFY_CHKSUM_KW - verify and register the target checksum value
 *            after the copy. The value is the md5 checksum value of the
 *            local file.
 * \param[in] locFilePath - the path of the local file to upload. This path
 *           can be a relative path.
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/

int
rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath ) {

    int status;
    portalOprOut_t *portalOprOut = NULL;
    bytesBuf_t dataObjInpBBuf;

    if ( dataObjInp->dataSize <= 0 ) {
        dataObjInp->dataSize = getFileSize( locFilePath );
        if ( dataObjInp->dataSize < 0 ) {
            return USER_FILE_DOES_NOT_EXIST;
        }
    }

    addKeyVal( &dataObjInp->condInput, DATA_SIZE_KW, std::to_string(dataObjInp->dataSize).c_str() );

    memset( &conn->transStat, 0, sizeof( transStat_t ) );
    memset( &dataObjInpBBuf, 0, sizeof( dataObjInpBBuf ) );

    rodsEnv rods_env;
    if ( int status = getRodsEnv( &rods_env ) ) {
        rodsLog( LOG_ERROR, "getRodsEnv failed in %s with status %s", __FUNCTION__, status );
        return status;
    }
    int single_buff_sz = rods_env.irodsMaxSizeForSingleBuffer * 1024 * 1024;
    if ( getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
        if ( dataObjInp->dataSize > single_buff_sz ) {
            rmKeyVal( &dataObjInp->condInput, DATA_INCLUDED_KW );
        }
        else {
            status = fillBBufWithFile( conn, &dataObjInpBBuf, locFilePath,
                                       dataObjInp->dataSize );
            if ( status < 0 ) {
                rodsLog( LOG_NOTICE,
                         "rcDataObjPut: fileBBufWithFile error for %s", locFilePath );
                return status;
            }
        }
    }
    else if ( dataObjInp->dataSize <= single_buff_sz ) {
        addKeyVal( &dataObjInp->condInput, DATA_INCLUDED_KW, "" );
        status = fillBBufWithFile( conn, &dataObjInpBBuf, locFilePath,
                                   dataObjInp->dataSize );
        if ( status < 0 ) {
            rodsLog( LOG_NOTICE,
                     "rcDataObjPut: fileBBufWithFile error for %s", locFilePath );
            return status;
        }
    }

    dataObjInp->oprType = PUT_OPR;

    status = _rcDataObjPut( conn, dataObjInp, &dataObjInpBBuf, &portalOprOut );

    clearBBuf( &dataObjInpBBuf );

    if ( status < 0 ||
            getValByKey( &dataObjInp->condInput, DATA_INCLUDED_KW ) != NULL ) {
        if ( portalOprOut != NULL ) {
            free( portalOprOut );
        }
        return status;
    }

    if ( portalOprOut->numThreads <= 0 ) {
        status = putFile( conn, portalOprOut->l1descInx,
                          locFilePath, dataObjInp->objPath, dataObjInp->dataSize );
    }
    else {
        if ( getValByKey( &dataObjInp->condInput, VERY_VERBOSE_KW ) != NULL ) {
            printf( "From server: NumThreads=%d, addr:%s, port:%d, cookie=%d\n",
                    portalOprOut->numThreads, portalOprOut->portList.hostAddr,
                    portalOprOut->portList.portNum, portalOprOut->portList.cookie );
        }
        /* some sanity check */
        rodsEnv rods_env;
        if ( int status = getRodsEnv( &rods_env ) ) {
            rodsLog( LOG_ERROR, "getRodsEnv failed in %s with status %s", __FUNCTION__, status );
            return status;
        }
        if ( portalOprOut->numThreads >= 20 * rods_env.irodsDefaultNumberTransferThreads ) {
            rcOprComplete( conn, SYS_INVALID_PORTAL_OPR );
            free( portalOprOut );
            return SYS_INVALID_PORTAL_OPR;
        }

        conn->transStat.numThreads = portalOprOut->numThreads;
        status = putFileToPortal( conn, portalOprOut, locFilePath,
                                  dataObjInp->objPath, dataObjInp->dataSize );
    }

    /* just send a complete msg */
    if ( status < 0 ) {
        rcOprComplete( conn, status );
    }
    else {
        status = rcOprComplete( conn, portalOprOut->l1descInx );
    }
    free( portalOprOut );

    if ( status >= 0 && conn->fileRestart.info.numSeg > 0 ) { /* file restart */
        clearLfRestartFile( &conn->fileRestart );
    }

    return status;
}

int
_rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp,
               bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut ) {
    int status;

    status = procApiRequest( conn, DATA_OBJ_PUT_AN,  dataObjInp,
                             dataObjInpBBuf, ( void ** ) portalOprOut, NULL );

    if ( *portalOprOut != NULL && ( *portalOprOut )->l1descInx < 0 ) {
        status = ( *portalOprOut )->l1descInx;
    }

    return status;
}
