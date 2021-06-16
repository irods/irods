#ifndef IRODS_CLIENT_CONNECTION_HPP
#define IRODS_CLIENT_CONNECTION_HPP

/// \file

#include <string_view>
#include <memory>

/// Forward declaration of the underlying connection type.
struct RcComm;

/// A namespace that encapsulates experimental implementations that 
/// may experience frequent changes.
namespace irods::experimental
{
    /// A tag type that indicates whether or not a client connection
    /// should allow the user to control when the connection and
    /// authentication occurs.
    ///
    /// \since 4.2.9
    struct defer_connection {} defer_connection;

    /// This move-only class provides a convenient way to connect to and
    /// disconnect from an iRODS server in a safe manner.
    ///
    /// \since 4.2.9
    class client_connection
    {
    public:
        /// Connects and authenticates using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \since 4.2.9
        client_connection();

        /// Connects to the iRODS server identified by the passed arguments
        /// and authenticates the user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _host     The host name of the iRODS server.
        /// \param[in] _port     The port to connect to.
        /// \param[in] _username The user to connect as.
        /// \param[in] _zone     The zone managed by the iRODS server.
        ///
        /// \since 4.2.9
        client_connection(const std::string_view _host,
                          const int _port,
                          const std::string_view _username,
                          const std::string_view _zone);

        /// Takes ownership of a raw iRODS connection and provides
        /// all of the safety guarantees found in other instantiations.
        ///
        /// \since 4.2.9
        explicit client_connection(RcComm& _conn);

        /// Constructs a client connection, but does not connect to a server.
        /// Instantiations of this class via this constructor must call \ref connect
        /// to establish a connection and authenticate the user.
        ///
        /// \since 4.2.9
        explicit client_connection(struct defer_connection);

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

        /// Connects to the iRODS server identified by the passed arguments
        /// and authenticates the user using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        ///
        /// \param[in] _host     The host name of the iRODS server.
        /// \param[in] _port     The port to connect to.
        /// \param[in] _username The user to connect as.
        /// \param[in] _zone     The zone managed by the iRODS server.
        ///
        /// \since 4.2.9
        auto connect(const std::string_view _host,
                     const int _port,
                     const std::string_view _username,
                     const std::string_view _zone) -> void;

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
        operator bool() const noexcept;

        /// Returns a reference to the underlying connection.
        ///
        /// \throws irods::exception If the underlying connection is null.
        ///
        /// \since 4.2.9
        operator RcComm&() const;

        /// Returns a pointer to the underlying connection.
        ///
        /// \since 4.2.9
        explicit operator RcComm*() const noexcept;

    private:
        auto connect_and_login(const std::string_view _host,
                               const int _port,
                               const std::string_view _username,
                               const std::string_view _zone) -> void;

        std::unique_ptr<RcComm, int (*)(RcComm*)> conn_;
    }; // class client_connection
}; // namespace irods::experimental

#endif // IRODS_CLIENT_CONNECTION_HPP

