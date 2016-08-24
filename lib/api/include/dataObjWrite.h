#ifndef DATA_OBJ_WRITE_H__
#define DATA_OBJ_WRITE_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "rodsDef.h"

/* prototype for the client call */
/* rcDataObjWrite - Write the content of dataObjWriteInpBBuf to
 * an opened iRODS data object descriptor.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjWriteInp_t *dataObjWriteInp - Relevant items are:
 *      l1descInx - the iRODS data object descriptor to write.
 *      len - the number of bytes to write
 *   bytesBuf_t *dataObjWriteInpBBuf - the bytesBuf for the write input.
 *
 * OutPut -
 *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjWrite( rcComm_t *conn, openedDataObjInp_t *dataObjWriteInp, bytesBuf_t *dataObjWriteInpBBuf );

#endif
