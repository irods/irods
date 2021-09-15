#ifndef IRODS_IO_STREAM_FACTORY_UTILITY_HPP
#define IRODS_IO_STREAM_FACTORY_UTILITY_HPP

#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "connection_pool.hpp"

#include <fstream>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace irods::experimental::io
{
    /// Reserved space for the iRODS implementation. Symbols within this namespace are not
    /// meant to be used in client code.
    namespace detail
    {
        // Catch-all case.
        constexpr auto has_special_close_function(...) -> void;

        // Used to detects if the sink stream object supports a close function
        // similar to the one provided by basic_dstream.
        template <typename SinkStreamType>
        constexpr auto has_special_close_function(SinkStreamType& _out)
            -> decltype((void)(_out.close(std::declval<on_close_success*>())), bool());
    } // namespace detail

    /// A stream class that packages an iRODS connection, transport, and dstream object together
    /// for use with the Parallel Transfer Engine.
    ///
    /// The members of this structure are public allowing developers to construct them however
    /// they like. 
    ///
    /// \since 4.2.9
    struct managed_dstream
        : public std::iostream
    {
        /// Default constructor.
        ///
        /// \param[in] _conn A connection proxy from a connection pool. Ownership of the
        ///                  connection will be transferred to the newly created stream object.
        managed_dstream(irods::connection_pool::connection_proxy _conn)
            : std::iostream{nullptr}
            , conn{std::move(_conn)}
            , transport{}
            , stream{}
        {
            this->rdbuf(stream.rdbuf());
        }

        /// Move constructor.
        ///
        /// \param[in] _other The stream to move from.
        managed_dstream(managed_dstream&& _other)
            : std::iostream{std::move(_other)}
            , conn{std::move(_other.conn)}
            , transport{std::move(_other.transport)}
            , stream{std::move(_other.stream)}
        {
            this->set_rdbuf(stream.rdbuf());
            this->setstate(_other.rdstate());
        }

        /// Move assignment operator.
        ///
        /// \param[in] _other The stream to move from.
        auto operator=(managed_dstream&& _other) -> managed_dstream&
        {
            std::iostream::operator=(std::move(_other));
            conn = std::move(_other.conn);
            transport = std::move(_other.transport);
            stream = std::move(_other.stream);

            this->set_rdbuf(stream.rdbuf());
            this->setstate(_other.rdstate());

            return *this;
        }

        /// Exposes the underlying close operation defined by the dstream interface.
        /// This enables iRODS specific options required for proper operation of dstream
        /// objects.
        ///
        /// \param[in] _on_close_success Optionally specifies what operations should happen
        ///                              following a successful close of a replica.
        auto close(io::on_close_success* _on_close_success = nullptr) -> void
        {
            stream.close(_on_close_success);
        }

        irods::connection_pool::connection_proxy conn;
        std::unique_ptr<io::client::native_transport> transport;
        io::dstream stream;
    }; // class managed_dstream

    /// Creates a factory that produces managed_dstream objects.
    ///
    /// \param[in] _conn_pool    \parblock
    ///                          The connection pool serving connections for each stream object created
    ///                          by the factory. The connection pool is not owned by the generated factory
    ///                          and must out live any streams produced by the factory.
    ///                          \endparblock
    /// \param[in] _logical_path The full path to a data object. All streams will point to this data object.
    ///
    /// \return A new factory function.
    ///
    /// \since 4.2.9
    auto make_dstream_factory(irods::connection_pool& _conn_pool, std::string _logical_path)
    {
        using openmode = std::ios_base::openmode;

        return [&_conn_pool, p = std::move(_logical_path)](openmode _mode, managed_dstream* _base)
            -> managed_dstream
        {
            managed_dstream d{_conn_pool.get_connection()};

            d.transport = std::make_unique<io::client::native_transport>(d.conn);

            if (_base) {
                auto& s = _base->stream;
                d.stream.open(*d.transport, s.replica_token(), p, s.replica_number(), _mode);
            }
            else {
                d.stream.open(*d.transport, p, _mode);
            }

            d.rdbuf(d.stream.rdbuf());

            return d;
        };
    } // make_dstream_factory

    /// Creates a factory that produces managed_dstream objects.
    ///
    /// \param[in] _conn_pool    \parblock
    ///                          The connection pool serving connections for each stream object created
    ///                          by the factory. The connection pool is not owned by the generated factory
    ///                          and must out live any streams produced by the factory.
    ///                          \endparblock
    /// \param[in] _logical_path The full path to a data object. All streams will point to this data object.
    /// \param[in] _replica_id   The replica number or resource name used to identify a specific replica.
    ///
    /// \tparam ReplicaIdentifier \parblock
    /// The type of the information used to identify a specific replica. The following types are supported:
    ///     - irods::experimental::io::replica_number
    ///     - irods::experimental::io::leaf_resource_name
    ///     - irods::experimental::io::root_resource_name
    /// \endparblock
    ///
    /// \return A new factory function.
    ///
    /// \since 4.2.11
    template <typename ReplicaIdentifier>
    auto make_dstream_factory(irods::connection_pool& _conn_pool,
                              std::string _logical_path,
                              ReplicaIdentifier&& _replica_id)
    {
        using openmode = std::ios_base::openmode;

        return [&_conn_pool, p = std::move(_logical_path), &_replica_id](openmode _mode, managed_dstream* _base)
            -> managed_dstream
        {
            managed_dstream d{_conn_pool.get_connection()};

            d.transport = std::make_unique<io::client::native_transport>(d.conn);

            if (_base) {
                auto& s = _base->stream;
                d.stream.open(*d.transport, s.replica_token(), p, s.replica_number(), _mode);
            }
            else {
                using unqualified_type = std::remove_reference_t<std::remove_cv_t<ReplicaIdentifier>>;

                static_assert(std::is_same_v<unqualified_type, io::replica_number> ||
                              std::is_same_v<unqualified_type, io::leaf_resource_name> ||
                              std::is_same_v<unqualified_type, io::root_resource_name>,
                              "Unsupported type for template parameter [ReplicaIdentifier].");

                d.stream.open(*d.transport, p, _replica_id, _mode);
            }

            d.rdbuf(d.stream.rdbuf());

            return d;
        };
    } // make_dstream_factory

    /// Create a factory that produces std::fstream objects.
    ///
    /// \since 4.2.9
    ///
    /// \param[in] _path The full path to a file on the local disk. All streams will poin to this file.
    ///
    /// \return A new factory function.
    auto make_fstream_factory(std::string _path)
    {
        return [p = std::move(_path)](std::ios_base::openmode _mode, std::fstream*) -> std::fstream
        {
            return std::fstream{p, _mode};
        };
    } // make_fstream_factory

    /// Closes a stream object.
    /// 
    /// This function is primarily meant to be used with an instance of parallel_transfer_engine. It
    /// will optimize the closing of streams supporting the on_close_success parameter. See
    /// dstream.hpp and transport.hpp for additional details.
    ///
    /// \tparam SinkStreamType The type of the sink stream. Provides compile-time information for
    ///                        controlling how the stream is handled.
    ///
    /// \param[in] _out         The stream to close.
    /// \param[in] _last_stream Indicates whether \p _out is the last open stream managed by the
    ///                         parallel_transfer_engine.
    ///
    /// \since 4.2.9
    template <typename SinkStreamType>
    auto close_stream(SinkStreamType& _out, bool _last_stream) -> void
    {
        if constexpr (std::is_same_v<decltype(detail::has_special_close_function(_out)), bool>) {
            if (!_last_stream) {
                on_close_success input;
                input.update_size = false;
                input.update_status = false;
                input.compute_checksum = false;
                input.send_notifications = false;

                _out.close(&input);
            }
            else {
                _out.close();
            }
        }
        else {
            _out.close();
        }
    } // close_stream
} // namespace irods::experimental::io

#endif // IRODS_IO_STREAM_FACTORY_UTILITY_HPP

