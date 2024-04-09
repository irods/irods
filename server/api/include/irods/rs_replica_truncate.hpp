#ifndef IRODS_RS_REPLICA_TRUNCATE_HPP
#define IRODS_RS_REPLICA_TRUNCATE_HPP

/// \file

#include "irods/dataObjInpOut.h"

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Truncate a replica for the specified data object to the specified size.
///
/// \parblock
/// This API selects a replica to truncate according to the rules of POSIX truncate(2). The caller may provide keywords
/// via condInput in order to influence the hierarchy resolution for selecting a replica to truncate.
/// \endparblock
///
/// \param[in] _comm A pointer to a RsComm.
/// \param[in] _inp \parblock
/// DataObjInp structure which requires the following inputs:
///     - objPath: The full logical path to the target data object.
///     - dataSize: The desired size of the replica after truncating.
///
/// The condInput supports the following keywords:
///     - REPL_NUM_KW: The replica number of the replica to truncate.
///     - RESC_NAME_KW: The name of the resource with the replica to truncate. Must be a root resource.
///     - DEF_RESC_NAME_KW: The default resource to target in the absence of any other inputs or policy.
///     - RESC_HIER_STR_KW: Full resource hierarchy to the replica to truncate. Use with caution.
///     - ADMIN_KW: Flag indicating that the operation is to be performed with elevated privileges. No value required.
/// \endparblock
/// \param[out] _out \parblock
/// Character string representing a JSON structure with the following form:
/// \code{.js}
/// {
///     // Resource hierarchy of the selected replica for truncate. If an error occurs before hierarchy resolution is
///     // completed, a null value will be here instead of a string.
///     "resource_hierarchy": <string | null>,
///     // Replica number of the selected replica for truncate. If an error occurs before hierarchy resolution is
///     // completed, a null value will be here instead of an integer.
///     "replica_number": <integer | null>,
///     // A string containing any relevant message the server may wish to send to the user (including error messages).
///     // This value will always be a string, even if it is empty.
///     "message": <string>
/// }
/// \endcode
/// \endparblock
///
/// \usage \parblock
/// \code{c}
///     // Because this is a server-side function, we will assume that there is access to an RsComm;
///     RsComm* comm;
///
///     // Don't forget to call clearKeyVal on truncate_doi.condInput before exiting to prevent leaks.
///     DataObjInp truncate_doi;
///     memset(&truncate_doi, 0, sizeof(DataObjInp));
///
///     // Set the path and the desired size.
///     strncpy(truncate_doi.objPath, "/tempZone/home/alice/foo", MAX_NAME_LEN);
///     truncate_doi.size = 0;
///
///     // Target a specific replica, if desired. If no replica is targeted via some key-value pair, the policy for
///     // selecting a preferred resource configured on the server will be used for resolving the hierarchy for a
///     // "write" operation.
///     addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "3");
///
///     // Need a character string to hold the output. Don't forget to free this before exiting to prevent leaks.
///     char* output_string = NULL;
///
///     const int ec = rs_replica_truncate(comm, &truncate_doi, &output_string);
///     if (ec < 0) {
///         // Error handling. Perhaps use the "message" field inside the output_string.
///     }
/// \endcode
/// \endparblock
///
/// \return An integer representing an iRODS error code, or 0.
/// \retval 0 on success.
/// \retval <0 on failure; an iRODS error code.
///
/// \since 4.3.2
int rs_replica_truncate(RsComm* _comm, DataObjInp* _inp, char** _out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_REPLICA_TRUNCATE_HPP
