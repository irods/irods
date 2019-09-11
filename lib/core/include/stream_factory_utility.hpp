#ifndef IRODS_IO_STREAM_FACTORY_UTILITY_HPP
#define IRODS_IO_STREAM_FACTORY_UTILITY_HPP

#include "dstream_factory.hpp"

#include <ios>
#include <type_traits>

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

    /// A convenience function that provides support for basic_dstream_factory and fstream_factory.
    /// in the context of parallel transfer.
    ///
    /// \since 4.2.9
    ///
    /// \tparam StreamFactory The deduced type of \p _factory.
    ///
    /// \param[in] _factory The stream factory that will be used to create new stream objects.
    /// \param[in] _mode    The mode to open the streams in.
    /// \param[in] _base    A pointer to the first stream object created by the factory. The pointer
    ///                     will be null when creating the first stream.
    template <typename StreamFactory>
    auto make_stream(StreamFactory& _factory,
                     std::ios_base::openmode _mode,
                     typename StreamFactory::stream_type* _base)
        -> typename StreamFactory::stream_type
    {
        if constexpr (std::is_same_v<StreamFactory, io::dstream_factory>) {
            // If "_base" is not pointing to null, then the parallel_transfer_engine
            // is requesting a stream that may need information only available
            // from the first stream (e.g. a replica number or leaf resource).
            if (_base) {
                return _factory(_base->replica_token(), _base->replica_number(), _mode);
            }
        }

        return _factory(_mode);
    };

    /// Closes a stream object.
    /// 
    /// This function is primarily meant to be used with an instance of parallel_transfer_engine. It
    /// will optimize the closing of streams supporting the on_close_success parameter. See
    /// dstream.hpp and transport.hpp for additional details.
    ///
    /// \since 4.2.9
    ///
    /// \tparam SinkStreamType The type of the sink stream. Provides compile-time information for
    ///                        controlling how the stream is handled.
    ///
    /// \param[in] _out         The stream to close.
    /// \param[in] _last_stream Indicates whether \p _out is the last open stream managed by the
    ///                         parallel_transfer_engine.
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
    }
} // namespace irods::experimental::io

#endif // IRODS_IO_STREAM_FACTORY_UTILITY_HPP

