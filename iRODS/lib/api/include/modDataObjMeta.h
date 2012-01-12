/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* modDataObjMeta.h
 */

#ifndef MOD_DATA_OBJ_META_H
#define MOD_DATA_OBJ_META_H

/* This is Object File I/O type API call */

#include "rods.h"
#include "rcMisc.h"
#include "procApiRequest.h"
#include "apiNumber.h"
#include "initServer.h"
#include "dataObjInpOut.h"

typedef struct {
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;
} modDataObjMeta_t;

#define ModDataObjMeta_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"

#if defined(RODS_SERVER)
#define RS_MOD_DATA_OBJ_META rsModDataObjMeta
/* prototype for the server handler */
int
rsModDataObjMeta (rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp);
int
_rsModDataObjMeta (rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp);
#else
#define RS_MOD_DATA_OBJ_META NULL
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/* prototype for the client call */
int
rcModDataObjMeta (rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp);

/* rcModDataObjMeta - Modify the metadata of a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInfo_t *dataObjInfo - the dataObjInfo to register
 *
 * OutPut -
 *   int status - status of the operation.
 */

int
clearModDataObjMetaInp (modDataObjMeta_t *modDataObjMetaInp);
#ifdef  __cplusplus
}
#endif

#endif	/* MOD_DATA_OBJ_META_H */
