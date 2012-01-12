/**
 * @file  rcDataObjGet.c
 *
 */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code.  */ 
/* See dataObjGet.h for a description of this API call.*/

#include "dataObjGet.h"
#include "rcPortalOpr.h"
#include "apiHeaderAll.h"

/**
 * \fn rcDataObjGet (rcComm_t *conn, dataObjInp_t *dataObjInp,
 *   char *locFilePath)
 *
 * \brief Get (download) a data object from iRODS.
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
 * Get (download) a data object /myZone/home/john/myfile:
 * \n dataObjInp_t dataObjInp;
 * \n char locFilePath[MAX_NAME_LEN];
 * \n bzero (&dataObjInp, sizeof (dataObjInp));
 * \n rstrcpy (dataObjInp.objPath, "/myZone/home/john/myfile", MAX_NAME_LEN);
 * \n rstrcpy (locFilePath, "./mylocalfile", MAX_NAME_LEN);
 * \n dataObjInp.dataSize = 0;
 * \n status = rcDataObjGet (conn, &dataObjInp, locFilePath);
 * \n if (status < 0) {
 * \n .... handle the error
 * \n }
 *
 * \param[in] conn - A rcComm_t connection handle to the server.
 * \param[in] dataObjInp - Elements of dataObjInp_t used :
 *    \li char \b objPath[MAX_NAME_LEN] - full path of the data object.
 *    \li rodsLong_t \b dataSize - the size of the data object.
 *      Input 0 if not known.
 *    \li int \b numThreads - the number of threads to use. Valid values are:
 *      \n NO_THREADING (-1) - no multi-thread
 *      \n 0 - the server will decide the number of threads.
 *        (recommanded setting).
 *      \n A positive integer - specifies the number of threads.
 *    \li keyValPair_t \b condInput - keyword/value pair input. Valid keywords:
 *    \n RESC_NAME_KW - The resource of the data object to open.
 *    \n REPL_NUM_KW - the replica number of the copy to open.
 *    \n FORCE_FLAG_KW - overwrite existing local copy. This keyWd has no value.
 *    \n VERIFY_CHKSUM_KW - verify the checksum value of the local file after 
 *	     the download. This keyWd has no value.
 *    \n RBUDP_TRANSFER_KW - use RBUDP for data transfer. This keyWd has no
 *             value
 *    \n RBUDP_SEND_RATE_KW - the number of RBUDP packet to send per second
 *          The default is 600000.
 *    \n RBUDP_PACK_SIZE_KW - the size of RBUDP packet. The default is 8192.
 * \param[in] locFilePath - the path of the local file to download. This path
 *           can be a relative path.
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
rcDataObjGet (rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath)
{
    int status;
    portalOprOut_t *portalOprOut = NULL;
    bytesBuf_t dataObjOutBBuf;
#ifndef windows_platform
    struct stat statbuf;
#else
	struct irodsntstat statbuf;
#endif

    if (strcmp (locFilePath, STDOUT_FILE_NAME) == 0) {
	/* no parallel I/O if pipe to stdout */
	dataObjInp->numThreads = NO_THREADING;
    } 
#ifndef windows_platform
	else if (stat (locFilePath, &statbuf) >= 0) 
#else
	else if(iRODSNt_stat(locFilePath, &statbuf) >= 0)
#endif
	{
	/* local file exists */
	if (getValByKey (&dataObjInp->condInput, FORCE_FLAG_KW) == NULL) {
	    return (OVERWRITE_WITHOUT_FORCE_FLAG);
		}
    }

    status = _rcDataObjGet (conn, dataObjInp, &portalOprOut, &dataObjOutBBuf);

    if (status < 0) {
        if (portalOprOut != NULL) free (portalOprOut);
        return (status);
    }

    if (status == 0 || dataObjOutBBuf.len > 0) {
	/* data included */
      /**** Removed by Raja as this can cause problems when the data sizes are different - say when post processing is done....Dec 2 2010
	if (dataObjInp->dataSize > 0 && 
	  dataObjInp->dataSize != dataObjOutBBuf.len) { 
             rodsLog (LOG_NOTICE,
              "putFile: totalWritten %lld dataSize %lld mismatch",
              dataObjOutBBuf.len, dataObjInp->dataSize);
            return (SYS_COPY_LEN_ERR);
	}
      ****/
	status = getIncludeFile (conn, &dataObjOutBBuf, locFilePath);
	free (dataObjOutBBuf.buf);
#ifdef RBUDP_TRANSFER
    } else if (getUdpPortFromPortList (&portalOprOut->portList) != 0) {
        int veryVerbose;
        /* rbudp transfer */
        /* some sanity check */
        if (portalOprOut->numThreads != 1) {
            rcOprComplete (conn, SYS_INVALID_PORTAL_OPR);
            free (portalOprOut);
            return (SYS_INVALID_PORTAL_OPR);
        }
        conn->transStat.numThreads = portalOprOut->numThreads;
        if (getValByKey (&dataObjInp->condInput, VERY_VERBOSE_KW) != NULL) {
            printf ("From server: NumThreads=%d, addr:%s, port:%d, cookie=%d\n",
              portalOprOut->numThreads, portalOprOut->portList.hostAddr,
              portalOprOut->portList.portNum, portalOprOut->portList.cookie);
            veryVerbose = 2;
        } else {
            veryVerbose = 0;
        }
        status = getFileToPortalRbudp (portalOprOut, locFilePath, 0,
          dataObjInp->dataSize, veryVerbose, 0);
        /* just send a complete msg */
        if (status < 0) {
            rcOprComplete (conn, status);
        } else {
            status = rcOprComplete (conn, portalOprOut->l1descInx);
        }
#endif  /* RBUDP_TRANSFER */
    } else {

        if (portalOprOut->numThreads <= 0) {
            status = getFile (conn, portalOprOut->l1descInx, 
              locFilePath, dataObjInp->objPath, dataObjInp->dataSize);
        } else {
        if (getValByKey (&dataObjInp->condInput, VERY_VERBOSE_KW) != NULL) {
            printf ("From server: NumThreads=%d, addr:%s, port:%d, cookie=%d\n",
              portalOprOut->numThreads, portalOprOut->portList.hostAddr,
              portalOprOut->portList.portNum, portalOprOut->portList.cookie);
        }
            /* some sanity check */
            if (portalOprOut->numThreads >= 20 * DEF_NUM_TRAN_THR) {
                rcOprComplete (conn, SYS_INVALID_PORTAL_OPR);
                free (portalOprOut);
                return (SYS_INVALID_PORTAL_OPR);
            }

            conn->transStat.numThreads = portalOprOut->numThreads;
            status = getFileFromPortal (conn, portalOprOut, locFilePath,
               dataObjInp->objPath, dataObjInp->dataSize);
	}
        /* just send a complete msg */
        if (status < 0) {
            rcOprComplete (conn, status);
        } else {
            status = rcOprComplete (conn, portalOprOut->l1descInx);
        }
    }

    if (getValByKey (&dataObjInp->condInput, VERIFY_CHKSUM_KW) != NULL) {
	if (portalOprOut == NULL || strlen (portalOprOut->chksum) == 0) {
	    rodsLog (LOG_ERROR, 
	      "rcDataObjGet: VERIFY_CHKSUM_KW set but no chksum from server");
	} else {
	    char chksumStr[NAME_LEN];

            status = chksumLocFile (locFilePath, chksumStr);

            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "rcDataObjGet: chksumLocFile error for %s, status = %d",
		  locFilePath, status);
		if (portalOprOut != NULL)
		    free (portalOprOut);
                return (status);
            }
 
	    if (strcmp (portalOprOut->chksum, chksumStr) != 0) {
	        status = USER_CHKSUM_MISMATCH;
                rodsLogError (LOG_ERROR, status,
                  "rcDataObjGet: chksum mismatch error for %s, status = %d",
                  locFilePath, status);
		if (portalOprOut != NULL)
		    free (portalOprOut);
                return (status);
            }
	}
    }
    if (portalOprOut != NULL) {
        free (portalOprOut);
    }

    return (status);
}

int
_rcDataObjGet (rcComm_t *conn, dataObjInp_t *dataObjInp, 
portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf)
{
    int status;

    *portalOprOut = NULL;

    memset (&conn->transStat, 0, sizeof (transStat_t));

    memset (dataObjOutBBuf, 0, sizeof (bytesBuf_t));

    dataObjInp->oprType = GET_OPR;

#ifndef PARA_OPR
    addKeyVal (&dataObjInp->condInput, NO_PARA_OP_KW, "");
#endif

    status = procApiRequest (conn, DATA_OBJ_GET_AN,  dataObjInp, NULL,
        (void **) portalOprOut, dataObjOutBBuf);

    if (*portalOprOut != NULL && (*portalOprOut)->l1descInx < 0) {
        status = (*portalOprOut)->l1descInx;
    }

    return (status);
}

