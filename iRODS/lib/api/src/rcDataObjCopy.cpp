/**
 * @file  rcDataObjCopy.c
 *
 */
/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjCopy.h for a description of this API call.*/

#include "dataObjCopy.h"

/**
 * \fn rcDataObjCopy (rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp)
 *
 * \brief Copy a data object from a iRODS path to another a iRODS path. 
 *
 * \user client
 *
 * \category data object operations
 *
 * \since 1.0
 *
 * \author  Mike Wan
 * \date    2007
 *
 * \remark none
 *
 * \note none
 *
 * \usage
 * Copy a data object /myZone/home/john/myfileA to /myZone/home/john/myfileB
 *     and store in myRescource:
 * \n dataObjCopyInp_t dataObjCopyInp;
 * \n bzero (&dataObjCopyInp, sizeof (dataObjCopyInp));
 * \n rstrcpy (dataObjCopyInp.destDataObjInp.objPath, 
 *       "/myZone/home/john/myfileB", MAX_NAME_LEN);
 * \n rstrcpy (dataObjCopyInp.srcDataObjInp.objPath, 
 *      "/myZone/home/john/myfileA", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjCopyInp.destDataObjInp.condInput, DEST_RESC_NAME_KW, 
 *      "myRescource");
 * \n status = rcDataObjCopy (conn, &dataObjCopyInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjCopyInp_t - Elements of dataObjCopyInp_t used :
 *    \li char \b srcDataObjInp.objPath[MAX_NAME_LEN] - full path of the 
 *         source data object.
 *    \li char \b destDataObjInp.objPath[MAX_NAME_LEN] - full path of the 
 *         target data object.
 *    \li int \b destDataObjInp.numThreads - the number of threads to use. 
 *        Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommanded setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b destDataObjInp.condInput - keyword/value pair input. 
 *       Valid keywords:
 *    \n DATA_TYPE_KW - the data type of the data object.
 *    \n FORCE_FLAG_KW - overwrite existing target. This keyWd has no value
 *    \n REG_CHKSUM_KW -  register the target checksum value after the copy.
 *	      This keyWd has no value.
 *    \n VERIFY_CHKSUM_KW - verify and register the target checksum value 
 *            after the copy. This keyWd has no value.
 *    \n DEST_RESC_NAME_KW - The resource to store this data object
 *    \n FILE_PATH_KW - The physical file path for this data object if the
 *             normal resource vault is not used.
 *    \n RBUDP_TRANSFER_KW - use RBUDP for data transfer. This keyWd has no 
 *             value
 *    \n RBUDP_SEND_RATE_KW - the number of RBUDP packet to send per second 
 *	    The default is 600000
 *    \n RBUDP_PACK_SIZE_KW - the size of RBUDP packet. The default is 8192 
 *
 * \return integer
 * \retval 0 on success

 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
 * \bug  no known bugs
**/

int
rcDataObjCopy (rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp)
{
    int status;
    transferStat_t *transferStat = NULL;

    memset (&conn->transStat, 0, sizeof (transferStat_t));

    dataObjCopyInp->srcDataObjInp.oprType = COPY_SRC;
    dataObjCopyInp->destDataObjInp.oprType = COPY_DEST;

    status = _rcDataObjCopy (conn, dataObjCopyInp, &transferStat);

    if (status >= 0 && transferStat != NULL) {
        conn->transStat = *(transferStat);
    } else if (status == SYS_UNMATCHED_API_NUM) {
         /* try older version */
        transStat_t *transStat = NULL;
        status = _rcDataObjCopy250 (conn, dataObjCopyInp, &transStat);
        if (status >= 0 && transStat != NULL) {
            conn->transStat.numThreads = transStat->numThreads;
            conn->transStat.bytesWritten = transStat->bytesWritten;
            conn->transStat.flags = 0;
        }
        if (transStat != NULL) free (transStat);
        return status;
    }
    if (transferStat != NULL) {
        free (transferStat);
    }

    return (status);
}

int
_rcDataObjCopy (rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp,
transferStat_t **transferStat)
{
    int status;

    status = procApiRequest (conn, DATA_OBJ_COPY_AN,  dataObjCopyInp, NULL,
        (void **) transferStat, NULL);

    return status;
}

int
_rcDataObjCopy250 (rcComm_t *conn, dataObjCopyInp_t *dataObjCopyInp,
transStat_t **transStat)
{
    int status;

    status = procApiRequest (conn, DATA_OBJ_COPY250_AN,  dataObjCopyInp, NULL,
        (void **) transStat, NULL);

    return status;
}

