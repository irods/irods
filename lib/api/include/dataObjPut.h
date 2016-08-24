#ifndef DATA_OBJ_PUT_H__
#define DATA_OBJ_PUT_H__

#include "rcConnect.h"
#include "rodsDef.h"
#include "procApiRequest.h"
#include "dataObjInpOut.h"

/* prototype for the client call */
/* rcDataObjPut - Put (upload) a local file to iRODS.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *      objPath - the path of the data object.
 *      numThreads - Number of threads to use. NO_THREADING ==> no threading,
 *         0 ==> server will decide (default), >0 ==> number of threads.
 *      openFlags - should be set to O_WRONLY.
 *      condInput - conditional Input
 *          FORCE_FLAG_KW - overwrite an existing data object
 *          ALL_KW - update all copies.
 *          DATA_TYPE_KW - "value" = the data type of the file.
 *          REPL_NUM_KW  - "value" = The replica number of the copy to
 *              upload.
 *          FILE_PATH_KW - "value" = the physical path of the
 *              destination file. Vaild only if O_CREAT is on.
 *          DEST_RESC_NAME_KW - "value" = The destination Resource. Vaild
 *              only if O_CREAT is on.
 *   return value - The status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp, char *locFilePath );
int _rcDataObjPut( rcComm_t *conn, dataObjInp_t *dataObjInp, bytesBuf_t *dataObjInpBBuf, portalOprOut_t **portalOprOut );
#endif
