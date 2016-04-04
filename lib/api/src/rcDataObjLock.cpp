/**
 * @file  rcDataObjLock.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */
/* See dataObjLock.h for a description of this API call.*/

#include "dataObjLock.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataObjLock (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief Lock a data object.
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
 * Lock and unlock a data object /myZone/home/john/myfile for write:
 * \n dataObjInp_t dataObjInp;
 * \n int lockFd;
 * \n char tmpStr[NAME_LEN];
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n dataObjInp.openFlags = O_WRONLY;
 * \n addKeyVal (&dataObjInp.condInput, LOCK_TYPE_KW, WRITE_LOCK_TYPE);
 * \n # LOCK_CMD_KW input is optional. If not specified, SET_LOCK_WAIT_CMD is assumed
 * \n lockFd = rcDataObjLock (conn, &dataObjInp);
 * \n if (lockFd < 0) {
 * \n .... handle the error
 * \n }
 * \n # now unlock it
 * \n addKeyVal (&dataObjInp.condInput, LOCK_TYPE_KW, UNLOCK_TYPE);
 * \n snprintf (tmpStr, NAME_LEN, "%-d", lockFd);
 * \n addKeyVal (&dataObjInp.condInput, LOCK_FD_KW, tmpStr);
 * \n status = rcDataObjLock (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n LOCK_TYPE_KW - the lock type. Valid values are READ_LOCK_TYPE, WRITE_LOCK_TYPE and UNLOCK_TYPE.
 *    \n LOCK_CMD_KW - the lock command.  Valid values are SET_LOCK_CMD, SET_LOCK_WAIT_CMD and GET_LOCK_CMD.
 *    \n LOCK_FD_KW - the file desc of the locked file. Needed only for UNLOCK_TYPE.
 *
 * \return integer
 * \retval an opened and locked file descriptor for READ_LOCK_TYPE, WRITE_LOCK_TYPE or 0 for UNLOCK_TYPE on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcDataObjLock( rcComm_t *conn, dataObjInp_t *dataObjInp ) {
    int status;
    status = procApiRequest( conn, DATA_OBJ_LOCK_AN,  dataObjInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}

int
rcDataObjUnlock( rcComm_t *conn, dataObjInp_t *dataObjInp ) {
    int status;
    status = procApiRequest( conn, DATA_OBJ_UNLOCK_AN,  dataObjInp, NULL,
                             ( void ** ) NULL, NULL );

    return status;
}

