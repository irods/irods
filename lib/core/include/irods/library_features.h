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

/// Defined if the development library supports the C++ zone administration library.
///
/// \since 4.3.1
#define IRODS_HAS_LIBRARY_ZONE_ADMINISTRATION 202210L

/// Defined if the development library supports ::rc_switch_user.
///
/// \since 4.3.1
#define IRODS_HAS_API_ENDPOINT_SWITCH_USER    202211L

/// Defined if the development library supports the C++ system_error library.
///
/// \since 4.3.1
#define IRODS_HAS_LIBRARY_SYSTEM_ERROR        202301L

/// Defined if the C++ client connection libraries support proxy users.
///
/// \since 4.3.1
#define IRODS_HAS_FEATURE_PROXY_USER_SUPPORT_FOR_CLIENT_CONNECTION_LIBRARIES 202306L

#endif // IRODS_LIBRARY_FEATURES_H
