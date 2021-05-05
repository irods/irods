#ifndef IRODS_DATA_OBJECT_FINALIZE_H
#define IRODS_DATA_OBJECT_FINALIZE_H

/// \file

const char* SET_TINE_TO_NOW_KW = "set time to now";

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Set the state of every replica for a data object in the catalog, atomically.
///
/// \parblock
/// The replicas field is a complete set of the columns in their current state as well as the
/// desired modifications is required in the form of a JSON structure. Each replica can also
/// have "file_modified" key which holds a set of key-value pairs for the file_modified plugin operation.
///
/// The trigger_file_modified field is a boolean indicating whether the file_modified plugin operation
/// should be called after the data object has been finalized.
///
/// irodsAdmin is a special keyword in iRODS to indicate that the operation requires elevated
/// privileges. When present and set to true, the API will honor elevated privileges. If the user
/// is not authorized to use elevated privileges, an error will be returned.
/// \endparblock
///
/// \p json_input must have the following JSON structure:
/// \code{.js}
/// {
///     "irods_admin": <bool>,
///     "replicas": [
///         {
///             "before": {
///                 "data_id": <string>,
///                 "coll_id": <string>,
///                 "data_repl_num": <string>,
///                 "data_version": <string>,
///                 "data_type_name": <string>,
///                 "data_size": <string>,
///                 "data_path": <string>,
///                 "data_owner_name": <string>,
///                 "data_owner_zone": <string>,
///                 "data_is_dirty": <string>,
///                 "data_status": <string>,
///                 "data_checksum": <string>,
///                 "data_expiry_ts": <string>,
///                 "data_map_id": <string>,
///                 "data_mode": <string>,
///                 "r_comment": <string>,
///                 "create_ts": <string>,
///                 "modify_ts": <string>,
///                 "resc_id": <string>
///             },
///             "after": {
///                 "data_id": <string>,
///                 "coll_id": <string>,
///                 "data_repl_num": <string>,
///                 "data_version": <string>,
///                 "data_type_name": <string>,
///                 "data_size": <string>,
///                 "data_path": <string>,
///                 "data_owner_name": <string>,
///                 "data_owner_zone": <string>,
///                 "data_is_dirty": <string>,
///                 "data_status": <string>,
///                 "data_checksum": <string>,
///                 "data_expiry_ts": <string>,
///                 "data_map_id": <string>,
///                 "data_mode": <string>,
///                 "r_comment": <string>,
///                 "create_ts": <string>,
///                 "modify_ts": <string>,
///                 "resc_id": <string>
///             },
///             "file_modified": {
///                 <string>: <string>,
///                 ...
///             }
///         },
///         ...
///     ],
///     "trigger_file_modified": <bool>
/// }
/// \endcode
///
/// The "modify_ts" field can be populated with the string value for SET_TIME_TO_NOW_KW and
/// the API plugin will fill the value with the current time.
///
/// On error, \p json_output will have the following JSON structure:
/// \code{.js}
/// {
///   "error_message": string
/// }
/// \endcode
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  A JSON string containing the replicas with before and after states.
/// \param[out] _json_output A JSON string containing the error information on failure.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.9
int rc_data_object_finalize(struct RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_DATA_OBJECT_FINALIZE_H

