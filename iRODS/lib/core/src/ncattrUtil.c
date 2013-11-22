/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "ncattrUtil.h"

int
ncattrUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    ncRegGlobalAttrInp_t ncRegGlobalAttrInp;


    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForNcattr (myRodsEnv, myRodsArgs, &ncRegGlobalAttrInp);

    for (i = 0; i < rodsPathInp->numSrc; i++) {
	if (rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T) {
	    getRodsObjType (conn, &rodsPathInp->srcPath[i]);
	    if (rodsPathInp->srcPath[i].objState == NOT_EXIST_ST) {
                rodsLog (LOG_ERROR,
                  "ncattrUtil: srcPath %s does not exist",
                  rodsPathInp->srcPath[i].outPath);
		savedStatus = USER_INPUT_PATH_ERR;
		continue;
	    }
	}

	if (rodsPathInp->srcPath[i].objType == DATA_OBJ_T) {
            rmKeyVal (&ncRegGlobalAttrInp.condInput, TRANSLATED_PATH_KW);
            if (myRodsArgs->reg == True) {
	       status = regAttrDataObjUtil (conn, 
                 rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs, 
                 &ncRegGlobalAttrInp);
            } else if (myRodsArgs->remove == True) {
               status = rmAttrDataObjUtil (conn,
                 rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs, 
                  &ncRegGlobalAttrInp);
            } else if (myRodsArgs->query == True) {
                rodsLog (LOG_ERROR,
                 "ncattrUtil: input path %s must be a collection for -q option",
                 rodsPathInp->srcPath[i].outPath);
                status = USER_INPUT_PATH_ERR;
            } else {
               status = listAttrDataObjUtil (conn,
                 rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs);
            }
	} else if (rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T) {
            /* The path given by collEnt.collName from rclReadCollection
             * has already been translated */
            addKeyVal (&ncRegGlobalAttrInp.condInput, TRANSLATED_PATH_KW, "");
            if (myRodsArgs->reg == True) {
	        status = regAttrCollUtil (conn, rodsPathInp->srcPath[i].outPath,
                  myRodsEnv, myRodsArgs, &ncRegGlobalAttrInp);
            } else if (myRodsArgs->remove == True) {
                status = rmAttrCollUtil (conn, rodsPathInp->srcPath[i].outPath,
                  myRodsEnv, myRodsArgs, &ncRegGlobalAttrInp);
            } else if (myRodsArgs->query == True) {
                status = queryAUVForDataObj (conn, 
                  rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs);
            } else {
                status = listAttrCollUtil (conn, 
                  rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs);
            }
	} else {
	    /* should not be here */
	    rodsLog (LOG_ERROR,
	     "ncattrUtil: invalid ncattr objType %d for %s", 
	     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath);
            clearRegGlobalAttrInp (&ncRegGlobalAttrInp);
	    return (USER_INPUT_PATH_ERR);
	}
	/* XXXX may need to return a global status */
	if (status < 0) {
	    rodsLogError (LOG_ERROR, status,
             "ncattrUtil: ncattr error for %s, status = %d", 
	      rodsPathInp->srcPath[i].outPath, status);
	    savedStatus = status;
	} 
    }
    clearRegGlobalAttrInp (&ncRegGlobalAttrInp);
    if (savedStatus < 0) {
        return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
        return (0);
    } else {
        return (status);
    }
}

int
regAttrDataObjUtil (rcComm_t *conn, char *srcPath, 
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int status;
    struct timeval startTime, endTime;
 
    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "regAttrDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->verbose == True) {
        (void) gettimeofday(&startTime, (struct timezone *)0);
    }

    rstrcpy (ncRegGlobalAttrInp->objPath, srcPath, MAX_NAME_LEN);

    status = rcNcRegGlobalAttr (conn, ncRegGlobalAttrInp);

    if (status >= 0 && rodsArgs->verbose == True) {
        (void) gettimeofday(&endTime, (struct timezone *)0);
        printTime (conn, ncRegGlobalAttrInp->objPath, &startTime, &endTime);
    }

    return (status);
}

int
initCondForNcattr (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{

    if (ncRegGlobalAttrInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForNcattr: NULL ncRegGlobalAttrInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (ncRegGlobalAttrInp, 0, sizeof (ncRegGlobalAttrInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->admin == True) {
        addKeyVal (&ncRegGlobalAttrInp->condInput, IRODS_ADMIN_KW, "");
    }

    if ((rodsArgs->reg == True || rodsArgs->remove == true) && 
      rodsArgs->attr == True && rodsArgs->attrStr != NULL) {
        char outBuf[MAX_NAME_LEN];
        char *inPtr = rodsArgs->attrStr;
        int inLen = strlen (rodsArgs->attrStr);
        ncRegGlobalAttrInp->attrNameArray = (char **) 
          calloc (MAX_NAME_LEN, sizeof (ncRegGlobalAttrInp->attrNameArray));
        while (getNextEleInStr (&inPtr, outBuf, &inLen, MAX_NAME_LEN) > 0) {
            ncRegGlobalAttrInp->attrNameArray[ncRegGlobalAttrInp->numAttrName] =
              strdup (outBuf);
            ncRegGlobalAttrInp->numAttrName++;
            if (ncRegGlobalAttrInp->numAttrName >= MAX_NAME_LEN) break;
        }
    }

    return (0);
}

int
regAttrCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int status;
    int savedStatus = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN];

    if (srcColl == NULL) {
       rodsLog (LOG_ERROR,
          "regAttrCollUtil: NULL srcColl input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->recursive != True) {
        rodsLog (LOG_ERROR,
        "regAttrCollUtil: -r option must be used for getting %s collection",
         srcColl);
        return (USER_INPUT_OPTION_ERR);
    }

    if (rodsArgs->verbose == True) {
        fprintf (stdout, "C- %s:\n", srcColl);
    }

    status = rclOpenCollection (conn, srcColl, 0, &collHandle);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "regAttrCollUtil: rclOpenCollection of %s error. status = %d",
          srcColl, status);
        return status;
    }
    if (collHandle.rodsObjStat->specColl != NULL &&
      collHandle.rodsObjStat->specColl->collClass != LINKED_COLL) {
        /* no reg for mounted coll */
        rclCloseCollection (&collHandle);
        return 0;
    }
    while ((status = rclReadCollection (conn, &collHandle, &collEnt)) >= 0) {
        if (collEnt.objType == DATA_OBJ_T) {
            snprintf (srcChildPath, MAX_NAME_LEN, "%s/%s",
              collEnt.collName, collEnt.dataName);

            status = regAttrDataObjUtil (conn, srcChildPath,
             myRodsEnv, rodsArgs, ncRegGlobalAttrInp);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "regAttrCollUtil: regAttrDataObjUtil failed for %s. status = %d",
                  srcChildPath, status);
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        } else if (collEnt.objType == COLL_OBJ_T) {
            ncRegGlobalAttrInp_t childNcRegGlobalAttr;
            childNcRegGlobalAttr = *ncRegGlobalAttrInp;
            if (collEnt.specColl.collClass != NO_SPEC_COLL) continue;
            status = regAttrCollUtil (conn, collEnt.collName, myRodsEnv,
              rodsArgs, &childNcRegGlobalAttr);
            if (status < 0 && status != CAT_NO_ROWS_FOUND) {
                savedStatus = status;
            }
	}
    }
    rclCloseCollection (&collHandle);

    if (savedStatus < 0) {
	return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
	return (0);
    } else {
        return (status);
    }
}

int
rmAttrDataObjUtil (rcComm_t *conn, char *srcPath,
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int status, i;
    struct timeval startTime, endTime;
    modAVUMetadataInp_t modAVUMetadataInp;

    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "rmAttrDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->verbose == True) {
        (void) gettimeofday(&startTime, (struct timezone *)0);
    }

    bzero (&modAVUMetadataInp, sizeof (modAVUMetadataInp));
    modAVUMetadataInp.arg0 = "rmw";
    modAVUMetadataInp.arg1 = "-d";
    modAVUMetadataInp.arg2 = srcPath;
    modAVUMetadataInp.arg4 = "%";
    modAVUMetadataInp.arg5 = "";
    modAVUMetadataInp.arg6 = "";
    modAVUMetadataInp.arg7 = "";
    modAVUMetadataInp.arg8 = "";
    modAVUMetadataInp.arg9 ="";


    if (ncRegGlobalAttrInp->numAttrName > 0 &&
      ncRegGlobalAttrInp->attrNameArray != NULL) {
        for (i = 0; i < ncRegGlobalAttrInp->numAttrName; i++) {
            modAVUMetadataInp.arg3 = ncRegGlobalAttrInp->attrNameArray[i];
            status = rcModAVUMetadata(conn, &modAVUMetadataInp);
            if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) {
                status = 0;
            } else if (status < 0) {
                break;
            }
        }
    } else {
        modAVUMetadataInp.arg3 = "%";
        status = rcModAVUMetadata(conn, &modAVUMetadataInp);

        if (status == CAT_SUCCESS_BUT_WITH_NO_INFO) status = 0;
        if (status >= 0 && rodsArgs->verbose == True) {
            (void) gettimeofday(&endTime, (struct timezone *)0);
            printTime (conn, srcPath, &startTime, &endTime);
        }
    }
    return (status);
}

int
rmAttrCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs, ncRegGlobalAttrInp_t *ncRegGlobalAttrInp)
{
    int status;
    int savedStatus = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN];

    if (srcColl == NULL) {
       rodsLog (LOG_ERROR,
          "rmAttrCollUtil: NULL srcColl input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->recursive != True) {
        rodsLog (LOG_ERROR,
        "rmAttrCollUtil: -r option must be used for getting %s collection",
         srcColl);
        return (USER_INPUT_OPTION_ERR);
    }

    if (rodsArgs->verbose == True) {
        fprintf (stdout, "C- %s:\n", srcColl);
    }

    status = rclOpenCollection (conn, srcColl, 0, &collHandle);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rmAttrCollUtil: rclOpenCollection of %s error. status = %d",
          srcColl, status);
        return status;
    }
    if (collHandle.rodsObjStat->specColl != NULL &&
      collHandle.rodsObjStat->specColl->collClass != LINKED_COLL) {
        /* no reg for mounted coll */
        rclCloseCollection (&collHandle);
        return 0;
    }
    while ((status = rclReadCollection (conn, &collHandle, &collEnt)) >= 0) {
        if (collEnt.objType == DATA_OBJ_T) {
            snprintf (srcChildPath, MAX_NAME_LEN, "%s/%s",
              collEnt.collName, collEnt.dataName);

            status = rmAttrDataObjUtil (conn, srcChildPath,
             myRodsEnv, rodsArgs, ncRegGlobalAttrInp);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "rmAttrCollUtil: regAttrDataObjUtil failed for %s. status = %d",
                  srcChildPath, status);
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        } else if (collEnt.objType == COLL_OBJ_T) {
            if (collEnt.specColl.collClass != NO_SPEC_COLL) continue;
            status = rmAttrCollUtil (conn, collEnt.collName, myRodsEnv,
              rodsArgs, ncRegGlobalAttrInp);
            if (status < 0 && status != CAT_NO_ROWS_FOUND) {
                savedStatus = status;
            }
	}
    }
    rclCloseCollection (&collHandle);

    if (savedStatus < 0) {
	return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
	return (0);
    } else {
        return (status);
    }
}

int
listAttrDataObjUtil (rcComm_t *conn, char *srcPath,
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs)
{
    int status, i;
    genQueryOut_t *genQueryOut = NULL;
    sqlResult_t *metaAttr, *metaVal, *metaUnits;
    char *metaAttrStr, *metaValStr, *metaUnitsStr;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    int mycount = 0;


    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "listAttrDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    if ((status = splitPathByKey (srcPath, myDir, myFile, '/')) < 0) {
        rodsLogError (LOG_NOTICE, status,
          "listAttrDataObjUtil: splitPathByKey for %s error, status = %d",
          srcPath, status);
        return (status);
    }

    printf ("%s :\n", myFile);

    status = queryDataObjForAUV (conn, srcPath, rodsArgs, &genQueryOut);

    if (status < 0) {
        if (status == CAT_NO_ROWS_FOUND) return 0;
        rodsLogError (LOG_ERROR, status,
         "listAttrDataObjUtil: svrQueryDataObjMetadata error for  %s",
          srcPath);
        return status;
    }

    if ((metaAttr = getSqlResultByInx (genQueryOut, COL_META_DATA_ATTR_NAME)) 
      == NULL) {
        rodsLog (LOG_ERROR,
          "listAttrDataObjUtil: getSqlResultByInx for COL_META_DATA_ATTR_NAME failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((metaVal = getSqlResultByInx (genQueryOut, COL_META_DATA_ATTR_VALUE)) 
      == NULL) {
        rodsLog (LOG_ERROR,
          "listAttrDataObjUtil: getSqlResultByInx for COL_META_DATA_ATTR_VALUE failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    if ((metaUnits = getSqlResultByInx (genQueryOut, COL_META_DATA_ATTR_UNITS))
      == NULL) {
        rodsLog (LOG_ERROR,
          "listAttrDataObjUtil: getSqlResultByInx for COL_META_DATA_ATTR_UNITS failed");
        return (UNMATCHED_KEY_OR_INDEX);
    }

    for (i = 0; i < genQueryOut->rowCnt; i++) {
        metaAttrStr = &metaAttr->value[metaAttr->len * i];
        metaValStr = &metaVal->value[metaVal->len * i];
        metaUnitsStr = &metaUnits->value[metaUnits->len * i];
        if (rodsArgs->longOption == True || rodsArgs->attr == True) {
            printf ("   %s = \n", metaAttrStr);
            printNice (metaValStr, "      ", 72);
            printf ("\n");
        } else {
            if (i == 0) printf ("        ");
            if (i >= genQueryOut->rowCnt - 1) {
                printf ("%s\n", metaAttrStr);
            } else if (mycount >= MAX_ATTR_NAME_PER_LINE - 1) { 
                printf ("%s\n", metaAttrStr);
                mycount = 0;
                printf ("        ");
            } else {
                printf ("%s, ", metaAttrStr);
                mycount++;
            }
        }
    }
    freeGenQueryOut (&genQueryOut);

    return status;
}

int
listAttrCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs)
{
    int status;
    int savedStatus = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN];

    if (srcColl == NULL) {
       rodsLog (LOG_ERROR,
          "rmAttrCollUtil: NULL srcColl input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->recursive != True) {
        rodsLog (LOG_ERROR,
        "rmAttrCollUtil: -r option must be used for getting %s collection",
         srcColl);
        return (USER_INPUT_OPTION_ERR);
    }

    fprintf (stdout, "C- %s:\n", srcColl);

    status = rclOpenCollection (conn, srcColl, 0, &collHandle);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "rmAttrCollUtil: rclOpenCollection of %s error. status = %d",
          srcColl, status);
        return status;
    }
    if (collHandle.rodsObjStat->specColl != NULL &&
      collHandle.rodsObjStat->specColl->collClass != LINKED_COLL) {
        /* no reg for mounted coll */
        rclCloseCollection (&collHandle);
        return 0;
    }
    while ((status = rclReadCollection (conn, &collHandle, &collEnt)) >= 0) {
        if (collEnt.objType == DATA_OBJ_T) {
            snprintf (srcChildPath, MAX_NAME_LEN, "%s/%s",
              collEnt.collName, collEnt.dataName);

            status = listAttrDataObjUtil (conn, srcChildPath,
             myRodsEnv, rodsArgs);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "rmAttrCollUtil: regAttrDataObjUtil failed for %s. status = %d",
                  srcChildPath, status);
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        } else if (collEnt.objType == COLL_OBJ_T) {
            if (collEnt.specColl.collClass != NO_SPEC_COLL) continue;
            status = listAttrCollUtil (conn, collEnt.collName, myRodsEnv,
              rodsArgs);
            if (status < 0 && status != CAT_NO_ROWS_FOUND) {
                savedStatus = status;
            }
	}
    }
    rclCloseCollection (&collHandle);

    if (savedStatus < 0) {
	return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
	return (0);
    } else {
        return (status);
    }
}

int
queryDataObjForAUV (rcComm_t *conn, char *objPath, rodsArguments_t *rodsArgs,
genQueryOut_t **genQueryOut)
{
    genQueryInp_t genQueryInp;
    int status;
    char tmpStr[MAX_NAME_LEN];
    char dataName[MAX_NAME_LEN], collection[MAX_NAME_LEN];

    if (objPath == NULL || genQueryOut == NULL) {
        return (USER__NULL_INPUT_ERR);
    }

    splitPathByKey(objPath, collection, dataName, '/');

    memset (&genQueryInp, 0, sizeof (genQueryInp_t));

    addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_VALUE, 1);
    addInxIval (&genQueryInp.selectInp, COL_META_DATA_ATTR_UNITS, 1);

    snprintf (tmpStr, MAX_NAME_LEN, " = '%s'", dataName);
    addInxVal (&genQueryInp.sqlCondInp, COL_DATA_NAME, tmpStr);
    snprintf (tmpStr, MAX_NAME_LEN, " = '%s'", collection);
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr);

    if (rodsArgs->attr == True && rodsArgs->attrStr != NULL) {
        char outBuf[MAX_NAME_LEN], tmpStr1[MAX_NAME_LEN];
        char *inPtr = rodsArgs->attrStr;
        int inLen = strlen (rodsArgs->attrStr);
        status = getNextEleInStr (&inPtr, outBuf, &inLen, MAX_NAME_LEN);
        if (status <= 0) {
            rodsLog (LOG_ERROR,
              "queryDataObjForAUV: No attrName for --attr");
            clearGenQueryInp(&genQueryInp);
            return (USER_OPTION_INPUT_ERR);
        }
        snprintf (tmpStr, MAX_NAME_LEN, " = '%s'", outBuf);
        while (getNextEleInStr (&inPtr, outBuf, &inLen, MAX_NAME_LEN) > 0) {
            snprintf (tmpStr1, MAX_NAME_LEN, " || = '%s'", outBuf);
            rstrcat (tmpStr, tmpStr1, MAX_NAME_LEN);
        }
        addInxVal (&genQueryInp.sqlCondInp, COL_META_DATA_ATTR_NAME, tmpStr);
    }
    genQueryInp.maxRows = 100*MAX_SQL_ROWS;
    status =  rcGenQuery (conn, &genQueryInp, genQueryOut);
    clearGenQueryInp (&genQueryInp);

    return (status);

}

/*
 * queryAUVForDataObj - this is similar to queryDataObj in imeta
 */
int 
queryAUVForDataObj (rcComm_t *conn, char *collPath, rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs) 
{
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    char attr[MAX_NAME_LEN], op[MAX_NAME_LEN], value[MAX_NAME_LEN], 
      tmpStr[MAX_NAME_LEN];
    char *inPtr, *outPtr;
    int status, count, inLen;
    int continueInx = 1;

    inPtr = rodsArgs->queryStr;
    if (inPtr == NULL) return USER__NULL_INPUT_ERR;

    memset (&genQueryInp, 0, sizeof (genQueryInp_t));

    addInxIval (&genQueryInp.selectInp, COL_COLL_NAME, 1);
    addInxIval (&genQueryInp.selectInp, COL_DATA_NAME, 1);
    snprintf (tmpStr, MAX_NAME_LEN, " = '%s' || like '%s/%s'", 
      collPath, collPath, "%");
    addInxVal (&genQueryInp.sqlCondInp, COL_COLL_NAME, tmpStr);

    inLen = strlen (inPtr);
    outPtr = attr;
    count = 0;
    while (getNextEleInStr (&inPtr, outPtr, &inLen, MAX_NAME_LEN) > 0) {
        if (count == 0) {
            snprintf (tmpStr, MAX_NAME_LEN, " = '%s'", attr);
            addInxVal(&genQueryInp.sqlCondInp, COL_META_DATA_ATTR_NAME, tmpStr);
            count++;
            outPtr = op;
        } else if (count == 1) {
            count++;
            outPtr = value;
        } else {
            snprintf (tmpStr, MAX_NAME_LEN, "%s '%s'", op, value);
            addInxVal(&genQueryInp.sqlCondInp, COL_META_DATA_ATTR_VALUE, tmpStr);
            count = 0;
            outPtr = attr;
        }
    }
    if (count != 0) {
        rodsLog (LOG_ERROR, 
          "queryAUVForDataObj: query inp must be in attr/op/value triplet: %s",
          rodsArgs->queryStr);
        clearGenQueryInp(&genQueryInp);
        return (USER_OPTION_INPUT_ERR);
    }
    genQueryInp.maxRows = MAX_SQL_ROWS;

    while (continueInx > 0) {
        sqlResult_t *dataName, *collection;
        char *dataNameStr, *collectionStr;
        int i;

        status = rcGenQuery (conn, &genQueryInp, &genQueryOut);
        if (status < 0) {
            if (status != CAT_NO_ROWS_FOUND) {
                rodsLogError (LOG_ERROR, status,
                  "queryAUVForDataObj: rsGenQuery error for %s",
                  collPath);
            }
            clearGenQueryInp (&genQueryInp);
            return (status);
        }
        if ((collection = getSqlResultByInx (genQueryOut, COL_COLL_NAME)) ==
          NULL) {
            rodsLog (LOG_ERROR,
              "queryAUVForDataObj: getSqlResultByInx for COL_COLL_NAME failed");
            clearGenQueryInp (&genQueryInp);
            return (UNMATCHED_KEY_OR_INDEX);
        }
        if ((dataName = getSqlResultByInx (genQueryOut, COL_DATA_NAME)) ==
          NULL) {
            rodsLog (LOG_ERROR,
              "queryAUVForDataObj: getSqlResultByInx for COL_DATA_NAME failed");
            clearGenQueryInp (&genQueryInp);
            return (UNMATCHED_KEY_OR_INDEX);
        }
        for (i = 0;i < genQueryOut->rowCnt; i++) {
            collectionStr = &collection->value[collection->len * i];
            dataNameStr = &dataName->value[dataName->len * i];
            fprintf (stdout, "    %s/%s\n",
              collectionStr, dataNameStr);
        }
        if (genQueryOut != NULL) {
            continueInx = genQueryInp.continueInx =
            genQueryOut->continueInx;
            freeGenQueryOut (&genQueryOut);
        } else {
            continueInx = 0;
        }
    }
    clearGenQueryInp (&genQueryInp);
    return (status);
}
 
