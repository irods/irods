#ifndef IRODS_SWITCH_USER_HPP
#define IRODS_SWITCH_USER_HPP

/// \file

struct RcComm;

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-trailing-return-type)

/// Switches the identity of the user associated with the connection.
///
/// Requires that the proxy user be a \p rodsadmin.
///
/// On success, the following things will be true:
/// - All API operations invoked via the \p RcComm will be carried out as the user that was just switched to.
/// - Server-to-server connections will be updated or disconnected based on the remote server's version.
///
/// On failure, the RcComm will remain as if this API was not invoked. If interacting with the server appears unstable,
/// the client should disconnect and establish a new connection. To avoid unknown issues, clients are encouraged to
/// use a new connection following an error. This provides the highest protection of data.
///
/// \param[in] _comm     A pointer to a RcComm.
/// \param[in] _username \parblock
/// The name of the iRODS user to become.
///
/// The passed string must meet the following requirements:
/// - The length must not exceed sizeof(UserInfo::userName) - 1 bytes
/// - It must be null-terminated
/// - It must be non-empty
/// - It must not include the zone.
/// \endparblock
/// \param[in] _zone     \parblock
/// The zone of the iRODS user to become.
///
/// The passed string must meet the following requirements:
/// - The length must not exceed sizeof(UserInfo::rodsZone) - 1 bytes
/// - It must be null-terminated
/// - It must be non-empty
/// \endparblock
///
/// \return An integer.
/// \retval 0        On success.
/// \retval non-zero On failure.
///
/// \since 4.3.1
int rc_switch_user(struct RcComm* _comm, const char* _username, const char* _zone);

// NOLINTEND(modernize-use-trailing-return-type)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // IRODS_SWITCH_USER_HPP
