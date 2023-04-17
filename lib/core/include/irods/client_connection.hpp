#ifndef IRODS_CLIENT_CONNECTION_HPP
#define IRODS_CLIENT_CONNECTION_HPP

/// \file

#include "irods/fully_qualified_username.hpp"

#include <string_view>
#include <memory>

/// Forward declaration of the underlying connection type.
struct RcComm;

/// A namespace that encapsulates experimental implementations that 
/// may experience frequent changes.
namespace irods::experimental
{
    // clang-format off

    /// A tag type that indicates whether or not a client connection
    /// should allow the user to control when the connection and
    /// authentication occurs.
    ///
    /// \since 4.2.9
    inline const struct defer_connection {} defer_connection;

    /// A tag type that indicates whether or not a client connection
    /// should allow the user to control when the authentication occurs.
    ///
    /// \since 4.3.1
    inline const struct defer_authentication {} defer_authentication;

    // clang-format on

    /// This move-only class provides a convenient way to connect to and
    /// disconnect from an iRODS server in a safe manner.
    ///
    /// \since 4.2.9
    class client_connection // NOLINT(cppcoreguidelines-special-member-functions)
    {
      public:
        /// Connects and authenticates using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \since 4.2.9
        client_connection();

        /// Connects using the information and credentials found in the user's
        /// irods_environment.json file. No authentication is performed.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag Indicates that authentication must be skipped.
        ///
        /// \since 4.3.1
        explicit client_connection(struct defer_authentication _defer_tag);

        /// Connects to the iRODS server identified by the passed arguments
        /// and authenticates the user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _host     The host name of the iRODS server.
        /// \param[in] _port     The port to connect to.
        /// \param[in] _username The user to connect as.
        ///
        /// \since 4.3.1
        client_connection(const std::string_view _host, const int _port, const fully_qualified_username& _username);

        /// Connects to the iRODS server identified by the passed arguments
        /// but does not perform any authentication.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag Indicates that authentication must be skipped.
        /// \param[in] _host      The host name of the iRODS server.
        /// \param[in] _port      The port to connect to.
        /// \param[in] _username  The name of the user to connect as.
        ///
        /// \since 4.3.1
        client_connection(struct defer_authentication _defer_tag,
                          const std::string_view _host,
                          const int _port,
                          const fully_qualified_username& _username);

        /// Connects to the iRODS server identified by the passed arguments
        /// and authenticates the proxy user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occured.
        ///
        /// \param[in] _host           The host name of the iRODS server.
        /// \param[in] _port           The port to connect to.
        /// \param[in] _proxy_username The name of the user acting as the proxy.
        /// \param[in] _username       The name of the user being proxied.
        ///
        /// \since 4.3.1
        client_connection(const std::string_view _host,
                          const int _port,
                          const fully_qualified_username& _proxy_username,
                          const fully_qualified_username& _username);

        /// Connects to the iRODS server identified by the passed arguments
        /// but does not perform any authentication.
        ///
        /// \throws irods::exception If an error occured.
        ///
        /// \param[in] _defer_tag      Indicates that authentication must be skipped.
        /// \param[in] _host           The host name of the iRODS server.
        /// \param[in] _port           The port to connect to.
        /// \param[in] _proxy_username The name of the user acting as the proxy.
        /// \param[in] _username       The name of the user being proxied.
        ///
        /// \since 4.3.1
        client_connection(struct defer_authentication _defer_tag,
                          const std::string_view _host,
                          const int _port,
                          const fully_qualified_username& _proxy_username,
                          const fully_qualified_username& _username);

        /// Takes ownership of a raw iRODS connection and provides
        /// all of the safety guarantees found in other instantiations.
        ///
        /// \since 4.2.9
        explicit client_connection(RcComm& _conn);

        /// Constructs a client connection, but does not connect to a server.
        /// Instantiations of this class via this constructor must call \ref connect
        /// to establish a connection and authenticate the user.
        ///
        /// \param[in] _defer_tag Indicates that no connection will be made to the server.
        ///
        /// \since 4.2.9
        explicit client_connection(struct defer_connection _defer_tag);

        client_connection(client_connection&& _other) = default;
        auto operator=(client_connection&& _other) -> client_connection& = default;

        /// Closes the underlying connection if active.
        ///
        /// \since 4.2.9
        ~client_connection() = default;

        /// Connects and authenticates using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \since 4.2.9
        auto connect() -> void;

        /// Connects using the information and credentials found in the user's
        /// irods_environment.json file. No authentication is performed.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag Indicates that authentication must be skipped.
        ///
        /// \since 4.3.1
        auto connect(struct defer_authentication _defer_tag) -> void;

        /// Connects to the iRODS server identified by the passed arguments
        /// and authenticates the user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _host     The host name of the iRODS server.
        /// \param[in] _port     The port to connect to.
        /// \param[in] _username The name of the user to connect as.
        ///
        /// \since 4.3.1
        auto connect(const std::string_view _host, const int _port, const fully_qualified_username& _username) -> void;

        /// Connects to the iRODS server identified by the passed arguments
        /// but does not perform any authentication.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag Indicates that authentication must be skipped.
        /// \param[in] _host      The host name of the iRODS server.
        /// \param[in] _port      The port to connect to.
        /// \param[in] _username  The name of the user to connect as.
        ///
        /// \since 4.3.1
        auto connect(struct defer_authentication _defer_tag,
                     const std::string_view _host,
                     const int _port,
                     const fully_qualified_username& _username) -> void;

        /// Connects to the iRODS server identified by the passed arguments
        /// but does not perform any authentication.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag      Indicates that authentication must be skipped.
        /// \param[in] _host           The host name of the iRODS server.
        /// \param[in] _port           The port to connect to.
        /// \param[in] _proxy_username The name of the user acting as the proxy.
        /// \param[in] _username       The name of the user to connect as.
        ///
        /// \since 4.3.1
        auto connect(struct defer_authentication _defer_tag,
                     const std::string_view _host,
                     const int _port,
                     const fully_qualified_username& _proxy_username,
                     const fully_qualified_username& _username) -> void;

        /// Connects to the iRODS server with the specified proxy user and zone
        /// and authenticates the proxy user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _proxy_user The user acting as the proxy.
        ///
        /// \since 4.3.1
        auto connect(const fully_qualified_username& _proxy_user) -> void;

        /// Connects to the iRODS server with the specified proxy user and zone
        /// but does not perform any authentication.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _defer_tag  Indicates that authentication must be skipped.
        /// \param[in] _proxy_user The user acting as the proxy.
        ///
        /// \since 4.3.1
        auto connect(struct defer_authentication _defer_tag, const fully_qualified_username& _proxy_user) -> void;

        /// Closes the underlying connection if active.
        ///
        /// \since 4.2.9
        auto disconnect() noexcept -> void;

        /// Checks whether \p *this is holding an active connection.
        ///
        /// \returns A boolean.
        /// \retval true  If the \p *this contains an active connection.
        /// \retval false Otherwise.
        ///
        /// \since 4.2.9
        operator bool() const noexcept; // NOLINT(google-explicit-constructor)

        /// Returns a reference to the underlying connection.
        ///
        /// \throws irods::exception If the underlying connection is null.
        ///
        /// \since 4.2.9
        operator RcComm&() const; // NOLINT(google-explicit-constructor)

        /// Returns a pointer to the underlying connection.
        ///
        /// \since 4.2.9
        explicit operator RcComm*() const noexcept;

      private:
        auto connect_and_login(const std::string& _host, const int _port, const fully_qualified_username& _username)
            -> void;

        auto connect_and_login(const std::string& _host,
                               const int _port,
                               const fully_qualified_username& _proxy_username,
                               const fully_qualified_username& _username) -> void;

        auto only_connect(const std::string& _host, const int _port, const fully_qualified_username& _username) -> void;

        auto only_connect(const std::string& _host,
                          const int _port,
                          const fully_qualified_username& _proxy_username,
                          const fully_qualified_username& _username) -> void;

        std::unique_ptr<RcComm, int (*)(RcComm*)> conn_;
    }; // class client_connection
}; // namespace irods::experimental

#endif // IRODS_CLIENT_CONNECTION_HPP

