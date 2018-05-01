/**
 * @file  rcDataObjUnlink.cpp
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */
/* See dataObjUnlink.h for a description of this API call.*/

#include "dataObjUnlink.h"
#include "procApiRequest.h"
#include "apiNumber.h"

/**
 * \fn rcDataObjUnlink (rcComm_t *conn, dataObjInp_t *dataObjUnlinkInp)
 *
 * \brief Unlink/delete a data object if the "forceFlag" is set. Otherwise,
 *	move the data object to trash.
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
 * Unlink the data object /myZone/home/john/myfile
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW, "");
 * \n status = rcDataObjUnlink (conn, &dataObjInp);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjUnlinkInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li char \b oprType - 0 normally.When set to UNREG_OPR, the data object
 *         is unregistered but the physical file is not deleted.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n FORCE_FLAG_KW - delete the data object. If it is not set, the data
 *         object is moved to trash. This keyWd has no value.
 *    \n REPL_NUM_KW - The replica number of the replica to delete.
 *    \n RESC_NAME_KW - delete replica stored in this resource.
 *
 * \return integer
 * \retval 0 on success
 * \sideeffect none
 * \pre none
 * \post none
 * \sa none
**/

int
rcDataObjUnlink( rcComm_t *conn, dataObjInp_t *dataObjUnlinkInp ) {
    int status;
    status = procApiRequest( conn, DATA_OBJ_UNLINK_AN,  dataObjUnlinkInp, nullptr,
                             ( void ** ) nullptr, nullptr );

    return status;
}
