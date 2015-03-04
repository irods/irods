/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjTruncate.h
 */

#ifndef DATA_OBJ_TRUNCATE_HPP
#define DATA_OBJ_TRUNCATE_HPP

/* This is a Object File I/O API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"
#include "fileTruncate.hpp"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_TRUNCATE rsDataObjTruncate
/* prototype for the server handler */
int
rsDataObjTruncate( rsComm_t *rsComm, dataObjInp_t *dataObjInp );
int
_rsDataObjTruncate( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                    dataObjInfo_t *dataObjInfoHead );
int
dataObjTruncateS( rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp,
                  dataObjInfo_t *dataObjInfo );
int
l3Truncate( rsComm_t *rsComm, dataObjInp_t *dataObjTruncateInp,
            dataObjInfo_t *dataObjInfo );
#else
#define RS_DATA_OBJ_TRUNCATE NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* prototype for the client call */
int
rcDataObjTruncate( rcComm_t *conn, dataObjInp_t *dataObjInp );

/* rcDataObjTruncate - Truncate a iRODS data object.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInp_t *dataObjInp - generic dataObj input. Relevant items are:
 *	objPath - the path of the data object.
 *      dataSize - the size to truncate to
 *
 * OutPut -
 *   return value - The status of the operation.
 */

#ifdef __cplusplus
}
#endif
#endif	/* DATA_OBJ_TRUNCATE_H */
