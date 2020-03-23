#ifndef IRODS_DATA_OBJECT_MODIFY_INFO_H
#define IRODS_DATA_OBJECT_MODIFY_INFO_H

#include "rcConnect.h"
#include "modDataObjMeta.h"

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
/// \user Client
///
/// \since 4.2.8
///
/// \param[in] comm  A pointer to a rcComm_t.
/// \param[in] input A pointer to a modDataObjMeta_t containing the information to modify.
///
/// \return An integer value
/// \retval 0        If the update was successful.
/// \retval non-zero Otherwise.
int rc_data_object_modify_info(rcComm_t* comm, modDataObjMeta_t* input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_DATA_OBJECT_MODIFY_INFO_H

