/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "phybunUtil.h"
#include "miscUtil.h"

int
phybunUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    rodsPath_t *collPath;
    structFileExtAndRegInp_t phyBundleCollInp;

    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    status = initCondForPhybunOpr (myRodsEnv, myRodsArgs, &phyBundleCollInp, 
      rodsPathInp);

    if (status < 0) return status;
    
    for (i = 0; i < rodsPathInp->numSrc; i++) {
        collPath = &rodsPathInp->srcPath[i];	/* iRODS Collection */

        getRodsObjType (conn, collPath);

	if (collPath->objType !=  COLL_OBJ_T) {
	    rodsLogError (LOG_ERROR, status,
             "phybunUtil: input path %s is not a collection", 
	      collPath->outPath);
	    return USER_INPUT_PATH_ERR;
	}

	rstrcpy (phyBundleCollInp.collection, collPath->outPath,
	  MAX_NAME_LEN);

	status = rcPhyBundleColl (conn, &phyBundleCollInp);

	if (status < 0) {
	    rodsLogError (LOG_ERROR, status,
             "phybunUtil: opr error for %s, status = %d", 
	      collPath->outPath, status);
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
initCondForPhybunOpr (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
structFileExtAndRegInp_t *phyBundleCollInp, 
rodsPathInp_t *rodsPathInp)
{
    if (phyBundleCollInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForPhybunOpr: NULL phyBundleCollInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (phyBundleCollInp, 0, sizeof (structFileExtAndRegInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->resource == True) {
        if (rodsArgs->resourceString == NULL) {
            rodsLog (LOG_ERROR,
              "initCondForPhybunOpr: NULL resourceString error");
            return (USER__NULL_INPUT_ERR);
        } else {
            addKeyVal (&phyBundleCollInp->condInput, 
	      DEST_RESC_NAME_KW, rodsArgs->resourceString);
            addKeyVal (&phyBundleCollInp->condInput, RESC_NAME_KW,
              rodsArgs->resourceString);
        }
    } else {
       rodsLog (LOG_ERROR,
          "initCondForPhybunOpr: A -Rresource must be input");
        return (USER__NULL_INPUT_ERR);
    }

    return (0);
}

