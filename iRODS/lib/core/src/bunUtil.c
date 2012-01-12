/*** Copyright (c), The BunOprents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "bunUtil.h"
#include "miscUtil.h"

int
bunUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    rodsPath_t *collPath, *structFilePath;
    structFileExtAndRegInp_t structFileExtAndRegInp;

    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    status = initCondForBunOpr (myRodsEnv, myRodsArgs, &structFileExtAndRegInp, 
      rodsPathInp);

    if (status < 0) return status;
    
    for (i = 0; i < rodsPathInp->numSrc; i++) {
        collPath = &rodsPathInp->destPath[i];	/* iRODS Collection */
        structFilePath = &rodsPathInp->srcPath[i];	/* iRODS StructFile */

        getRodsObjType (conn, collPath);

	rstrcpy (structFileExtAndRegInp.collection, collPath->outPath,
	      MAX_NAME_LEN);
        rstrcpy (structFileExtAndRegInp.objPath, structFilePath->outPath,
          MAX_NAME_LEN);

	if (myRodsArgs->extract == True) {		/* -x */
	    if (myRodsArgs->condition == True) {
		rodsLog (LOG_ERROR,
                  "bunUtil: cannot use -x and -c at the same time");
		return -1;
	    }
	    status = rcStructFileExtAndReg (conn, &structFileExtAndRegInp);
	} else if (myRodsArgs->condition == True) {  /* -c - create */
	    status = rcStructFileBundle (conn, &structFileExtAndRegInp);
	} else {
            rodsLog (LOG_ERROR,
              "bunUtil: -x or -c must be specified");
            return -1;
	}

	/* XXXX may need to return a global status */
	if (status < 0) {
	    if (status == SYS_CACHE_STRUCT_FILE_RESC_ERR && 
	      myRodsArgs->condition == True) {
                rodsLogError (LOG_ERROR, status,
                 "bunUtil: A resc must be entered for non-existing structFile");
	    } else {
	        rodsLogError (LOG_ERROR, status,
                 "bunUtil: opr error for %s, status = %d", 
	          collPath->outPath, status);
                savedStatus = status;
	    }
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
initCondForBunOpr (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
structFileExtAndRegInp_t *structFileExtAndRegInp, 
rodsPathInp_t *rodsPathInp)
{
    if (structFileExtAndRegInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForBunOpr: NULL structFileExtAndRegInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (structFileExtAndRegInp, 0, sizeof (structFileExtAndRegInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->dataType == True) {
        if (rodsArgs->dataTypeString != NULL) {
	    if (strcmp (rodsArgs->dataTypeString, "t") == 0 ||
	      strcmp (rodsArgs->dataTypeString, "tar") == 0) {
                addKeyVal (&structFileExtAndRegInp->condInput, DATA_TYPE_KW,
                  "tar file");
	    } else {
                addKeyVal (&structFileExtAndRegInp->condInput, DATA_TYPE_KW,
                  rodsArgs->dataTypeString);
	    }
        }
    } else if (rodsArgs->condition == True) {	/* -c */
        addKeyVal (&structFileExtAndRegInp->condInput, DATA_TYPE_KW,
          "tar file");

    }

    if (rodsArgs->resource == True) {
        if (rodsArgs->resourceString == NULL) {
            rodsLog (LOG_ERROR,
              "initCondForBunOpr: NULL resourceString error");
            return (USER__NULL_INPUT_ERR);
        } else {
            addKeyVal (&structFileExtAndRegInp->condInput, 
	      DEST_RESC_NAME_KW, rodsArgs->resourceString);
	    /* RESC_NAME_KW is need for unbundle. DEST_RESC_NAME_KW or
	     * DEF_RESC_NAME_KW are neede for bundle */
            addKeyVal (&structFileExtAndRegInp->condInput, RESC_NAME_KW,
              rodsArgs->resourceString);
        }
    } else if (myRodsEnv != NULL && strlen (myRodsEnv->rodsDefResource) > 0) {
        /* use rodsDefResource but set the DEF_RESC_NAME_KW instead.
         * Used by dataObjCreate. Will only make a new replica only if
         * DEST_RESC_NAME_KW is set */
        addKeyVal (&structFileExtAndRegInp->condInput, DEF_RESC_NAME_KW,
          myRodsEnv->rodsDefResource);
    } 

    if (rodsArgs->force == True) {
        addKeyVal (&structFileExtAndRegInp->condInput, FORCE_FLAG_KW, "");
    }

    if (rodsArgs->bulk == True) {
        addKeyVal (&structFileExtAndRegInp->condInput, BULK_OPR_KW, "");
    }

    return (0);
}

