#ifndef IRODS_IO_STREAM_FACTORY_UTILITY_HPP
#define IRODS_IO_STREAM_FACTORY_UTILITY_HPP

#include "dstream_factory.hpp"

#include <ios>
#include <type_traits>

namespace irods::experimental::io
{
    template <typename StreamFactory>
    auto make_stream(StreamFactory& _factory,
                     std::ios_base::openmode _mode,
                     typename StreamFactory::stream_type* _base)
        -> typename StreamFactory::stream_type
    {
        if constexpr (std::is_same_v<StreamFactory, io::dstream_factory>) {
            // If "_base" is not pointing to null, then the parallel_transfer_engine
            // is requesting a stream that may need information only available
            // from the first stream (e.g. a replica number or resource hierarchy).
            if (_base) {
                return _factory(_base->replica_number(), _mode);
            }
        }

        //return _factory("child_b", _mode);      // ERROR: DIRECT_CHILD_ACCESS
        //return _factory("root;child_b", _mode); // ERROR: SYS_RESOURCE_DOES_NOT_EXIST
        //return _factory("other", _mode);
        return _factory(_mode);
    };
} // namespace irods::experimental::io

#endif // IRODS_IO_STREAM_FACTORY_UTILITY_HPP
