#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "lsUtil.h"
#include "miscUtil.h"

int
lsUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    genQueryInp_t genQueryInp;


    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForLs (myRodsEnv, myRodsArgs, &genQueryInp);

    for (i = 0; i < rodsPathInp->numSrc; i++) {
	if (rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T || 
	  rodsPathInp->srcPath[i].objState == UNKNOWN_ST) {
	    status = getRodsObjType (conn, &rodsPathInp->srcPath[i]);
	    if (rodsPathInp->srcPath[i].objState == NOT_EXIST_ST) {
                if (status == NOT_EXIST_ST) {
                    rodsLog (LOG_ERROR,
                      "lsUtil: srcPath %s does not exist or user lacks access permission",
                      rodsPathInp->srcPath[i].outPath);
		}
		savedStatus = USER_INPUT_PATH_ERR;
		continue;
	    }
	}

	if (rodsPathInp->srcPath[i].objType == DATA_OBJ_T) {
	    status = lsDataObjUtil (conn, &rodsPathInp->srcPath[i], 
	     myRodsEnv, myRodsArgs, &genQueryInp);
	} else if (rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T) {
	    status = lsCollUtil (conn, &rodsPathInp->srcPath[i],
              myRodsEnv, myRodsArgs);
	} else {
	    /* should not be here */
	    rodsLog (LOG_ERROR,
	     "lsUtil: invalid ls objType %d for %s", 
	     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath);
	    return (USER_INPUT_PATH_ERR);
	}
	/* XXXX may need to return a global status */
	if (status < 0 && status != CAT_NO_ROWS_FOUND && 
	  status != SYS_SPEC_COLL_OBJ_NOT_EXIST) {
	    rodsLogError (LOG_ERROR, status,
             "lsUtil: ls error for %s, status = %d", 
	      rodsPathInp->srcPath[i].outPath, status);
	    savedStatus = status;
	} 
    }
    if (savedStatus < 0) {
        return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND || 
      status == SYS_SPEC_COLL_OBJ_NOT_EXIST) {
        return (0);
    } else {
        return (status);
    }
}

int
lsDataObjUtil (rcComm_t *conn, rodsPath_t *srcPath, 
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
genQueryInp_t *genQueryInp)
{
    int status = 0;
 

    if (rodsArgs->longOption == True) {
	if (srcPath->rodsObjStat != NULL &&
	  srcPath->rodsObjStat->specColl != NULL) {
	    if (srcPath->rodsObjStat->specColl->collClass == LINKED_COLL) {
                lsDataObjUtilLong (conn, 
		  srcPath->rodsObjStat->specColl->objPath, myRodsEnv, rodsArgs,
                  genQueryInp);
	    } else {
	        lsSpecDataObjUtilLong (conn, srcPath,
	          myRodsEnv, rodsArgs);
	    }
	} else {
            lsDataObjUtilLong (conn, srcPath->outPath, myRodsEnv, rodsArgs, 
	      genQueryInp);
	}
    } else {
	printLsStrShort (srcPath->outPath);
        if (rodsArgs->accessControl == True) {
            printDataAcl (conn, srcPath->dataId);
        }
    }

    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "lsDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    return (status);
}

int
printLsStrShort (char *srcPath)
{
    char srcElement[MAX_NAME_LEN];

    getLastPathElement (srcPath, srcElement);

    printf ("  %s\n", srcPath);

    return 0;
}

int
lsDataObjUtilLong (rcComm_t *conn, char *srcPath, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs, genQueryInp_t *genQueryInp)
{
    int status;
    genQueryOut_t *genQueryOut = NULL;
    char myColl[MAX_NAME_LEN], myData[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];
    int queryFlags;

    queryFlags = setQueryFlag (rodsArgs);
    setQueryInpForData (queryFlags, genQueryInp);
    genQueryInp->maxRows = MAX_SQL_ROWS;

    memset (myColl, 0, MAX_NAME_LEN);
    memset (myData, 0, MAX_NAME_LEN);

    if ((status = splitPathByKey (
      srcPath, myColl, myData, '/')) < 0) {
        rodsLogError (LOG_ERROR, status,
          "setQueryInpForLong: splitPathByKey for %s error, status = %d",
          srcPath, status);
        return (status);
    }

    snprintf (condStr, MAX_NAME_LEN, "='%s'", myColl);
    addInxVal (&genQueryInp->sqlCondInp, COL_COLL_NAME, condStr);
    snprintf (condStr, MAX_NAME_LEN, "='%s'", myData);
    addInxVal (&genQueryInp->sqlCondInp, COL_DATA_NAME, condStr);

    status =  rcGenQuery (conn, genQueryInp, &genQueryOut);

    if (status < 0) {
	if (status == CAT_NO_ROWS_FOUND) {
	    rodsLog (LOG_ERROR, "%s does not exist or user lacks access permission",
		     srcPath);
	} else {
            rodsLogError (LOG_ERROR, status,
	      "lsDataObjUtilLong: rcGenQuery error for %s", srcPath);
	}
	return (status);
    }
    printLsLong (conn, rodsArgs, genQueryOut);

    return (0);
}

int
printLsLong (rcComm_t *conn, rodsArguments_t *rodsArgs, 
genQueryOut_t *genQueryOut)
{
    int i;
    sqlResult_t *dataName, *replNum, *dataSize, *rescName, 
      *replStatus, *dataModify, *dataOwnerName, *dataId;
    sqlResult_t *chksumStr, *dataPath, *rescGrp;
    char *tmpDataId;
    int queryFlags;

   if (genQueryOut == NULL) {
        return (USER__NULL_INPUT_ERR);
    }

    queryFlags = setQueryFlag (rodsArgs);

    if (rodsArgs->veryLongOption == True) {
        if ((chksumStr = getSqlResultByInx (genQueryOut, COL_D_DATA_CHECKSUM)) 
	  == NULL) {
            rodsLog (LOG_ERROR,
              "printLsLong: getSqlResultByInx for COL_D_DATA_CHECKSUM failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((dataPath = getSqlResultByInx (genQueryOut, COL_D_DATA_PATH)) 
          == NULL) {
            rodsLog (LOG_ERROR,
              "printLsLong: getSqlResultByInx for COL_D_DATA_PATH failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }

        if ((rescGrp = getSqlResultByInx (genQueryOut, COL_D_RESC_GROUP_NAME))
          == NULL) {
            rodsLog (LOG_ERROR,
             "printLsLong: getSqlResultByInx for COL_D_RESC_GROUP_NAME failed");
            return (UNMATCHED_KEY_OR_INDEX);
        }
    }

    if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_DATA_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_DATA_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((replNum = getSqlResultByInx (genQueryOut, COL_DATA_REPL_NUM)) == 
      NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_DATA_REPL_NUM failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataSize = getSqlResultByInx (genQueryOut, COL_DATA_SIZE)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_DATA_SIZE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((rescName = getSqlResultByInx (genQueryOut, COL_D_RESC_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_RESC_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((replStatus = getSqlResultByInx (genQueryOut, COL_D_REPL_STATUS)) == 
     NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_REPL_STATUS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataModify = getSqlResultByInx (genQueryOut, COL_D_MODIFY_TIME)) ==
     NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_MODIFY_TIME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataOwnerName = getSqlResultByInx (genQueryOut, COL_D_OWNER_NAME)) ==
     NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_OWNER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

   for (i = 0;i < genQueryOut->rowCnt; i++) {
	collEnt_t collEnt;

	bzero (&collEnt, sizeof (collEnt));

	collEnt.dataName = &dataName->value[dataName->len * i];
	collEnt.replNum = atoi (&replNum->value[replNum->len * i]);
	collEnt.dataSize = strtoll (&dataSize->value[dataSize->len * i], 0, 0);
	collEnt.resource = &rescName->value[rescName->len * i];
	collEnt.ownerName = &dataOwnerName->value[dataOwnerName->len * i];
	collEnt.replStatus = atoi (&replStatus->value[replStatus->len * i]);
	collEnt.modifyTime = &dataModify->value[dataModify->len * i];
        if (rodsArgs->veryLongOption == True) {
	    collEnt.chksum = &chksumStr->value[chksumStr->len * i];
	    collEnt.phyPath = &dataPath->value[dataPath->len * i];
	    collEnt.rescGrp = &rescGrp->value[dataPath->len * i];
	}

	printDataCollEntLong (&collEnt, queryFlags);

	if (rodsArgs->accessControl == True) {
            tmpDataId = &dataId->value[dataId->len * i];
	    printDataAcl (conn, tmpDataId);
	}
    }

    return (0);
}

int
lsSpecDataObjUtilLong (rcComm_t *conn, rodsPath_t *srcPath, rodsEnv *myRodsEnv,
rodsArguments_t *rodsArgs)
{
    char sizeStr[NAME_LEN];
    int status;
    rodsObjStat_t *rodsObjStat = srcPath->rodsObjStat;

    snprintf (sizeStr, NAME_LEN, "%lld", rodsObjStat->objSize);
    status = printSpecLsLong (srcPath->outPath, rodsObjStat->ownerName,
     sizeStr, rodsObjStat->modifyTime, rodsObjStat->specColl,
     rodsArgs);

    return (status);
}


int
printLsShort (rcComm_t *conn,  rodsArguments_t *rodsArgs, 
genQueryOut_t *genQueryOut)
{
    int i;

    sqlResult_t *dataName, *dataId;
    char *tmpDataName, *tmpDataId;

   if (genQueryOut == NULL) {
        return (USER__NULL_INPUT_ERR);
    }

    if ((dataId = getSqlResultByInx (genQueryOut, COL_D_DATA_ID)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_D_DATA_ID failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printLsLong: getSqlResultByInx for COL_DATA_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0;i < genQueryOut->rowCnt; i++) {
        tmpDataName = &dataName->value[dataName->len * i];
	printLsStrShort (tmpDataName);

        if (rodsArgs->accessControl == True) {
            tmpDataId = &dataId->value[dataId->len * i];
            printDataAcl (conn, tmpDataId);
        }
    }

    return (0);
}

int
initCondForLs (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
genQueryInp_t *genQueryInp)
{
    if (genQueryInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForLs: NULL genQueryInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (genQueryInp, 0, sizeof (genQueryInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    return (0);
}

int
lsCollUtil (rcComm_t *conn, rodsPath_t *srcPath, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs)
{
    int savedStatus = 0;
    char *srcColl;
    int status;
    int queryFlags;
    collHandle_t collHandle;
    collEnt_t collEnt;

    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "lsCollUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    srcColl = srcPath->outPath;

    /* print this collection */
    printf ("%s:\n", srcColl);

    if (rodsArgs->accessControl == True) {
       printCollAcl (conn, srcColl);
       printCollInheritance (conn, srcColl);
    }

    queryFlags = DATA_QUERY_FIRST_FG;
    if (rodsArgs->veryLongOption == True) {
	/* need to check veryLongOption first since it will have both
	 * veryLongOption and longOption flags on. */
	queryFlags = queryFlags | VERY_LONG_METADATA_FG | NO_TRIM_REPL_FG;
    } else if (rodsArgs->longOption == True) { 
	queryFlags |= LONG_METADATA_FG | NO_TRIM_REPL_FG;;
    }

    status = rclOpenCollection (conn, srcColl, queryFlags,
      &collHandle);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "lsCollUtil: rclOpenCollection of %s error. status = %d",
          srcColl, status);
        return status;
    }
    while ((status = rclReadCollection (conn, &collHandle, &collEnt)) >= 0) {
	if (collEnt.objType == DATA_OBJ_T) {
	    printDataCollEnt (&collEnt, queryFlags);
	    if (rodsArgs->accessControl == True) {
		printDataAcl (conn, collEnt.dataId);
	    }
	} else {
	    printCollCollEnt (&collEnt, queryFlags);
	    /* need to drill down */
            if (rodsArgs->recursive == True && 
	      strcmp (collEnt.collName, "/") != 0) {
                rodsPath_t tmpPath;
                memset (&tmpPath, 0, sizeof (tmpPath));
                rstrcpy (tmpPath.outPath, collEnt.collName, MAX_NAME_LEN);
                status = lsCollUtil (conn, &tmpPath, myRodsEnv, rodsArgs);
	        if (status < 0) savedStatus = status;
	    }
	}
    }
    rclCloseCollection (&collHandle);
    if (savedStatus < 0 && savedStatus != CAT_NO_ROWS_FOUND) {
        return (savedStatus);
    } else {
        return (0);
    }
}

int
printDataCollEnt (collEnt_t *collEnt, int flags)
{
    if ((flags & (LONG_METADATA_FG | VERY_LONG_METADATA_FG)) == 0) {
	/* short */
	printLsStrShort (collEnt->dataName);
    } else {
	printDataCollEntLong (collEnt, flags);
    }
    return 0;
}

int
printDataCollEntLong (collEnt_t *collEnt, int flags)
{
    char *tmpReplStatus;
    char localTimeModify[20];
    char typeStr[NAME_LEN];

    if (collEnt->replStatus == OLD_COPY) {
        tmpReplStatus = " ";
    } else {
        tmpReplStatus = "&";
    }

    getLocalTimeFromRodsTime (collEnt->modifyTime, localTimeModify);

    if (collEnt->specColl.collClass == NO_SPEC_COLL ||
      collEnt->specColl.collClass == LINKED_COLL) {
        printf ("  %-12.12s %6d %-20.20s %12lld %16.16s %s %s\n",
         collEnt->ownerName, collEnt->replNum, collEnt->resource, 
         collEnt->dataSize, localTimeModify, tmpReplStatus, collEnt->dataName);
    } else {
        getSpecCollTypeStr (&collEnt->specColl, typeStr);
        printf ("  %-12.12s %6.6s %-20.20s %12lld %16.16s %s %s\n",
         collEnt->ownerName, typeStr, collEnt->resource,
         collEnt->dataSize, localTimeModify, tmpReplStatus, collEnt->dataName);
    }


    if ((flags & VERY_LONG_METADATA_FG) != 0) {
        printf ("    %s    %s    %s\n", collEnt->chksum, collEnt->phyPath,
	  collEnt->rescGrp);
    }
    return 0;
}

int
printCollCollEnt (collEnt_t *collEnt, int flags)
{
	char typeStr[NAME_LEN];

    typeStr[0] = '\0';
    if (collEnt->specColl.collClass != NO_SPEC_COLL) {
        getSpecCollTypeStr (&collEnt->specColl, typeStr);
    }
    if ((flags & LONG_METADATA_FG) != 0) {
	printf ("  C- %s  %s\n", collEnt->collName, typeStr);
    } else if ((flags & VERY_LONG_METADATA_FG) != 0) {
	if (collEnt->specColl.collClass == NO_SPEC_COLL) {
	    printf ("  C- %s\n", collEnt->collName);
        } else {
	    if (collEnt->specColl.collClass == MOUNTED_COLL ||
	      collEnt->specColl.collClass == LINKED_COLL) {
                printf ("  C- %s  %6.6s  %s  %s   %s\n",
                  collEnt->collName, typeStr,
                  collEnt->specColl.objPath, collEnt->specColl.phyPath,
                  collEnt->specColl.resource);
	    } else {
	        printf ("  C- %s  %6.6s  %s  %s;;;%s;;;%d\n", 
	          collEnt->collName, typeStr,
                  collEnt->specColl.objPath, collEnt->specColl.cacheDir,
	          collEnt->specColl.resource, collEnt->specColl.cacheDirty);
	    }
	}
    } else {
        /* short */
	printf ("  C- %s\n", collEnt->collName);
    }
    return (0);
}

int
printSpecLsLong (char *objPath, char *ownerName, char *objSize, 
char *modifyTime, specColl_t *specColl, rodsArguments_t *rodsArgs)
{
    char srcElement[MAX_NAME_LEN];
    collEnt_t collEnt;
    int queryFlags;

    bzero (&collEnt, sizeof (collEnt));

    getLastPathElement (objPath, srcElement);
    collEnt.dataName = objPath;
    collEnt.ownerName = ownerName;
    collEnt.dataSize = strtoll (objSize, 0, 0);
    collEnt.resource = specColl->resource;
    collEnt.modifyTime = modifyTime;
    collEnt.specColl = *specColl;
    collEnt.objType = DATA_OBJ_T;
    queryFlags = setQueryFlag (rodsArgs);

    printDataCollEntLong (&collEnt, queryFlags);
    return (0);
}

int
printDataAcl (rcComm_t *conn, char *dataId)
{
    genQueryOut_t *genQueryOut = NULL;
    int status;
    int i;
    sqlResult_t *userName, *userZone, *dataAccess;
    char *userNameStr, *userZoneStr, *dataAccessStr;

    status = queryDataObjAcl (conn, dataId, &genQueryOut);

    printf ("        ACL - ");

    if (status < 0) {
	printf ("\n");
        return (status);
    }

    if ((userName = getSqlResultByInx (genQueryOut, COL_USER_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printDataAcl: getSqlResultByInx for COL_USER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((userZone = getSqlResultByInx (genQueryOut, COL_USER_ZONE)) == NULL) {
        rodsLog (LOG_ERROR,
          "printDataAcl: getSqlResultByInx for COL_USER_ZONE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataAccess = getSqlResultByInx (genQueryOut, COL_DATA_ACCESS_NAME)) 
      == NULL) {
        rodsLog (LOG_ERROR,
          "printDataAcl: getSqlResultByInx for COL_DATA_ACCESS_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0; i < genQueryOut->rowCnt; i++) {
	userNameStr = &userName->value[userName->len * i];
	userZoneStr = &userZone->value[userZone->len * i];
	dataAccessStr = &dataAccess->value[dataAccess->len * i];
	printf ("%s#%s:%s   ", userNameStr, userZoneStr, dataAccessStr);
    }
    
    printf ("\n");

    freeGenQueryOut (&genQueryOut);

    return (status);
}

int
printCollAcl (rcComm_t *conn, char *collName)
{
    genQueryOut_t *genQueryOut = NULL;
    int status;
    int i;
    sqlResult_t *userName, *userZone, *dataAccess ;
    char *userNameStr, *userZoneStr, *dataAccessStr;

    status = queryCollAcl (conn, collName, &genQueryOut);

    printf ("        ACL - ");

    if (status < 0) {
	printf ("\n");
        return (status);
    }

    if ((userName = getSqlResultByInx (genQueryOut, COL_COLL_USER_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printCollAcl: getSqlResultByInx for COL_COLL_USER_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }
    if ((userZone = getSqlResultByInx (genQueryOut, COL_COLL_USER_ZONE)) == NULL) {
        rodsLog (LOG_ERROR,
          "printCollAcl: getSqlResultByInx for COL_COLL_USER_ZONE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((dataAccess = getSqlResultByInx (genQueryOut, COL_COLL_ACCESS_NAME)) == NULL) {
        rodsLog (LOG_ERROR,
          "printCollAcl: getSqlResultByInx for COL_COLL_ACCESS_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0; i < genQueryOut->rowCnt; i++) {
	userNameStr = &userName->value[userName->len * i];
	userZoneStr = &userZone->value[userZone->len * i];
	dataAccessStr = &dataAccess->value[dataAccess->len * i];
	printf ("%s#%s:%s   ",  userNameStr, userZoneStr, dataAccessStr);
    }
    
    printf ("\n");

    freeGenQueryOut (&genQueryOut);

    return (status);
}

int
printCollInheritance (rcComm_t *conn, char *collName)
{
    genQueryOut_t *genQueryOut = NULL;
    int status;
    sqlResult_t *inheritResult;
    char *inheritStr;

    status = queryCollInheritance (conn, collName, &genQueryOut);

    if (status < 0) {
        return (status);
    }

    if ((inheritResult = getSqlResultByInx (genQueryOut, COL_COLL_INHERITANCE)) == NULL) {
        rodsLog (LOG_ERROR,
          "printCollInheritance: getSqlResultByInx for COL_COLL_INHERITANCE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    inheritStr = &inheritResult->value[0];
    printf ("        Inheritance - ");
    if (*inheritStr=='1') {
       printf("Enabled\n");
    }
    else {
       printf("Disabled\n");
    }

    freeGenQueryOut (&genQueryOut);

    return (status);
}

void 
printCollOrDir (char *myName, objType_t myType, rodsArguments_t *rodsArgs,
specColl_t *specColl)
{
    char *typeStr;

    if (rodsArgs->verbose == False) return;

    if (myType == COLL_OBJ_T)
	typeStr = "C";
    else
        typeStr = "D";

    if (specColl != NULL) {
        char objType[NAME_LEN];
	int status;
        status = getSpecCollTypeStr (specColl, objType);
        if (status < 0) objType[0] = '\0';
        fprintf (stdout, "%s- %s    %-5.5s :\n", typeStr, myName, objType);
    } else {
        fprintf (stdout, "%s- %s :\n", typeStr, myName);
    }
}

