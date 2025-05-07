#include "irods/dataObjRepl.h"
#include "irods/procApiRequest.h"
#include "irods/apiNumber.h"

#include <cstring>

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
 * \n memset(&dataObjInp, 0, sizeof(dataObjInp));
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
 *        (recommended setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n REPL_NUM_KW - The replica number of the copy to be used as source.
 *    \n RESC_NAME_KW - The copy stored in this resource to be used as source.
 *    \n DEST_RESC_NAME_KW - The resource to store the new replica.
 *    \n ADMIN_KW - admin user backup/replicate other user's files.
 *            This keyWd has no value.
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
    transferStat_t *transferStat = NULL;

    memset( &conn->transStat, 0, sizeof( transferStat_t ) );

    dataObjInp->oprType = REPLICATE_OPR;

    status = _rcDataObjRepl( conn, dataObjInp, &transferStat );

    if ( status >= 0 && transferStat != NULL ) {
        conn->transStat = *( transferStat );
    }
    if ( transferStat != NULL ) {
        free( transferStat );
    }
    return status;
}

int
_rcDataObjRepl( rcComm_t *conn, dataObjInp_t *dataObjInp,
                transferStat_t **transferStat ) {
    int status;

    status = procApiRequest( conn, DATA_OBJ_REPL_AN,  dataObjInp, NULL,
                             ( void ** ) transferStat, NULL );

    return status;
}

