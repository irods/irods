#ifndef IRODS_IO_TRANSPORT_HPP
#define IRODS_IO_TRANSPORT_HPP

/// \file

#include "filesystem/path.hpp"

#include <ios>
#include <string>

namespace irods::experimental::io
{
    /// A wrapper type used to represent a replica token usually
    /// obtained via the replica access table.
    ///
    /// \since 4.2.9
    struct replica_token
    {
        std::string value;
    };

    /// A wrapper type used to represent a root resource name.
    ///
    /// \since 4.2.9
    struct root_resource_name
    {
        std::string value;
    };

    /// A wrapper type used to represent a leaf resource name.
    ///
    /// \since 4.2.9
    struct leaf_resource_name
    {
        std::string value;
    };

    /// A wrapper type used to represent a replica number.
    ///
    /// \since 4.2.9
    struct replica_number
    {
        int value;
    };

    /// A type that holds instructions about how the server should
    /// proceed during a replica close operation.
    ///
    /// \since 4.2.9
    struct on_close_success
    {
        bool update_size = true;                   ///< Instructs the server to update the replica size.
        bool update_status = true;                 ///< Instructs the server to update the replica status.
        bool compute_checksum = false;             ///< Instructs the server to compute a checksum.
        bool send_notifications = true;            ///< Instructs the server to notify other services.
        bool preserve_replica_state_table = false; ///< Instructs the server to update the replica state table.
    };

    /// \brief The base interface that serves as a way to extend and/or customize how bytes are
    ///        transferred to and from an iRODS server.
    ///
    /// This interface serves as a way to represent a transport protocol in regards to iRODS.
    /// The member functions should only focus on carrying out the operation requested. It is
    /// not the transport's responsibility to guarantee that pre-conditions are satisfied
    /// before use. That responsibility belongs to the types that accept a transport instance.
    ///
    /// \since 4.2.6
    template <typename CharT,
              typename Traits = std::char_traits<CharT>>
    class transport
    {
    public:
        // clang-format off
        using char_type   = CharT;
        using traits_type = Traits;
        using int_type    = typename traits_type::int_type;
        using pos_type    = typename traits_type::pos_type;
        using off_type    = typename traits_type::off_type;
        // clang-format on

        virtual ~transport() {}

        /// Creates or opens a data object in the specified mode.
        ///
        /// \param[in] _path The logical path to a data object.
        /// \param[in] _mode Specifies how the data object should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the data object was opened successfully.
        /// \retval false If the data object could not be opened.
        virtual bool open(const irods::experimental::filesystem::path& _path,
                          std::ios_base::openmode _mode) = 0;

        /// Opens a specific replica of a data object in the specified mode.
        ///
        /// Implementations of this function are not allowed to create new replicas.
        ///
        /// \param[in] _path           The logical path to a data object.
        /// \param[in] _replica_number The replica number that identifies the replica to open.
        /// \param[in] _mode           Specifies how the replica should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the replica was opened successfully.
        /// \retval false If the replica could not be opened.
        virtual bool open(const irods::experimental::filesystem::path& _path,
                          const replica_number& _replica_number,
                          std::ios_base::openmode _mode) = 0;

        /// Creates or opens a specific replica of a data object in the specified mode.
        ///
        /// \param[in] _path               The logical path to a data object.
        /// \param[in] _root_resource_name Specifies under which root resource to create a new replica
        ///                                or under which root resource an existing replica resides.
        /// \param[in] _mode               Specifies how the replica should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the replica was opened successfully.
        /// \retval false If the replica could not be opened.
        virtual bool open(const irods::experimental::filesystem::path& _path,
                          const root_resource_name& _root_resource_name,
                          std::ios_base::openmode _mode) = 0;

        /// Creates or opens a specific replica of a data object in the specified mode.
        ///
        /// \param[in] _path               The logical path to a data object.
        /// \param[in] _leaf_resource_name Specifies on which leaf resource to create a new replica
        ///                                or on which leaf resource an existing replica resides.
        /// \param[in] _mode               Specifies how the replica should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the replica was opened successfully.
        /// \retval false If the replica could not be opened.
        virtual bool open(const irods::experimental::filesystem::path& _path,
                          const leaf_resource_name& _leaf_resource_name,
                          std::ios_base::openmode _mode) = 0;

        /// Opens a previously opened replica of a data object in the specified mode.
        ///
        /// \param[in] _replica_token  The token used to allow simultaneous write access to a
        ///                            previously opened replica.
        /// \param[in] _path           The logical path to a data object.
        /// \param[in] _replica_number The replica number that identifies the replica to open.
        /// \param[in] _mode           Specifies how the replica should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the replica was opened successfully.
        /// \retval false If the replica could not be opened.
        virtual bool open(const replica_token& _replica_token,
                          const irods::experimental::filesystem::path& _path,
                          const replica_number& _replica_number,
                          std::ios_base::openmode _mode) = 0;

        /// Opens a previously opened replica of a data object in the specified mode.
        ///
        /// \param[in] _replica_token      The token used to allow simultaneous write access to a
        ///                                previously opened replica.
        /// \param[in] _path               The logical path to a data object.
        /// \param[in] _leaf_resource_name Specifies on which leaf resource an existing replica resides.
        /// \param[in] _mode               Specifies how the replica should be opened.
        ///
        /// \return A boolean.
        /// \retval true  If the replica was opened successfully.
        /// \retval false If the replica could not be opened.
        virtual bool open(const replica_token& _replica_token,
                          const irods::experimental::filesystem::path& _path,
                          const leaf_resource_name& _leaf_resource_name,
                          std::ios_base::openmode _mode) = 0;

        /// Closes an open replica.
        ///
        /// \param[in] _on_close_success A pointer to an ::on_close_success object that instructs
        ///                              the server to skip or perform certain operations. If the pointer
        ///                              is null, implementations of this function must default to
        ///                              instructing the server to update the catalog and skip computing
        ///                              a checksum.
        ///
        /// \return A boolean.
        /// \retval true  On success.
        /// \retval false On failure.
        virtual bool close(const on_close_success* _on_close_success = nullptr) = 0;

        /// Fills the specified buffer with bytes received from the server.
        ///
        /// \param[in] _buffer      The buffer to fill up to \p _buffer_size.
        /// \param[in] _buffer_size The expected number of bytes to receive from the server.
        ///
        /// \return The number of bytes actually received.
        virtual std::streamsize receive(char_type* _buffer, std::streamsize _buffer_size) = 0;

        /// Sends the contents of a buffer to the server.
        ///
        /// \param[in] _buffer      The buffer to send containing \p _buffer_size bytes.
        /// \param[in] _buffer_size The expected number of bytes to send to the server.
        ///
        /// \return The number of bytes actually sent.
        virtual std::streamsize send(const char_type* _buffer, std::streamsize _buffer_size) = 0;

        /// Updates the location of the read/write position within a replica.
        ///
        /// \param[in] _offset The byte offset based on \p _dir.
        /// \param[in] _dir    An indicator for how \p _offset should be interpreted.
        ///
        /// \return The new location of the read/write position.
        virtual pos_type seekpos(off_type _offset, std::ios_base::seekdir _dir) = 0;

        /// Indicates whether a replica is open or not.
        ///
        /// \return A boolean.
        /// \retval true  If the replica is open.
        /// \retval false Otherwise.
        virtual bool is_open() const noexcept = 0;

        /// The underlying file descriptor.
        ///
        /// The meaning of the value returned is implementation defined.
        ///
        /// \return An integer.
        /// \retval Implementation defined.
        virtual int file_descriptor() const noexcept = 0;

        /// The name of the root resource where the replica resides.
        virtual const root_resource_name& root_resource_name() const = 0;

        /// The name of the leaf resource where the replica resides.
        virtual const leaf_resource_name& leaf_resource_name() const = 0;

        /// The replica number that identifies the replica.
        virtual const replica_number& replica_number() const = 0;

        /// The replica token attached to the replica.
        ///
        /// Replica tokens only apply to write operations. Replica opened only for reads do
        /// not produce replica tokens.
        virtual const replica_token& replica_token() const = 0;
    }; // class transport
} // irods::experimental::io

#endif // IRODS_IO_TRANSPORT_HPP

