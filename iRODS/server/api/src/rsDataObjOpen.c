/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See dataObjOpen.h for a description of this API call.*/

#include "dataObjOpen.h"
#include "dataObjOpenAndStat.h"
#include "rodsLog.h"
#include "objMetaOpr.h"
#include "resource.h"
#include "resource.h"
#include "dataObjOpr.h"
#include "physPath.h"
#include "dataObjCreate.h"
#include "dataObjLock.h"

#include "fileOpen.h"
#include "subStructFileOpen.h"
#include "rsGlobalExtern.h"
#include "rcGlobalExtern.h"
#include "reGlobalsExtern.h"
#include "reDefines.h"
#include "reDefines.h"
#include "getRemoteZoneResc.h"
#include "regReplica.h"
#include "regDataObj.h"
#include "dataObjClose.h"
#include "dataObjRepl.h"

int
rsDataObjOpen (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    int status, l1descInx;
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;

    remoteFlag = getAndConnRemoteZone (rsComm, dataObjInp, &rodsServerHost,
      REMOTE_OPEN);

    if (remoteFlag < 0) {
        return (remoteFlag);
    } else if (remoteFlag == REMOTE_HOST) {
	openStat_t *openStat = NULL;
        status = rcDataObjOpenAndStat (rodsServerHost->conn, dataObjInp,
	  &openStat);
        if (status < 0) return status;
        l1descInx = allocAndSetL1descForZoneOpr (status, dataObjInp,
	  rodsServerHost, openStat);
	if (openStat != NULL) free (openStat);
        return (l1descInx);
    } else {
        l1descInx = _rsDataObjOpen (rsComm, dataObjInp);
    }

    return (l1descInx);
}

/* _rsDataObjOpen - handle internal server dataObj open request.
 * valid phyOpenFlag are DO_PHYOPEN, DO_NOT_PHYOPEN and PHYOPEN_BY_SIZE
 */
 
int
_rsDataObjOpen (rsComm_t *rsComm, dataObjInp_t *dataObjInp)
{
    int status;
    dataObjInfo_t *dataObjInfoHead = NULL;
    dataObjInfo_t *otherDataObjInfo = NULL;
    dataObjInfo_t *nextDataObjInfo = NULL;
    dataObjInfo_t *tmpDataObjInfo; 
    dataObjInfo_t *compDataObjInfo = NULL;
    dataObjInfo_t *cacheDataObjInfo = NULL;
    rescInfo_t *compRescInfo = NULL;
#if 0 // JMC - backport 4590
	rescGrpInfo_t *myRescGrpInfo = NULL; // JMC - backport 4547
#endif
    int l1descInx;
    int writeFlag;
    int phyOpenFlag = DO_PHYOPEN;
    char *lockType = NULL; // JMC - backport 4604
    int lockFd = -1; // JMC - backport 4604

    if (getValByKey (&dataObjInp->condInput, NO_OPEN_FLAG_KW) != NULL) {
	phyOpenFlag = DO_NOT_PHYOPEN;
    } else if (getValByKey (&dataObjInp->condInput, PHYOPEN_BY_SIZE_KW) 
      != NULL) {
	phyOpenFlag = PHYOPEN_BY_SIZE;
    }
    // =-=-=-=-=-=-=-
	// JMC - backport 4604
    lockType = getValByKey (&dataObjInp->condInput, LOCK_TYPE_KW);
    if (lockType != NULL) {
        lockFd = rsDataObjLock (rsComm, dataObjInp);
        if (lockFd > 0) {
            /* rm it so it won't be done again causing deadlock */
            rmKeyVal (&dataObjInp->condInput, LOCK_TYPE_KW);
        } else {
            rodsLogError (LOG_ERROR, lockFd,
              "_rsDataObjOpen: rsDataObjLock error for %s. lockType = %s",
              dataObjInp->objPath, lockType);
            return lockFd;
        }
    }
    // =-=-=-=-=-=-=-
/* query rcat for dataObjInfo and sort it */

status = getDataObjInfoIncSpecColl (rsComm, dataObjInp, &dataObjInfoHead);

    writeFlag = getWriteFlag (dataObjInp->openFlags);

    if (status < 0) {
        if (dataObjInp->openFlags & O_CREAT && writeFlag > 0) {
            l1descInx = rsDataObjCreate (rsComm, dataObjInp);
           status = l1descInx; // JMC - backport 4604
	}
	   // =-=-=-=-=-=-=-
 	   // JMC - backport 4604
       if (lockFd >= 0) {
           if (status > 0) {
               L1desc[l1descInx].lockFd = lockFd;
           } else {
                rsDataObjUnlock (rsComm, dataObjInp, lockFd);
           }
       }
       return (status);
       // =-=-=-=-=-=-=-
    } else {
        /* screen out any stale copies */
        status = sortObjInfoForOpen (rsComm, &dataObjInfoHead, 
	  &dataObjInp->condInput, writeFlag);
        if (status < 0) { // JMC - backport 4604
           if (lockFd > 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd);
           return status;
       }

        status = applyPreprocRuleForOpen (rsComm, dataObjInp, &dataObjInfoHead);
        if (status < 0) { // JMC - backport 4604
           if (lockFd > 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd);
           return status;
       }
    }

    if (getStructFileType (dataObjInfoHead->specColl) >= 0) {
	/* special coll. Nothing to do */
	} else if (writeFlag > 0) {
		// JMC :: had to reformat this code to find a missing {
	    //     :: i seriously hope its in the right place...
       status = procDataObjOpenForWrite (rsComm, dataObjInp, &dataObjInfoHead, &cacheDataObjInfo, &compDataObjInfo, &compRescInfo); 
    } else {
        status = procDataObjOpenForRead (rsComm, dataObjInp, &dataObjInfoHead,
          &cacheDataObjInfo, &compDataObjInfo, &compRescInfo);
    }
    if (status < 0) {
		if (lockFd > 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd); // JMC - backport 4604
        freeAllDataObjInfo (dataObjInfoHead);
       return status;
    }
#if 0  /* refactor with procDataObjOpenForRead and procDataObjOpenForWrite */

		/* put the copy with destResc on top */
		status = requeDataObjInfoByDestResc (&dataObjInfoHead, &dataObjInp->condInput, writeFlag, 1);

		/* status < 0 means there is no copy in the DEST_RESC */
		if (status < 0 &&getValByKey (&dataObjInp->condInput, DEST_RESC_NAME_KW) != NULL) {
			/* we don't have a copy in the DEST_RESC_NAME */
			status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
			if (status < 0) 
			    return status;
			if (getRescClass (myRescGrpInfo->rescInfo) == COMPOUND_CL) {
				/* get here because the comp object does not exist. Find
				* a cache copy. If one does not exist, stage one to cache */
				
				status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,dataObjInfoHead, NULL, myRescGrpInfo, NULL, &cacheDataObjInfo);

				if (status < 0) {
					rodsLog (LOG_ERROR,"_rsDataObjOpen: getCacheDataInfo of %s failed, stat=%d",dataObjInfoHead->objPath, status);
					freeAllDataObjInfo (dataObjInfoHead);
					freeAllRescGrpInfo (myRescGrpInfo);
					return status;
				} else {
				    compRescInfo = myRescGrpInfo->rescInfo;
			    }
			} else {     /* dest resource is not a compound resource */
				status = createEmptyRepl (rsComm, dataObjInp, &dataObjInfoHead);
				if (status < 0) {
					rodsLog (LOG_ERROR,
					"_rsDataObjOpen: createEmptyRepl of %s failed, stat=%d",
					dataObjInfoHead->objPath, status);
					freeAllDataObjInfo (dataObjInfoHead);
					freeAllRescGrpInfo (myRescGrpInfo);
					return status;
				}
			}

		} else if (getRescClass (dataObjInfoHead->rescInfo) == COMPOUND_CL) {
			/* The target data object exists and it is a COMPOUND_CL. Save the 
			* comp object. It can be requeued by stageAndRequeDataToCache */
			compDataObjInfo = dataObjInfoHead;
			status = stageAndRequeDataToCache (rsComm, &dataObjInfoHead);
			if (status < 0 && status != SYS_COPY_ALREADY_IN_RESC) {
				rodsLog (LOG_ERROR,"_rsDataObjOpen:stageAndRequeDataToCache %s failed stat=%d",dataObjInfoHead->objPath, status);
				freeAllDataObjInfo (dataObjInfoHead);
				return status;
			}
			cacheDataObjInfo = dataObjInfoHead;
		// =-=-=-=-=-=-=-
		// JMC - backport 4547
		} else if ( getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL  && 
		            getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo) >= 0 &&
		            strlen (myRescGrpInfo->rescGroupName) > 0) {

			if( getRescInGrpByClass ( rsComm, myRescGrpInfo->rescGroupName,COMPOUND_CL, &compRescInfo, NULL) >= 0) {
				status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,dataObjInfoHead, NULL, myRescGrpInfo, NULL,&cacheDataObjInfo);
				if (status < 0) {
					rodsLog (LOG_NOTICE,
					"_rsDataObjOpen: getCacheDataInfo of %s failed, stat=%d",
					dataObjInfoHead->objPath, status);
				} else {
					if (getDataObjByClass (dataObjInfoHead, COMPOUND_CL, &compDataObjInfo) >= 0) {
						/* we have a compDataObjInfo */
						compRescInfo = NULL;
					}
				}
			}
		// =-=-=-=-=-=-=-
		} else if (getRescClass (dataObjInfoHead->rescInfo) == COMPOUND_CL) {
			/* open for read */
			status = stageAndRequeDataToCache (rsComm, &dataObjInfoHead);
			if (status < 0 && status != SYS_COPY_ALREADY_IN_RESC) {
				rodsLog (LOG_ERROR,
				"_rsDataObjOpen: stageAndRequeDataToCache of %s failed stat=%d",
				dataObjInfoHead->objPath, status);
				freeAllDataObjInfo (dataObjInfoHead);
				return status;
			}
			cacheDataObjInfo = dataObjInfoHead;
		// =-=-=-=-=-=-=-
		// JMC - backport 4547
		} else if (getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL &&strlen (dataObjInfoHead->rescGroupName) > 0) {
			if (getRescInGrpByClass (rsComm, dataObjInfoHead->rescGroupName,COMPOUND_CL, &compRescInfo, &myRescGrpInfo) >= 0) {
				status = getCacheDataInfoOfCompResc (rsComm, dataObjInp, dataObjInfoHead, NULL, myRescGrpInfo, NULL,&cacheDataObjInfo);
				if (status < 0) {
					rodsLog (LOG_NOTICE,	  "_rsDataObjOpen: getCacheDataInfo of %s failed, stat=%d",dataObjInfoHead->objPath, status);
				} else {
					if (getDataObjByClass (dataObjInfoHead, COMPOUND_CL,  &compDataObjInfo) >= 0) {
						/* we have a compDataObjInfo */
						compRescInfo = NULL;
					}
				}
			}
		}
        // =-=-=-=-=-=-=-
	} // writeFlag > 0
	
	freeAllRescGrpInfo( myRescGrpInfo);
#endif  /* refactor with procDataObjOpenForRead and procDataObjOpenForWrite */ // JMC - backport 4590

    if (getRescClass (dataObjInfoHead->rescInfo) == BUNDLE_CL) {
	status = stageBundledData (rsComm, &dataObjInfoHead);
        if (status < 0) {
            rodsLog (LOG_ERROR,
              "_rsDataObjOpen: stageBundledData of %s failed stat=%d",
              dataObjInfoHead->objPath, status);
            freeAllDataObjInfo (dataObjInfoHead);
			if (lockFd >= 0) rsDataObjUnlock (rsComm, dataObjInp, lockFd); // JMC - backport 4604
            return status;
        }
    }

    /* If cacheDataObjInfo != NULL, this is the staged copy of
     * the compound obj. This copy must be opened. 
     * If compDataObjInfo != NULL, an existing COMPOUND_CL DataObjInfo exist. 
     * Need to replicate to * this DataObjInfo in rsdataObjClose. 
     * At this point, cacheDataObjInfo is left in the dataObjInfoHead queue
     * but not compDataObjInfo.
     * If compRescInfo != NULL, writing to a compound resource where there 
     * is no existing copy in the resource. Need to replicate to this 
     * resource in rsdataObjClose.  
     * For read, both compDataObjInfo and compRescInfo should be NULL.
     */
    tmpDataObjInfo = dataObjInfoHead;

    while (tmpDataObjInfo != NULL) {
        nextDataObjInfo = tmpDataObjInfo->next;
        tmpDataObjInfo->next = NULL;
	    if (getRescClass (tmpDataObjInfo->rescInfo) == COMPOUND_CL) {
			/* this check is not necessary but won't hurt */
	        if (compDataObjInfo != tmpDataObjInfo)  {
				/* save it in otherDataObjInfo so no mem leak */ // JMC - backport 4590
		        queDataObjInfo (&otherDataObjInfo, tmpDataObjInfo, 1, 1);
            }
	        tmpDataObjInfo = nextDataObjInfo;
	        continue;
	    } else if ( writeFlag > 0 && cacheDataObjInfo != NULL && 
	                tmpDataObjInfo != cacheDataObjInfo) {
	        /* skip anything that does not match cacheDataObjInfo */
            queDataObjInfo (&otherDataObjInfo, tmpDataObjInfo, 1, 1);
            tmpDataObjInfo = nextDataObjInfo;
	        continue;
	    }
	    
		status = l1descInx = _rsDataObjOpenWithObjInfo (rsComm, dataObjInp,phyOpenFlag, tmpDataObjInfo, cacheDataObjInfo);

        if (status >= 0) {
	        if (compDataObjInfo != NULL) {
               L1desc[l1descInx].replDataObjInfo = compDataObjInfo;
           } else if (compRescInfo != NULL) {
               L1desc[l1descInx].replRescInfo = compRescInfo;
		   } 

		    queDataObjInfo (&otherDataObjInfo, nextDataObjInfo, 0, 1); // JMC - backport 4542
		    L1desc[l1descInx].otherDataObjInfo = otherDataObjInfo; // JMC - backport 4542

	        if (writeFlag > 0) {
	            L1desc[l1descInx].openType = OPEN_FOR_WRITE_TYPE;
	        } else {
               L1desc[l1descInx].openType = OPEN_FOR_READ_TYPE;
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
	    
		} // if status >= 0

        tmpDataObjInfo = nextDataObjInfo;
    } // while
    freeAllDataObjInfo (otherDataObjInfo);

    return (status);
} // BAD

/* _rsDataObjOpenWithObjInfo - given a dataObjInfo, open a single copy
 * of the data object.
 *
 * return l1descInx
 */

int
_rsDataObjOpenWithObjInfo (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
int phyOpenFlag, dataObjInfo_t *dataObjInfo, dataObjInfo_t *cacheDataObjInfo) // JMC - backport 4537
{
    int replStatus;
    int status;
    int l1descInx;

    l1descInx = allocL1desc ();

    if (l1descInx < 0) return l1descInx;

    replStatus = dataObjInfo->replStatus | OPEN_EXISTING_COPY;

    /* the size was set to -1 because we don't know the target size.
     * For copy and replicate, the calling routine should modify this
     * dataSize */
    fillL1desc (l1descInx, dataObjInp, dataObjInfo, replStatus, -1);
    if (dataObjInfo == cacheDataObjInfo &&  // JMC - backport 4537
       getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL) {
       L1desc[l1descInx].purgeCacheFlag = 1;
    }
    if (phyOpenFlag == DO_NOT_PHYOPEN) {
        /* don't actually physically open the file */
        status = 0;
    } else if (phyOpenFlag == PHYOPEN_BY_SIZE) {
        /* open for put or get. May do "dataInclude" */
        if (getValByKey (&dataObjInp->condInput, DATA_INCLUDED_KW) != NULL
          && dataObjInfo->dataSize <= MAX_SZ_FOR_SINGLE_BUF) {
            status = 0;
        } else if (dataObjInfo->dataSize != UNKNOWN_FILE_SZ && 
	  dataObjInfo->dataSize < MAX_SZ_FOR_SINGLE_BUF) {
            status = 0;
        } else {
            status = dataOpen (rsComm, l1descInx);
        }
    } else {
        status = dataOpen (rsComm, l1descInx);
    }

    if (status < 0) {
        freeL1desc (l1descInx);
	return (status);
    } else {
        return (l1descInx);
    }
}

int
dataOpen (rsComm_t *rsComm, int l1descInx)
{
    dataObjInfo_t *myDataObjInfo = L1desc[l1descInx].dataObjInfo;
    int status;


    status = l3Open (rsComm, l1descInx);

    if (status <= 0) {
        rodsLog (LOG_NOTICE,
          "dataOpen: l3Open of %s failed, status = %d",
          myDataObjInfo->filePath, status);
        return (status);
    } else {
        L1desc[l1descInx].l3descInx = status;
	return (0);
    }
}

int
l3Open (rsComm_t *rsComm, int l1descInx)
{
    dataObjInfo_t *dataObjInfo;
    int l3descInx;
    int mode, flags;

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
        subFile.flags = getFileFlags (l1descInx);
	l3descInx = rsSubStructFileOpen (rsComm, &subFile); 
    } else {
        mode = getFileMode (L1desc[l1descInx].dataObjInp);
        flags = getFileFlags (l1descInx);
	l3descInx = _l3Open (rsComm, dataObjInfo, mode, flags);
    }
    return (l3descInx);
}

int
_l3Open (rsComm_t *rsComm, dataObjInfo_t *dataObjInfo, int mode, int flags)
{
    int rescTypeInx;
    int l3descInx;
    fileOpenInp_t fileOpenInp;

    rescTypeInx = dataObjInfo->rescInfo->rescTypeInx;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
        memset (&fileOpenInp, 0, sizeof (fileOpenInp));
        fileOpenInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (fileOpenInp.addr.hostAddr,  dataObjInfo->rescInfo->rescLoc,
          NAME_LEN);
        rstrcpy (fileOpenInp.fileName, dataObjInfo->filePath, MAX_NAME_LEN);
        fileOpenInp.mode = mode;
        fileOpenInp.flags = flags;
        l3descInx = rsFileOpen (rsComm, &fileOpenInp);
        break;
      default:
        rodsLog (LOG_NOTICE,
          "l3Open: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        l3descInx = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (l3descInx);
}

/* l3OpenByHost - call level3 open - this differs from l3Open in that 
 * rsFileOpenByHost is called instead of rsFileOpen
 */

int 
l3OpenByHost (rsComm_t *rsComm, int rescTypeInx, int l3descInx, int flags)
{
    fileOpenInp_t fileOpenInp;
    int newL3descInx;

    switch (RescTypeDef[rescTypeInx].rescCat) {
      case FILE_CAT:
        memset (&fileOpenInp, 0, sizeof (fileOpenInp));
        fileOpenInp.fileType = (fileDriverType_t)RescTypeDef[rescTypeInx].driverType;
        rstrcpy (fileOpenInp.fileName, FileDesc[l3descInx].fileName, 
	  MAX_NAME_LEN);
        fileOpenInp.mode = FileDesc[l3descInx].mode;
        fileOpenInp.flags = flags;
        newL3descInx = rsFileOpenByHost (rsComm, &fileOpenInp, 
	  FileDesc[l3descInx].rodsServerHost);
        break;
      default:
        rodsLog (LOG_NOTICE,
          "l3OpenByHost: rescCat type %d is not recognized",
          RescTypeDef[rescTypeInx].rescCat);
        l3descInx = SYS_INVALID_RESC_TYPE;
        break;
    }
    return (newL3descInx);
}

int
applyPreprocRuleForOpen (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead)
{
    int status;
    ruleExecInfo_t rei;

    initReiWithDataObjInp (&rei, rsComm, dataObjInp);
    rei.doi = *dataObjInfoHead;

    status = applyRule ("acPreprocForDataObjOpen", NULL, &rei, NO_SAVE_REI);

    if (status < 0) {
        if (rei.status < 0) {
            status = rei.status;
        }
        rodsLog (LOG_ERROR,
         "applyPreprocRuleForOpen:acPreprocForDataObjOpen error for %s,stat=%d",
          dataObjInp->objPath, status);
    } else {
        *dataObjInfoHead = rei.doi;
    }
    return (status);
}

/* createEmptyRepl - Physically create a zero length file and register
 * as a replica and queue the dataObjInfo on the top of dataObjInfoHead.
 */
int
createEmptyRepl (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead)
{
    int status;
    char *rescName;
    rescInfo_t *rescInfo;
    rescGrpInfo_t *tmpRescGrpInfo;
    regReplica_t regReplicaInp;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    keyValPair_t *condInput = &dataObjInp->condInput;
    dataObjInfo_t *myDataObjInfo;

    if ((rescName = getValByKey (condInput, DEST_RESC_NAME_KW)) == NULL &&
      (rescName = getValByKey (condInput, BACKUP_RESC_NAME_KW)) == NULL &&
      (rescName = getValByKey (condInput, DEF_RESC_NAME_KW)) == NULL) {
	return USER_NO_RESC_INPUT_ERR;
    }

    status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
    if (status < 0 || myRescGrpInfo == NULL ) return status; // JMC cppcheck

    myDataObjInfo = (dataObjInfo_t*)malloc (sizeof (dataObjInfo_t));
    *myDataObjInfo = *(*dataObjInfoHead);
    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
	rescInfo = tmpRescGrpInfo->rescInfo;
        myDataObjInfo->rescInfo = rescInfo;
        rstrcpy (myDataObjInfo->rescName, rescInfo->rescName, NAME_LEN);
        rstrcpy (myDataObjInfo->rescGroupName, myRescGrpInfo->rescGroupName, 
	  NAME_LEN);
        status = getFilePathName (rsComm, myDataObjInfo, dataObjInp);
	if (status < 0) {
	    tmpRescGrpInfo = tmpRescGrpInfo->next;
	    continue;
	}
	status = l3CreateByObjInfo (rsComm, dataObjInp, myDataObjInfo);
	if (status < 0) {
	    tmpRescGrpInfo = tmpRescGrpInfo->next;
            continue;
        }
	/* close it */
	_l3Close (rsComm, rescInfo->rescTypeInx, status);

	/* register the replica */
        memset (&regReplicaInp, 0, sizeof (regReplicaInp));
        regReplicaInp.srcDataObjInfo = *dataObjInfoHead;
        regReplicaInp.destDataObjInfo = myDataObjInfo;
        if (getValByKey (&dataObjInp->condInput, IRODS_ADMIN_KW) != NULL) {
            addKeyVal (&regReplicaInp.condInput, IRODS_ADMIN_KW, "");
        }
        status = rsRegReplica (rsComm, &regReplicaInp);
        clearKeyVal (&regReplicaInp.condInput);

	break;
    }

    freeAllRescGrpInfo (myRescGrpInfo);

    if (status < 0) {
	free (myDataObjInfo);
    } else {
	/* queue it on top */
	myDataObjInfo->next = *dataObjInfoHead;
	*dataObjInfoHead = myDataObjInfo;
    }
    return status;
}

// =-=-=-=-=-=-=-
// JMC - backport 4590
int
procDataObjOpenForWrite (rsComm_t *rsComm, dataObjInp_t *dataObjInp, 
dataObjInfo_t **dataObjInfoHead, dataObjInfo_t **cacheDataObjInfo, 
dataObjInfo_t **compDataObjInfo, rescInfo_t **compRescInfo)
{
    int status = 0;
    rescGrpInfo_t *myRescGrpInfo = NULL;

    /* put the copy with destResc on top */
    status = requeDataObjInfoByDestResc (dataObjInfoHead,
      &dataObjInp->condInput, 1, 1);
    /* status < 0 means there is no copy in the DEST_RESC */
    if (status < 0 &&
      getValByKey (&dataObjInp->condInput, DEST_RESC_NAME_KW) != NULL) {
        /* we don't have a copy in the DEST_RESC_NAME */
        status = getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo);
        if (status < 0) return status;
        if (getRescClass (myRescGrpInfo->rescInfo) == COMPOUND_CL) {
            /* get here because the comp object does not exist. Find
             * a cache copy. If one does not exist, stage one to cache */
            status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,
              *dataObjInfoHead, NULL, myRescGrpInfo, NULL,
              cacheDataObjInfo);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "procDataObjForOpenWrite: getCacheDataInfo of %s failed",
                  (*dataObjInfoHead)->objPath);
                freeAllRescGrpInfo (myRescGrpInfo);
                return status;
            } else {
               /* replicate to compRescInfo after write is done */
                *compRescInfo = myRescGrpInfo->rescInfo;
            }
        } else {     /* dest resource is not a compound resource */
           /* we don't have a copy, so create an empty dataObjInfo */
            status = createEmptyRepl (rsComm, dataObjInp, dataObjInfoHead);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "procDataObjForOpenWrite: createEmptyRepl of %s failed",
                  (*dataObjInfoHead)->objPath);
                freeAllRescGrpInfo (myRescGrpInfo);
                return status;
            }
        }
    } else {   /*  The target data object exists */
        status = procDataObjOpenForExistObj (rsComm, dataObjInp, 
         dataObjInfoHead, cacheDataObjInfo, compDataObjInfo, compRescInfo);
#if 0  /* refactored by procDataObjOpenForExistObj */
        if (getRescClass ((*dataObjInfoHead)->rescInfo) == COMPOUND_CL) {
        /* It is a COMPOUND_CL. Save the comp object because it can be 
         * requeued by stageAndRequeDataToCache */
            *compDataObjInfo = *dataObjInfoHead;
            status = stageAndRequeDataToCache (rsComm, dataObjInfoHead);
            if (status < 0 && status != SYS_COPY_ALREADY_IN_RESC) {
                rodsLogError (LOG_ERROR, status,
                  "procDataObjForOpenWrite:stageAndRequeDataToCache %s failed",
                  (*dataObjInfoHead)->objPath);
                return status;
            }
            *cacheDataObjInfo = *dataObjInfoHead;
        } else if (getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL
          && getRescGrpForCreate (rsComm, dataObjInp, &myRescGrpInfo) >= 0 &&
          strlen (myRescGrpInfo->rescGroupName) > 0) {
           /* Do purge cache and destResc is a resource group. See if we
            * a COMPOUND_CL resource in the group */ 
            if (getRescInGrpByClass (rsComm, myRescGrpInfo->rescGroupName,
              COMPOUND_CL, compRescInfo, NULL) >= 0) {
               /* get cacheDataObjInfo */
                status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,
                 *dataObjInfoHead, NULL, myRescGrpInfo, NULL, cacheDataObjInfo);
                if (status < 0) {
                    rodsLogError (LOG_NOTICE, status,
                      "procDataObjForOpenWrite: getCacheDataInfo of %s failed",
                      (*dataObjInfoHead)->objPath);
                } else {
                    if (getDataObjByClass (*dataObjInfoHead, COMPOUND_CL,
                      compDataObjInfo) >= 0) {
                        /* we have a compDataObjInfo */
                        *compRescInfo = NULL;
                    }
                }
            }
        }
#endif /*   refactored by procDataObjOpenForExistObj */
    }
    if (*compDataObjInfo != NULL) {
        dequeDataObjInfo (dataObjInfoHead, *compDataObjInfo);
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return status;
}

/* procDataObjOpenForExistObj - process a dataObj for COMPOUND_CL special
 * situation. The object must be :
 *    read - already exist 
 *    write - already exist in dest Resource
 */
int
procDataObjOpenForExistObj (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead, dataObjInfo_t **cacheDataObjInfo,
dataObjInfo_t **compDataObjInfo, rescInfo_t **compRescInfo)
{
    int status = 0;
    rescGrpInfo_t *myRescGrpInfo = NULL;

    if (getRescClass ((*dataObjInfoHead)->rescInfo) == COMPOUND_CL) {
        /* It is a COMPOUND_CL. Save the comp object because it can be
         * requeued by stageAndRequeDataToCache */
        *compDataObjInfo = *dataObjInfoHead;
        status = stageAndRequeDataToCache (rsComm, dataObjInfoHead);
        if (status < 0 && status != SYS_COPY_ALREADY_IN_RESC) {
            rodsLogError (LOG_ERROR, status,
             "procDataObjOpenForExistObj:stageAndRequeDataToCache of %s failed",
              (*dataObjInfoHead)->objPath);
            return status;
        }
        *cacheDataObjInfo = *dataObjInfoHead;
    } else if (getValByKey (&dataObjInp->condInput, PURGE_CACHE_KW) != NULL &&
      strlen ((*dataObjInfoHead)->rescGroupName) > 0) {
        /* Do purge cache and destResc is a resource group. See if we
         * a COMPOUND_CL resource in the group */
        if (getRescInGrpByClass (rsComm, (*dataObjInfoHead)->rescGroupName,
          COMPOUND_CL, compRescInfo, &myRescGrpInfo) >= 0) {
           /* get cacheDataObjInfo */
            status = getCacheDataInfoOfCompResc (rsComm, dataObjInp,
              *dataObjInfoHead, NULL, myRescGrpInfo, NULL,
              cacheDataObjInfo);
            if (status < 0) {
                rodsLogError (LOG_NOTICE, status,
                  "procDataObjOpenForExistObj: getCacheDataInfo of %s failed",
                  (*dataObjInfoHead)->objPath);
            } else {
                if (getDataObjByClass (*dataObjInfoHead, COMPOUND_CL,
                  compDataObjInfo) >= 0) {
                    /* we have a compDataObjInfo */
                    *compRescInfo = NULL;
                }
            }
        }
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return status;
}

int
procDataObjOpenForRead (rsComm_t *rsComm, dataObjInp_t *dataObjInp,
dataObjInfo_t **dataObjInfoHead, dataObjInfo_t **cacheDataObjInfo,
dataObjInfo_t **compDataObjInfo, rescInfo_t **compRescInfo)
{
    int status = 0;

    status = procDataObjOpenForExistObj (rsComm, dataObjInp, dataObjInfoHead,
      cacheDataObjInfo, compDataObjInfo, compRescInfo);

    if (*compDataObjInfo != NULL) {
       dequeDataObjInfo (dataObjInfoHead, *compDataObjInfo);
       freeDataObjInfo (*compDataObjInfo);
        *compDataObjInfo = NULL;
    }
    *compRescInfo = NULL;
    return status;
}

// =-=-=-=-=-=-=-
