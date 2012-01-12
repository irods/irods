/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjGet.h for a description of this API call.*/

#include "dataObjGet.h"
#include "rodsLog.h"
#include "dataGet.h"
#include "fileGet.h"
#include "dataObjOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "subStructFileGet.h"
#include "getRemoteZoneResc.h"

int
rsDataObjGet (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf)
{
  int status;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache,
      &dataObjInp->condInput);
    remoteFlag = getAndConnRemoteZone (rsComm, dataObjInp, &rodsServerHost,
      REMOTE_OPEN);

    if (remoteFlag < 0) {
	return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
        status = _rsDataObjGet (rsComm, dataObjInp, portalOprOut, 
          dataObjOutBBuf, BRANCH_MSG);
    } else {
       int l1descInx;
	status = _rcDataObjGet (rodsServerHost->conn, dataObjInp, portalOprOut,
	  dataObjOutBBuf);

        if (status < 0) {
            return (status);
        }
        if (status == 0 || 
	  (dataObjOutBBuf != NULL && dataObjOutBBuf->len > 0)) {
            /* data included in buf */
            return status;
        } else {
            /* have to allocate a local l1descInx to keep track of things
             * since the file is in remote zone. It sets remoteL1descInx,
             * oprType = REMOTE_ZONE_OPR and remoteZoneHost so that  
             * rsComplete knows what to do */
	    l1descInx = allocAndSetL1descForZoneOpr (
	      (*portalOprOut)->l1descInx, dataObjInp, rodsServerHost, NULL);
            if (l1descInx < 0) return l1descInx;
            (*portalOprOut)->l1descInx = l1descInx;
            return status;
        }
    }

    return (status);
}

int
_rsDataObjGet (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf, int handlerFlag)
{
    int status;
    dataObjInfo_t *dataObjInfo;
    int l1descInx;
    char *chksumStr = NULL;
    int retval;
    openedDataObjInp_t dataObjCloseInp;


    /* PHYOPEN_BY_SIZE ask it to check whether "dataInclude" should be done */
    addKeyVal (&dataObjInp->condInput, PHYOPEN_BY_SIZE_KW, "");
    l1descInx = _rsDataObjOpen (rsComm, dataObjInp);

    if (l1descInx < 0)
        return l1descInx;

    L1desc[l1descInx].oprType = GET_OPR;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if (getValByKey (&dataObjInp->condInput, VERIFY_CHKSUM_KW) != NULL) {
        if (strlen (dataObjInfo->chksum) > 0) {
            /* a chksum already exists */
	    chksumStr = strdup (dataObjInfo->chksum);
        } else {

            status = dataObjChksumAndReg (rsComm, dataObjInfo, &chksumStr);
            if (status < 0) {
                return status;
            }
	    rstrcpy (dataObjInfo->chksum, chksumStr, NAME_LEN);
        }
    }

    if (L1desc[l1descInx].l3descInx <= 2) {
	/* no physical file was opened */
        status = l3DataGetSingleBuf (rsComm, l1descInx, dataObjOutBBuf,
	  portalOprOut);
	if (status >= 0) {
            int status2;
            /** since the object is read here, we apply post procesing RAJA
             * Dec 2 2010 **/
            status2 = applyRuleForPostProcForRead(rsComm, dataObjOutBBuf,
             dataObjInp->objPath);
            if (status2 >= 0) {
                status = 0;
            } else {
                status = status2;
	    }
            /** since the object is read here, we apply post procesing 
             * RAJA Dec 2 2010 **/
            if (chksumStr != NULL) {
                rstrcpy ((*portalOprOut)->chksum, chksumStr, NAME_LEN);
                free (chksumStr);
	    }
        }
	return (status);
    }


    status = preProcParaGet (rsComm, l1descInx, portalOprOut);

    if (status < 0) {
        memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
         dataObjCloseInp.l1descInx = l1descInx;
         rsDataObjClose (rsComm, &dataObjCloseInp);
	if (chksumStr != NULL) {
	    free (chksumStr);
	}
        return (status);
    }

    status = l1descInx;		/* means file not included */
    if (chksumStr != NULL) {
        rstrcpy ((*portalOprOut)->chksum, chksumStr, NAME_LEN);
        free (chksumStr);
    }

    /* return portalOprOut to the client and wait for the rcOprComplete
     * call. That is when the parallel I/O is done */
    retval = sendAndRecvBranchMsg (rsComm, rsComm->apiInx, status,
      (void *) *portalOprOut, dataObjOutBBuf);

    if (retval < 0) {
        memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
        dataObjCloseInp.l1descInx = l1descInx;
        rsDataObjClose (rsComm, &dataObjCloseInp);
    }

    if (handlerFlag & INTERNAL_SVR_CALL) {
        /* internal call. want to know the real status */
        return (retval);
    } else {
        /* already send the client the status */
        return (SYS_NO_HANDLER_REPLY_MSG);
    }
}

/* preProcParaGet - preprocessing for parallel get. Basically it calls
 * rsDataGet to setup portalOprOut with the resource server.
 */
int
preProcParaGet (rsComm_t *rsComm, int l1descInx, portalOprOut_t **portalOprOut)
{
    int l3descInx;
    int status;
    dataOprInp_t dataOprInp;

    l3descInx = L1desc[l1descInx].l3descInx;

    initDataOprInp (&dataOprInp, l1descInx, GET_OPR);
    /* add RESC_NAME_KW for getNumThreads */
    if (L1desc[l1descInx].dataObjInfo != NULL && 
      L1desc[l1descInx].dataObjInfo->rescInfo != NULL) {
	addKeyVal (&dataOprInp.condInput, RESC_NAME_KW, 
	  L1desc[l1descInx].dataObjInfo->rescInfo->rescName);
    } 
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        status =  remoteDataGet (rsComm, &dataOprInp, portalOprOut,
        L1desc[l1descInx].remoteZoneHost);
    } else {
        status =  rsDataGet (rsComm, &dataOprInp, portalOprOut);
    }

    if (status >= 0) {
        (*portalOprOut)->l1descInx = l1descInx;
    }
    clearKeyVal (&dataOprInp.condInput);
    return (status);
}

int
l3DataGetSingleBuf (rsComm_t *rsComm, int l1descInx,
bytesBuf_t *dataObjOutBBuf, portalOprOut_t **portalOprOut)
{
    int status = 0;
    int bytesRead;
    openedDataObjInp_t dataObjCloseInp;
    dataObjInfo_t *dataObjInfo;

    /* just malloc an empty portalOprOut */

    *portalOprOut = (portalOprOut_t*)malloc (sizeof (portalOprOut_t));
    memset (*portalOprOut, 0, sizeof (portalOprOut_t));

    dataObjInfo = L1desc[l1descInx].dataObjInfo;

    if (dataObjInfo->dataSize > 0) { 
        dataObjOutBBuf->buf = malloc (dataObjInfo->dataSize);
        bytesRead = l3FileGetSingleBuf (rsComm, l1descInx, dataObjOutBBuf);
    } else {
	bytesRead = 0;
    }

#if 0   /* tested in _rsFileGet. don't need to go it again */
    if (bytesRead != dataObjInfo->dataSize) {
	free (dataObjOutBBuf->buf);
	memset (dataObjOutBBuf, 0, sizeof (bytesBuf_t));
	if (bytesRead >= 0) { 
            rodsLog (LOG_NOTICE,
              "l3DataGetSingleBuf:Bytes toread %d don't match read %d",
              dataObjInfo->dataSize, bytesRead);
            bytesRead = SYS_COPY_LEN_ERR - errno;
	}
    }
#endif

    memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx;
    status = rsDataObjClose (rsComm, &dataObjCloseInp);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
          "l3DataGetSingleBuf: rsDataObjClose of %d error, status = %d",
            l1descInx, status);
    }

    if (bytesRead < 0)
        return (bytesRead);
    else
	return status;
}

/* l3FileGetSingleBuf - Get the content of a small file into a single buffer 
 * in dataObjOutBBuf->buf for an opened data obj in l1descInx. 
 * Return value - int - number of bytes read. 
 */

int
l3FileGetSingleBuf (rsComm_t *rsComm, int l1descInx,  
bytesBuf_t *dataObjOutBBuf)
{
    dataObjInfo_t *dataObjInfo;
    int rescTypeInx;
    fileOpenInp_t fileGetInp;
    int bytesRead;
    dataObjInp_t *dataObjInp;

    dataObjInfo = L1desc[l1descInx].dataObjInfo;


    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy (subFile.subFilePath, dataObjInfo->subPath,
          MAX_NAME_LEN);
        rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        subFile.specColl = dataObjInfo->specColl;
	subFile.mode = getFileMode (L1desc[l1descInx].dataObjInp);
        subFile.flags = O_RDONLY;
	subFile.offset = dataObjInfo->dataSize;
        bytesRead = rsSubStructFileGet (rsComm, &subFile, dataObjOutBBuf);
	return (bytesRead);
    }

    rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
        memset (&fileGetInp, 0, sizeof (fileGetInp));
        dataObjInp = L1desc[l1descInx].dataObjInp;
        fileGetInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (fileGetInp.addr.hostAddr,  dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        rstrcpy (fileGetInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
        fileGetInp.mode = getFileMode (dataObjInp);
        fileGetInp.flags = O_RDONLY;
	fileGetInp.dataSize = dataObjInfo->dataSize;
	/* XXXXX need to be able to handle structured file */
        bytesRead = rsFileGet (rsComm, &fileGetInp, dataObjOutBBuf);
        break;
      default:
        rodsLog (LOG_NOTICE,
          "l3Open: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        bytesRead = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (bytesRead);
}

