/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* ncArchTimeSeries.h
 */

#ifndef NC_ARCH_TIME_SERIES_H
#define NC_ARCH_TIME_SERIES_H

/* This is a NETCDF API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"
#include "ncInq.h"
#include "ncGetAggInfo.h"

#define READ_TIME_SIZE	100
#define ARCH_FILE_SIZE  (1024*1024*1024)   /* archival file size */
#define ONE_MILLION	(1024*1024)

typedef struct {
    rodsLong_t fileSizeLimit;
    char objPath[MAX_NAME_LEN];	/* The time series netcdf path to archive */ 
    char aggCollection[MAX_NAME_LEN];  /* target aggregate collection */
    keyValPair_t condInput;
} ncArchTimeSeriesInp_t;

#define NcArchTimeSeriesInp_PI "double fileSizeLimit; str objPath[MAX_NAME_LEN]; str aggCollection[MAX_NAME_LEN]; struct KeyValPair_PI;"

#if defined(RODS_SERVER) && defined(NETCDF_API)
#define RS_NC_ARCH_TIME_SERIES rsNcArchTimeSeries
/* prototype for the server handler */
int
rsNcArchTimeSeries (rsComm_t *rsComm, 
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp);
int
_rsNcArchTimeSeries (rsComm_t *rsComm,
ncArchTimeSeriesInp_t *ncArchTimeSeriesInp);
int
getTimeInxForArch (rsComm_t *rsComm, int ncid, ncInqOut_t *ncInqOut,
int dimInx, int varInx, unsigned int prevEndTime, rodsLong_t *startTimeInx);
int
archPartialTimeSeries (rsComm_t *rsComm, ncInqOut_t *ncInqOut,
ncAggInfo_t *ncAggInfo, int srcNcid, int timeVarInx, char *aggCollection,
rescGrpInfo_t *myRescGrpInfo, rodsLong_t startTimeInx, rodsLong_t endTimeInx,
rodsLong_t archFileSize);
#else
#define RS_NC_ARCH_TIME_SERIES NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
/* rcNcArchTimeSeries - Archive a time series data set given in objPath to
 * a aggregate collection.
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncArchTimeSeriesInp_t *ncArchTimeSeriesInp - generic archive time series
 *   input. Relevant items are:
 *	objPath - the path of the time series data object to archive.
 *      aggCollection - target aggregate collection
 *	condInput - condition input.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource for the
 *            aggregate file.
 * OutPut - 
 *   None
 * return value - The status of the operation. 
 */

/* prototype for the client call */
int
rcNcArchTimeSeries (rcComm_t *conn, ncArchTimeSeriesInp_t *ncArchTimeSeriesInp);

#ifdef  __cplusplus
}
#endif

#endif	/* NC_ARCH_TIME_SERIES_H */
