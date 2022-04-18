#ifndef IRODS_RS_SET_DELAY_SERVER_MIGRATION_INFO_HPP
#define IRODS_RS_SET_DELAY_SERVER_MIGRATION_INFO_HPP

/// \file

#include "irods/plugins/api/delay_server_migration_types.h"

struct RsComm;

#ifdef __cplusplus
extern "C" {
#endif

/// Atomically sets the leader and successor hostnames for delay server migration in the
/// R_GRID_CONFIGURATION table.
///
/// The invoking user of this API must be a \p rodsadmin.
///
/// This API will verify the following before updating the catalog:
/// - Verify that input arguments are not null
/// - Verify that hostname requirements imposed by the OS have not been violated
/// - Verify that hostnames are not identical
/// - Verify that hostnames refer to iRODS servers in the local zone
///
/// \param[in] _comm  A pointer to a RsComm.
/// \param[in] _input \parblock An object holding the new hostname values for the leader and successor.
///
/// Although the API allows updating the leader and successor simultaneously, users aren't required to
/// do so. Users can choose to skip updating either field. This is achieved by setting the respective
/// member variable to the value of \p KW_DELAY_SERVER_MIGRATION_IGNORE. For example:
/// \code{.c}
/// DelayServerMigrationInput input;
/// memset(&input, 0, sizeof(DelayServerMigrationInput));
/// strcpy(input.leader, KW_DELAY_SERVER_MIGRATION_IGNORE);
/// \endcode
/// \endparblock
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero On failure.
///
/// \since 4.3.0
int rs_set_delay_server_migration_info(RsComm* _comm, const DelayServerMigrationInput* _input);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_RS_SET_DELAY_SERVER_MIGRATION_INFO_HPP

