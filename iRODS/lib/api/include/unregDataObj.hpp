/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regDataObj.h
 */

#ifndef UNREG_DATA_OBJ_HPP
#define UNREG_DATA_OBJ_HPP

/* This is Object File I/O type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "dataObjInpOut.hpp"

typedef struct {
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput;
} unregDataObj_t;

#define UnregDataObj_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_UNREG_DATA_OBJ rsUnregDataObj
/* prototype for the server handler */
int
rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp );
int
_rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp );
#else
#define RS_UNREG_DATA_OBJ NULL
#endif

/* prototype for the client call */
int
rcUnregDataObj( rcComm_t *conn, unregDataObj_t *unregDataObjInp );

/* rcUnregDataObj - Unregister a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   unregDataObj_t *unregDataObjInp - the dataObjInfo to unregister
 *
 * OutPut -
 *   int status - status of the operation.
 */

//int
//clearUnregDataObj( unregDataObj_t *unregDataObjInp );

#endif	/* UNREG_DATA_OBJ_H */
