/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "miscUtil.h"
#include "rodsLog.h"
#include "rmUtil.h"

int
rmUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    collInp_t collInp;
    dataObjInp_t dataObjInp;


    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForRm (myRodsEnv, myRodsArgs, &dataObjInp, &collInp);

    for (i = 0; i < rodsPathInp->numSrc; i++) {
	if (rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T) {
	    getRodsObjType (conn, &rodsPathInp->srcPath[i]);
	    if (rodsPathInp->srcPath[i].objState == NOT_EXIST_ST) {
                rodsLog (LOG_ERROR,
                  "rmUtil: srcPath %s does not exist",
                  rodsPathInp->srcPath[i].outPath);
		savedStatus = USER_INPUT_PATH_ERR;
		continue;
	    }
	}

	if (rodsPathInp->srcPath[i].objType == DATA_OBJ_T) {
	    status = rmDataObjUtil (conn, rodsPathInp->srcPath[i].outPath, 
	     myRodsEnv, myRodsArgs, &dataObjInp);
	} else if (rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T) {
	    status = rmCollUtil (conn, rodsPathInp->srcPath[i].outPath,
              myRodsEnv, myRodsArgs, &dataObjInp, &collInp);
	} else {
	    /* should not be here */
	    rodsLog (LOG_ERROR,
	     "rmUtil: invalid rm objType %d for %s", 
	     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath);
	    return (USER_INPUT_PATH_ERR);
	}
	/* XXXX may need to return a global status */
	if (status < 0) {
	    rodsLogError (LOG_ERROR, status,
             "rmUtil: rm error for %s, status = %d", 
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
rmDataObjUtil (rcComm_t *conn, char *srcPath, 
rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
dataObjInp_t *dataObjInp)
{
    int status;
    struct timeval startTime, endTime;
 
    if (srcPath == NULL) {
       rodsLog (LOG_ERROR,
          "rmDataObjUtil: NULL srcPath input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->verbose == True) {
        (void) gettimeofday(&startTime, (struct timezone *)0);
    }

    rstrcpy (dataObjInp->objPath, srcPath, MAX_NAME_LEN);

    status = rcDataObjUnlink (conn, dataObjInp);

    if (status >= 0 && rodsArgs->verbose == True) {
        (void) gettimeofday(&endTime, (struct timezone *)0);
        printTiming (conn, dataObjInp->objPath, -1, NULL,
         &startTime, &endTime);
    }

    return (status);
}

int
initCondForRm (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
dataObjInp_t *dataObjInp, collInp_t *collInp)
{

#ifdef _WIN32
	struct _timeb timebuffer;
#endif

    if (dataObjInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForRm: NULL dataObjInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (dataObjInp, 0, sizeof (dataObjInp_t));
    memset (collInp, 0, sizeof (collInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->unmount == True) {
	dataObjInp->oprType = collInp->oprType = UNREG_OPR;
    }

    if (rodsArgs->force == True) { 
        addKeyVal (&dataObjInp->condInput, FORCE_FLAG_KW, "");
        addKeyVal (&collInp->condInput, FORCE_FLAG_KW, "");
    }

    if (rodsArgs->replNum == True) {
        addKeyVal (&dataObjInp->condInput, REPL_NUM_KW, 
	  rodsArgs->replNumValue);
	if (rodsArgs->recursive == True) {
            rodsLog (LOG_NOTICE,
            "initCondForRm: -n option is only for dataObj removal. It will be ignored for collection removal");
	}
    }

    if (rodsArgs->recursive == True) {
        addKeyVal (&dataObjInp->condInput, RECURSIVE_OPR__KW, "");
        addKeyVal (&collInp->condInput, RECURSIVE_OPR__KW, "");
    }


    /* XXXXX need to add -u register cond */

    dataObjInp->openFlags = O_RDONLY;

    seedRandom ();
#if 0
#ifdef _WIN32
	_ftime64( &timebuffer );
	srand(timebuffer.time);
#else

    srandom((unsigned int) time(0) % getpid());
#endif
#endif

    return (0);
}

int
rmCollUtil (rcComm_t *conn, char *srcColl, rodsEnv *myRodsEnv, 
rodsArguments_t *rodsArgs, dataObjInp_t *dataObjInp, collInp_t *collInp)
{
    int status;

    if (srcColl == NULL) {
       rodsLog (LOG_ERROR,
          "rmCollUtil: NULL srcColl input");
        return (USER__NULL_INPUT_ERR);
    }

    if (rodsArgs->recursive != True) {
        rodsLog (LOG_ERROR,
        "rmCollUtil: -r option must be used for collection %s",
         srcColl);
        return (USER_INPUT_OPTION_ERR);
    }

    rstrcpy (collInp->collName, srcColl, MAX_NAME_LEN);
    status = rcRmColl (conn, collInp, rodsArgs->verbose);

    return (status);
}

int
mvDataObjToTrash (rcComm_t *conn, dataObjInp_t *dataObjInp)
{
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;

    status = mkTrashPath (conn, dataObjInp, trashPath);

    if (status < 0) {
        return (status);
    }

    memset (&dataObjRenameInp, 0, sizeof (dataObjRenameInp));

    dataObjRenameInp.srcDataObjInp.oprType =
      dataObjRenameInp.destDataObjInp.oprType = RENAME_DATA_OBJ;

    rstrcpy (dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy (dataObjRenameInp.srcDataObjInp.objPath, dataObjInp->objPath, 
      MAX_NAME_LEN);

    status = rcDataObjRename (conn, &dataObjRenameInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "mvDataObjToTrash: rcDataObjRename error for %s",
          dataObjRenameInp.destDataObjInp.objPath);
        return (status);
    }

    return (status);
}

int
mkTrashPath (rcComm_t *conn, dataObjInp_t *dataObjInp, char *trashPath)
{
    int status;
    char *tmpStr;
    char startTrashPath[MAX_NAME_LEN];
    char destTrashColl[MAX_NAME_LEN], myFile[MAX_NAME_LEN];

    /* skip the zone */
    tmpStr = strstr (dataObjInp->objPath + 1, "/");

    if (tmpStr == NULL) {
        rodsLog (LOG_ERROR,
          "mkTrashPath: Input obj path %s error", dataObjInp->objPath);
        return (USER_INPUT_PATH_ERR);
    }

    *tmpStr = '\0';     /* take out the / */

    tmpStr ++;

    sprintf (trashPath, "%s/trash/%s.%d", dataObjInp->objPath, tmpStr,
      (uint) random ());

    if (strncmp (tmpStr, "home/", 5) == 0) {
        /* from home collection */
        sprintf (startTrashPath, "%s/trash/home", dataObjInp->objPath);
    } else {
        sprintf (startTrashPath, "%s/trash", dataObjInp->objPath);
    }

    *(tmpStr - 1) = '/';        /* put back the / */

    if ((status = splitPathByKey (trashPath, destTrashColl, myFile, '/')) < 0) {
        rodsLog (LOG_ERROR,
          "mkTrashPath: splitPathByKey error for %s ", trashPath);
        return (USER_INPUT_PATH_ERR);
    }

    status = mkCollR (conn, startTrashPath, destTrashColl);

    if (status < 0) {
        rodsLog (LOG_ERROR,
          "mkTrashPath: mkCollR error for startPath %s, destPath %s ",
          startTrashPath, destTrashColl);
    }

    return (status);
}

int
mvCollToTrash (rcComm_t *conn, dataObjInp_t *dataObjInp)
{
    int status;
    char trashPath[MAX_NAME_LEN];
    dataObjCopyInp_t dataObjRenameInp;

    status = mkTrashPath (conn, dataObjInp, trashPath);

    if (status < 0) {
        return (status);
    }

    memset (&dataObjRenameInp, 0, sizeof (dataObjRenameInp));

    dataObjRenameInp.srcDataObjInp.oprType =
      dataObjRenameInp.destDataObjInp.oprType = RENAME_COLL;

    rstrcpy (dataObjRenameInp.destDataObjInp.objPath, trashPath, MAX_NAME_LEN);
    rstrcpy (dataObjRenameInp.srcDataObjInp.objPath, dataObjInp->objPath,
      MAX_NAME_LEN);

    status = rcDataObjRename (conn, &dataObjRenameInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
          "mvCollToTrash: rcDataObjRename error for %s",
          dataObjRenameInp.destDataObjInp.objPath);
        return (status);
    }

    return (status);
}


