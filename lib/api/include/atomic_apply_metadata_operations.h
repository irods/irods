#ifndef IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_H
#define IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Executes a list of metadata operations on a single object atomically.
///
/// Sequentially executes all \p operations on \p entity_name as a single transaction. If an
/// error occurs, all updates are rolled back and an error is returned. \p json_output will
/// contain specific information about the error.
///
/// \p json_input must have the following JSON structure:
/// \code{.js}
/// {
///   "admin_mode": boolean,
///   "entity_name": string,
///   "entity_type": string,
///   "operations": [
///     {
///       "operation": string,
///       "attribute": string,
///       "value": string,
///       "units": string
///     }
///   ]
/// }
/// \endcode
///
/// \p admin_mode a boolean value instructing the server to execute the operations as an
/// administrator (i.e. rodsadmin).
///
/// \p entity_name must be one of the following:
/// - A logical path pointing to a data object.
/// - A logical path pointing to a collection.
/// - A user name.
/// - A resource name.
///
/// \p entity_type must be one of the following:
/// - collection
/// - data_object
/// - resource
/// - user
///
/// \p operations is the list of metadata operations to execute atomically. They will be
/// executed in order.
///
/// \p operation must be one of the following:
/// - add
/// - remove
///
/// \p units are optional.
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
/// \param[in]  _json_input  A JSON string containing the batch of metadata operations.
/// \param[out] _json_output A JSON string containing the error information on failure.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.8
int rc_atomic_apply_metadata_operations(struct RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_H

