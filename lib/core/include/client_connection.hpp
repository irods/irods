#ifndef IRODS_CLIENT_CONNECTION_HPP
#define IRODS_CLIENT_CONNECTION_HPP

/// \file

#include <string_view>
#include <memory>

/// \brief Forward declaration of the underlying connection type.
struct RcComm;

/// \brief A namespace that encapsulates experimental implementations that 
///        may experience frequent changes.
namespace irods::experimental
{
    /// \brief A tag type that indicates whether or not a client connection
    ///        should allow the user to control when the connection and
    ///        authentication occurs.
    struct defer_connection {} defer_connection;

    /// \brief This move-only class provides a convenient way to connect to and
    ///        disconnect from an iRODS server in a safe manner.
    class client_connection
    {
    public:
        /// \brief Connects to an iRODS server.
        ///
        /// Connects and authenticates using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        client_connection();

        /// \brief Connects to an iRODS server.
        ///
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
        client_connection(const std::string_view _host,
                          const int _port,
                          const std::string_view _username,
                          const std::string_view _zone);

        /// \brief Takes ownership of a raw iRODS connection and provides
        ///        all of the safety guarantees found in other instantations.
        explicit client_connection(RcComm& _conn);

        /// \brief Constructs a client connection, but does not connect to a server.
        ///
        /// Instantiations of this class via this constructor must call \ref connect
        /// to establish a connection and authenticate the user.
        explicit client_connection(struct defer_connection);

        client_connection(client_connection&& _other) = default;
        auto operator=(client_connection&& _other) -> client_connection& = default;

        /// \brief Closes the underlying connection if active.
        ~client_connection() = default;

        /// \brief Connects to an iRODS server.
        ///
        /// Connects and authenticates using the information and credentials
        /// found in the user's irods_environment.json file.
        ///
        /// \throws irods::exception If an error occurred.
        auto connect() -> void;

        /// \brief Connects to an iRODS server.
        ///
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
        auto connect(const std::string_view _host,
                     const int _port,
                     const std::string_view _username,
                     const std::string_view _zone) -> void;

        /// \brief Closes the underlying connection if active.
        auto disconnect() noexcept -> void;

        /// \brief Checks whether \p *this is holding an active connection.
        ///
        /// \returns A boolean.
        /// \retval true  If the \p *this contains an active connection.
        /// \retval false Otherwise.
        operator bool() const noexcept;

        /// \brief Returns a reference to the underlying connection.
        ///
        /// \throws irods::exception If the underlying connection is null.
        operator RcComm&() const;

        /// \brief Returns a pointer to the underlying connection.
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

