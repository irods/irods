#ifndef IRODS_CLIENT_API_ALLOWLIST_HPP
#define IRODS_CLIENT_API_ALLOWLIST_HPP

#include "irods/rcConnect.h"

#include <vector>

namespace irods
{
    /// This class provides a convenient interface for managing and querying
    /// the client-side API allowlist.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.8
    class client_api_allowlist
    {
    public:
        /// Provides access to a shared instance of this class.
        ///
        /// The object returned is a singleton and is shared by all threads.
        ///
        /// \return A reference to an instance of this class.
        static auto instance() -> client_api_allowlist&;

        client_api_allowlist(const client_api_allowlist&) = delete;
        auto operator=(const client_api_allowlist&) -> client_api_allowlist& = delete;

        /// Returns whether the allowlist should be enforced based on the
        /// server configuration and the connection representation.
        ///
        /// \param[in] comm The server communication object.
        ///
        /// \return A boolean value
        /// \retval true  If the \b server configuration has \b client_api_allowlist_policy
        ///               set to \b enforce and \b comm represents a \b client-to-agent connection
        ///               (i.e. the source of the connection is \b NOT an iRODS consumer or provider).
        /// \retval false Otherwise.
        auto enforce(const rsComm_t& comm) const noexcept -> bool;

        /// Adds an API number to the allowlist.
        ///
        /// This function is not thread-safe.
        ///
        /// \param[in] api_number The API number to add to the allowlist.
        auto add(int api_number) -> void;

        /// Checks if the allowlist contains a particular API number.
        ///
        /// \param[in] api_number The API number to look for.
        ///
        /// \return A boolean value.
        /// \retval true  If the allowlist contains api_number.
        /// \retval false Otherwise.
        auto contains(int api_number) const noexcept -> bool;

    private:
        client_api_allowlist();

        std::vector<int> api_numbers_;
    }; // class client_api_allowlist
} // namespace irods

#endif // IRODS_CLIENT_API_ALLOWLIST_HPP

