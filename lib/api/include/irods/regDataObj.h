#ifndef IRODS_REG_DATA_OBJ_H
#define IRODS_REG_DATA_OBJ_H

#include "irods/rcConnect.h"
#include "irods/objInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Register a new data object in the iRODS catalog.
///
/// \param[in] conn A pointer to \p RcComm.
/// \param[in] dataObjInfo \p DataObjInfo with the information to use in the new catalog entry.
/// \param[out] outDataObjInfo \p DataObjInfo with the data ID generated for the registration.
///
/// \return An integer representing an iRODS error code.
/// \retval 0 on success.
/// \retval <0 on failure.
int rcRegDataObj( rcComm_t *conn, dataObjInfo_t *dataObjInfo, dataObjInfo_t **outDataObjInfo );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_REG_DATA_OBJ_H
