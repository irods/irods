#ifndef IRODS_AUTHENTICATION_CLIENT_UTILS_HPP
#define IRODS_AUTHENTICATION_CLIENT_UTILS_HPP

#include <string>

/// \file

namespace irods::authentication
{
    /// Get a password from stdin. Attempts to disable echo mode.
    ///
    /// \retval The password provided to stdin.
    ///
    /// \since 5.1.0
    auto get_password_from_client_stdin() -> std::string;
} // namespace irods::authentication

#endif // IRODS_AUTHENTICATION_CLIENT_UTILS_HPP
