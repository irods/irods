#ifndef IRODS_GET_FILE_DESCRIPTOR_INFO_HPP
#define IRODS_GET_FILE_DESCRIPTOR_INFO_HPP

#include "rcConnect.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn int rc_get_file_descriptor_info(rcComm_t* _comm, const char* _json_input, char* _json_output)
 *
 * \brief Returns information about an iRODS file descriptor.
 *
 * The caller is expected to pass the same rsComm_t* used to open the data object.
 * This API function does not redirect. 
 *
 * \user Client
 *
 * \since 4.3.0
 *
 * \param[in] _comm - A pointer to a rcComm_t.
 * \param[in] _json_input - A JSON string containing the file descriptor.
 * \param[out] _json_output - A JSON string containing the file descriptor information.
 *
 * \return integer
 * \retval 0 on success, non-zero on failure.
 **/
int rc_get_file_descriptor_info(rcComm_t* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_GET_FILE_DESCRIPTOR_INFO_HPP

