/**
 * @file  rcDataObjPhymv.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */
/* See dataObjPhymv.h for a description of this API call.*/

#include "dataObjPhymv.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataObjPhymv (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief  Physically move a data object in iRODS from one resource to
 *            another storage resource.
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
 * Phymv a data object /myZone/home/john/myfile to myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjPhymv (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommanded setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n REPL_NUM_KW - the replica number of the copy to phymv.
 *    \n RESC_NAME_KW - the source resource for the phymv.
 *    \n DEST_RESC_NAME_KW - The target resource for the phymv.
 *    \n ADMIN_KW - admin user phymove other users files.
 *             This keyWd has no value.
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcDataObjPhymv( rcComm_t *conn, dataObjInp_t *dataObjInp ) {
    int status;
    transferStat_t *transferStat = nullptr;

    memset( &conn->transStat, 0, sizeof( transferStat_t ) );

    dataObjInp->oprType = PHYMV_OPR;

    status = _rcDataObjPhymv( conn, dataObjInp, &transferStat );

    if ( status >= 0 && transferStat != nullptr ) {
        conn->transStat = *( transferStat );
    }
    if ( transferStat != nullptr ) {
        free( transferStat );
    }
    return status;
}

int
_rcDataObjPhymv( rcComm_t *conn, dataObjInp_t *dataObjInp,
                 transferStat_t **transferStat ) {
    int status;

    status = procApiRequest( conn, DATA_OBJ_PHYMV_AN,  dataObjInp, nullptr,
                             ( void ** ) transferStat, nullptr );
    return status;
}
