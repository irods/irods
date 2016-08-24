#ifndef DATA_OBJ_GET_H__
#define DATA_OBJ_GET_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "dataObjInpOut.h"


/* prototype for the client call */
/* rcDataObjGet - Get (download) a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      numThreads - Number of threads to use. NO_THREADING ==> no threading,
 *         0 ==> server will decide (default), >0 ==> number of threads.
 *      openFlags - should be set to O_RDONLY.
 *      condInput - conditional Input
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              download.
 *          VERIFY_CHKSUM_KW - verify the checksum of the download file.
 *   return value - The status of the operation.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjGet( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath );
int _rcDataObjGet( rcComm_t *conn, dataObjInp_t *dataObjInp, portalOprOut_t **portalOprOut, bytesBuf_t *dataObjOutBBuf );

#endif
