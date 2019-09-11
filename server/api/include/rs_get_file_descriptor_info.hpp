#ifndef IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP
#define IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP

/// \file

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns information about an iRODS file descriptor.
///
/// The caller is expected to pass the same RsComm pointer used to open the replica.
///
/// \since 4.2.8
///
/// \param[in]  _comm        A pointer to a RsComm.
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
int rs_get_file_descriptor_info(RsComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP

