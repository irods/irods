/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjChksum.h
 */

#ifndef DATA_OBJ_CHKSUM_HPP
#define DATA_OBJ_CHKSUM_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"
#include "fileChksum.hpp"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_CHKSUM rsDataObjChksum
/* prototype for the server handler */
int
rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjChksumInp,
                 char **outChksum );
int
_rsDataObjChksum( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                  char **outChksumStr, dataObjInfo_t **dataObjInfoHead );
int
dataObjChksumAndRegInfo( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                         char **outChksumStr );
int
verifyDatObjChksum( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                    char **outChksumStr );
#else
#define RS_DATA_OBJ_CHKSUM NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcDataObjChksum( rcComm_t *conn, dataObjInp_t *dataObjChksumInp,
                 char **outChksum );

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

#ifdef __cplusplus
}
#endif
#endif	/* DATA_OBJ_CHKSUM_H */
