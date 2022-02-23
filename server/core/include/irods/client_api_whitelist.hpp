#ifndef IRODS_CLIENT_API_WHITELIST_HPP
#define IRODS_CLIENT_API_WHITELIST_HPP

#include "irods/rcConnect.h"

#include <vector>

namespace irods
{
    /// This class provides a convenient interface for managing and querying
    /// the client-side API whitelist.
    ///
    /// Instances of this class are not copyable or moveable.
    ///
    /// \since 4.2.8
    class client_api_whitelist
    {
    public:
        /// Provides access to a shared instance of this class.
        ///
        /// The object returned is a singleton and is shared by all threads.
        ///
        /// \return A reference to an instance of this class.
        static auto instance() -> client_api_whitelist&;

        client_api_whitelist(const client_api_whitelist&) = delete;
        auto operator=(const client_api_whitelist&) -> client_api_whitelist& = delete;

        /// Returns whether the whitelist should be enforced based on the
        /// server configuration and the connection representation.
        ///
        /// \param[in] comm The server communication object.
        ///
        /// \return A boolean value
        /// \retval true  If the \b server configuration has \b client_api_whitelist_policy
        ///               set to \b enforce and \b comm represents a \b client-to-agent connection
        ///               (i.e. the source of the connection is \b NOT an iRODS consumer or provider).
        /// \retval false Otherwise.
        auto enforce(const rsComm_t& comm) const noexcept -> bool;

        /// Adds an API number to the whitelist.
        ///
        /// This function is not thread-safe.
        ///
        /// \param[in] api_number The API number to add to the whitelist.
        auto add(int api_number) -> void;

        /// Checks if the whitelist contains a particular API number.
        ///
        /// \param[in] api_number The API number to look for.
        ///
        /// \return A boolean value.
        /// \retval true  If the whitelist contains api_number.
        /// \retval false Otherwise.
        auto contains(int api_number) const noexcept -> bool;

    private:
        client_api_whitelist();

        std::vector<int> api_numbers_;
    }; // class client_api_whitelist
} // namespace irods

#endif // IRODS_CLIENT_API_WHITELIST_HPP

