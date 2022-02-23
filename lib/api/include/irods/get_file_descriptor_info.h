#ifndef IRODS_GET_FILE_DESCRIPTOR_INFO_H
#define IRODS_GET_FILE_DESCRIPTOR_INFO_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns information about an iRODS file descriptor.
///
/// The caller is expected to pass the same RcComm pointer used to open the replica.
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  \parblock
/// A JSON string containing the file descriptor.
///
/// The JSON string must have the following structure:
/// \code{.js}
/// {
///   "fd": integer
/// }
/// \endcode
/// \endparblock
/// \param[out] _json_output A JSON string containing the file descriptor information.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.2.8
int rc_get_file_descriptor_info(struct RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_FILE_DESCRIPTOR_INFO_H

