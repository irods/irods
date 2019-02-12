#ifndef IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP
#define IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP

#include "rcConnect.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn int rs_get_file_descriptor_info(rsComm_t* _comm, const char* _json_input, char* _json_output)
 *
 * \brief Returns information about an iRODS file descriptor.
 *
 * The caller is expected to pass the same rsComm_t* used to open the data object.
 *
 * \user Server
 *
 * \since 4.2.8
 *
 * \param[in] _comm - A pointer to a rsComm_t.
 * \param[in] _json_input - A JSON string containing the file descriptor.
 * \param[out] _json_output - A JSON string containing the file descriptor information.
 *
 * \return integer
 * \retval 0 on success, non-zero on failure.
 **/
int rs_get_file_descriptor_info(rsComm_t* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_GET_FILE_DESCRIPTOR_INFO_HPP

