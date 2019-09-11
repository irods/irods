#ifndef IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_HPP
#define IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_HPP

#include "rcConnect.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn int rc_atomic_apply_metadata_operations(rcComm_t* _comm, const char* _json_input, char* _json_output)
 *
 * \brief Executes a list of metadata operations on a single object atomically.
 *
 * Sequentially executes all "operations" on "entity_name" as a single transaction. If an
 * error occurs, all updates are rolled back and an error is returned. "_json_output" will
 * contain specific information about the error.
 *
 * "_json_input" must have the following structure:
 * {
 *   "entity_name": "/logical/path",
 *   "entity_type": "collection, data_object, resource, or user",
 *   "operations": [
 *     {
 *       "operation": "set or remove",
 *       "attribute": "a0",
 *       "value": "v0",
 *       "units": "u0"
 *     },
 *     ...
 *   ]
 * }
 *
 * "units" are optional.
 *
 * On error, "_json_output" will have the following structure:
 * {
 *   "operation": string,
 *   "operation_index": number,
 *   "error_message": string
 * }
 *
 * \user Client
 *
 * \since 4.3.0
 *
 * \param[in] _comm - A pointer to a rcComm_t.
 * \param[in] _json_input - A JSON string containing the batch of metadata updates.
 * \param[out] _json_output - A JSON string containing the error information on failure.
 *
 * \return integer
 * \retval 0 on success, non-zero on failure.
 **/
int rc_atomic_apply_metadata_operations(rcComm_t* _comm, const char* _json_input, char** _json_output);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_ATOMIC_APPLY_METADATA_OPERATIONS_HPP

