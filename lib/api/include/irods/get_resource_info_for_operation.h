#ifndef IRODS_GET_RESOURCE_INFO_FOR_OPERATION_H
#define IRODS_GET_RESOURCE_INFO_FOR_OPERATION_H

/// \file

#include "irods/dataObjInpOut.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Get preferred-resource information for the specified operation.
///
/// \param[in] _comm An RcComm connection handle to the server.
/// \param[in] _dataObjInp \parblock A DataObjInp describing the data object operation.
/// - \p objPath: The logical path of the target data object.
/// - \p condInput: A list of options that influence the results of the operation.
///     - \p GET_RESOURCE_INFO_FOR_OPERATION_KW: The operation of interest. This option is required. The following
///       values are supported:
///         - \p "CREATE"
///         - \p "WRITE"
///         - \p "OPEN"
///         - \p "UNLINK"
///     - \p RESC_NAME_KW: Resource name hint. Given the name of a root resource, influences voting to select the
///                        appropriate replica.
///     - \p RESC_HIER_STR_KW: Resource hierarchy hint. Given a hierarchy string, specifies the desired replica.
///     - \p REPL_NUM_KW: Replica number hint. Given a replica number, specifies the desired replica.
/// \endparblock
/// \param[out] _out_info \parblock a JSON string with keys \p "host" and \p "resource_hierarchy", identifying target
///                       server and supplying a resource hierarchy string denoting a specific data object replica for
///                       the logical path given as input.
///
/// On success, \p _out_info will have the following structure:
/// \code{.js}
/// {
///     "host": string,
///     "resource_hierarchy": string
/// }
/// \endcode
/// \endparblock
///
/// \b Example
/// \code{.cpp}
/// RcComm* comm = // Our iRODS connection handle.
///
/// DataObjInp inp;
/// memset(&inp, 0, sizeof(DataObjInp));
///
/// // Set the full logical path to the data object of interest.
/// // The data object may or may not exist depending on what the client is
/// // attempting to do.
/// strncpy(inp.objPath, "/tempZone/home/rods/data_object", MAX_NAME_LEN);
///
/// // Set the operation type for resource hierarchy resolution. This is required.
/// // This influences how voting is carried out.
/// addKeyVal(&inp.condInput, GET_RESOURCE_INFO_OP_TYPE_KW, "CREATE");
///
/// // A pointer that will point to a heap-allocated buffer holding the JSON
/// // string if the API call succeeds.
/// char* char_buffer = NULL;
///
/// const int ec = rc_get_resource_info_for_operation(comm, &inp, &char_buffer);
/// if (ec < 0) {
///     // Handle error.
/// }
///
/// // Here, we can parse and use the JSON-formatted string contained in char_buffer.
///
/// // Free the buffer when we're done.
/// free(char_buffer);
/// \endcode
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.1
int rc_get_resource_info_for_operation(struct RcComm* _comm, const struct DataObjInp* _dataObjInp, char** _out_info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_RESOURCE_INFO_FOR_OPERATION_H
