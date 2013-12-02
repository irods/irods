/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "rcPortalOpr.h"
#include "ncUtil.h"

int
ncUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    ncOpenInp_t ncOpenInp;
    ncVarSubset_t ncVarSubset;


    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForNcOper (myRodsEnv, myRodsArgs, &ncOpenInp, &ncVarSubset);

    for (i = 0; i < rodsPathInp->numSrc; i++) {
	if (rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T) {
	    getRodsObjType (conn, &rodsPathInp->srcPath[i]);
	    if (rodsPathInp->srcPath[i].objState == NOT_EXIST_ST) {
                rodsLog (LOG_ERROR,
                  "ncUtil: srcPath %s does not exist",
                  rodsPathInp->srcPath[i].outPath);
		savedStatus = USER_INPUT_PATH_ERR;
		continue;
	    }
	}

	if (rodsPathInp->srcPath[i].objType == DATA_OBJ_T) {
            if (myRodsArgs->agginfo == True) {
                rodsLog (LOG_ERROR,
                 "ncUtil: %s is not a collection for --agginfo",
                 rodsPathInp->srcPath[i].outPath);
                continue;
            }
            rmKeyVal (&ncOpenInp.condInput, TRANSLATED_PATH_KW);
	   status = ncOperDataObjUtil (conn, 
             rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs, 
             &ncOpenInp, &ncVarSubset);
	} else if (rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T) {
            /* The path given by collEnt.collName from rclReadCollection
             * has already been translated */
            addKeyVal (&ncOpenInp.condInput, TRANSLATED_PATH_KW, "");
            if (myRodsArgs->agginfo == True) {
                if (myRodsArgs->longOption==True) {
                    /* list the content of .aggInfo file */
                    status = catAggInfo (conn, rodsPathInp->srcPath[i].outPath);
                } else {
                    status = setAggInfo (conn, rodsPathInp->srcPath[i].outPath,
                      myRodsEnv, myRodsArgs, &ncOpenInp);
                }
            } else if (myRodsArgs->recursive == True) {
                status = ncOperCollUtil (conn, rodsPathInp->srcPath[i].outPath,
                  myRodsEnv, myRodsArgs, &ncOpenInp, &ncVarSubset);
            } else {
                /* assume it is an aggr collection */
                status = ncOperDataObjUtil (conn,
                 rodsPathInp->srcPath[i].outPath, myRodsEnv, myRodsArgs,
                 &ncOpenInp, &ncVarSubset);
            }
	} else {
	    /* should not be here */
	    rodsLog (LOG_ERROR,
	     "ncUtil: invalid nc objType %d for %s", 
	     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath);
	    return (USER_INPUT_PATH_ERR);
	}
	/* XXXX may need to return a global status */
	if (status < 0) {
	    rodsLogError (LOG_ERROR, status,
             "ncUtil: nc error for %s, status = %d", 
	      rodsPathInp->srcPath[i].outPath, status);
	    savedStatus = status;
	} 
    }
    if (savedStatus < 0) {
        return (savedStatus);
    } else if (status == CAT_NO_ROWS_FOUND) {
        return (0);
    } else {
        return (status);
    }
}

int
ncOperDataObjUtil (rcComm_t *conn, char *srcPath, 
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset)
{
    int status, status1;
    ncCloseInp_t ncCloseInp;
    ncInqInp_t ncInqInp;
    ncInqOut_t *ncInqOut = NULL;
    int ncid;

 
    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "ncOperDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    rstrcpy (ncOpenInp->objPath, srcPath, MAX_NAME_LEN);

    status = rcNcOpen (conn, ncOpenInp, &ncid);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "ncOperDataObjUtil: rcNcOpen error for %s", ncOpenInp->objPath);
        return status;
    }

    /* do the general inq */
    bzero (&ncInqInp, sizeof (ncInqInp));
    ncInqInp.ncid = ncid;
    ncInqInp.paramType = NC_ALL_TYPE;
    ncInqInp.flags = NC_ALL_FLAG;

    status = rcNcInq (conn, &ncInqInp, &ncInqOut);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "ncOperDataObjUtil: rcNcInq error for %s", ncOpenInp->objPath);
        return status;
    }
    if (rodsArgs->subsetByVal == True) {
        status = resolveSubsetVar (conn, ncid, ncInqOut, ncVarSubset);
        if (status < 0) {
            rodsLogError (LOG_ERROR, status,
              "ncOperDataObjUtil: resolveSubsetVar error for %s", 
              ncOpenInp->objPath);
            return status;
        }
    }
    if (rodsArgs->option == False) {
        /* stdout */
        prFirstNcLine (ncOpenInp->objPath);
        if (rodsArgs->header == True) {
            status = prNcHeader (conn, ncid, rodsArgs->noattr, ncInqOut);
        }
        if (rodsArgs->header + rodsArgs->dim + rodsArgs->var + 
          rodsArgs->subset > 1)
            printf 
              ("===========================================================\n");
        if (rodsArgs->dim == True) {
            status = prNcDimVar (conn, ncOpenInp->objPath, ncid, 
              rodsArgs->ascitime, ncInqOut);
        }
        if (rodsArgs->header + rodsArgs->dim + rodsArgs->var + 
          rodsArgs->subset + rodsArgs->subsetByVal > 1)
            printf 
              ("===========================================================\n");
        if (rodsArgs->var + rodsArgs->subset + rodsArgs->subsetByVal > 0) {
            status = prNcVarData (conn, ncOpenInp->objPath, ncid, 
              rodsArgs->ascitime, ncInqOut, ncVarSubset);
        }
        printf ("}\n");
    } else {
#ifdef NETCDF_API
        if (rodsArgs->var + rodsArgs->subset + rodsArgs->subsetByVal > 0) {
            status = dumpSubsetToFile (conn, ncid, rodsArgs->noattr, ncInqOut,
                ncVarSubset, rodsArgs->optionString);
        } else {
        /* output is a NETCDF file */
            status = dumpNcInqOutToNcFile (conn, ncid, rodsArgs->noattr, 
              ncInqOut, rodsArgs->optionString);
        }
#else
        return USER_OPTION_INPUT_ERR;
#endif
    }

    bzero (&ncCloseInp, sizeof (ncCloseInp_t));
    ncCloseInp.ncid = ncid;
    status1 = rcNcClose (conn, &ncCloseInp);
    if (status1 < 0) {
        /* nc_close on the server sometime returns -62 for opendap */
        rodsLog (LOG_NOTICE,
          "ncOperDataObjUtil: rcNcClose error for %s, status = %d", 
          ncOpenInp->objPath, status1);
        return 0;
    }
    return status;
}

int
initCondForNcOper (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset)
{
    int status = 0;

    if (ncOpenInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForNcOper: NULL ncOpenInp input");
        return (USER__NULL_INPUT_ERR);
    }

    bzero (ncOpenInp, sizeof (ncOpenInp_t));
    bzero (ncVarSubset, sizeof (ncVarSubset_t));
    if (rodsArgs == NULL) {
        return (0);
    }
    if (rodsArgs->option == True) {
#ifndef NETCDF_API
        rodsLog (LOG_ERROR,
          "initCondForNcOper: -o option cannot be used if client is built with NETCDF_CLIENT only");
        return USER_OPTION_INPUT_ERR;
#endif
        if (rodsArgs->recursive == True) {
            rodsLog (LOG_ERROR,
              "initCondForNcOper: -o and -r options cannot be used together");
            return USER_OPTION_INPUT_ERR;
        }
    }

    if ((rodsArgs->dim + rodsArgs->header + rodsArgs->var + 
      rodsArgs->option + rodsArgs->subset + rodsArgs->subsetByVal) == False) {
        rodsArgs->header = True;
    }

    if (rodsArgs->agginfo == True) {
        if (rodsArgs->writeFlag == True) {
            if (rodsArgs->resource == True) {
                addKeyVal (&ncOpenInp->condInput, DEST_RESC_NAME_KW, 
                  rodsArgs->resourceString);
            }
        } else if (rodsArgs->longOption == False) {
            rodsArgs->longOption = True;
        }
    }
#ifdef NETCDF4_API
    ncOpenInp->mode = NC_NOWRITE|NC_NETCDF4;
#else
    ncOpenInp->mode = NC_NOWRITE;
#endif
    addKeyVal (&ncOpenInp->condInput, NO_STAGING_KW, "");
    if (rodsArgs->var == True) {
        status = parseVarStrForSubset (rodsArgs->varStr, ncVarSubset);
        if (status < 0) return status;
#if 0
        i = 0;
        inLen = strlen (rodsArgs->varStr);
        inPtr = rodsArgs->varStr;
        while (getNextEleInStr (&inPtr, 
          &ncVarSubset->varName[i][LONG_NAME_LEN],
          &inLen, LONG_NAME_LEN) > 0) {
            ncVarSubset->numVar++;
            i++;
            if (ncVarSubset->numVar >= MAX_NUM_VAR) break;
        }
#endif
    }
    if (rodsArgs->subset == True || rodsArgs->subsetByVal) {
        status = parseSubsetStr (rodsArgs->subsetStr, ncVarSubset);
        if (status < 0) return status;
#if 0
        i = 0;
        inLen = strlen (rodsArgs->subsetStr);
        inPtr = rodsArgs->subsetStr;
        while (getNextEleInStr (&inPtr, 
         ncVarSubset->ncSubset[i].subsetVarName, 
         &inLen, LONG_NAME_LEN) > 0) {
            status = parseNcSubset (&ncVarSubset->ncSubset[i]);
            if (status < 0) return status;
            ncVarSubset->numSubset++;
            i++;
            if (ncVarSubset->numSubset >= MAX_NUM_VAR) break;
        }
#endif
    }
    return (0);
}

int
ncOperCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs, ncOpenInp_t *ncOpenInp, ncVarSubset_t *ncVarSubset)
{
    int status;
    int savedStatus = 0;
    collHandle_t collHandle;
    collEnt_t collEnt;
    char srcChildPath[MAX_NAME_LEN];

    if (srcColl == NULL) {
       rodsLog (LOG_ERROR,
          "ncOperCollUtil: NULL srcColl input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->recursive != True) {
        rodsLog (LOG_ERROR,
        "ncOperCollUtil: -r option must be used for getting %s collection",
         srcColl);
        return (USER_INPUT_OPTION_ERR);
    }

    if (rodsArgs->verbose == True) {
        fprintf (stdout, "C- %s:\n", srcColl);
    }

    status = rclOpenCollection (conn, srcColl, 0, &collHandle);
    if (status < 0) {
        rodsLog (LOG_ERROR,
          "ncOperCollUtil: rclOpenCollection of %s error. status = %d",
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

            status = ncOperDataObjUtil (conn, srcChildPath,
             myRodsEnv, rodsArgs, ncOpenInp, ncVarSubset);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "ncOperCollUtil: ncOperDataObjUtil failed for %s. status = %d",
                  srcChildPath, status);
                /* need to set global error here */
                savedStatus = status;
                status = 0;
            }
        } else if (collEnt.objType == COLL_OBJ_T) {
            ncOpenInp_t childNcOpen;
            childNcOpen = *ncOpenInp;
            if (collEnt.specColl.collClass != NO_SPEC_COLL) continue;
            status = ncOperCollUtil (conn, collEnt.collName, myRodsEnv,
              rodsArgs, &childNcOpen, ncVarSubset);
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
catAggInfo (rcComm_t *conn, char *srcColl)
{
    char aggInfoPath[MAX_NAME_LEN];
    int status;

    snprintf (aggInfoPath, MAX_NAME_LEN, "%s/%s", srcColl, 
      NC_AGG_INFO_FILE_NAME);

    status = catDataObj (conn, aggInfoPath);

    return status;
}

int
setAggInfo (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv,
rodsArguments_t *rodsArgs, ncOpenInp_t *ncOpenInp)
{
    int status;
    ncAggInfo_t *ncAggInfo = NULL;

    rstrcpy (ncOpenInp->objPath, srcColl, MAX_NAME_LEN);
    ncOpenInp->mode = NC_WRITE;
    status = rcNcGetAggInfo (conn, ncOpenInp, &ncAggInfo);
    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "setAggInfo: rcNcGetAggInfo error for %s", ncOpenInp->objPath);
    } else {
        freeAggInfo (&ncAggInfo);
    }
    return status;
}

