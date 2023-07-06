#ifndef IRODS_SWITCH_USER_HPP
#define IRODS_SWITCH_USER_HPP

/// \file

#include "irods/plugins/api/switch_user_types.h"

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-trailing-return-type)

/// Switches the identity of the user associated with the connection.
///
/// Requires the proxy user to be a \p rodsadmin.
///
/// \see SwitchUserInput for information about available options.
///
/// On success, the following things will be true:
/// - API operations invoked via the \p RcComm will be carried out as the user that was just switched to.
/// - Server-to-server connections will be updated or disconnected based on the input and the remote server's version.
/// - Tickets enabled on the connection will be disabled.
///
/// On failure, the RcComm MUST be closed to avoid potential issues.
///
/// \param[in] _comm  A pointer to a RcComm.
/// \param[in] _input A pointer to a SwitchUserInput.
///
/// \return An integer.
/// \retval 0                     On success.
/// \retval <0                    On failure.
/// \retval SYS_PROXYUSER_NO_PRIV If the proxy user is not a \p rodsadmin.
/// \retval SYS_NOT_ALLOWED       If there are open replicas and \p KW_CLOSE_OPEN_REPLICAS is not set.
///
/// \b Example
/// \code{.cpp}
/// RcComm* comm = // Connected to iRODS server as alice#tempZone.
///
/// //
/// // IMPORTANT: alice#tempZone is a rodsadmin.
/// //
///
/// // Configure the input object for the API call.
/// struct SwitchUserInput input;
/// memset(&input, 0, sizeof(struct SwitchUserInput));
///
/// // Notice how the username and zone are copied into separate member variables
/// // of the input struct. Remember, the username and zone MUST be null-terminated.
/// strncpy(input.username, "bob", sizeof(SwitchUserInput::username) - 1);
/// strncpy(input.zone, "tempZone", sizeof(SwitchUserInput::zone) - 1);
///
/// // (optional)
/// // Switch the identity of the proxy user to bob#tempZone too.
/// // Without this, alice#tempZone would be acting as the proxy for bob#tempZone.
/// addKeyVal(&input.options, KW_SWITCH_PROXY_USER, "");
///
/// // Become bob#tempZone.
/// const int ec = rc_switch_user(comm, &input);
///
/// if (ec < 0) {
///     // The safest thing to do here is to close the connection. We don't know
///     // what state the server-side is in.
///     rcDisconnect(comm);
/// }
/// \endcode
///
/// \since 4.3.1
int rc_switch_user(struct RcComm* _comm, const struct SwitchUserInput* _input);

// NOLINTEND(modernize-use-trailing-return-type)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_SWITCH_USER_HPP
