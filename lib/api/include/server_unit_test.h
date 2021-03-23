#ifndef IRODS_SERVER_UNIT_TEST_H
#define IRODS_SERVER_UNIT_TEST_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Executes one or more server-side unit tests.
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  \parblock A JSON string containing the batch of ACL operations.
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
/// \endparblock
/// \param[out] _json_output \parblock A JSON string containing the error information on failure.
/// \code{.js}
/// {
///   "operation": string,
///   "operation_index": integer,
///   "error_message": string
/// }
/// \endcode
/// \endparblock
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.2.9
int rc_server_unit_test(struct RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_SERVER_UNIT_TEST_H

