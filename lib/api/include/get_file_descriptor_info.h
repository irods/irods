#ifndef IRODS_GET_FILE_DESCRIPTOR_INFO_H
#define IRODS_GET_FILE_DESCRIPTOR_INFO_H

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Returns information about an iRODS file descriptor.
///
/// The caller is expected to pass the same RcComm* used to open the replica.
///
/// \user Client
///
/// \since 4.2.8
///
/// \param[in]  _comm        A pointer to a RcComm.
/// \param[in]  _json_input  A JSON string containing the file descriptor.
/// \param[out] _json_output A JSON string containing the file descriptor information.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
int rc_get_file_descriptor_info(RcComm* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_FILE_DESCRIPTOR_INFO_H

