#ifndef IRODS_ATOMIC_APPLY_ACL_OPERATIONS_H
#define IRODS_ATOMIC_APPLY_ACL_OPERATIONS_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Executes a list of ACL operations on a single data object or collection atomically.
///
/// Sequentially executes all \p operations on \p logical_path as a single transaction. If an
/// error occurs, all updates are rolled back and an error is returned. \p json_output will
/// contain specific information about the error.
///
/// \p json_input must have the following JSON structure:
/// \code{.js}
/// {
///   "logical_path": string,
///   "operations": [
///     {
///       "entity_name": string,
///       "acl": string
///     }
///   ]
/// }
/// \endcode
///
/// \p logical_path must be an absolute path to a data object or collection.
///
/// \p operations is the list of ACL operations to execute atomically. They will be executed in order.
///
/// \p entity_name is the name of the user or group for which the ACL is being set.
///
/// \p acl must be one of the following:
/// - read
/// - write
/// - own
/// - null (removes the ACL)
///
/// On error, \p json_output will have the following JSON structure:
/// \code{.js}
/// {
///   "operation": string,
///   "operation_index": integer,
///   "error_message": string
/// }
/// \endcode
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  A JSON string containing the batch of ACL operations.
/// \param[out] _json_output A JSON string containing the error information on failure.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.9
int rc_atomic_apply_acl_operations(struct RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_ATOMIC_APPLY_ACL_OPERATIONS_H

