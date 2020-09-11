#include "catch.hpp"

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"
#include "parallel_transfer_engine.hpp"
#include "replica.hpp"
#include "rodsClient.h"
#include "stream_factory_utility.hpp"
#include "transport/default_transport.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>

namespace ix = irods::experimental;
namespace io = irods::experimental::io;

template <typename Stream>
using stream_factory_functor = std::function<Stream(std::ios_base::openmode, Stream*)>;

auto create_local_file(const boost::filesystem::path& _p, std::size_t _size) noexcept -> bool;

constexpr auto operator ""_mb(unsigned long long _x) noexcept -> std::size_t
{
    return _x * 1024 * 1024;
}

template <typename SourceStream,
          typename SinkStream>
auto parallel_transfer(stream_factory_functor<SourceStream> _source_stream_factory,
                       stream_factory_functor<SinkStream> _sink_stream_factory,
                       std::int64_t _total_bytes_to_transfer,
                       std::int8_t _number_of_channels,
                       std::int64_t _offset) -> void
{
    REQUIRE_NOTHROW([&] {
        // Cannot use temporary builder with Clang right now.
        // See clang bug 41450 for details.
        io::parallel_transfer_engine_builder<SourceStream, SinkStream>
            builder{_source_stream_factory, _sink_stream_factory, io::close_stream<SinkStream>, _total_bytes_to_transfer};

        auto transfer = builder.number_of_channels(_number_of_channels)
                               .offset(_offset)
                               .build();

        transfer.wait(); // Wait for the transfer to complete or fail.

        REQUIRE(transfer.success());
        REQUIRE(transfer.errors().empty());
    }());
}

template <typename SourceStream,
          typename SinkStream>
auto parallel_transfer_restart(stream_factory_functor<SourceStream> _source_stream_factory,
                               stream_factory_functor<SinkStream> _sink_stream_factory,
                               std::int64_t _total_bytes_to_transfer,
                               std::int8_t _number_of_channels,
                               std::int64_t _offset) -> void
{
    REQUIRE_NOTHROW([&] {
        using restart_handle_type = typename io::parallel_transfer_engine<SourceStream, SinkStream>::restart_handle_type;

        restart_handle_type restart_handle;

        {
            // Cannot use temporary builder with Clang right now.
            // See clang bug 41450 for details.
            io::parallel_transfer_engine_builder<SourceStream, SinkStream>
                builder{_source_stream_factory, _sink_stream_factory, io::close_stream<SinkStream>, _total_bytes_to_transfer};

            auto transfer = builder.number_of_channels(_number_of_channels)
                                   .offset(_offset)
                                   .build();

            restart_handle = transfer.restart_handle();

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(250ms);

            transfer.stop_and_wait();
        }

        io::parallel_transfer_engine<SourceStream, SinkStream>
            transfer{restart_handle, _source_stream_factory, _sink_stream_factory, io::close_stream<SinkStream>};

        transfer.wait();

        REQUIRE(transfer.success());
        REQUIRE(transfer.errors().empty());
    }());
}

TEST_CASE("parallel transfer engine")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    // Create the local file for testing.
    const auto local_file = boost::filesystem::current_path() / "irods_parallel_transfer_engine_test_file";
    const auto file_from_iRODS = boost::filesystem::current_path() / "irods_parallel_transfer_engine_test_file.from_iRODS";
    REQUIRE(create_local_file(local_file, 50_mb));
    irods::at_scope_exit remove_local_file{[&local_file, &file_from_iRODS] {
        boost::filesystem::remove(local_file);
        boost::filesystem::remove(file_from_iRODS);
    }};

    // Create the connection pool and transport factory.
    const auto stream_count = 5;
    auto conn_pool = irods::make_connection_pool(stream_count);

    // Create the sandbox collection.
    const auto sandbox = ix::filesystem::path{env.rodsHome} / "unit_testing_sandbox";

    if (!ix::filesystem::client::exists(conn_pool->get_connection(), sandbox)) {
        ix::filesystem::client::create_collection(conn_pool->get_connection(), sandbox);
    }

    irods::at_scope_exit remove_sandbox{[conn_pool, &sandbox] {
        namespace fs = ix::filesystem;
        fs::client::remove_all(conn_pool->get_connection(), sandbox, fs::remove_options::no_trash);
    }};

    // Define the path for the data object that will be written to and read from.
    const auto data_object = sandbox / local_file.filename().c_str();

    const auto offset = 0;
    const auto dstream_fac = io::make_dstream_factory(*conn_pool, data_object.string());
    const auto fstream_fac = io::make_fstream_factory(local_file.c_str());

    SECTION("no restart")
    {
        const auto local_file_size = boost::filesystem::file_size(local_file);

        // Push file from the local filesystem into iRODS.
        {
            const auto total_bytes_to_transfer = static_cast<std::int64_t>(local_file_size);

            parallel_transfer<std::fstream, io::managed_dstream>(fstream_fac, dstream_fac, total_bytes_to_transfer, stream_count, offset);

            // Verify the size of the sink file.
            auto conn = conn_pool->get_connection();
            REQUIRE(irods::experimental::replica::replica_size<rcComm_t>(conn, data_object.c_str(), 0) == local_file_size);
        }

        // Pull file from iRODS to the local filesystem.
        {
            std::int64_t data_object_size = 0;

            {
                namespace replica = irods::experimental::replica;
                auto conn = conn_pool->get_connection();
                data_object_size = static_cast<std::int64_t>(replica::replica_size<rcComm_t>(conn, data_object.c_str(), 0));
            }

            parallel_transfer<io::managed_dstream, std::fstream>(dstream_fac, fstream_fac, data_object_size, stream_count, offset);

            // Verify the size of the sink file.
            REQUIRE(boost::filesystem::file_size(local_file) == local_file_size);
        }
    }

    SECTION("with restart")
    {
        namespace fs = boost::filesystem;

        const auto local_file_size = fs::file_size(local_file);
        const auto total_bytes_to_transfer = static_cast<std::int64_t>(local_file_size);

        parallel_transfer_restart<std::fstream, io::managed_dstream>(fstream_fac, dstream_fac, total_bytes_to_transfer, stream_count, offset);

        // Verify the size of the sink file.
        auto conn = conn_pool->get_connection();
        REQUIRE(irods::experimental::replica::replica_size<rcComm_t>(conn, data_object.c_str(), 0) == local_file_size);
    }
}

auto create_local_file(const boost::filesystem::path& _p, std::size_t _size) noexcept -> bool
{
    std::array<char, 1024 * 1024> buf{};
    std::fill(std::begin(buf), std::end(buf), 'K');

    if (std::ofstream out{_p.c_str(), std::ios::binary | std::ios::out}; out) {
        for (std::size_t i = 0; i < _size; i += buf.size()) {
            out.write(buf.data(), buf.size());
        }

        return true;
    }

    return false;
}

