/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* resource.c - resorce metadata operation */

#include "resource.h"
#include "genQuery.h"
#include "rodsClient.h"

/* getRescInfo - Given the rescName or rescgrpName in condInput keyvalue
 * pair or defaultResc, return the rescGrpInfo containing the info on
 * this resource or resource group.
 * The resource given in condInput is taken first, then defaultResc.
 * If it is a resource, the returned integer can be 0, SYS_RESC_IS_DOWN
 * SYS_RESC_QUOTA_EXCEEDED.
 * 
 */

int
getRescInfo (rsComm_t *rsComm, char *defaultResc, keyValPair_t *condInput,
rescGrpInfo_t **rescGrpInfo)
{
    char *rescName;
    int status;

    if ((rescName = getValByKey (condInput, BACKUP_RESC_NAME_KW)) == NULL &&
      (rescName = getValByKey (condInput, DEST_RESC_NAME_KW)) == NULL &&
      (rescName = getValByKey (condInput, DEF_RESC_NAME_KW)) == NULL &&
      (rescName = getValByKey (condInput, RESC_NAME_KW)) == NULL &&
      ((rescName = defaultResc) == NULL || strcmp (defaultResc, "null") == 0)) {
        return (USER_NO_RESC_INPUT_ERR);
    }
    status = _getRescInfo (rsComm, rescName, rescGrpInfo);

    return (status);
}

/* _getRescInfo -  given the input string rescGroupName (which can be
 * the name of a resource or resource group), return the rescGrpInfo 
 * containing the info on this resource or resource group.
 * If it is a resource, the returned integer can be 0, SYS_RESC_IS_DOWN
 * SYS_RESC_QUOTA_EXCEEDED.
 */

int
_getRescInfo (rsComm_t *rsComm, char *rescGroupName,
rescGrpInfo_t **rescGrpInfo)
{
    int status;

    *rescGrpInfo = NULL;

    /* a resource ? */

    status = resolveAndQueResc (rescGroupName, NULL, rescGrpInfo);
    if (status >= 0 || status == SYS_RESC_IS_DOWN) {
        return (status);
    }
    /* assume it is a rescGrp */

    status = resolveRescGrp (rsComm, rescGroupName, rescGrpInfo);

    return (status);
}

/* getRescStatus - Given a resource name in inpRescName or condInput
 * key value pair, return the status of the resource. The returned
 * value can be INT_RESC_STATUS_DOWN or INT_RESC_STATUS_UP.
 */
int
getRescStatus (rsComm_t *rsComm, char *inpRescName, keyValPair_t *condInput)
{
    char *rescName;
    int status;
    rescGrpInfo_t *rescGrpInfo = NULL;

    if (inpRescName == NULL) {
        if ((rescName = getValByKey (condInput, BACKUP_RESC_NAME_KW)) == NULL &&
          (rescName = getValByKey (condInput, DEST_RESC_NAME_KW)) == NULL &&
          (rescName = getValByKey (condInput, DEF_RESC_NAME_KW)) == NULL) {
            return (INT_RESC_STATUS_DOWN);
        }
    } else {
        rescName = inpRescName;
    }
    status = _getRescInfo (rsComm, rescName, &rescGrpInfo);

    freeAllRescGrpInfo (rescGrpInfo);

    if (status < 0) {
        return INT_RESC_STATUS_DOWN;
    } else {
        return INT_RESC_STATUS_UP;
    }
}

/* resolveRescGrp - Given the rescGroupName string, returns info for
 * all resources in this resource group.
 * rescGroupName can only be a resource group.
 */

int
resolveRescGrp (rsComm_t *rsComm, char *rescGroupName,
rescGrpInfo_t **rescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    int status;

    if ((status = initRescGrp (rsComm)) < 0) return status;

    /* see if it is in cache */

    tmpRescGrpInfo = CachedRescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        if (strcmp (rescGroupName, tmpRescGrpInfo->rescGroupName)
          == 0) {
            replRescGrpInfo (tmpRescGrpInfo, rescGrpInfo);
            /* *rescGrpInfo = tmpRescGrpInfo; */
            return (0);
        }
        tmpRescGrpInfo = tmpRescGrpInfo->cacheNext;
    }

    return CAT_NO_ROWS_FOUND;
}

/* replRescGrpInfo - Replicate a rescGrpInfo_t struct. The source struct
 * is srcRescGrpInfo and the result is destRescGrpInfo.
 */

int
replRescGrpInfo (rescGrpInfo_t *srcRescGrpInfo, rescGrpInfo_t **destRescGrpInfo)
{
    rescGrpInfo_t *tmpSrcRescGrpInfo, *tmpDestRescGrpInfo,
      *lastDestRescGrpInfo;

    *destRescGrpInfo = lastDestRescGrpInfo = NULL;

    tmpSrcRescGrpInfo = srcRescGrpInfo;
    while (tmpSrcRescGrpInfo != NULL) {
        tmpDestRescGrpInfo = (rescGrpInfo_t *) malloc (sizeof (rescGrpInfo_t));
        memset (tmpDestRescGrpInfo, 0, sizeof (rescGrpInfo_t));
	tmpDestRescGrpInfo->status = tmpSrcRescGrpInfo->status;
        tmpDestRescGrpInfo->rescInfo = tmpSrcRescGrpInfo->rescInfo;
        rstrcpy (tmpDestRescGrpInfo->rescGroupName,
          tmpSrcRescGrpInfo->rescGroupName, NAME_LEN);
        if (*destRescGrpInfo == NULL) {
            *destRescGrpInfo = tmpDestRescGrpInfo;
        } else {
            lastDestRescGrpInfo->next = tmpDestRescGrpInfo;
        }
        lastDestRescGrpInfo = tmpDestRescGrpInfo;
        tmpSrcRescGrpInfo = tmpSrcRescGrpInfo->next;
    }

    return (0);
}

/* queryRescInRescGrp - Given the rescGroupName string which must be the
 * name of a resource group, query all resources in this resource group.
 * The output of the query is given in genQueryOut.
 */

int
queryRescInRescGrp (rsComm_t *rsComm, char *rescGroupName,
genQueryOut_t **genQueryOut)
{
    genQueryInp_t genQueryInp;
    char tmpStr[NAME_LEN];
    int status;

    memset (&genQueryInp, 0, sizeof (genQueryInp_t));

    snprintf (tmpStr, NAME_LEN, "='%s'", rescGroupName);
    addInxVal (&genQueryInp.sqlCondInp, COL_RESC_GROUP_NAME, tmpStr);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);

    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery (rsComm, &genQueryInp, genQueryOut);

    clearGenQueryInp (&genQueryInp);

    return (status);

}

/* resolveAndQueResc - Given the rescName string, get the resource info
 * in a rescGrpInfo_t struct and queue this struct in the rescGrpInfo
 * link list by resource type. Also copy the rescGroupName string to this 
 * rescGrpInfo_t struct.
 */
int
resolveAndQueResc (char *rescName, char *rescGroupName,
rescGrpInfo_t **rescGrpInfo)
{
    rescInfo_t *myRescInfo;
    int status;

    status = resolveResc (rescName, &myRescInfo);

    if (status < 0) {
        return (status);
    } else if (myRescInfo->rescStatus == INT_RESC_STATUS_DOWN) {
        return SYS_RESC_IS_DOWN;
    } else {
        queResc (myRescInfo, rescGroupName, rescGrpInfo, BY_TYPE_FLAG);
        return (0);
    }
}

/* resolveResc - Given the rescName string, get the info of this resource
 * in rescInfo.
 */

int
resolveResc (char *rescName, rescInfo_t **rescInfo)
{
    rescInfo_t *myRescInfo;
    rescGrpInfo_t *tmpRescGrpInfo;


    *rescInfo = NULL;

    /* search the global RescGrpInfo */

    tmpRescGrpInfo = RescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        myRescInfo = tmpRescGrpInfo->rescInfo;
        if (strcmp (rescName, myRescInfo->rescName) == 0) {
            *rescInfo = myRescInfo;
            return (0);
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    /* no match */
    rodsLog (LOG_DEBUG1,
      "resolveResc: resource %s not configured in RCAT", rescName);
    return (SYS_INVALID_RESC_INPUT);
}

#if 0	/* replaced by getRescCnt */
/* getNumResc - count the number of resources in the rescGrpInfo link list.
 */

int
getNumResc (rescGrpInfo_t *rescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    int numResc = 0;

    tmpRescGrpInfo = rescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        numResc++;
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    return (numResc);
}
#endif

/* sortResc - Sort the resources given in the rescGrpInfo link list
 * according to the sorting scheme given in sortScheme. sortScheme
 * can be "random", "byRescClass" or "byLoad". The sorted rsources
 * is also given in rescGrpInfo.
 */

int
sortResc (rsComm_t *rsComm, rescGrpInfo_t **rescGrpInfo,
char *sortScheme)
{
    int numResc;

    if (sortScheme == NULL) {
        return (0);
    }

    if (rescGrpInfo == NULL || *rescGrpInfo == NULL) {
        return (0);
    }

    numResc = getRescCnt (*rescGrpInfo);

    if (numResc <= 1) {
        return (0);
    }

    if (sortScheme == NULL) {
        return (0);
    }

    if (strcmp (sortScheme, "random") == 0) {
        sortRescRandom (rescGrpInfo);
    } else if (strcmp (sortScheme, "byRescClass") == 0) {
        sortRescByType (rescGrpInfo);
        } else if (strcmp (sortScheme, "byLoad") == 0) {
        sortRescByLoad (rsComm, rescGrpInfo);
    } else {
            rodsLog (LOG_ERROR,
              "sortResc: unknown sortScheme %s", sortScheme);
    }

    return (0);
}

/* sortRescRandom - source the resources in the rescGrpInfo link list
 * randomly.
 */

int
sortRescRandom (rescGrpInfo_t **rescGrpInfo)
{
    int *randomArray;
    int numResc;
    int i, j, status;
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t **rescInfoArray;


    tmpRescGrpInfo = *rescGrpInfo;
    numResc = getRescCnt (tmpRescGrpInfo);

    if (numResc <= 1) {
        return (0);
    }
    if ((status = getRandomArray (&randomArray, numResc)) < 0) return status;
    rescInfoArray = ( rescInfo_t** )malloc (numResc * sizeof (rescInfo_t *));
    for (i = 0; i < numResc; i++) {
        j = randomArray[i] - 1;
        rescInfoArray[j] = tmpRescGrpInfo->rescInfo;
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    tmpRescGrpInfo = *rescGrpInfo;
    for (i = 0; i < numResc; i++) {
        tmpRescGrpInfo->rescInfo = rescInfoArray[i];
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    free (rescInfoArray);
    free (randomArray);
    return 0;
}

/* sortRescByType - sort the resource in the rescGrpInfo link list by
 * resource type in the order of CACHE_CL, ARCHIVAL_CL and the rest.
 */

int
sortRescByType (rescGrpInfo_t **rescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo, *tmp1RescGrpInfo;
    rescInfo_t *tmpRescInfo, *tmp1RescInfo;

    tmpRescGrpInfo = *rescGrpInfo;

    /* float CACHE_CL to top */

    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (RescClass[tmpRescInfo->rescClassInx].classType == CACHE_CL) {
            /* find a slot to exchange rescInfo */
            tmp1RescGrpInfo = *rescGrpInfo;
            while (tmp1RescGrpInfo != NULL) {
                if (tmp1RescGrpInfo == tmpRescGrpInfo) break;
                tmp1RescInfo = tmp1RescGrpInfo->rescInfo;
                if (RescClass[tmp1RescInfo->rescClassInx].classType > CACHE_CL) {
                    tmpRescGrpInfo->rescInfo = tmp1RescInfo;
                    tmp1RescGrpInfo->rescInfo = tmpRescInfo;
                    break;
                }
                tmp1RescGrpInfo = tmp1RescGrpInfo->next;
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    /* float ARCHIVAL_CL to second */

    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (RescClass[tmpRescInfo->rescClassInx].classType == ARCHIVAL_CL) {
            /* find a slot to exchange rescInfo */
            tmp1RescGrpInfo = *rescGrpInfo;
            while (tmp1RescGrpInfo != NULL) {
                if (tmp1RescGrpInfo == tmpRescGrpInfo) break;
                tmp1RescInfo = tmp1RescGrpInfo->rescInfo;
                if (RescClass[tmp1RescInfo->rescClassInx].classType > 
		 ARCHIVAL_CL) {
                    tmpRescGrpInfo->rescInfo = tmp1RescInfo;
                    tmp1RescGrpInfo->rescInfo = tmpRescInfo;
                    break;
                }
                tmp1RescGrpInfo = tmp1RescGrpInfo->next;
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    return 0;
}

/* sortRescByLoad - sort the resource in the rescGrpInfo link list by
 * load with the lightest load first.
 */

int
sortRescByLoad (rsComm_t *rsComm, rescGrpInfo_t **rescGrpInfo)
{
    int i, j, loadmin, loadList[MAX_NSERVERS], nresc, status, 
     timeList[MAX_NSERVERS];
    char rescList[MAX_NSERVERS][MAX_NAME_LEN], *tResult;
    rescGrpInfo_t *tmpRescGrpInfo, *tmp1RescGrpInfo;
    rescInfo_t *tmpRescInfo;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    time_t timenow;

    /* query the database in order to retrieve the information on the 
     * resources' load */
    memset(&genQueryInp, 0, sizeof (genQueryInp));
    addInxIval(&genQueryInp.selectInp, COL_SLD_RESC_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_SLD_LOAD_FACTOR, 1);
    addInxIval(&genQueryInp.selectInp, COL_SLD_CREATE_TIME, SELECT_MAX);
    /* XXXXX a tmp fix to increase no. of resource to 2560 */
    genQueryInp.maxRows = MAX_SQL_ROWS * 10;
    status =  rsGenQuery(rsComm, &genQueryInp, &genQueryOut);
    if ( status == 0 && NULL != genQueryOut) { // JMC cppcheck - nullptr
        nresc = genQueryOut->rowCnt;
        for (i=0; i<genQueryOut->attriCnt; i++) {
            for (j=0; j<nresc; j++) {
                tResult = genQueryOut->sqlResult[i].value;
                tResult += j*genQueryOut->sqlResult[i].len;
                switch (i) {
                  case 0:
                    rstrcpy(rescList[j], tResult, 
		      genQueryOut->sqlResult[i].len);
                    break;
                  case 1:
                    loadList[j] = atoi(tResult);
                    break;
                  case 2:
                    timeList[j] = atoi(tResult);
                    break;
                }
            }
	}
    } else {
        return (0);
    }
    clearGenQueryInp(&genQueryInp);
    freeGenQueryOut(&genQueryOut);

    /* find the least loaded resource among the ones for which the 
     * information is available */
    tmpRescGrpInfo = *rescGrpInfo;
    tmpRescInfo = tmpRescGrpInfo->rescInfo;
    tmp1RescGrpInfo = tmpRescGrpInfo;
    loadmin = 100;
    /* retrieve local time in order to check if the load information is up 
     * to date, ie less than MAX_ELAPSE_TIME seconds old */
    (void) time(&timenow);
    while (tmpRescGrpInfo != NULL) {
        for (i=0; i<nresc; i++) {
            if (strcmp(rescList[i], tmpRescGrpInfo->rescInfo->rescName) == 0) {
                if ( loadList[i] >= 0 && loadmin > loadList[i] && 
		  (timenow - timeList[i]) < MAX_ELAPSE_TIME ) {
                    loadmin = loadList[i];
                    tmpRescInfo = tmpRescGrpInfo->rescInfo;
                    tmp1RescGrpInfo = tmpRescGrpInfo;
                }
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    /* exchange rescInfo with the head */
    tmp1RescGrpInfo->rescInfo = (*rescGrpInfo)->rescInfo;
    (*rescGrpInfo)->rescInfo = tmpRescInfo;

    return 0;
}

/* sortRescByLocation - float LOCAL_HOST resources to the top */
int
sortRescByLocation (rescGrpInfo_t **rescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo, *tmp1RescGrpInfo;
    rescInfo_t *tmpRescInfo, *tmp1RescInfo;

    tmpRescGrpInfo = *rescGrpInfo;

    /* float CACHE_CL to top */

    while (tmpRescGrpInfo != NULL) {
        int tmpClass, tmp1Class;
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (isLocalHost (tmpRescInfo->rescLoc)) {
            tmpClass = RescClass[tmpRescInfo->rescClassInx].classType;
            /* find a slot to exchange rescInfo */
            tmp1RescGrpInfo = *rescGrpInfo;
            while (tmp1RescGrpInfo != NULL) {
                if (tmp1RescGrpInfo == tmpRescGrpInfo) break;
                tmp1RescInfo = tmp1RescGrpInfo->rescInfo;
                tmp1Class = RescClass[tmp1RescInfo->rescClassInx].classType;
                if (tmp1Class > tmpClass ||
                 (tmp1Class == tmpClass &&
                  isLocalHost (tmp1RescInfo->rescLoc) == 0)) {
                    tmpRescGrpInfo->rescInfo = tmp1RescInfo;
                    tmp1RescGrpInfo->rescInfo = tmpRescInfo;
                    break;
                }
                tmp1RescGrpInfo = tmp1RescGrpInfo->next;
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    return 0;
}

/* getRescClass - Given the rescInfo, returns the classType of the resource.
 */

int
getRescClass (rescInfo_t *rescInfo)
{
    int classType;

    if (rescInfo == NULL) return USER__NULL_INPUT_ERR;

    classType = RescClass[rescInfo->rescClassInx].classType;

    return classType;
}

/* getRescGrpClass - Get the classType of the resources in a resource group
 * (given in rescGrpInfo). Return COMPOUND_CL if one of the resource is
 * this class. Otherwise return the class of the top one in the list.
 */

int
getRescGrpClass (rescGrpInfo_t *rescGrpInfo, rescInfo_t **outRescInfo)
{
    rescInfo_t *tmpRescInfo;
    rescGrpInfo_t *tmpRescGrpInfo = rescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (getRescClass (tmpRescInfo) == COMPOUND_CL) {
            *outRescInfo = tmpRescInfo;
            return COMPOUND_CL;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    *outRescInfo = NULL;
    /* just use the top */
    return (getRescClass(rescGrpInfo->rescInfo));
}

/* compareRescAddr - Given 2 rescInfo, if they have the same host address,
 * return 1. Otherwise, return 0.
 */

int
compareRescAddr (rescInfo_t *srcRescInfo, rescInfo_t *destRescInfo)
{
    rodsHostAddr_t srcAddr;
    rodsHostAddr_t destAddr;
    rodsServerHost_t *srcServerHost = NULL;
    rodsServerHost_t *destServerHost = NULL;

    bzero (&srcAddr, sizeof (srcAddr));
    bzero (&destAddr, sizeof (destAddr));

    rstrcpy (srcAddr.hostAddr, srcRescInfo->rescLoc, NAME_LEN);
    rstrcpy (destAddr.hostAddr, destRescInfo->rescLoc, NAME_LEN);

    resolveHost (&srcAddr, &srcServerHost);
    resolveHost (&destAddr, &destServerHost);

    if (srcServerHost == destServerHost)
        return 1;
    else
        return 0;
}

/* getCacheRescInGrp - get the cache resc in the resource group specified
 * by rescGroupName. If rescGroupName does not exist, find the rescGroupName
 * that include memberRescInfo. Either rescGroupName or memberRescInfo must
 * exist. If rescGroupName is zero len, the rescGroupName will be updated.
 */
int
getCacheRescInGrp (rsComm_t *rsComm, char *rescGroupName,
rescInfo_t *memberRescInfo, rescInfo_t **outCacheResc)
{
    int status;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;

    *outCacheResc = NULL;

    if (rescGroupName == NULL || strlen (rescGroupName) == 0) {

        if (memberRescInfo == NULL) {
            rodsLog (LOG_ERROR,
              "getCacheRescInGrp: no rescGroupName input");
            return SYS_NO_CACHE_RESC_IN_GRP;
        }

        /* no input rescGrp. Try to find one that matches rescInfo. */
        status = getRescGrpOfResc (rsComm, memberRescInfo, &myRescGrpInfo);
        if (status < 0 || NULL == myRescGrpInfo ) { // JMC cppcheck - nullptr
            rodsLog (LOG_NOTICE,
              "getCacheRescInGrp:getRescGrpOfResc err for %s. stat=%d",
              memberRescInfo->rescName, status);
            return status;
        } else if (rescGroupName != NULL) {
            rstrcpy (rescGroupName, myRescGrpInfo->rescGroupName, NAME_LEN);
        }
    } else {
        status = resolveRescGrp (rsComm, rescGroupName, &myRescGrpInfo);
        if (status < 0) return status;
    }
    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        rescInfo_t *tmpRescInfo;
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (RescClass[tmpRescInfo->rescClassInx].classType == CACHE_CL) {
            *outCacheResc = tmpRescInfo;
            freeAllRescGrpInfo (myRescGrpInfo);
            return 0;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return SYS_NO_CACHE_RESC_IN_GRP;
}

/* getRescInGrp - Given the rescName string and the rescGroupName string
 * get the rescInfo of the resource in the resource group. If rescName
 * is not in rescGroupName, return SYS_UNMATCHED_RESC_IN_RESC_GRP.
 */

int
getRescInGrp (rsComm_t *rsComm, char *rescName, char *rescGroupName,
rescInfo_t **outRescInfo)
{
    int status;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;

    if (rescGroupName == NULL || strlen (rescGroupName) == 0)
        return USER__NULL_INPUT_ERR;


    status = resolveRescGrp (rsComm, rescGroupName, &myRescGrpInfo);

    if (status < 0) return status;

    tmpRescGrpInfo = myRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        rescInfo_t *tmpRescInfo;
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        if (strcmp (tmpRescInfo->rescName, rescName) == 0) {
            if (outRescInfo != NULL)
                *outRescInfo = tmpRescInfo;
            freeAllRescGrpInfo (myRescGrpInfo);
            return 0;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    freeAllRescGrpInfo (myRescGrpInfo);
    return SYS_UNMATCHED_RESC_IN_RESC_GRP;
}

/* getRescGrpOfResc - Given a resource represented by rescInfo, get
 * the resource group where this resource is a member. The output is
 * the resource group link list.
 */
  
int
getRescGrpOfResc (rsComm_t *rsComm, rescInfo_t * rescInfo,
rescGrpInfo_t **rescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo, *myRescGrpInfo;
    rescGrpInfo_t *outRescGrpInfo = NULL;
    rescInfo_t *myRescInfo;
    int status;

    *rescGrpInfo = NULL;

    if ((status = initRescGrp (rsComm)) < 0) return status;

    /* in cache ? */
    tmpRescGrpInfo = CachedRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        myRescGrpInfo = tmpRescGrpInfo;
        while (myRescGrpInfo != NULL) {
            myRescInfo = myRescGrpInfo->rescInfo;
            if (strcmp (rescInfo->rescName, myRescInfo->rescName) == 0) {
                replRescGrpInfo (tmpRescGrpInfo, &outRescGrpInfo);
                outRescGrpInfo->cacheNext = *rescGrpInfo;
                *rescGrpInfo = outRescGrpInfo;
                break;
            }
            myRescGrpInfo = myRescGrpInfo->next;
        }

        tmpRescGrpInfo = tmpRescGrpInfo->cacheNext;
    }

    if (*rescGrpInfo == NULL)
        return SYS_NO_CACHE_RESC_IN_GRP;
    else
        return 0;
}

/* isRescsInSameGrp - check if 2 rescources are in the same rescGroup
 * returns 1 if match, 0 for no match.
 */

int
isRescsInSameGrp (rsComm_t *rsComm, char *rescName1, char *rescName2,
rescGrpInfo_t **outRescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo, *myRescGrpInfo;
    rescInfo_t *myRescInfo;
    int status;
    int match1, match2;

    if (outRescGrpInfo != NULL)
        *outRescGrpInfo = NULL;

    if ((status = initRescGrp (rsComm)) < 0) return status;

    /* in cache ? */
    tmpRescGrpInfo = CachedRescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        myRescGrpInfo = tmpRescGrpInfo;
        match1 = match2 = 0;
        while (myRescGrpInfo != NULL) {
            myRescInfo = myRescGrpInfo->rescInfo;
            if (match1 == 0 &&
              strcmp (rescName1, myRescInfo->rescName) == 0) {
                match1 = 1;
            } else if (match2 == 0 &&
              strcmp (rescName2, myRescInfo->rescName) == 0) {
                match2 = 1;
            }
            if (match1 == 1 && match2 == 1) {
                if (outRescGrpInfo != NULL) *outRescGrpInfo = tmpRescGrpInfo;
                return 1;
            }
            myRescGrpInfo = myRescGrpInfo->next;
        }

        tmpRescGrpInfo = tmpRescGrpInfo->cacheNext;
    }
    return 0;
}

/* initRescGrp - Initialize the CachedRescGrpInfo queue. Query all resource
 * groups and queue them it the CachedRescGrpInfo link list. Resources in
 * each resource group are queued by class.
 */

int
initRescGrp (rsComm_t *rsComm)
{
    genQueryInp_t genQueryInp;
    rescGrpInfo_t *tmpRescGrpInfo;
    genQueryOut_t *genQueryOut = NULL;
    sqlResult_t *rescName, *rescGrpName;
    char *rescNameStr, *rescGrpNameStr;
    char *curRescGrpNameStr = NULL;
    int i;
    int status = 0;
    int savedRescGrpStatus = 0;

    if (RescGrpInit > 0) return 0;

    RescGrpInit = 1;

    /* query all resource groups */
    memset (&genQueryInp, 0, sizeof (genQueryInp_t));

    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_RESC_GROUP_NAME, ORDER_BY);

    /* increased to 2560 */
    genQueryInp.maxRows = MAX_SQL_ROWS * 10;

    status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

    clearGenQueryInp (&genQueryInp);

    if (status < 0) {
        if (status == CAT_NO_ROWS_FOUND)
            return 0;
        else
            return status;
    }

    if ((rescName = getSqlResultByInx (genQueryOut, COL_R_RESC_NAME)) ==
      NULL) {
        rodsLog (LOG_NOTICE,
          "initRescGrp: getSqlResultByInx for COL_R_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescGrpName = getSqlResultByInx (genQueryOut, COL_RESC_GROUP_NAME)) ==
      NULL) {
        rodsLog (LOG_NOTICE,
          "initRescGrp: getSqlResultByInx for COL_RESC_GROUP_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    tmpRescGrpInfo = NULL;
    for (i = 0;i < genQueryOut->rowCnt; i++) {
        rescNameStr = &rescName->value[rescName->len * i];
        rescGrpNameStr = &rescGrpName->value[rescGrpName->len * i];
        if (tmpRescGrpInfo != NULL && curRescGrpNameStr != NULL) {
            if (strcmp (rescGrpNameStr, curRescGrpNameStr) != 0) {
		rescGrpInfo_t *myRescGrpInfo;
                /* a new rescGrp. queue the current one */
                tmpRescGrpInfo->cacheNext = CachedRescGrpInfo;
		if (savedRescGrpStatus != 0) {
		    myRescGrpInfo = tmpRescGrpInfo;
		    while (myRescGrpInfo != NULL) {
                        myRescGrpInfo->status = savedRescGrpStatus;
			myRescGrpInfo = myRescGrpInfo->next;
		    }
		}
                savedRescGrpStatus = 0;
                CachedRescGrpInfo = tmpRescGrpInfo;
                tmpRescGrpInfo = NULL;
            }
        }
        curRescGrpNameStr = rescGrpNameStr;
        status = resolveAndQueResc (rescNameStr, rescGrpNameStr,
          &tmpRescGrpInfo);
        if (status < 0) {
            if (status == SYS_RESC_IS_DOWN) {
                savedRescGrpStatus = SYS_RESC_IS_DOWN;
            } else {
                rodsLog (LOG_NOTICE,
                  "initRescGrp: resolveAndQueResc error for %s. status = %d",
                  rescNameStr, status);
                freeGenQueryOut (&genQueryOut);
                return (status);
            }
        }
    }
    /* query the remaining in cache */
    if (tmpRescGrpInfo != NULL) {
        tmpRescGrpInfo->status = savedRescGrpStatus;
        tmpRescGrpInfo->cacheNext = CachedRescGrpInfo;
        CachedRescGrpInfo = tmpRescGrpInfo;
    }
    if (genQueryOut != NULL &&  genQueryOut->continueInx > 0) {
        rodsLog (LOG_NOTICE,
          "initRescGrp: number of resources in resc groups exceed 2560");
        freeGenQueryOut (&genQueryOut);
	return SYS_INVALID_RESC_INPUT;
    } else {
        freeGenQueryOut (&genQueryOut);
        return 0;
    }
}

/* setDefaultResc - set the default resource and put the result in the
 * outRescGrpInfo link list. This is normally called by the msiSetDefaultResc
 * micro-service. The defaultRescList and optionStr are input 
 * parameters of msiSetDefaultResc. 
 * If optionStr is "force" and this user is not LOCAL_PRIV_USER_AUTH, 
 * only a resource in the defaultRescList will be used. 
 * If optionStr is "preferred", the resource group is taken from condInput
 * if it exists and the preferred resource in this resource group is given 
 * in the defaultRescList.
 * Otherwise, use the resource given in condInput, then in defaultRescList.
 * 
 */

int
setDefaultResc (rsComm_t *rsComm, char *defaultRescList, char *optionStr,
keyValPair_t *condInput, rescGrpInfo_t **outRescGrpInfo)
{
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *defRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo, *prevRescGrpInfo;
    char *value = NULL;
    strArray_t strArray;
    int i, status;
    char *defaultResc;
    int startInx;

    if (defaultRescList != NULL && strcmp (defaultRescList, "null") != 0 &&
      optionStr != NULL &&  strcmp (optionStr, "force") == 0 &&
      rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH) {
        condInput = NULL;
    }

    memset (&strArray, 0, sizeof (strArray));

    status = parseMultiStr (defaultRescList, &strArray);

#if 0   /* this will produce no output */
    if (status <= 0)
        return (0);
#endif

    value = strArray.value;
    if (strArray.len <= 1) {
        startInx = 0;
        defaultResc = value;
    } else {
        /* select one on the list randomly */
        startInx = random() % strArray.len;
        defaultResc = &value[startInx * strArray.size];
    }
    status = getRescInfoAndStatus (rsComm, defaultResc, NULL, &defRescGrpInfo);
    if (status < 0) {
        /* the random defaultResc is not good. pick other */
        if (strArray.len <= 1) {
            defaultResc = NULL;
        } else {
            for (i = 0; i < strArray.len; i++) {
                defaultResc =  &value[i * strArray.size];
                status = getRescInfoAndStatus (rsComm, defaultResc, NULL,
                &defRescGrpInfo);
                if (status >= 0) break;
            }
            if (status < 0) defaultResc = NULL;
        }
    }

    if (strcmp (optionStr, "preferred") == 0) {
        /* checkinput first, then default */
        status = getRescInfoAndStatus (rsComm, NULL, condInput,
          &myRescGrpInfo);
        if (status >= 0) {
            freeAllRescGrpInfo (defRescGrpInfo);
            if (strlen (myRescGrpInfo->rescGroupName) > 0) {
                for (i = 0; i < strArray.len; i++) {
                    int j;
                    j = startInx + i;
                    if (j >= strArray.len) {
                        /* wrap around */
                        j = strArray.len - j;
                    }
                    tmpRescGrpInfo = myRescGrpInfo;
                    prevRescGrpInfo = NULL;
                    while (tmpRescGrpInfo != NULL) {
                        rescInfo_t *myResc = tmpRescGrpInfo->rescInfo;
                        if (strcmp (&value[j * strArray.size],
                          myResc->rescName) == 0 &&
                            myResc->rescStatus != INT_RESC_STATUS_DOWN) {
                            /* put it on top */
                            if (prevRescGrpInfo != NULL) {
                                prevRescGrpInfo->next = tmpRescGrpInfo->next;
                                tmpRescGrpInfo->next = myRescGrpInfo;
                                myRescGrpInfo = tmpRescGrpInfo;
                            }
                            break;
                        }
                        prevRescGrpInfo = tmpRescGrpInfo;
                        tmpRescGrpInfo = tmpRescGrpInfo->next;
                    }
                }
            }
        } else {
            /* error may mean there is no input resource. try to use the
             * default resource by dropping down */
            if (defRescGrpInfo != NULL) {
                myRescGrpInfo = defRescGrpInfo;
                status = 0;
            }
        }
    } else if (strcmp (optionStr, "forced") == 0) {
        if (defRescGrpInfo != NULL) {
            myRescGrpInfo = defRescGrpInfo;
        }
    } else {
        /* input first. If not good, use def */
        status = getRescInfo (rsComm, NULL, condInput,
          &myRescGrpInfo);
        if (status < 0) {
            if (status == USER_NO_RESC_INPUT_ERR && defRescGrpInfo != NULL) {
                /* user have not input a resource. Use default */
                myRescGrpInfo = defRescGrpInfo;
                status = 0;
            } else {
                freeAllRescGrpInfo (defRescGrpInfo);
            }
        } else {
            freeAllRescGrpInfo (defRescGrpInfo);
        }
    }

    if (status == CAT_NO_ROWS_FOUND)
      status = SYS_RESC_DOES_NOT_EXIST;


    if (value != NULL)
        free (value);

    if (status >= 0) {
        *outRescGrpInfo = myRescGrpInfo;
    } else {
        *outRescGrpInfo = NULL;
    }

    return (status);
}

/* getRescInfoAndStatus - Given a resource or rescGroup name, see if the
 * resource is up or down.  
 * The resource given in condInput is taken first, then rescName.
 * If the resource is a resource group, a INT_RESC_STATUS_UP will be
 * returned if any one of the resource in the group is up.
 * If the resource is up, the resource info will be put in the rescGrpInfo
 * link list. 
 */
int
getRescInfoAndStatus (rsComm_t *rsComm, char *rescName, keyValPair_t *condInput,
rescGrpInfo_t **rescGrpInfo)
{
    int status;
    rescGrpInfo_t *myRescGrpInfo = NULL;
    rescGrpInfo_t *tmpRescGrpInfo;

    status = getRescInfo (rsComm, rescName, condInput, &myRescGrpInfo);
    if (status >= 0 && *myRescGrpInfo->rescGroupName != '\0') {
        /* make sure at least one resc is not down */
        tmpRescGrpInfo = myRescGrpInfo;
        while (tmpRescGrpInfo != NULL) {
            if (tmpRescGrpInfo->rescInfo->rescStatus != INT_RESC_STATUS_DOWN)
                break;
            tmpRescGrpInfo = tmpRescGrpInfo->next;
        }
        if (tmpRescGrpInfo == NULL) {
            status = SYS_RESC_IS_DOWN;
            if (myRescGrpInfo != NULL) freeAllRescGrpInfo (myRescGrpInfo);
        }
    }
    if (status >= 0) {
        *rescGrpInfo = myRescGrpInfo;
    } else {
        *rescGrpInfo = NULL;
    }
    return status;
}

/* initResc - Initialize the global resource link list RescGrpInfo which
 * contains all resources known to the system.
 */

int
initResc (rsComm_t *rsComm)
{
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status;
    int continueInx;

    memset (&genQueryInp, 0, sizeof (genQueryInp));

    addInxIval (&genQueryInp.selectInp, COL_R_RESC_ID, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_ZONE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_TYPE_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_CLASS_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_LOC, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_VAULT_PATH, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_FREE_SPACE, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_INFO, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_COMMENT, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_CREATE_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_MODIFY_TIME, 1);
    addInxIval (&genQueryInp.selectInp, COL_R_RESC_STATUS, 1);

    genQueryInp.maxRows = MAX_SQL_ROWS;

    if (RescGrpInfo != NULL) {
        /* we are updating RescGrpInfo */
        freeAllRescGrp (RescGrpInfo);
        RescGrpInfo = NULL;
    }

    continueInx = 1;	/* a fake one so it will do the first query */
    while (continueInx > 0) {
        status =  rsGenQuery (rsComm, &genQueryInp, &genQueryOut);

        if (status < 0) {
            if (status !=CAT_NO_ROWS_FOUND) {
                rodsLog (LOG_NOTICE,
                  "initResc: rsGenQuery error, status = %d",
                  status);
            }
            clearGenQueryInp (&genQueryInp);
            return (status);
        }

        status = procAndQueRescResult (genQueryOut);

        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "initResc: rsGenQuery error, status = %d", status);
            freeGenQueryOut (&genQueryOut);
	    break;
        } else {
	    if (genQueryOut != NULL) {
		continueInx = genQueryInp.continueInx = 
		genQueryOut->continueInx;
                freeGenQueryOut (&genQueryOut);
	    } else {
		continueInx = 0;
	    }
	}
    }
    clearGenQueryInp (&genQueryInp);
    return (status);
}

/* procAndQueRescResult - Process the query results from initResc ().
 * Queue the results in the global resource link list RescGrpInfo.
 */

int
procAndQueRescResult (genQueryOut_t *genQueryOut)
{
    sqlResult_t *rescId, *rescName, *zoneName, *rescType, *rescClass;
    sqlResult_t *rescLoc, *rescVaultPath, *freeSpace, *rescInfo;
    sqlResult_t *rescComments, *rescCreate, *rescModify, *rescStatus;
    char *tmpRescId, *tmpRescName, *tmpZoneName, *tmpRescType, *tmpRescClass;
    char *tmpRescLoc, *tmpRescVaultPath, *tmpFreeSpace, *tmpRescInfo;
    char *tmpRescComments, *tmpRescCreate, *tmpRescModify, *tmpRescStatus;
    int i, status;
    rodsHostAddr_t addr;
    rodsServerHost_t *tmpRodsServerHost;
    rescInfo_t *myRescInfo;

    if (genQueryOut == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: NULL genQueryOut");
        return (0);
    }

    if ((rescId = getSqlResultByInx (genQueryOut, COL_R_RESC_ID)) == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_RESC_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescName = getSqlResultByInx(genQueryOut, COL_R_RESC_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((zoneName = getSqlResultByInx(genQueryOut, COL_R_ZONE_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_ZONE_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescType = getSqlResultByInx(genQueryOut, COL_R_TYPE_NAME)) == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_TYPE_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescClass = getSqlResultByInx(genQueryOut,COL_R_CLASS_NAME))==NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult: getSqlResultByInx for COL_R_CLASS_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescLoc = getSqlResultByInx (genQueryOut, COL_R_LOC)) == NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_LOC failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescVaultPath = getSqlResultByInx (genQueryOut, COL_R_VAULT_PATH))
      == NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult: getSqlResultByInx for COL_R_VAULT_PATH failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((freeSpace = getSqlResultByInx (genQueryOut, COL_R_FREE_SPACE)) ==
      NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult: getSqlResultByInx for COL_R_FREE_SPACE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescInfo = getSqlResultByInx (genQueryOut, COL_R_RESC_INFO)) ==
      NULL) {
        rodsLog (LOG_NOTICE,
          "procAndQueResResult: getSqlResultByInx for COL_R_RESC_INFO failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescComments = getSqlResultByInx (genQueryOut, COL_R_RESC_COMMENT))
      == NULL) {
        rodsLog (LOG_NOTICE,
        "procAndQueResResult:getSqlResultByInx for COL_R_RESC_COMMENT failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescCreate = getSqlResultByInx (genQueryOut, COL_R_CREATE_TIME))
      == NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult:getSqlResultByInx for COL_R_CREATE_TIME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((rescModify = getSqlResultByInx (genQueryOut, COL_R_MODIFY_TIME))
      == NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult:getSqlResultByInx for COL_R_MODIFY_TIME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescStatus = getSqlResultByInx (genQueryOut, COL_R_RESC_STATUS))
      == NULL) {
        rodsLog (LOG_NOTICE,
         "procAndQueResResult:getSqlResultByInx for COL_R_RESC_STATUS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

#if 0	/* do multiple continueInx */
    if (RescGrpInfo != NULL) {
        /* we are updating RescGrpInfo */
        freeAllRescGrp (RescGrpInfo);
        RescGrpInfo = NULL;
    }
#endif
    for (i = 0;i < genQueryOut->rowCnt; i++) {
        tmpRescId = &rescId->value[rescId->len * i];
        tmpRescName = &rescName->value[rescName->len * i];
        tmpZoneName = &zoneName->value[zoneName->len * i];
        tmpRescType = &rescType->value[rescType->len * i];
        tmpRescClass = &rescClass->value[rescClass->len * i];
        tmpRescLoc = &rescLoc->value[rescLoc->len * i];
        tmpRescVaultPath = &rescVaultPath->value[rescVaultPath->len * i];
        tmpFreeSpace = &freeSpace->value[freeSpace->len * i];
        tmpRescInfo = &rescInfo->value[rescInfo->len * i];
        tmpRescComments = &rescComments->value[rescComments->len * i];
        tmpRescCreate = &rescCreate->value[rescCreate->len * i];
        tmpRescModify = &rescModify->value[rescModify->len * i];
        tmpRescStatus = &rescStatus->value[rescStatus->len * i];

        /* queue the host. XXXXX resolveHost does not deal with zone yet.
         * need to do so */
        memset (&addr, 0, sizeof (addr));
        rstrcpy (addr.hostAddr, tmpRescLoc, LONG_NAME_LEN);
        rstrcpy (addr.zoneName, tmpZoneName, NAME_LEN);
        status = resolveHost (&addr, &tmpRodsServerHost);
        if (status < 0) {
            rodsLog (LOG_NOTICE,
              "procAndQueRescResult: resolveHost error for %s",
              addr.hostAddr);
        }
        /* queue the resource */
        myRescInfo = ( rescInfo_t* )malloc (sizeof (rescInfo_t));
        memset (myRescInfo, 0, sizeof (rescInfo_t));
        myRescInfo->rodsServerHost = tmpRodsServerHost;
        myRescInfo->rescId = strtoll (tmpRescId, 0, 0);
        rstrcpy (myRescInfo->zoneName, tmpZoneName, NAME_LEN);
        rstrcpy (myRescInfo->rescName, tmpRescName, NAME_LEN);
        rstrcpy (myRescInfo->rescLoc, tmpRescLoc, NAME_LEN);
        rstrcpy (myRescInfo->rescType, tmpRescType, NAME_LEN);
        /* convert rescType to rescTypeInx */
        myRescInfo->rescTypeInx = getRescTypeInx (tmpRescType);
        if (myRescInfo->rescTypeInx < 0) {
            rodsLog (LOG_ERROR,
             "procAndQueResResult: Unknown rescType %s. Resource %s will not be configured",
              tmpRescType, myRescInfo->rescName);
            continue;
        }
        rstrcpy (myRescInfo->rescClass, tmpRescClass, NAME_LEN);
        myRescInfo->rescClassInx = getRescClassInx (tmpRescClass);
        if (myRescInfo->rescClassInx < 0) {
            rodsLog (LOG_ERROR,
             "procAndQueResResult: Unknown rescClass %s. Resource %s will not be configured",
              tmpRescClass, myRescInfo->rescName);
            continue;
        }
        rstrcpy (myRescInfo->rescVaultPath, tmpRescVaultPath, MAX_NAME_LEN);
        if (RescTypeDef[myRescInfo->rescTypeInx].driverType == WOS_FILE_TYPE &&
          tmpRodsServerHost->localFlag == LOCAL_HOST) {
	    /* For WOS_FILE_TYPE, the vault path is wosHost/wosPolicy */
	    char wosHost[MAX_NAME_LEN], wosPolicy[MAX_NAME_LEN];
	    char *tmpStr;
            if (splitPathByKey (tmpRescVaultPath, wosHost, wosPolicy, '/') < 0)
            {
		rodsLog (LOG_ERROR,
                  "procAndQueResResult:splitPathByKey of wosHost error for %s",
		  wosHost, wosPolicy);
	    } else {
		        tmpStr = (char*)malloc (strlen (wosHost) + 40);
                snprintf (tmpStr, MAX_NAME_LEN, "%s=%s", WOS_HOST_ENV, wosHost);
                putenv (tmpStr);
				free( tmpStr ); // JMC cppcheck - leak
		        tmpStr = (char*)malloc (strlen (wosPolicy) + 40);
                snprintf (tmpStr, MAX_NAME_LEN, "%s=%s",  WOS_POLICY_ENV, wosPolicy);
                putenv (tmpStr);
				free( tmpStr ); // JMC cppcheck - leak
		if (ProcessType == SERVER_PT) {
		    rodsLog (LOG_NOTICE,
		     "Set WOS env wosHost=%s, wosPolicy=%s", 
		      wosHost, wosPolicy);
		}
	    }
        }
        rstrcpy (myRescInfo->rescInfo, tmpRescInfo, LONG_NAME_LEN);
        rstrcpy (myRescInfo->rescComments, tmpRescComments, LONG_NAME_LEN);
        myRescInfo->freeSpace = strtoll (tmpFreeSpace, 0, 0);
        rstrcpy (myRescInfo->rescCreate, tmpRescCreate, TIME_LEN);
        rstrcpy (myRescInfo->rescModify, tmpRescModify, TIME_LEN);
        if (strstr (tmpRescStatus, RESC_DOWN) != NULL) {
            myRescInfo->rescStatus = INT_RESC_STATUS_DOWN;
        } else {
            myRescInfo->rescStatus = INT_RESC_STATUS_UP;
        }
        myRescInfo->quotaLimit = RESC_QUOTA_UNINIT;     /* not initialized */
        queResc (myRescInfo, NULL, &RescGrpInfo, BOTTOM_FLAG);
    }
    return (0);
}

/* getHostStatusByRescInfo - get the status host based on rescStatus */
int
getHostStatusByRescInfo (rodsServerHost_t *rodsServerHost)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    rescInfo_t *myRescInfo;
    int match = 0;

    tmpRescGrpInfo = RescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
        myRescInfo = tmpRescGrpInfo->rescInfo;
        if (myRescInfo->rodsServerHost == rodsServerHost) {
            if (myRescInfo->rescStatus == INT_RESC_STATUS_UP) {
                return INT_RESC_STATUS_UP;
            }
            match = 1;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    if (match == 0) {
        /* no match in resc. assume up */
        return INT_RESC_STATUS_UP;
    } else {
        return INT_RESC_STATUS_DOWN;
    }
}

/* printLocalResc - Print the global resource link list RescGrpInfo which
 * contains all resources known to the system. This routine is normally 
 * used by the irodsServer for logging.
 */

int
printLocalResc ()
{
    rescInfo_t *myRescInfo;
    rescGrpInfo_t *tmpRescGrpInfo;
    rodsServerHost_t *tmpRodsServerHost;
    int localRescCnt = 0;

#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"Local Resource configuration: \n");
#else /* IRODS_SYSLOG */
    fprintf (stderr, "Local Resource configuration: \n");
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_NOTICE,"Local Resource configuration: \n");
#endif

    /* search the global RescGrpInfo */

    tmpRescGrpInfo = RescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        myRescInfo = tmpRescGrpInfo->rescInfo;
        tmpRodsServerHost = (rodsServerHost_t*)myRescInfo->rodsServerHost;
        if (tmpRodsServerHost->localFlag == LOCAL_HOST) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"   RescName: %s, VaultPath: %s\n",
              myRescInfo->rescName, myRescInfo->rescVaultPath);
#else /* IRODS_SYSLOG */
            fprintf (stderr, "   RescName: %s, VaultPath: %s\n",
              myRescInfo->rescName, myRescInfo->rescVaultPath);
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_NOTICE,"   RescName: %s, VaultPath: %s\n",
              myRescInfo->rescName, myRescInfo->rescVaultPath);
#endif
            localRescCnt ++;
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"\n");
#else /* IRODS_SYSLOG */
    fprintf (stderr, "\n");
#endif /* IRODS_SYSLOG */
#else
    rodsLog (LOG_NOTICE,"\n");
#endif

    if (localRescCnt == 0) {
#ifndef windows_platform
#ifdef IRODS_SYSLOG
        rodsLog (LOG_NOTICE,"   No Local Resource Configured\n");
#else /* IRODS_SYSLOG */
        fprintf (stderr, "   No Local Resource Configured\n");
#endif /* IRODS_SYSLOG */
#else
        rodsLog (LOG_NOTICE,"   No Local Resource Configured\n");
#endif
    }

    return (0);
}

/* queResc - Queue a resource given in myRescInfo to the rescGrpInfoHead
 * link list. If rescGroupName is not NULL, copy the string to 
 * myRescGrpInfo->rescGroupName. If topFlag == TOP_FLAG, queue it
 * to the top of the link list. Otherwise queue it to the bottom.
 */

int
queResc (rescInfo_t *myRescInfo, char *rescGroupName,
rescGrpInfo_t **rescGrpInfoHead, int topFlag)
{
    rescGrpInfo_t *myRescGrpInfo;
    int status;

    if (myRescInfo == NULL)
        return (0);

    myRescGrpInfo = (rescGrpInfo_t *) malloc (sizeof (rescGrpInfo_t));
    memset (myRescGrpInfo, 0, sizeof (rescGrpInfo_t));

    myRescGrpInfo->rescInfo = myRescInfo;

    if (rescGroupName != NULL) {
        rstrcpy (myRescGrpInfo->rescGroupName, rescGroupName, NAME_LEN);
    }

    status = queRescGrp (rescGrpInfoHead, myRescGrpInfo, topFlag);

    return (status);

}

/* queRescGrp - Queue a resource given in myRescGrpInfo to the 
 * rescGrpInfoHead link list.  If flag == TOP_FLAG, queue it
 * to the top of the link list. Otherwise queue it to the bottom.
 */

int
queRescGrp (rescGrpInfo_t **rescGrpInfoHead, rescGrpInfo_t *myRescGrpInfo,
int flag)
{
    rescInfo_t *tmpRescInfo, *myRescInfo;
    rescGrpInfo_t *tmpRescGrpInfo, *lastRescGrpInfo = NULL;

    myRescInfo = myRescGrpInfo->rescInfo;
    tmpRescGrpInfo = *rescGrpInfoHead;
    if (flag == TOP_FLAG) {
        *rescGrpInfoHead = myRescGrpInfo;
        myRescGrpInfo->next = tmpRescGrpInfo;
    } else {
        while (tmpRescGrpInfo != NULL) {
            tmpRescInfo = tmpRescGrpInfo->rescInfo;
            if (flag == BY_TYPE_FLAG && myRescInfo != NULL &&
              tmpRescInfo != NULL) {
                if (RescClass[myRescInfo->rescClassInx].classType <
                  RescClass[tmpRescInfo->rescClassInx].classType) {
                    break;
                }
            }
            lastRescGrpInfo = tmpRescGrpInfo;
            tmpRescGrpInfo = tmpRescGrpInfo->next;
        }

        if (lastRescGrpInfo == NULL) {
            *rescGrpInfoHead = myRescGrpInfo;
        } else {
            lastRescGrpInfo->next = myRescGrpInfo;
        }
        myRescGrpInfo->next = tmpRescGrpInfo;
    }

    return (0);
}

/* freeAllRescGrp - Free the rescGrpInfo_t link list given in rescGrpHead
 */

int
freeAllRescGrp (rescGrpInfo_t *rescGrpHead)
{
    rescGrpInfo_t *tmpRrescGrp, *nextRrescGrp;

    tmpRrescGrp = rescGrpHead;
    while (tmpRrescGrp != NULL) {
        nextRrescGrp = tmpRrescGrp->next;
        if (tmpRrescGrp->rescInfo != NULL) free (tmpRrescGrp->rescInfo);
        free (tmpRrescGrp);
        tmpRrescGrp = tmpRrescGrp->next;
    }
    return 0;
}

/* getRescType - Return a fileDriverType_t in the rescInfo. The return value
 * can be UNIX_FILE_TYPE, HPSS_FILE_TYPE, NT_FILE_TYPE ----
 */

int
getRescType (rescInfo_t *rescInfo)
{
    int rescTypeInx;

    if (rescInfo == NULL) return USER__NULL_INPUT_ERR;
    rescTypeInx = rescInfo->rescTypeInx;
    if (rescTypeInx >= NumRescTypeDef) return RESCTYPEINX_EMPTY_IN_STRUCT_ERR;
    return (RescTypeDef[rescTypeInx].driverType);
}

/* getRescTypeInx - Given a resource type string token (e.g., 
 * output of "iadmin lt resc_type" - "unix file system", hpss ...),
 * get the index number into the RescTypeDef array.
 */

int
getRescTypeInx (char *rescType)
{
    int i;

    if (rescType == NULL) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    for (i = 0; i < NumRescTypeDef; i++) {
        if (strstr (rescType, RescTypeDef[i].typeName) != NULL) {
            return (i);
        }
    }
    rodsLog (LOG_NOTICE,
      "getRescTypeInx: No match for input rescType %s", rescType);

    return (UNMATCHED_KEY_OR_INDEX);
}

/* getRescClassInx - Given a resource class string token (e.g.,
 * output of "iadmin lt resc_class" - cache, archive, compound ...),
 * get the index number into the RescClass array.
 */

int
getRescClassInx (char *rescClass)
{
    int i;

    if (rescClass == NULL) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    for (i = 0; i < NumRescClass; i++) {
        if (strstr (rescClass, RescClass[i].className) != NULL) {
	    /* "primary" is currently not used */
            if (strstr (rescClass, "primary") != NULL) {
                return (i | PRIMARY_FLAG);
            } else {
                return (i);
            }
        }
    }
    rodsLog (LOG_NOTICE,
      "getRescClassInx: No match for input rescClass %s", rescClass);

    return (UNMATCHED_KEY_OR_INDEX);
}

/* getMultiCopyPerResc - call the acSetMultiReplPerResc rule to see if 
 * multiple copies can exist in a resource. If the rule allows multi copy,
 * return 1, otherwise retun 0.
 */

int
getMultiCopyPerResc ()
{
    ruleExecInfo_t rei;

    memset (&rei, 0, sizeof (rei));
    applyRule ("acSetMultiReplPerResc", NULL, &rei, NO_SAVE_REI);
    if (strcmp (rei.statusStr, MULTI_COPIES_PER_RESC) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* getRescCnt - count the number of resources in the rescGrpInfo link list.
 */

int
getRescCnt (rescGrpInfo_t *myRescGrpInfo)
{
    rescGrpInfo_t *tmpRescGrpInfo;
    int rescCnt = 0;

    tmpRescGrpInfo = myRescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        rescCnt ++;
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }

    return (rescCnt);
}

int
updateResc (rsComm_t *rsComm)
{
    int status;

    rescGrpInfo_t *tmpRescGrpInfo, *nextRescGrpInfo;

    /* free CachedRescGrpInfo */
    freeAllRescGrpInfo (CachedRescGrpInfo);
    CachedRescGrpInfo = NULL;
    RescGrpInit = 0;

    /* free the configured rescInfo */
    tmpRescGrpInfo = RescGrpInfo;
    while (tmpRescGrpInfo != NULL) {
	nextRescGrpInfo = tmpRescGrpInfo->next;
	free (tmpRescGrpInfo->rescInfo);
        free (tmpRescGrpInfo);
        tmpRescGrpInfo = nextRescGrpInfo;
    }
    RescGrpInfo = NULL;
    status = initResc (rsComm);

    return status;
}

/* matchSameHostRescByType - find a resource on the same host
 * as the input myRescInfo but with the input driverType.
 */

rescInfo_t *
matchSameHostRescByType (rescInfo_t *myRescInfo, int driverType)
{
    rescInfo_t *tmpRescInfo;
    rescGrpInfo_t *tmpRescGrpInfo;

    tmpRescGrpInfo = RescGrpInfo;

    while (tmpRescGrpInfo != NULL) {
        tmpRescInfo = tmpRescGrpInfo->rescInfo;
        /* same location ? */
        if (strcmp (myRescInfo->rescLoc, tmpRescInfo->rescLoc) == 0) {
            /* match driverType ? */
            int rescTypeInx = tmpRescInfo->rescTypeInx;
            if (RescTypeDef[rescTypeInx].driverType == driverType) {
                return tmpRescInfo;
            }
        }
        tmpRescGrpInfo = tmpRescGrpInfo->next;
    }
    return NULL;
}

