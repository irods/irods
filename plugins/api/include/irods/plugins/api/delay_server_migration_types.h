#ifndef IRODS_DELAY_SERVER_MIGRATION_TYPES_H
#define IRODS_DELAY_SERVER_MIGRATION_TYPES_H

/// Indicates whether the leader or successor should be updated.
///
/// See the following APIs:
/// - rc_set_delay_server_migration_info()
/// - rs_set_delay_server_migration_info()
/// 
/// \since 4.3.0
#define KW_DELAY_SERVER_MIGRATION_IGNORE "ignore"

/// The input data type used for updating the leader and successor for migration of the
/// delay server.
///
/// The \p leader and \p successor member variables must be set to hostnames known to the
/// local iRODS zone.
///
/// See the following APIs:
/// - rc_set_delay_server_migration_info()
/// - rs_set_delay_server_migration_info()
///
/// \since 4.3.0
typedef struct DelayServerMigrationInput
{
    char leader[2700];
    char successor[2700];
} delayServerMigrationInp_t;

#define DelayServerMigrationInp_PI "str leader[2700]; str successor[2700];"

#endif // IRODS_DELAY_SERVER_MIGRATION_TYPES_H

