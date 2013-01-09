/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjPut.h for a description of this API call.*/

#include "dataObjPut.h"
#include "rodsLog.h"
#include "dataPut.h"
#include "filePut.h"
#include "objMetaOpr.h"
#include "physPath.h"
#include "specColl.h"
#include "dataObjOpen.h"
#include "dataObjCreate.h"
#include "regDataObj.h"
#include "dataObjUnlink.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "rsApiHandler.h"
#include "subStructFilePut.h"
#include "dataObjRepl.h"
#include "getRemoteZoneResc.h"
#include "icatHighLevelRoutines.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_backport.h"
#include "eirods_hierarchy_parser.h"

int
rsDataObjPut (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
              bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut)
{
    int status;
    int status2;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;

    resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput);
    remoteFlag = getAndConnRemoteZone (rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == LOCAL_HOST) {
        /** since the object is written here, we apply pre procesing RAJA 
         * Dec 2 2010 **/
        status2 = applyRuleForPostProcForWrite(rsComm, dataObjInpBBuf, 
                                               dataObjInp->objPath);
        if (status2 < 0) 
            return(status2); /* need to dealloc anything??? */
        /** since the object is written here, we apply pre procesing RAJA 
         * Dec 2 2010 **/

        dataObjInp->openFlags = O_RDWR;
        status = _rsDataObjPut (rsComm, dataObjInp, dataObjInpBBuf,
                                portalOprOut, BRANCH_MSG);
    } else {
        int l1descInx;
        status = _rcDataObjPut (rodsServerHost->conn, dataObjInp, 
                                dataObjInpBBuf, portalOprOut);
        if (status < 0 || 
            getValByKey (&dataObjInp->condInput, DATA_INCLUDED_KW) != NULL) {
            return (status);
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

/* _rsDataObjPut - process put request 
 * The reply to this API can go off the main part of the API's
 * request/reply protocol and uses the sendAndRecvOffMainMsg call
 * to handle a sequence of request/reply until a return value of
 * SYS_HANDLER_NO_ERROR.
 * handlerFlag - INTERNAL_SVR_CALL - called internally by the server.
 *                 affect how return values are handled
 */

int
_rsDataObjPut (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
               bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut, int handlerFlag)
{
    int status;
    int l1descInx;
    int retval;
    openedDataObjInp_t dataObjCloseInp;
    int allFlag;
    transferStat_t *transStat = NULL;
    dataObjInp_t replDataObjInp;

    if (getValByKey (&dataObjInp->condInput, ALL_KW) != NULL) {
        allFlag = 1;
    } else {
        allFlag = 0;
    }

    if (getValByKey (&dataObjInp->condInput, DATA_INCLUDED_KW) != NULL) {
	/* single buffer put */
    rodsLog( LOG_NOTICE, "XXXX - _rsDataObjPut :: calling l3DataPutSingleBuf" );
        status = l3DataPutSingleBuf (rsComm, dataObjInp, dataObjInpBBuf);
    rodsLog( LOG_NOTICE, "XXXX - _rsDataObjPut :: calling l3DataPutSingleBuf. done." );
        if (status >= 0 && allFlag == 1) {
            /* update the rest of copies */
            addKeyVal (&dataObjInp->condInput, UPDATE_REPL_KW, "");
            status = rsDataObjRepl (rsComm, dataObjInp, &transStat);
            if (transStat!= NULL) free (transStat);
        }
        if (status >= 0) {
            int status2;
            /** since the object is written here, we apply pre procesing RAJA
             * Dec 2 2010 **/
            status2 = applyRuleForPostProcForWrite(rsComm, dataObjInpBBuf,
                                                   dataObjInp->objPath);
            if (status2 >= 0) {
                status = 0;
            } else {
                status = status2;
            }
            /** since the object is written here, we apply pre procesing RAJA
             * Dec 2 2010 **/
        }
        return (status);
    }

    /* get down here. will do parallel I/O */
    /* so that mmap will work */
    dataObjInp->openFlags |= O_RDWR;

    rodsLog( LOG_NOTICE, "XXXX - _rsDataObjPut :: calling rsDataObjCreate" );
    l1descInx = rsDataObjCreate (rsComm, dataObjInp);
    rodsLog( LOG_NOTICE, "XXXX - _rsDataObjPut :: calling rsDataObjCreate. done." );
 
    if (l1descInx < 0) 
        return l1descInx;

    L1desc[l1descInx].oprType = PUT_OPR;
    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    if (getStructFileType (L1desc[l1descInx].dataObjInfo->specColl) >= 0) { // JMC - backport 4682
        *portalOprOut = (portalOprOut_t *) malloc (sizeof (portalOprOut_t));
        bzero (*portalOprOut,  sizeof (portalOprOut_t));
        (*portalOprOut)->l1descInx = l1descInx;
        return l1descInx;
    }


    status = preProcParaPut (rsComm, l1descInx, portalOprOut);

    if (status < 0) {
        memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = status;
        rsDataObjClose (rsComm, &dataObjCloseInp);
        return (status);
    } 

    if (allFlag == 1) {
        /* need to save dataObjInp. get freed in sendAndRecvBranchMsg */
        memset (&replDataObjInp, 0, sizeof (replDataObjInp));
        rstrcpy (replDataObjInp.objPath, dataObjInp->objPath, MAX_NAME_LEN);
        addKeyVal (&replDataObjInp.condInput, UPDATE_REPL_KW, "");
        addKeyVal (&replDataObjInp.condInput, ALL_KW, "");
    }

    /* return portalOprOut to the client and wait for the rcOprComplete
     * call. That is when the parallel I/O is done */
    retval = sendAndRecvBranchMsg (rsComm, rsComm->apiInx, status,
                                   (void *) *portalOprOut, NULL);

    if (retval < 0) {
        memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
        dataObjCloseInp.l1descInx = l1descInx;
        L1desc[l1descInx].oprStatus = retval;
        rsDataObjClose (rsComm, &dataObjCloseInp);
        if (allFlag == 1) clearKeyVal (&replDataObjInp.condInput);
    } else if (allFlag == 1) {
        status = rsDataObjRepl (rsComm, &replDataObjInp, &transStat);
        if (transStat!= NULL) free (transStat);
        clearKeyVal (&replDataObjInp.condInput);
    }

    if (handlerFlag & INTERNAL_SVR_CALL) {
        /* internal call. want to know the real status */
        return (retval);
    } else {
        /* already send the client the status */
        return (SYS_NO_HANDLER_REPLY_MSG);
    }
}

/* preProcParaPut - preprocessing for parallel put. Basically it calls
 * rsDataPut to setup portalOprOut with the resource server.
 */

int
preProcParaPut (rsComm_t *rsComm, int l1descInx, 
                portalOprOut_t **portalOprOut)
{
    int l3descInx;
    int status;
    dataOprInp_t dataOprInp;

    l3descInx = L1desc[l1descInx].l3descInx;

    initDataOprInp (&dataOprInp, l1descInx, PUT_OPR);
    /* add RESC_NAME_KW for getNumThreads */
    if (L1desc[l1descInx].dataObjInfo != NULL && 
        L1desc[l1descInx].dataObjInfo->rescInfo != NULL) {
        addKeyVal (&dataOprInp.condInput, RESC_NAME_KW, 
                   L1desc[l1descInx].dataObjInfo->rescInfo->rescName);
    }
    if (L1desc[l1descInx].remoteZoneHost != NULL) {
        status =  remoteDataPut (rsComm, &dataOprInp, portalOprOut,
                                 L1desc[l1descInx].remoteZoneHost);
    } else {
        status =  rsDataPut (rsComm, &dataOprInp, portalOprOut);
    }

    if (status >= 0) {
        (*portalOprOut)->l1descInx = l1descInx;
        L1desc[l1descInx].bytesWritten = dataOprInp.dataSize;
    }
    clearKeyVal (&dataOprInp.condInput);
    return (status);
}

int
l3DataPutSingleBuf (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                    bytesBuf_t *dataObjInpBBuf)
{
    int bytesWritten;
    int l1descInx;
    dataObjInfo_t *myDataObjInfo;
    char rescGroupName[NAME_LEN];
    rescInfo_t *rescInfo;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int status;
    openedDataObjInp_t dataObjCloseInp;

    /* don't actually physically open the file */
    addKeyVal (&dataObjInp->condInput, NO_OPEN_FLAG_KW, "");
    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: calling rsDataObjCreate" );
    l1descInx = rsDataObjCreate (rsComm, dataObjInp);
    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: calling rsDataObjCreate. done." );
    
    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: rescInfo Name 1. %s", L1desc[l1descInx].dataObjInfo->rescInfo->rescName );

    if (l1descInx <= 2) {
        if (l1descInx >= 0) {
            rodsLog (LOG_ERROR,
                     "l3DataPutSingleBuf: rsDataObjCreate of %s error, status = %d",
                     dataObjInp->objPath, l1descInx);
            return SYS_FILE_DESC_OUT_OF_RANGE;
        } else {
            return l1descInx;
        }
    }

    
    bytesWritten = _l3DataPutSingleBuf (rsComm, l1descInx, dataObjInp, dataObjInpBBuf );
      
    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: rescInfo Name 2. %s", L1desc[l1descInx].dataObjInfo->rescInfo->rescName );

    if (bytesWritten < 0) {
        myDataObjInfo = L1desc[l1descInx].dataObjInfo;
        if (getStructFileType (myDataObjInfo->specColl) < 0 &&
            strlen (myDataObjInfo->rescGroupName) > 0 &&
            (L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY) == 0) {
            /* getValByKey (&dataObjInp->condInput, FORCE_FLAG_KW) == NULL) { */
            /* File not in specColl and resc is a resc group and not 
             * overwriting existing data. Save resc info in case the put fail 
             */
            rstrcpy (rescGroupName, myDataObjInfo->rescGroupName, NAME_LEN);
            rescInfo = myDataObjInfo->rescInfo;
        } else {
            rescGroupName[0] = '\0';
            rescInfo = NULL;
        }
    }
    
    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: rescInf Name 3. %s", L1desc[l1descInx].dataObjInfo->rescInfo->rescName );

    memset (&dataObjCloseInp, 0, sizeof (dataObjCloseInp));
    dataObjCloseInp.l1descInx = l1descInx;
    L1desc[l1descInx].oprStatus = bytesWritten;
    status = rsDataObjClose (rsComm, &dataObjCloseInp);
    if (status < 0) {
        rodsLog (LOG_NOTICE,
                 "l3DataPutSingleBuf: rsDataObjClose of %d error, status = %d",
                 l1descInx, status);
    }

    if (bytesWritten >= 0) {
        return status;
    } else if (strlen (rescGroupName) == 0) {
        return bytesWritten;
    }

    rodsLog( LOG_NOTICE, "XXXX - l3DataPutSingleBuf :: rescInf Name 4. %s", L1desc[l1descInx].dataObjInfo->rescInfo->rescName );

    /* get here when Put failed. and rescGroupName is a valid resc group. 
     * Try other resc in the resc group */
    status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
    if (status < 0) return bytesWritten;
    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (rescInfo == tmpRescInfo) {
            /* already tried this resc */
            tmpRescGrpInfo = tmpRescGrpInfo->next;
            continue;
        }
        l1descInx = _rsDataObjCreateWithRescInfo (rsComm, 
                                                  dataObjInp, tmpRescInfo, myRescGrpInfo->rescGroupName);
        if (l1descInx <= 2) {
            if (l1descInx >= 0) {
                rodsLog (LOG_ERROR,
                         "l3DataPutSingleBuf:_rsDataObjCreateWithRI %s err,stat = %d",
                         dataObjInp->objPath, l1descInx);
            }
        } else {
            bytesWritten = _l3DataPutSingleBuf (rsComm, l1descInx, dataObjInp,
                                                dataObjInpBBuf);
            dataObjCloseInp.l1descInx = l1descInx;
            L1desc[l1descInx].oprStatus = bytesWritten;
            status = rsDataObjClose (rsComm, &dataObjCloseInp);
            if (status < 0) {
                rodsLog (LOG_NOTICE,
                         "l3DataPutSingleBuf: rsDataObjClose of %d error, status = %d",
                         l1descInx, status);
            }
            if (bytesWritten >= 0) {
                bytesWritten = status;
                break;
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return bytesWritten;
}
 
int
_l3DataPutSingleBuf (rsComm_t *rsComm, int l1descInx, dataObjInp_t *dataObjInp,
                     bytesBuf_t *dataObjInpBBuf)
{
    int status = 0;
    int bytesWritten;
    dataObjInfo_t *myDataObjInfo;

    myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    
    bytesWritten = l3FilePutSingleBuf (rsComm, l1descInx, dataObjInpBBuf);

    if (bytesWritten >= 0) {
        if (L1desc[l1descInx].replStatus == NEWLY_CREATED_COPY && 
            myDataObjInfo->specColl == NULL && 
            L1desc[l1descInx].remoteZoneHost == NULL) {
            /* the check for remoteZoneHost host is not needed because
             * the put would have done in the remote zone. But it make
             * the code easier to read (similar ro copy). 
             */
            status = svrRegDataObj (rsComm, myDataObjInfo);
            if (status < 0) {
                rodsLog (LOG_NOTICE,
                         "l3DataPutSingleBuf: rsRegDataObj for %s failed, status = %d",
                         myDataObjInfo->objPath, status);
                if (status != CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME)
                    l3Unlink (rsComm, myDataObjInfo);
                return (status);
            } else {
                myDataObjInfo->replNum = status;
            }
        }
        /* myDataObjInfo->dataSize = bytesWritten; update size problem */
        if (bytesWritten == 0 && myDataObjInfo->dataSize > 0) {
            /* overwrite with 0 len file */
            L1desc[l1descInx].bytesWritten = 1;
        } else {
            L1desc[l1descInx].bytesWritten = bytesWritten;
        }
    }
    L1desc[l1descInx].dataSize = dataObjInp->dataSize;

    return (bytesWritten);
}

/**
 * @brief Updates the data obj and resources according to the resource hierarchy string
 */
static eirods::error
_updateDbWithRescHier(
    rsComm_t* rsComm,
    const std::string& _resc_hier,
    int _object_count_delta) {

    eirods::error result = SUCCESS();
    eirods::error ret;
    int status;

    keyValPair_t regParam;
    memset(&regParam, 0, sizeof(regParam));
    addKeyVal(&regParam, "rescHier", _resc_hier.c_str());

    std::string leaf_resc;
    eirods::hierarchy_parser hparse;
    if(!(ret = hparse.set_string(_resc_hier)).ok()) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to parse the hierarchy string \"" << _resc_hier << "\"";
        result = PASSMSG(msg.str(), ret);
    } else if(!(ret = hparse.last_resc(leaf_resc)).ok()) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to retrieve the leaf resource.";
        result = PASSMSG(msg.str(), ret);
    } else if((status = chlUpdateRescObjCount(leaf_resc, _object_count_delta)) < 0) {
        std::stringstream msg;
        msg << __FUNCTION__ << " - Failed to update the object count for the resource \"" << leaf_resc << "\"";
        result = ERROR(status, msg.str());
    }
    
    return result;
}

int
l3FilePutSingleBuf (rsComm_t *rsComm, int l1descInx, bytesBuf_t *dataObjInpBBuf)
{
    dataObjInfo_t *dataObjInfo;
    int rescTypeInx;
    fileOpenInp_t filePutInp;
    int bytesWritten;
    dataObjInp_t *dataObjInp;
    int retryCnt = 0;
    int chkType = 0; // JMC - backport 4774

    dataObjInfo = L1desc[l1descInx].dataObjInfo;
    dataObjInp = L1desc[l1descInx].dataObjInp;

    if (getStructFileType (dataObjInfo->specColl) >= 0) {
        subFile_t subFile;
        
        memset (&subFile, 0, sizeof (subFile));
        rstrcpy (subFile.subFilePath, dataObjInfo->subPath,MAX_NAME_LEN);
        rstrcpy (subFile.addr.hostAddr, dataObjInfo->rescInfo->rescLoc,NAME_LEN);
        subFile.specColl = dataObjInfo->specColl;
        subFile.mode = getFileMode (dataObjInp);
        subFile.flags = O_WRONLY | dataObjInp->openFlags;
            
        if ((L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY) != 0) {
            subFile.flags |= FORCE_FLAG;
        }
        
        bytesWritten = rsSubStructFilePut (rsComm, &subFile, dataObjInpBBuf);
        return (bytesWritten);

    } // struct file type >= 0

    std::string prev_resc_hier;
    #if 0 // JMC legacy resource 
    rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;
    switch (RescTypeDef[rescTypeInx].rescCat) {
    case FILE_CAT:
    #endif // JMC - legacy resource
    memset (&filePutInp, 0, sizeof (filePutInp));
    rstrcpy( filePutInp.resc_name_, dataObjInfo->rescInfo->rescName, MAX_NAME_LEN );
    rstrcpy( filePutInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
    if ((L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY) != 0) {
        filePutInp.otherFlags |= FORCE_FLAG;
    }
                
    filePutInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
                
    rstrcpy (filePutInp.addr.hostAddr,  dataObjInfo->rescInfo->rescLoc,NAME_LEN);
    rstrcpy (filePutInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
                
    filePutInp.mode = getFileMode (dataObjInp);
    filePutInp.flags = O_WRONLY | dataObjInp->openFlags;
                
    // =-=-=-=-=-=-=-
    // JMC - backport 4774
    chkType = getchkPathPerm (rsComm, L1desc[l1descInx].dataObjInp,L1desc[l1descInx].dataObjInfo);

    if(chkType == DISALLOW_PATH_REG) {
        return PATH_REG_NOT_ALLOWED;
    } else if (chkType == NO_CHK_PATH_PERM) {
        // =-=-=-=-=-=-=-
        filePutInp.otherFlags |= NO_CHK_PERM_FLAG; // JMC - backport 4758
    }

    prev_resc_hier = filePutInp.resc_hier_;
    bytesWritten = rsFilePut (rsComm, &filePutInp, dataObjInpBBuf);
    if(prev_resc_hier != std::string(filePutInp.resc_hier_)) {
        rstrcpy(dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN);
        eirods::error ret =_updateDbWithRescHier(rsComm, filePutInp.resc_hier_, 1);
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - Unable to update database with resc hier info.";
            eirods::log(LOG_ERROR, msg.str());
            return ret.code();
        }
    }

    /* file already exists ? */
    while( bytesWritten < 0 && retryCnt < 10 &&
           ( filePutInp.otherFlags & FORCE_FLAG ) == 0 &&
           getErrno (bytesWritten) == EEXIST) {

        if (resolveDupFilePath (rsComm, dataObjInfo, dataObjInp) < 0) {
            break;
        }
        rstrcpy (filePutInp.fileName, dataObjInfo->filePath,MAX_NAME_LEN);
        bytesWritten = rsFilePut (rsComm, &filePutInp, dataObjInpBBuf);
        if(prev_resc_hier != std::string(filePutInp.resc_hier_)) {
            rstrcpy(dataObjInfo->rescHier, filePutInp.resc_hier_, MAX_NAME_LEN);
            eirods::error ret =_updateDbWithRescHier(rsComm, filePutInp.resc_hier_, 1);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - Unable to update database with resc hier info.";
                eirods::log(LOG_ERROR, msg.str());
                return ret.code();
            }
        }
        retryCnt ++;
    } // while
#if 0 // JMC - legacy resource
    break;

default:
    rodsLog( LOG_NOTICE,"l3Open: rescCat type %d is not recognized",
             RescTypeDef[rescTypeInx].rescCat );
    bytesWritten = SYS_INVALID_RESC_TYPE;
    break;

} // switch

#endif // JMC - legacy resource
    return (bytesWritten);

} // l3FilePutSingleBuf

