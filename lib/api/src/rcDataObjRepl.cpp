/**
 * @file  rcDataObjRepl.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */
/* See dataObjRepl.h for a description of this API call.*/

#include "dataObjRepl.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataObjRepl (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief Replicate a data object to another resource.
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
 * Replicate a data object /myZone/home/john/myfile to myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjRepl (conn, &dataObjInp);
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
 *        (recommanded setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n REPL_NUM_KW - The replica number of the copy to be used as source.
 *    \n RESC_NAME_KW - The copy stored in this resource to be used as source.
 *    \n DEST_RESC_NAME_KW - The resource to store the new replica.
 *    \n BACKUP_RESC_NAME_KW - The resource to store the new replica.
 *             In backup mode. If a good copy already exists in this
 *             resource, don't make another one.
 *    \n ADMIN_KW - admin user backup/replicate other user's files.
 *            This keyWd has no value.
 *    \n FILE_PATH_KW - The physical file path for this data object if the
 *             normal resource vault is not used.
 *    \n RBUDP_TRANSFER_KW - use RBUDP for data transfer. This keyWd has no
 *             value.
 *    \n RBUDP_SEND_RATE_KW - the number of RBUDP packet to send per second
 *          The default is 600000.
 *    \n RBUDP_PACK_SIZE_KW - the size of RBUDP packet. The default is 8192.
 *    \n LOCK_TYPE_KW - set advisory lock type. valid value - READ_LOCK_TYPE.
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 **/

int
rcDataObjRepl( rcComm_t *conn, dataObjInp_t *dataObjInp ) {
    int status;
    transferStat_t *transferStat = nullptr;

    memset( &conn->transStat, 0, sizeof( transferStat_t ) );

    dataObjInp->oprType = REPLICATE_OPR;

    status = _rcDataObjRepl( conn, dataObjInp, &transferStat );

    if ( status >= 0 && transferStat != nullptr ) {
        conn->transStat = *( transferStat );
    }
    if ( transferStat != nullptr ) {
        free( transferStat );
    }
    return status;
}

int
_rcDataObjRepl( rcComm_t *conn, dataObjInp_t *dataObjInp,
                transferStat_t **transferStat ) {
    int status;

    status = procApiRequest( conn, DATA_OBJ_REPL_AN,  dataObjInp, nullptr,
                             ( void ** ) transferStat, nullptr );

    return status;
}

