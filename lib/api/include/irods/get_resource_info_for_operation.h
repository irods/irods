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
/// \param[in] _comm An rcComm_t connection handle to the server.
/// \param[in] _dataObjInp \parblock A DataObjInp describing the data object operation.
/// - \p objPath: The logical path of the target data object.
/// - \p condInput: A list of options that influence the results of the operation.
///     - \p REPL_NUM_KW  - The replica number of the copy to upload.
///     - \p GET_RESOURCE_INFO_FOR_OPERATION_KW - The operation of interest. The following values are supported:
///         - "CREATE"
///         - "WRITE"
///         - "OPEN"
///         - "UNLINK"
///     - \p RESC_NAME_KW - Resource name hint.  Given the name of a root resource, influences voting to select the appropriate replica.
///     - \p RESC_HIER_STR_KW -  Resource hierarchy hint.  Given a hierarchy string, specifies the desired replica.
/// \endparblock
/// \param[out] _out_info \parblock a JSON string with keys "host" and "resource_hierarchy", identifying target server and supplying a
///                                 resource hierarchy string denoting a specific data object replica for the logical path given as input.
/// \endparblock
///
/// \b Example _out_info:
///
/// \code{.js}
/// {
///     "host": <string>,
///     "resource_hierarchy": <string>
/// }
/// \endcode
///
/// \b Example usage:
///
/// \code{.cpp}
/// DataObjInp inp;
/// memset(&inp, 0, sizeof(DataObjInp));
/// strncpy(inp.objPath, "/tempZone/home/rods/data_object", MAX_NAME_LEN);
/// char* char_buffer = 0;
/// addKeyVal(&input.condInput, "GET_RESOURCE_INFO_OP_TYPE_KW", "CREATE");
/// addKeyVal(&input.condInput, "RESC_NAME_KW", "my_root_resource");
/// const int ec = rc_get_resource_info_for_operation(&comm, &inp, &char_buffer);
/// if (ec < 0 || NULL == char_buffer) {
///     // Handle error.
/// }
/// // Here, we can parse and use the JSON-formatted _out_info parameter value contained in char_buffer.
/// free(char_buffer);
/// \endcode
///
/// \return integer
/// \retval 0 on success.
///
/// \since 4.3.1
int rc_get_resource_info_for_operation(struct RcComm* _comm, const struct DataObjInp* _dataObjInp, char** _out_info);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_RESOURCE_INFO_FOR_OPERATION_H
