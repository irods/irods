#ifndef DATA_OBJ_LSEEK_H__
#define DATA_OBJ_LSEEK_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "rodsType.h"
#include "fileLseek.h"


/* prototype for the client call */
/* rcDataObjLseek - Lseek an opened iRODS data object descriptor.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   fileLseekInp_t *dataObjLseekInp - Relevant items are:
 *      l1descInx - the iRODS data object descriptor to lseek.
 *	offset - the offset
 *	whence - SEEK_SET, SEEK_CUR and SEEK_END
 *
 * OutPut -
 *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
 *   fileLseekOut_t **dataObjLseekOut. Relevant items are:
 *	offset - the new offset
 */

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjLseek( rcComm_t *conn, openedDataObjInp_t *dataObjLseekInp, fileLseekOut_t **dataObjLseekOut );

#endif
