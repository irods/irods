/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjCreate.h for a description of this API call.*/

#include "dataObjCreate.h"
#include "dataObjCreateAndStat.h"
#include "dataObjOpen.h"
#include "fileCreate.h"
#include "subStructFileCreate.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "specColl.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "dataObjUnlink.h"
#include "dataObjLock.h" // JMC - backport 4604
#include "regDataObj.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "icatHighLevelRoutines.h"

#include "eirods_hierarchy_parser.h"

/* rsDataObjCreate - handle dataObj create request.
 *
 * The NO_OPEN_FLAG_KW in condInput specifies that no physical create
 * and registration will be done.
 *
 * return value -  > 2 - success with phy open
 *                < 0 - failure
 */

int
rsDataObjCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    
    int l1descInx;
    int status;
    rodsObjStat_t *rodsObjStatOut = NULL;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    specCollCache_t *specCollCache = NULL;
    char *lockType = NULL; // JMC - backport 4604
    int lockFd = -1; // JMC - backport 4604

    resolveLinkedPath (rsComm, dataObjInp->objPath, &specCollCache,
                       &dataObjInp->condInput);
    remoteFlag = getAndConnRemoteZone (rsComm, dataObjInp, &rodsServerHost,
                                       REMOTE_CREATE);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
        openStat_t *openStat = NULL;
        addKeyVal (&dataObjInp->condInput, CROSS_ZONE_CREATE_KW, "");
        status = rcDataObjCreateAndStat( rodsServerHost->conn, dataObjInp, &openStat );
        
        /* rm it to avoid confusion */
        rmKeyVal( &dataObjInp->condInput, CROSS_ZONE_CREATE_KW );
        if (status < 0) 
            return status;
        l1descInx = allocAndSetL1descForZoneOpr( status, dataObjInp,rodsServerHost, openStat );
        
        if (openStat != NULL) 
            free (openStat);
        return (l1descInx);
    }


    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    lockType = getValByKey( &dataObjInp->condInput, LOCK_TYPE_KW );
    if (lockType != NULL) {
        lockFd = rsDataObjLock (rsComm, dataObjInp);
        if (lockFd >= 0) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal (&dataObjInp->condInput, LOCK_TYPE_KW);
        } else {
            rodsLogError (LOG_ERROR, lockFd,
                          "rsDataObjCreate: rsDataObjLock error for %s. lockType = %s",
                          dataObjInp->objPath, lockType);
            return lockFd;
        }
    }
    
    // =-=-=-=-=-=-=-
    // Gets here means local zone operation stat dataObj 
    addKeyVal (&dataObjInp->condInput, SEL_OBJ_TYPE_KW, "dataObj");

    status = rsObjStat (rsComm, dataObjInp, &rodsObjStatOut); 

    if(rodsObjStatOut != NULL && rodsObjStatOut->objType == COLL_OBJ_T) {
        if (lockFd >= 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd); // JMC - backport 4604
        return (USER_INPUT_PATH_ERR);
    }

    if( rodsObjStatOut                      != NULL && 
        rodsObjStatOut->specColl            != NULL &&
        rodsObjStatOut->specColl->collClass == LINKED_COLL ) {
        /*  should not be here because if has been translated */
        if( lockFd >= 0 ) {
            rsDataObjUnlock (rsComm, dataObjInp, lockFd); // JMC - backport 4604
        }
    
        return SYS_COLL_LINK_PATH_ERR;
    }


    if ( rodsObjStatOut  == NULL                     ||  
         ( rodsObjStatOut->objType  == UNKNOWN_OBJ_T &&
           rodsObjStatOut->specColl == NULL ) ) {
        /* does not exist. have to create one */
        /* use L1desc[l1descInx].replStatus & OPEN_EXISTING_COPY instead */
        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */

        l1descInx = _rsDataObjCreate (rsComm, dataObjInp);

    } else if( rodsObjStatOut->specColl != NULL &&
               rodsObjStatOut->objType == UNKNOWN_OBJ_T) {

        /* newly created. take out FORCE_FLAG since it could be used by put */
        /* rmKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW); */
        l1descInx = specCollSubCreate (rsComm, dataObjInp);
    } else {

        /* dataObj exist */
        if (getValByKey (&dataObjInp->condInput, FORCE_FLAG_KW) != NULL) {

            dataObjInp->openFlags |= O_TRUNC | O_RDWR;
            l1descInx = _rsDataObjOpen (rsComm, dataObjInp);
        } else {

            l1descInx = OVERWRITE_WITHOUT_FORCE_FLAG;
        }
    }

    if( rodsObjStatOut != NULL ) { 
        freeRodsObjStat (rodsObjStatOut);
    }

    // =-=-=-=-=-=-=-
    // JMC - backport 4604
    if (lockFd >= 0) {
        if (l1descInx >= 0) {
            L1desc[l1descInx].lockFd = lockFd;
        } else {
            rsDataObjUnlock (rsComm, dataObjInp, lockFd);
        }
    }

    // =-=-=-=-=-=-=-
    return (l1descInx);
}

int
_rsDataObjCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    
    int status;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *tmpRescInfo;
    int l1descInx;
    int copiesNeeded;
    int failedCount = 0;
    int rescCnt;

    /* query rcat for resource info and sort it */

    status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
    if (status < 0) return status;

    rescCnt = getRescCnt (myRescGrpInfo);

    copiesNeeded = getCopiesFromCond (&dataObjInp->condInput);


    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        status      = l1descInx = _rsDataObjCreateWithRescInfo( rsComm, dataObjInp, 
                                                                tmpRescInfo, myRescGrpInfo->rescGroupName );
        /* loop till copyCount is satisfied */
        if (status < 0) {
            failedCount++;
            if (copiesNeeded == ALL_COPIES || (rescCnt - failedCount < copiesNeeded)) {
                /* XXXXX cleanup */
                freeAllRescGrpInfo (myRescGrpInfo);
                return (status);
            }
        } else {
            /* success. queue the rest of the resource if needed */
            if (copiesNeeded == ALL_COPIES || copiesNeeded > 1) {
                if (tmpRescGrpInfo->next != NULL) {
                    L1desc[l1descInx].moreRescGrpInfo = tmpRescGrpInfo->next;
                    /* in cache - don't change. tmpRescGrpInfo->next = NULL; */
                    L1desc[l1descInx].copiesNeeded = copiesNeeded;
                }
            }
            L1desc[l1descInx].openType = CREATE_TYPE;
            freeAllRescGrpInfo (myRescGrpInfo);
                    
                        
            return (l1descInx);
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    /* should not be here */

    freeAllRescGrpInfo (myRescGrpInfo);

    if (status < 0) {
        return (status);
    } else {
        rodsLog (LOG_NOTICE,
                 "rsDataObjCreate: Internal error");
        return (SYS_INTERNAL_NULL_INPUT_ERR);
    }
}

int
specCollSubCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    int status;
    int l1descInx;
    dataObjInfo_t *dataObjInfo = NULL;

    status = resolvePathInSpecColl (rsComm, dataObjInp->objPath, 
                                    WRITE_COLL_PERM, 0, &dataObjInfo);
    if( dataObjInfo == NULL ) { // JMC cppcheck
        rodsLog( LOG_ERROR, "specCollSubCreate :: dataObjInp is null" );
        return status;
    }
    if (status >= 0) {
        rodsLog (LOG_ERROR,
                 "specCollSubCreate: phyPath %s already exist", 
                 dataObjInfo->filePath);
        freeDataObjInfo (dataObjInfo);
        return (SYS_COPY_ALREADY_IN_RESC);
    } else if (status != SYS_SPEC_COLL_OBJ_NOT_EXIST) {
        return (status);
    }

    l1descInx = allocL1desc ();

    if (l1descInx < 0) return l1descInx;

    dataObjInfo->replStatus = NEWLY_CREATED_COPY;
    fillL1desc (l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize);

    if (getValByKey (&dataObjInp->condInput, NO_OPEN_FLAG_KW) == NULL) {
        status = dataCreate (rsComm, l1descInx);
        if (status < 0) {
            freeL1desc (l1descInx);
            return (status);
        }
    }

    return l1descInx;
}

/* _rsDataObjCreateWithRescInfo - Create a single copy of the data Object
 * given the rescInfo.
 *
 * return l1descInx.
 */

int
_rsDataObjCreateWithRescInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                              rescInfo_t *rescInfo, char *rescGroupName)
{
    
    dataObjInfo_t *dataObjInfo;
    int l1descInx;
    int status;

    l1descInx = allocL1desc ();
    if (l1descInx < 0) return l1descInx;

    dataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    initDataObjInfoWithInp (dataObjInfo, dataObjInp);
    
    if (getRescClass (rescInfo) == COMPOUND_CL) {
        rescInfo_t *cacheResc = NULL;
        char myRescGroupName[NAME_LEN];

        rstrcpy (myRescGroupName, rescGroupName, NAME_LEN);
        status = getCacheRescInGrp (rsComm, myRescGroupName, rescInfo, &cacheResc);
          
        if (status < 0 || cacheResc == NULL ) { // JMC cppcheck
            rodsLog (LOG_ERROR,
                     "DataObjCreateWithResInfo:getCacheRescInGrp %s err for %s stat=%d",
                     rescGroupName, dataObjInfo->objPath, status);
            free (dataObjInfo);
            freeL1desc (l1descInx);
            return status;
        }

        L1desc[l1descInx].replRescInfo = rescInfo;  /* repl to this resc */
        dataObjInfo->rescInfo = cacheResc;
        rstrcpy (dataObjInfo->rescName, cacheResc->rescName, NAME_LEN);
        rstrcpy (dataObjInfo->rescHier, cacheResc->rescName, MAX_NAME_LEN);
        rstrcpy (dataObjInfo->rescGroupName, myRescGroupName, NAME_LEN);
        if (getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL) { // JMC - backport 4537
            L1desc[l1descInx].purgeCacheFlag = 1;
        }
    
    } else {
        dataObjInfo->rescInfo = rescInfo;
        rstrcpy (dataObjInfo->rescName, rescInfo->rescName, NAME_LEN);
        rstrcpy (dataObjInfo->rescHier, rescInfo->rescName, MAX_NAME_LEN);
        rstrcpy (dataObjInfo->rescGroupName, rescGroupName, NAME_LEN);
        // =-=-=-=-=-=-=-
        // JMC - backport 4544
        if( getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL &&
            getRescClass (rescInfo) == CACHE_CL) {
            rescInfo_t *compResc = NULL;
            if( getRescInGrpByClass (rsComm, rescGroupName, COMPOUND_CL,
                                     &compResc, NULL) >= 0) { // JMC - backport 4547
                L1desc[l1descInx].replRescInfo = compResc;
                L1desc[l1descInx].purgeCacheFlag = 1;
            }
        }
        // =-=-=-=-=-=-=-
    }
    dataObjInfo->replStatus = NEWLY_CREATED_COPY; // JMC - backport 4754
    fillL1desc( l1descInx, dataObjInp, dataObjInfo, NEWLY_CREATED_COPY,
                dataObjInp->dataSize );

    status = getFilePathName (rsComm, dataObjInfo, L1desc[l1descInx].dataObjInp);

    if (status < 0) {
        freeL1desc (l1descInx);
        return (status);
    }

    /* output of _rsDataObjCreate - filePath stored in
     * dataObjInfo struct */

    if (getValByKey (&dataObjInp->condInput, NO_OPEN_FLAG_KW) != NULL) {
        
        /* don't actually physically open the file */
        status = 0;
    } else {
        
        status = dataObjCreateAndReg (rsComm, l1descInx);
    }

    if (status < 0) {
        freeL1desc (l1descInx);
        return (status);
    } else {
        return (l1descInx);
    }
}

/* dataObjCreateAndReg - Given the l1descInx, physically the file (dataCreate)
 * and register the new data object with the rcat
 */
 
int
dataObjCreateAndReg (rsComm_t *rsComm, int l1descInx)
{

    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;

    status = dataCreate (rsComm, l1descInx);

    if (status < 0) {
        return (status);
    }

    /* only register new copy */
    status = svrRegDataObj (rsComm, myDataObjInfo);
    if (status < 0) {
        l3Unlink (rsComm, myDataObjInfo);
        rodsLog (LOG_NOTICE,
                 "dataObjCreateAndReg: rsRegDataObj for %s failed, status = %d",
                 myDataObjInfo->objPath, status);
        return (status);
    } else {
        myDataObjInfo->replNum = status;
        return (0);
    }  
}

/* dataCreate - given the l1descInx, physically create the file
 * and save the l3descInx in L1desc[l1descInx].
 */

int
dataCreate (rsComm_t *rsComm, int l1descInx)
{
    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;

    /* should call resolveHostForTansfer to find host. gateway rodsServerHost
     * should be in l3desc */
    status = l3Create (rsComm, l1descInx);

    if (status <= 0) {
        rodsLog (LOG_NOTICE,
                 "dataCreate: l3Create of %s failed, status = %d",
                 myDataObjInfo->filePath, status);
        return (status);
    } else {
        L1desc[l1descInx].l3descInx = status;
    }

    return (0);
}

int
l3Create (rsComm_t *rsComm, int l1descInx)
{
    dataObjInfo_t *dataObjInfo;
    int l3descInx;

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
        l3descInx = rsSubStructFileCreate (rsComm, &subFile);
    } else {
        /* normal or mounted file */
        l3descInx = l3CreateByObjInfo (rsComm, L1desc[l1descInx].dataObjInp,
                                       L1desc[l1descInx].dataObjInfo);
    }

    return (l3descInx);
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
l3CreateByObjInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
                   dataObjInfo_t *dataObjInfo)
{
    int rescTypeInx;
    int l3descInx;

    rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

    switch (RescTypeDef[rescTypeInx].rescCat) {
    case FILE_CAT:
    {
        int retryCnt = 0;
        int chkType = 0; // JMC - backport 4774

        fileCreateInp_t fileCreateInp;
        memset (&fileCreateInp, 0, sizeof (fileCreateInp));
        rstrcpy( fileCreateInp.resc_name_, dataObjInfo->rescInfo->rescName, MAX_NAME_LEN );
        rstrcpy( fileCreateInp.resc_hier_, dataObjInfo->rescHier, MAX_NAME_LEN );
        fileCreateInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (fileCreateInp.addr.hostAddr,  dataObjInfo->rescInfo->rescLoc,
                 NAME_LEN);
        rstrcpy (fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
        fileCreateInp.mode = getFileMode (dataObjInp);
        // =-=-=-=-=-=-=-
        // JMC - backport 4774
        chkType = getchkPathPerm (rsComm, dataObjInp, dataObjInfo);
        if (chkType == DISALLOW_PATH_REG) {
            return PATH_REG_NOT_ALLOWED;
        } else if (chkType == NO_CHK_PATH_PERM) {
            fileCreateInp.otherFlags |= NO_CHK_PERM_FLAG;  // JMC - backport 4758
        }
        // =-=-=-=-=-=-=-
        std::string prev_resc_hier = fileCreateInp.resc_hier_;
        l3descInx = rsFileCreate (rsComm, &fileCreateInp);
        if(prev_resc_hier != std::string(fileCreateInp.resc_hier_)) {
            rstrcpy(dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN);
            eirods::error ret =_updateDbWithRescHier(rsComm, fileCreateInp.resc_hier_, 1);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - Unable to update database with resc hier info.";
                eirods::log(LOG_ERROR, msg.str());
                return ret.code();
            }
        }
        
        /* file already exists ? */
        while( l3descInx <= 2 && retryCnt < 100 && 
               getErrno (l3descInx) == EEXIST ) {
            if (resolveDupFilePath (rsComm, dataObjInfo, dataObjInp) < 0) {
                break;
            }
            rstrcpy (fileCreateInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
            l3descInx = rsFileCreate (rsComm, &fileCreateInp);
            if(prev_resc_hier != std::string(fileCreateInp.resc_hier_)) {
                rstrcpy(dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN);
                eirods::error ret =_updateDbWithRescHier(rsComm, fileCreateInp.resc_hier_, 1);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - Unable to update database with resc hier info.";
                    eirods::log(LOG_ERROR, msg.str());
                    return ret.code();
                }
            }
            retryCnt ++; 
        }
        rstrcpy(dataObjInfo->rescHier, fileCreateInp.resc_hier_, MAX_NAME_LEN);
        break;
    }
    default:
        rodsLog (LOG_NOTICE,
                 "l3Create: rescCat type %d is not recognized",
                 RescTypeDef[rescTypeInx].rescCat);
        l3descInx = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (l3descInx);
}

/* getRescGrpForCreate - given the resource input in dataObjInp, get the
 * rescGrpInfo_t of the resource after applying the acSetRescSchemeForCreate
 * rule. 
 * Return 1 of the "random" sorting scheme is used. Otherwise return 0
 * or an error code.
 */

int
getRescGrpForCreate (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                     rescGrpInfo_t **myRescGrpInfo)
{ 
    int status;
    ruleExecInfo_t rei;

    /* query rcat for resource info and sort it */

    initReiWithDataObjInp (&rei, rsComm, dataObjInp);
    if (dataObjInp->oprType == REPLICATE_OPR) { // JMC - backport 4660
        status = applyRule ("acSetRescSchemeForRepl", NULL, &rei, 
                            NO_SAVE_REI);
    } else {
        status = applyRule ("acSetRescSchemeForCreate", NULL, &rei, 
                            NO_SAVE_REI);
    }


    if (status < 0) {
        if (rei.status < 0)
            status = rei.status;
        rodsLog (LOG_NOTICE,
                 "getRescGrpForCreate:acSetRescSchemeForCreate error for %s,status=%d",
                 dataObjInp->objPath, status);
        return (status);
    }
    if (rei.rgi == NULL) {
        /* def resc group has not been initialized yet */
        status = setDefaultResc (rsComm, NULL, NULL,
                                 &dataObjInp->condInput, myRescGrpInfo);
        if (status < 0) status = SYS_INVALID_RESC_INPUT;
    } else {
        *myRescGrpInfo = rei.rgi;
    }

    status = setRescQuota (rsComm, dataObjInp->objPath, myRescGrpInfo,
                           dataObjInp->dataSize);
    if (status == SYS_RESC_QUOTA_EXCEEDED) return SYS_RESC_QUOTA_EXCEEDED;

    if (strstr (rei.statusStr, "random") == NULL) {
        /* not a random scheme */
        sortRescByLocation (myRescGrpInfo);
        return 0;
    } else {
        return 1;
    }

}

