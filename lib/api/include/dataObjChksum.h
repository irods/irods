#ifndef DATA_OBJ_CHKSUM_H__
#define DATA_OBJ_CHKSUM_H__

#include "rcConnect.h"
#include "dataObjInpOut.h"
#include "objInfo.h"

#ifdef __cplusplus
extern "C"
#endif
int rcDataObjChksum( rcComm_t *conn, dataObjInp_t *dataObjChksumInp, char **outChksum );

/* rcDataObjChksum - Chksum a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *	openFlags - should be set to O_RDONLY.
 *   condInput - FORCE_CHKSUM_KW - force checksum even if one exist.
 *		 CHKSUM_ALL_KW - checksum all replica.
 *		 VERIFY_CHKSUM_KW - verify the checksum value in icat.
 * 		 REPL_NUM_KW - the "value" gives the replica number of the
 *		   copy to checksum.
 * OutPut -
 *   char **outChksum - the chksum string
 *   return value - The status of the operation.
 */

#endif
