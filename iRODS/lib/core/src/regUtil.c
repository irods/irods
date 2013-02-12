/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "regUtil.h"
#include "miscUtil.h"

int
regUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int i;
    int status; 
    int savedStatus = 0;
    rodsPath_t *destPath, *srcPath;
    dataObjInp_t dataObjOprInp;

    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForReg (myRodsEnv, myRodsArgs, &dataObjOprInp, rodsPathInp);

    for (i = 0; i < rodsPathInp->numSrc; i++) {
        destPath = &rodsPathInp->destPath[i];	/* iRODS path */
        srcPath = &rodsPathInp->srcPath[i];	/* file Path */

        getRodsObjType (conn, destPath);

	if (destPath->objState == EXIST_ST &&
	 myRodsArgs->force == False &&
	 myRodsArgs->mountCollection == False &&
	 myRodsArgs->regRepl != True) {
	    rodsLog (LOG_ERROR,
	      "regUtil: iRODSPath %s already exist", destPath->outPath);
	    return (CAT_NAME_EXISTS_AS_DATAOBJ);
	}

        if (myRodsArgs->collection == False && myRodsArgs->checksum == True) {
            status = rcChksumLocFile (srcPath->outPath, REG_CHKSUM_KW,
              &dataObjOprInp.condInput);
            if (status < 0) {
                rodsLogError (LOG_ERROR, status,
                  "regUtil: rcChksumLocFile error for %s, status = %d",
                  srcPath, status);
                return (status);
            }
        }
        else if (myRodsArgs->collection == False && myRodsArgs->verifyChecksum == True) {
            addKeyVal (&dataObjOprInp.condInput, VERIFY_CHKSUM_KW, "");
        }

        addKeyVal (&dataObjOprInp.condInput, FILE_PATH_KW, srcPath->outPath);
	rstrcpy (dataObjOprInp.objPath, destPath->outPath, MAX_NAME_LEN);

	status = rcPhyPathReg (conn, &dataObjOprInp);

	/* XXXX may need to return a global status */
	if (status < 0) {
	    rodsLogError (LOG_ERROR, status,
             "regUtil: reg error for %s, status = %d", 
	      destPath->outPath, status);
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
initCondForReg (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
dataObjInp_t *dataObjOprInp, rodsPathInp_t *rodsPathInp)
{
    if (dataObjOprInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForReg: NULL dataObjOprInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (dataObjOprInp, 0, sizeof (dataObjInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->dataType == True) {
        if (rodsArgs->dataTypeString == NULL) {
	    addKeyVal (&dataObjOprInp->condInput, DATA_TYPE_KW, "generic");
        } else {
	    addKeyVal (&dataObjOprInp->condInput, DATA_TYPE_KW, 
	      rodsArgs->dataTypeString);
        }
    } else {
	addKeyVal (&dataObjOprInp->condInput, DATA_TYPE_KW, "generic");
    }

    if (rodsArgs->collection == True) {
            addKeyVal (&dataObjOprInp->condInput, COLLECTION_KW, "");
    }

    if (rodsArgs->force == True) {
        addKeyVal (&dataObjOprInp->condInput, FORCE_FLAG_KW, "");
    }

    if (rodsArgs->resource == True) {
        if (rodsArgs->resourceString == NULL) {
            rodsLog (LOG_ERROR,
              "initCondForReg: NULL resourceString error");
            return (USER__NULL_INPUT_ERR);
        } else {
            addKeyVal (&dataObjOprInp->condInput, DEST_RESC_NAME_KW,
              rodsArgs->resourceString);
        }
    } else if (myRodsEnv != NULL && strlen (myRodsEnv->rodsDefResource) > 0) {
        addKeyVal (&dataObjOprInp->condInput, DEST_RESC_NAME_KW,
          myRodsEnv->rodsDefResource);
    } 

    if (rodsArgs->rescGroup == True) {
	if (rodsArgs->resource != True) {
            rodsLog (LOG_ERROR,
            "initCondForReg: rescGroup must be input together with -Rresource");
            return (USER__NULL_INPUT_ERR);
        } else if (rodsArgs->rescGroupString == NULL) {
            rodsLog (LOG_ERROR,
              "initCondForReg: NULL rescGroupString error");
            return (USER__NULL_INPUT_ERR);
        } else {
            addKeyVal (&dataObjOprInp->condInput, RESC_GROUP_NAME_KW,
              rodsArgs->rescGroupString);
        }
    }
    if (rodsArgs->regRepl == True) {
        addKeyVal (&dataObjOprInp->condInput, REG_REPL_KW, "");
    }

    if (rodsArgs->excludeFile == True) {
        if (rodsArgs->excludeFileString == NULL) {
            rodsLog (LOG_ERROR, "initCondForReg: NULL excludeFileString error");
            return (USER__NULL_INPUT_ERR);
        }
        else {
            addKeyVal (&dataObjOprInp->condInput, EXCLUDE_FILE_KW,
                       rodsArgs->excludeFileString);
        }
    }

    return (0);
}

