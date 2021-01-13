#ifndef RS_DATA_OBJECT_FINALIZE_HPP
#define RS_DATA_OBJECT_FINALIZE_HPP

/// \file

#include "rodsDef.h"

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Set the state of every replica for a data object in the catalog, atomically.
///
/// A complete set of the columns in their current state as well as the desired modifications
/// is required in the form of a JSON structure.
///
/// \p json_input must have the following JSON structure:
/// \code{.js}
/// {
///   "data_id": string,
///   "replicas": [
///     {
///       "before": {
///         "data_id": string,
///         "coll_id": string
///         "data_name": string
///         "data_repl_num": string
///         "data_version": string
///         "data_type_name": string
///         "data_size": string
///         "data_path": string
///         "data_owner_name": string
///         "data_owner_zone": string
///         "data_is_dirty": string
///         "data_status": string
///         "data_checksum": string
///         "data_expiry_ts": string
///         "data_map_id": string
///         "data_mode": string
///         "r_comment": string
///         "create_ts": string
///         "modify_ts": string
///         "resc_id: string
///       },
///       "after": {
///         "data_id": string,
///         "coll_id": string
///         "data_name": string
///         "data_repl_num": string
///         "data_version": string
///         "data_type_name": string
///         "data_size": string
///         "data_path": string
///         "data_owner_name": string
///         "data_owner_zone": string
///         "data_is_dirty": string
///         "data_status": string
///         "data_checksum": string
///         "data_expiry_ts": string
///         "data_map_id": string
///         "data_mode": string
///         "r_comment": string
///         "create_ts": string
///         "modify_ts": string
///         "resc_id: string
///       }
///     }
///   ]
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
/// \since 4.2.9
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  A JSON string containing the replicas with before and after states.
/// \param[out] _json_output A JSON string containing the error information on failure.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
auto rs_data_object_finalize(RsComm* _comm, const char* _json_input, char** _json_output) -> int;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // #ifndef RS_DATA_OBJECT_FINALIZE_HPP

