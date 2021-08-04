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
/// desired modifications in the form of a JSON structure. A replica can also have a
/// "file_modified" key which holds a set of key-value pairs for the file_modified plugin operation.
/// Note that only one file_modified key is recognized by the API plugin so only the replica which
/// was modified should contain this key.
///
/// The trigger_file_modified field is a boolean indicating whether the file_modified plugin operation
/// should be called after the data object has been finalized.
///
/// irods_admin is a special keyword in iRODS to indicate that the operation requires elevated
/// privileges. When present and set to true, the API will honor elevated privileges. If the user
/// is not authorized to use elevated privileges, an error will be returned.
///
/// bytes_written is used to indicate the number of bytes written in the operation so that some
/// features can perform checks for bytes written limits (e.g. tickets).
///
/// logical_path is an optional parameter which holds the full logical path to the data object being
/// finalized. The purpose for this parameter is to bypass a query required to make fileModified work.
/// If the logical_path is not passed, the query will need to be run to collect the information. This
/// is needed because the logical path is not actually stored in R_DATA_MAIN and the information cannot
/// be derived from the inputs given to R_DATA_MAIN.
/// \endparblock
///
/// \p json_input must have the following JSON structure:
/// \code{.js}
/// {
///     "bytes_written": <long long>,
///     "irods_admin": <bool>,
///     "logical_path": <string>,
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
/// \parblock
/// The "modify_ts" field can be populated with the string value for SET_TIME_TO_NOW_KW and
/// the API plugin will fill the value with the current time.
///
/// On error, json_output will have the following JSON structure:
/// \code{.js}
/// {
///   "error_message": <string>,
///   "database_updated": <bool>
/// }
/// \endcode
///
/// error_message, as the name implies, contains a description of any failure which may have
/// occurred during the operation (for humans).
///
/// database_updated indicates whether the finalize step completed successfully - that is,
/// were the updates specified applied successfully to the database? If so, any failure which
/// may have occurred happened as a result of file_modified, not updating the database. If not,
/// the database transaction itself failed for some reason. This can be used by the caller to
/// take appropriate action based on which piece of the operation failed.
/// \endparblock
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

