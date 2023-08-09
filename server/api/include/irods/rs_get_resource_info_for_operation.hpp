#ifndef IRODS_RS_GET_RESOURCE_INFO_FOR_OPERATION_HPP
#define IRODS_RS_GET_RESOURCE_INFO_FOR_OPERATION_HPP
/// \file

#include "irods/dataObjInpOut.h"

struct RsComm;

/// Get preferred-resource information for the specified operation.
///
/// \param[in] _rsComm An rsComm_t connection handle to the server.
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
/// \return integer
/// \retval 0 on success.
///
/// \since 4.3.1
int rs_get_resource_info_for_operation(RsComm* _rsComm, DataObjInp* _dataObjInp, char** _out_info);

#endif // IRODS_RS_GET_RESOURCE_INFO_FOR_OPERATION_HPP
