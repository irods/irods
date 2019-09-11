#ifndef IRODS_SYNC_WITH_PHYSICAL_OBJECT_H
#define IRODS_SYNC_WITH_PHYSICAL_OBJECT_H

#include "rcConnect.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \fn int rc_sync_with_physical_object(rcComm_t* _comm, const char* _json_input)
 *
 * \brief Synchronizes data object information in the catalog with the physical information on disk.
 *
 * \user Client
 *
 * \since 4.3.0
 *
 * \param[in] _comm - A pointer to a rcComm_t.
 * \param[in] _json_input - JSON data containing all file descriptor info about an open data object as well
 *                          atomic metadata operations, acls, and whether to sync the data object.
 *
 * \return integer
 * \retval 0 on success, non-zero on failure.
 **/
int rc_sync_with_physical_object(rcComm_t* _comm, const char* _json_input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_SYNC_WITH_PHYSICAL_OBJECT_H

