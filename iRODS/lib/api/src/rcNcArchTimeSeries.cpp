/* This is script-generated code.  */ 
/* See ncArchTimeSeries.h for a description of this API call.*/

#include "ncArchTimeSeries.h"

/* rcNcArchTimeSeries - Archive a time series data set given in objPath to
 * a aggregate collection.
 * Input - 
 *   rcComm_t *conn - The client connection handle.
 *   ncArchTimeSeriesInp_t *ncArchTimeSeriesInp - generic archive time series
 *   input. Relevant items are:
 *      objPath - the path of the time series data object to archive.
 *      aggCollection - target aggregate collection
 *      condInput - condition input.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource for the
 *            aggregate file.
 * OutPut -
 *   None
 * return value - The status of the operation.
 */

int
rcNcArchTimeSeries (rcComm_t *conn, ncArchTimeSeriesInp_t *ncArchTimeSeriesInp)
{
    int status;
    status = procApiRequest (conn, NC_ARCH_TIME_SERIES_AN, 
      ncArchTimeSeriesInp, NULL, (void **) NULL, NULL);

    return (status);
}
