#ifndef IRODS_AUTHENTICATION_SERVER_UTILS_HPP
#define IRODS_AUTHENTICATION_SERVER_UTILS_HPP

#include <string>

/// \file

struct RsComm;

namespace irods::authentication
{
    /// Generate a \p std::string which can be used as a session token for authenticating with iRODS.
    ///
    /// \since 5.1.0
    auto generate_session_token() -> std::string;

    /// Set privilege level for the specified user in the \p RsComm based on that user's type in the catalog.
    ///
    /// \param[in,out] _comm The server communication object.
    /// \param[in] _user_name The name of the user.
    /// \param[in] _zone_name The zone of the user.
    ///
    /// \throws \p irods::exception \parblock In the following situations:
    ///   - Fetching information about the user or zone fails
    ///   - The proxy user does not have sufficient privilege to act on behalf of a different client user
    /// \endparblock
    ///
    /// \since 5.1.0
    auto set_privileges_in_rs_comm(RsComm& _comm, const std::string& _user_name, const std::string& _zone_name) -> void;
} //namespace irods::authentication

#endif // IRODS_AUTHENTICATION_SERVER_UTILS_HPP
