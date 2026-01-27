#ifndef IRODS_LIBRARY_FEATURES_H
#define IRODS_LIBRARY_FEATURES_H

// This file SHOULD NOT be processed by clang-tidy because it is required to define
// preprocessor macros for compile-time decision making, hence the use of the following
// clang-tidy directive.
//
// NOLINTBEGIN

/// \file
///
/// \brief iRODS Feature Test Macros
///
/// This file defines macros that can be used to test whether one or more features
/// are supported by the iRODS library.
///
/// Each macro expands to the year and month at which it was first introduced. If a feature
/// experiences significant changes, the value of the macro is allowed to be updated.
///
/// \since 4.3.1

/// Defined if the development library supports the C++ ticket administration library.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_TICKET_ADMINISTRATION
///
/// \since 4.3.1
#define IRODS_HAS_LIBRARY_TICKET_ADMINISTRATION                              202209L

/// An alias for #IRODS_HAS_LIBRARY_TICKET_ADMINISTRATION.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_TICKET_ADMINISTRATION                          IRODS_HAS_LIBRARY_TICKET_ADMINISTRATION

/// Defined if the development library supports the C++ zone administration library.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_ZONE_ADMINISTRATION
///
/// \since 4.3.1
#define IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION                                202210L

/// An alias for #IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_ZONE_ADMINISTRATION                            IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION

/// Defined if the development library supports #rc_switch_user.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_SWITCH_USER
///
/// \since 4.3.1
#define IRODS_HAS_API_ENDPOINT_SWITCH_USER                                   202211L

/// An alias for #IRODS_HAS_API_ENDPOINT_SWITCH_USER.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_SWITCH_USER                                    IRODS_HAS_API_ENDPOINT_SWITCH_USER

/// Defined if the development library supports the C++ system_error library.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_SYSTEM_ERROR
///
/// \since 4.3.1
#define IRODS_HAS_LIBRARY_SYSTEM_ERROR                                       202301L

/// An alias for #IRODS_HAS_LIBRARY_SYSTEM_ERROR.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_SYSTEM_ERROR                                   IRODS_HAS_LIBRARY_SYSTEM_ERROR

/// Defined if the C++ client connection libraries support proxy users.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_CLIENT_CONNECTION_LIBRARIES_SUPPORT_PROXY_USERS
///
/// \since 4.3.1
#define IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES 202306L

/// An alias for #IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_CLIENT_CONNECTION_LIBRARIES_SUPPORT_PROXY_USERS \
  IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES

/// Defined if the development library supports #rc_check_auth_credentials.
///
/// \sa Prefer IRODS_LIBRARY_FEATURE_CHECK_AUTH_CREDENTIALS
///
/// \since 4.3.1
#define IRODS_HAS_API_ENDPOINT_CHECK_AUTH_CREDENTIALS 202307L

/// An alias for #IRODS_HAS_API_ENDPOINT_CHECK_AUTH_CREDENTIALS.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_CHECK_AUTH_CREDENTIALS  IRODS_HAS_API_ENDPOINT_CHECK_AUTH_CREDENTIALS

/// Defined if the development library supports #rc_replica_truncate.
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_REPLICA_TRUNCATE        202403L

/// Defined if the development library supports #rc_genquery2.
///
/// - 202503L: User is granted control over the DISTINCT keyword
/// - 202404L: Initial implementation
///
/// \since 4.3.2
#define IRODS_LIBRARY_FEATURE_GENQUERY2                                   202503L

/// Defined if the development library supports the delay rule locking API.
///
/// \since 5.0.0
#define IRODS_LIBRARY_FEATURE_DELAY_RULE_LOCKING      202411L

/// Defined if the "irods" authentication scheme and plugin are supported.
///
/// This feature flag covers the following features as well:
/// - Session token support
///   - remove_session_tokens option for GeneralAdmin API
///   - Library free functions for reading and writing session tokens to a local file for client use
/// - Hashed password support
/// - no-scramble option for GeneralAdmin and UserAdmin API's password-setting functionality
/// - remove_password functionality for GeneralAdmin API's "modify" option for "user" entities
///
/// \since 5.1.0
#define IRODS_LIBRARY_FEATURE_IRODS_AUTHENTICATION    202512L

/// Defined if the development library supports access time for replicas.
///
/// \since 5.0.0
#define IRODS_LIBRARY_FEATURE_ACCESS_TIME                                 202503L

/// Defined if the development library supports #rc_authenticate_client.
///
/// \since 5.0.0
#define IRODS_LIBRARY_FEATURE_AUTHENTICATE_CLIENT                         202504L

/// Defined if the development library supports #procApiRequest_raw.
///
/// \since 4.3.5
#define IRODS_LIBRARY_FEATURE_OVERRIDE_PACKING_INSTRUCTIONS               202511L

/// Defined if the development library supports CRC64/NVME as a checksum algorithm.
///
/// \since 5.1.0
#define IRODS_LIBRARY_FEATURE_CHECKSUM_ALGORITHM_CRC64NVME                202511L

/// Defined if the development library supports resources that can read checksum information
/// from the storage device.
///
/// This means the RESOURCE_OP_READ_CHECKSUM_FROM_STORAGE_DEVICE operation is supported by
/// the resource plugin interface.
///
/// \since 5.1.0
#define IRODS_LIBRARY_FEATURE_RESOURCES_READ_CHECKSUM_FROM_STORAGE_DEVICE 202511L

// NOLINTEND

#endif // IRODS_LIBRARY_FEATURES_H
