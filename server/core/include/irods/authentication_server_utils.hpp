#ifndef IRODS_AUTHENTICATION_SERVER_UTILS_HPP
#define IRODS_AUTHENTICATION_SERVER_UTILS_HPP

#include <string>

/// \file

namespace irods::authentication
{
    /// Generate a \p std::string which can be used as a session token for authenticating with iRODS.
    ///
    /// \since 5.1.0
    auto generate_session_token() -> std::string;
} //namespace irods::authentication

#endif // IRODS_AUTHENTICATION_SERVER_UTILS_HPP
