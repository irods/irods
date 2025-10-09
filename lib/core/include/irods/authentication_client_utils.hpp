#ifndef IRODS_AUTHENTICATION_CLIENT_UTILS_HPP
#define IRODS_AUTHENTICATION_CLIENT_UTILS_HPP

#include <cstdint>
#include <string>

/// \file

namespace irods::authentication
{
    constexpr std::uint16_t session_token_length = 36;

    /// Get a password from stdin. Attempts to disable echo mode.
    ///
    /// \retval The password provided to stdin.
    ///
    /// \since 5.1.0
    auto get_password_from_client_stdin() -> std::string;

    /// Get the user's stored session token from the session token file.
    ///
    /// The session token file is typically found in \p $HOME/.irods/.irods_secret. The environment variable
    /// \p IRODS_SESSION_TOKEN_FILE_PATH can be used to set to an alternative location. If \p $HOME or
    /// \p IRODS_SESSION_TOKEN_FILE_PATH are not set, the location of the session token file is unknown.
    /// If the session token file cannot be determined, this function does nothing.
    ///
    /// \throws \p irods::exception if the session token file fails to open.
    ///
    /// \retval The session token as a string.
    ///
    /// \since 5.1.0
    auto read_session_token_from_file() -> std::string;

    /// Write the provided session token to the session token file.
    ///
    /// The session token file is typically found in \p $HOME/.irods/.irods_secret. The environment variable
    /// \p IRODS_SESSION_TOKEN_FILE_PATH can be used to set to an alternative location. If \p $HOME or
    /// \p IRODS_SESSION_TOKEN_FILE_PATH are not set, the location of the session token file is unknown.
    /// If the session token file cannot be determined, this function does nothing.
    ///
    /// \param[in] _session_token The session token to record.
    ///
    /// \throws \p irods::exception if the session token file fails to open.
    ///
    /// \since 5.1.0
    auto write_session_token_to_file(const std::string& _session_token) -> void;

    /// Remove the session token file.
    ///
    /// \retval 0 On successful removal
    /// \retval <0 On failure to remove or if session token file does not exist
    ///
    /// \since 5.1.0
    auto remove_session_token_file() -> int;
} // namespace irods::authentication

#endif // IRODS_AUTHENTICATION_CLIENT_UTILS_HPP
