/**
 * @file  rcDataObjRsync.c
 *
 */

/* This is script-generated code.  */ 
/* See dataObjRsync.h for a description of this API call.*/

#include "dataObjRsync.h"
#include "dataObjPut.h"
#include "dataObjGet.h"

/**
 * \fn rcDataObjRsync (rcComm_t *conn, dataObjInp_t *dataObjInp)
 *
 * \brief Synchronize an iRODS data object with a local file or with another
 *        data object.
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
 * Synchronize a data object /myZone/home/john/myfileA to another data object
 *    /myZone/home/john/myfileB and put the new data object in
 *    resource myRescource:
 * \n dataObjInp_t dataObjInp;
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfileA", MAX_NAME_LEN);
 * \n addKeyVal (&dataObjInp.condInput, RSYNC_MODE_KW, IRODS_TO_IRODS);
 * \n addKeyVal (&dataObjInp.condInput, RSYNC_DEST_PATH_KW, 
 *      "/myZone/home/john/myfileB");
 * \n addKeyVal (&dataObjInp.condInput, DEST_RESC_NAME_KW, "myRescource");
 * \n status = rcDataObjRsync (conn, &dataObjInp);
 * \n if (status < 0) {
* \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - For LOCAL_TO_IRODS or 
 *         IRODS_TO_LOCAL modes, this is the iRODS data object path.
 *         For IRODS_TO_IRODS mode, this is the source path.
 *    \li int \b createMode - the file mode of the data object. Meaningful
 *	    only for LOCAL_TO_IRODS mode. 
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommanded setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RSYNC_MODE_KW - The mode of the Synchronization. Valid modes are:
 *	 LOCAL_TO_IRODS - synchronize from a local file to a iRODS data object.
 *	 IRODS_TO_LOCAL - synchronize from a iRODS data object to a local file.
 *	 IRODS_TO_IRODS - synchronize from a iRODS data object to a 
 *            iRODS data object..
 *    \n RSYNC_DEST_PATH_KW - For LOCAL_TO_IRODS or IRODS_TO_LOCAL modes,
 *         this is the local file path. For IRODS_TO_IRODS mode, this is 
 *         the target path. 
 *    \n RSYNC_CHKSUM_KW - The md5 checksum value of the local file. valid
 *            only for LOCAL_TO_IRODS or IRODS_TO_LOCAL modes.
 *    \n DEST_RESC_NAME_KW - The resource to store the new data object. 
 *            Valid only for LOCAL_TO_IRODS or IRODS_TO_IRODS modes.
 *    \n ALL_KW - replicate to all resources in the resource group if the
 *             input resource (via DEST_RESC_NAME_KW) is a resource group.
 *            This keyWd has no value. Valid only for LOCAL_TO_IRODS or 
 *            IRODS_TO_IRODS modes.
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
rcDataObjRsync (rcComm_t *conn, dataObjInp_t *dataObjInp) 
{
    int status;
    msParamArray_t *outParamArray = NULL;
    char *locFilePath;

    status = _rcDataObjRsync (conn, dataObjInp, &outParamArray);

    if (status == SYS_SVR_TO_CLI_PUT_ACTION) {
        if ((locFilePath = getValByKey (&dataObjInp->condInput,
          RSYNC_DEST_PATH_KW)) == NULL) {
	    return USER_INPUT_PATH_ERR;
	} else {
	    status = rcDataObjPut (conn, dataObjInp, locFilePath);
	    if (status >= 0) {
		return SYS_RSYNC_TARGET_MODIFIED;
	    } else {
	        return status;
	    }
	}
    } else if (status == SYS_SVR_TO_CLI_GET_ACTION) {
        if ((locFilePath = getValByKey (&dataObjInp->condInput,
          RSYNC_DEST_PATH_KW)) == NULL) {
            return USER_INPUT_PATH_ERR;
        } else {
            status = rcDataObjGet (conn, dataObjInp, locFilePath);
            if (status >= 0) {
                return SYS_RSYNC_TARGET_MODIFIED;
            } else {
                return status;
            }
	}
    }

    /* below is for backward compatibility */
    while (status == SYS_SVR_TO_CLI_MSI_REQUEST) {
	/* it is a server request */
        msParam_t *myMsParam;
        dataObjInp_t *dataObjInp = NULL;
	int l1descInx; 

	myMsParam = getMsParamByLabel (outParamArray, CL_ZONE_OPR_INX);
	if (myMsParam == NULL) {
	    l1descInx = -1;
	} else {
	    l1descInx = *(int*) myMsParam->inOutStruct;
	}

	if ((myMsParam = getMsParamByLabel (outParamArray, CL_PUT_ACTION))
	  != NULL) { 

	    dataObjInp = (dataObjInp_t *) myMsParam->inOutStruct;
	    if ((locFilePath = getValByKey (&dataObjInp->condInput, 
	      RSYNC_DEST_PATH_KW)) == NULL) {
		if (l1descInx >= 0) {
		    rcOprComplete (conn, l1descInx);
		} else {
		        rcOprComplete (conn, USER_FILE_DOES_NOT_EXIST);
		}
	    } else {
	        status = rcDataObjPut (conn, dataObjInp, locFilePath);
                if (l1descInx >= 0) {
                    rcOprComplete (conn, l1descInx);
                } else {
		    rcOprComplete (conn, status);
		}
	    }
	} else if ((myMsParam = getMsParamByLabel (outParamArray, 
	  CL_GET_ACTION)) != NULL) {
            dataObjInp = (dataObjInp_t *) myMsParam->inOutStruct;
            if ((locFilePath = getValByKey (&dataObjInp->condInput,
              RSYNC_DEST_PATH_KW)) == NULL) {
                if (l1descInx >= 0) {
                    rcOprComplete (conn, l1descInx);
                } else {
                    rcOprComplete (conn, USER_FILE_DOES_NOT_EXIST);
		}
            } else {
                status = rcDataObjGet (conn, dataObjInp, locFilePath);
                if (l1descInx >= 0) {
                    rcOprComplete (conn, l1descInx);
                } else {
                    rcOprComplete (conn, status);
		}
            }
	} else {
            if (l1descInx >= 0) {
                rcOprComplete (conn, l1descInx);
            } else {
                rcOprComplete (conn, SYS_SVR_TO_CLI_MSI_NO_EXIST);
	    }
	}
	/* free outParamArray */
	if (dataObjInp != NULL) {
	    clearKeyVal (&dataObjInp->condInput); 
	}
	clearMsParamArray (outParamArray, 1);
	free (outParamArray);

	/* read the reply from the eariler call */

        status = branchReadAndProcApiReply (conn, DATA_OBJ_RSYNC_AN, 
        (void **)&outParamArray, NULL);
        if (status < 0) {
            rodsLogError (LOG_DEBUG, status,
              "rcDataObjRsync: readAndProcApiReply failed. status = %d", 
	      status);
        }
    }

    return (status);
}

int
_rcDataObjRsync (rcComm_t *conn, dataObjInp_t *dataObjInp, 
msParamArray_t **outParamArray)
{
    int status;

    status = procApiRequest (conn, DATA_OBJ_RSYNC_AN, dataObjInp, NULL,
        (void **)outParamArray, NULL);

    return (status);
}
