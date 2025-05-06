#ifndef IRODS_CLIENT_API_ALLOWLIST_HPP
#define IRODS_CLIENT_API_ALLOWLIST_HPP

struct RsComm;

/// Defines various functions for managing and querying the client-side API allowlist.
///
/// \since 4.2.8
namespace irods::client_api_allowlist
{
    /// Initializes the client api allowlist for use.
    ///
    /// This function should be called at program start only once.
    ///
    /// \throw irods::exception If an error occurred.
    ///
    /// \since 4.3.1
    auto init() -> void;

    /// Returns whether the allowlist should be enforced.
    ///
    /// \param[in] _comm The server communication object.
    ///
    /// \return A boolean value
    /// \retval false If \p _comm represents a user of type rodsadmin.
    /// \retval false If \p _comm is a server-to-server connection.
    /// \retval true  Otherwise.
    ///
    /// \since 5.0.0
    auto enforce(const RsComm& _comm) noexcept -> bool;

    /// Checks if the allowlist contains a particular API number.
    ///
    /// \param[in] _api_number The API number to look for.
    ///
    /// \return A boolean value.
    /// \retval true  If the allowlist contains the API number.
    /// \retval false Otherwise.
    ///
    /// \since 4.2.8
    auto contains(int _api_number) noexcept -> bool;

    /// Adds an API number to the allowlist.
    ///
    /// This function is not thread-safe.
    ///
    /// \param[in] _api_number The API number to add to the allowlist.
    ///
    /// \since 4.2.8
    auto add(int _api_number) -> void;
} // namespace irods::client_api_allowlist

#endif // IRODS_CLIENT_API_ALLOWLIST_HPP
