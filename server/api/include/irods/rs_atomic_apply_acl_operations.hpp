#ifndef IRODS_RS_ATOMIC_APPLY_ACL_OPERATIONS_HPP
#define IRODS_RS_ATOMIC_APPLY_ACL_OPERATIONS_HPP

/// \file

struct RsComm;

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
///   "admin_mode": boolean,
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
/// \p admin_mode a boolean value instructing the server to execute the operations as an
/// administrator (i.e. rodsadmin).
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
/// \since 4.2.9
///
/// \param[in]  _comm        A pointer to a RsComm.
/// \param[in]  _json_input  A JSON string containing the batch of ACL operations.
/// \param[out] _json_output A JSON string containing the error information on failure.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
int rs_atomic_apply_acl_operations(RsComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_ATOMIC_APPLY_ACL_OPERATIONS_HPP

