#ifndef IRODS_DATA_OBJECT_MODIFY_INFO_H
#define IRODS_DATA_OBJECT_MODIFY_INFO_H

/// \file

#include "irods/modDataObjMeta.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Modifies catalog information about a data object or replica.
///
/// The following keywords are supported:
/// - DATA_COMMENTS_KW
/// - DATA_EXPIRY_KW
/// - DATA_MODIFY_KW
/// - DATA_TYPE_KW
///
/// \param[in] _comm  A pointer to a RcComm.
/// \param[in] _input A pointer to a ModDataObjMetaInp containing the information to modify.
///
/// \return An integer. 
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.8
int rc_data_object_modify_info(struct RcComm* _comm, struct ModDataObjMetaInp* _input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_DATA_OBJECT_MODIFY_INFO_H

