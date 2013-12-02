/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifndef windows_platform
#include <sys/time.h>
#endif
#include "rodsPath.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"
#include "miscUtil.h"
#include "ncarchUtil.h"

int
ncarchUtil (rcComm_t *conn, rodsEnv *myRodsEnv, rodsArguments_t *myRodsArgs,
rodsPathInp_t *rodsPathInp)
{
    int status; 
    ncArchTimeSeriesInp_t ncArchTimeSeriesInp;


    if (rodsPathInp == NULL) {
	return (USER__NULL_INPUT_ERR);
    }

    initCondForNcarch (myRodsEnv, myRodsArgs, &ncArchTimeSeriesInp);

    if (rodsPathInp->numSrc != 1) {
        rodsLog (LOG_ERROR,
         "ncarchUtil: Number of input path must be 2");
         status = USER_INPUT_PATH_ERR;
    }

    rstrcpy (ncArchTimeSeriesInp.objPath, rodsPathInp->srcPath[0].outPath, 
      MAX_NAME_LEN);
    rstrcpy (ncArchTimeSeriesInp.aggCollection, 
      rodsPathInp->destPath[0].outPath, MAX_NAME_LEN);

    status = rcNcArchTimeSeries (conn, &ncArchTimeSeriesInp);

    if (status < 0) {
        rodsLogError (LOG_ERROR, status,
         "ncarchUtil: rcNcArchTimeSeries error for %s to %s",
         ncArchTimeSeriesInp.objPath, ncArchTimeSeriesInp.aggCollection);
    }

    return (status);
}

int
initCondForNcarch (rodsEnv *myRodsEnv, rodsArguments_t *rodsArgs, 
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp)
{

    if (ncArchTimeSeriesInp == NULL) {
       rodsLog (LOG_ERROR,
          "initCondForNcarch: NULL ncArchTimeSeriesInp input");
        return (USER__NULL_INPUT_ERR);
    }

    memset (ncArchTimeSeriesInp, 0, sizeof (ncArchTimeSeriesInp_t));

    if (rodsArgs == NULL) {
	return (0);
    }

    if (rodsArgs->newFlag == True) {
        addKeyVal (&ncArchTimeSeriesInp->condInput, NEW_NETCDF_ARCH_KW, 
          rodsArgs->startTimeInxStr);
    }
    if (rodsArgs->sizeFlag == True) {
        ncArchTimeSeriesInp->fileSizeLimit = rodsArgs->size;
    }

    return (0);
}

