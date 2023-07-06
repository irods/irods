#ifndef IRODS_SWITCH_USER_TYPES_H
#define IRODS_SWITCH_USER_TYPES_H

#include "irods/objInfo.h" // For KeyValPair.
#include "irods/rodsKeyWdDef.h" // For keyword options.

/// The input data type used to invoke #rc_switch_user.
///
/// \since 4.3.1
typedef struct SwitchUserInput // NOLINT(modernize-use-using)
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

    /// The part of a fully-qualified iRODS username preceding the pound sign.
    ///
    /// The following requirements must be satisfied:
    /// - The length must not exceed sizeof(UserInfo::userName) - 1 bytes
    /// - It must be null-terminated
    /// - It must be non-empty
    /// - It must not include the pound sign
    /// - It must not include the zone
    ///
    /// \since 4.3.1
    char username[64];

    /// The part of a fully-qualified iRODS username following the pound sign.
    ///
    /// The following requirements must be satisfied:
    /// - The length must not exceed sizeof(UserInfo::rodsZone) - 1 bytes
    /// - It must be null-terminated
    /// - It must be non-empty
    /// - It must not include the pound sign
    /// - It must not include the username
    ///
    /// \since 4.3.1
    char zone[64];

    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    // NOLINTEND(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)

    /// The set of key-value pair strings used to influence the behavior of
    /// #rc_switch_user.
    ///
    /// Valid options include the following:
    /// - \p KW_SWITCH_PROXY_USER
    ///
    ///     Influences how #rc_switch_user handles the proxy user.
    ///
    ///     If set, the proxy user is switched as well. This option will result in losing
    ///     the ability to invoke #rc_switch_user if it is set and the user to switch to is
    ///     not a \p rodsadmin.
    ///
    ///     This option is never applied to server-to-server connections.
    ///
    /// - \p KW_CLOSE_OPEN_REPLICAS
    ///
    ///     Influences how #rc_switch_user handles open replicas.
    ///
    ///     If set, #rc_switch_user will attempt to close any open replicas before switching
    ///     the identity associated with the connection. Any replicas closed as a result of
    ///     this option will be marked stale.
    ///
    ///     If not set, an error is returned when open replicas are detected.
    ///
    /// - \p KW_KEEP_SVR_TO_SVR_CONNECTIONS
    ///
    ///     Influences how #rc_switch_user handles server-to-server connections.
    ///
    ///     If set, #rc_switch_user is invoked on all server-to-server connections. Otherwise,
    ///     all connections created through redirection will be closed.
    ///
    ///     If the server-to-server connection represents a connection to an iRODS server
    ///     earlier than 4.3.1, the connection is closed.
    ///
    /// \since 4.3.1
    struct KeyValPair options;
} switchUserInp_t;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SwitchUserInp_PI "str username[64]; str zone[64]; struct KeyValPair_PI;"

#endif // IRODS_SWITCH_USER_TYPES_H
