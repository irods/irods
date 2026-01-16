#ifndef IRODS_UNREG_DATA_OBJ_H
#define IRODS_UNREG_DATA_OBJ_H

#include "irods/rcConnect.h"
#include "irods/objInfo.h"

typedef struct {
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *condInput;
} unregDataObj_t;

#define UnregDataObj_PI "struct *DataObjInfo_PI; struct *KeyValPair_PI;"

#ifdef __cplusplus
extern "C" {
#endif

/// Unregister all replicas of an data object in the iRODS catalog without removing the underlying data in storage.
///
/// \param[in] conn A pointer to \p RcComm.
/// \param[in] unregDataObjInp \p unregDataObj_t which minimally requires a \p DataObjInfo with a valid objPath.
///
/// \return An integer representing an iRODS error code.
/// \retval 0 on success.
/// \retval <0 on failure.
int rcUnregDataObj( rcComm_t *conn, unregDataObj_t *unregDataObjInp );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_UNREG_DATA_OBJ_H
