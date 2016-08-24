#ifndef MOD_DATA_OBJ_META_H__
#define MOD_DATA_OBJ_META_H__

#include "rcConnect.h"
#include "objInfo.h"

typedef struct {
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;
} modDataObjMeta_t;
#define ModDataObjMeta_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"


/* rcModDataObjMeta - Modify the metadata of a iRODS dataObject.
 * Input -
 *   rcComm_t *conn - The client connection handle.
 *   dataObjInfo_t *dataObjInfo - the dataObjInfo to register
 *
 * OutPut -
 *   int status - status of the operation.
 */
#ifdef __cplusplus
extern "C"
#endif
int rcModDataObjMeta( rcComm_t *conn, modDataObjMeta_t *modDataObjMetaInp );

#endif
