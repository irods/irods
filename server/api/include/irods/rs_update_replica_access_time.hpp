#ifndef IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP
#define IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP

/// \file

struct RsComm;

#include "irods/rodsDef.h" // For BytesBuf.

/// Updates the access time of one or more replicas.
///
/// This operation is atomic. All updates are rolled back if an error occurs during the catalog update.
///
/// Requires rodsadmin level privileges.
///
/// \param[in]  _comm       The connection to an iRODS server.
/// \param[in]  _json_input \parblock A JSON string containing access time update information. The
/// payload may contain replicas belonging to different data objects. Duplicates are allowed.
/// \endparblock
/// \param[out] _output     A pointer which will hold error details about the operation, if available.
///
/// \return An integer.
/// \retval >=0 The number of replicas that were affected.
/// \retval  <0 On failure.
///
/// \b Example
/// \code{.cpp}
/// RsComm comm = // Connected to iRODS server as a rodsadmin.
///
/// // This JSON string contains the access time updates we are going to apply.
/// // Notice it identifies two unrelated replicas.
/// const char* json_input =
/// "{"
///   "\"access_time_updates\": ["
///     "{"
///       "\"data_id\": 20000,"
///       "\"replica_number\": 2,"
///       "\"atime\": \"01715984523\""
///     "},"
///     "{"
///       "\"data_id\": 40500,"
///       "\"replica_number\": 0,"
///       "\"atime\": \"01715984000\""
///     "}"
///   "]"
/// "}";
///
/// // The output parameter is required, but isn't used by the API at this time.
/// char* output = NULL;
///
/// const int ec = rs_update_replica_access_time(comm, json_input, &output);
/// if (ec < 0) {
///     // Handle error.
/// }
///
/// // Check the output variable for details about the failure.
/// if (output) {
///     // Do something with the string.
///     // Deallocate memory.
///     free(output);
/// }
/// \endcode
///
/// \since 5.0.0
int rs_update_replica_access_time(RsComm* _comm, BytesBuf* _json_input, char** _output);

#endif // IRODS_RS_UPDATE_REPLICA_ACCESS_TIME_HPP
