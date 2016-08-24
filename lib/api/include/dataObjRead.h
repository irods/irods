#ifndef DATA_OBJ_READ_H__
#define DATA_OBJ_READ_H__

#include "rodsDef.h"
#include "dataObjInpOut.h"
#include "rcConnect.h"
#include "fileRead.h"


/* prototype for the client call */
/* rcDataObjRead - Read an opened iRODS data object descriptor.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *fileReadInp - Relevant items are:
 *      l1descInx - the iRODS data object descriptor to read.
 *      len - the number of bytes to read
 *
 * OutPut -
 *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
 *   bytesBuf_t *dataObjReadOutBBuf - the bytesBuf for the read output.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcDataObjRead( rcComm_t *conn, openedDataObjInp_t *dataObjReadInp, bytesBuf_t *dataObjReadOutBBuf );

#endif
