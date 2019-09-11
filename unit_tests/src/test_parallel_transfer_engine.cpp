#include "catch.hpp"

#include "rodsClient.h"
#include "connection_pool.hpp"
#include "dstream_factory.hpp"
#include "fstream_factory.hpp"
#include "stream_factory_utility.hpp"
#include "transport/default_transport_factory.hpp"
#include "parallel_transfer_engine.hpp"
#include "filesystem.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_query.hpp"

#include <boost/filesystem.hpp>

#include <ios>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <algorithm>

namespace ix = irods::experimental;
namespace io = irods::experimental::io;

auto create_local_file(const boost::filesystem::path& _p, std::size_t _size) noexcept -> bool;

constexpr auto operator ""_mb(unsigned long long _x) noexcept -> std::size_t
{
    return _x * 1024 * 1024;
}

template <typename SourceStreamFactory,
          typename SinkStreamFactory>
auto parallel_transfer(SourceStreamFactory& _source_stream_factory,
                       SinkStreamFactory& _sink_stream_factory,
                       std::int64_t _total_bytes_to_transfer,
                       std::int8_t _number_of_streams,
                       std::int64_t _offset) -> void
{
#ifdef IRODS_ENABLE_TIME_TRACKING
    using std::chrono::system_clock;

    const auto start = system_clock::now();
#endif // IRODS_ENABLE_TIME_TRACKING

    REQUIRE_NOTHROW([&] {
        // Cannot use temporary builder with Clang right now.
        // See clang bug 41450 for details.
        io::parallel_transfer_engine_builder builder{_source_stream_factory,
                                                     _sink_stream_factory,
                                                     io::make_stream<SourceStreamFactory>,
                                                     io::make_stream<SinkStreamFactory>,
                                                     io::close_stream<typename SinkStreamFactory::stream_type>,
                                                     _total_bytes_to_transfer};

        auto transfer = builder.number_of_streams(_number_of_streams)
                               .offset(_offset)
                               .build();

        transfer.wait(); // Wait for the transfer to complete or fail.

#ifdef IRODS_ENABLE_TIME_TRACKING
        using std::chrono::duration;

        std::cout << "time elapsed: " << duration<float>(system_clock::now() - start).count() << "s\n";
#endif // IRODS_ENABLE_TIME_TRACKING

        REQUIRE(transfer.success());
        REQUIRE(transfer.errors().empty());
    }());
}

template <typename SourceStreamFactory,
          typename SinkStreamFactory>
auto parallel_transfer_restart(SourceStreamFactory& _source_stream_factory,
                               SinkStreamFactory& _sink_stream_factory,
                               std::int64_t _total_bytes_to_transfer,
                               std::int8_t _number_of_streams,
                               std::int64_t _offset) -> void
{
#ifdef IRODS_ENABLE_TIME_TRACKING
    using std::chrono::system_clock;
    using std::chrono::duration;

    auto start = system_clock::now();
#endif // IRODS_ENABLE_TIME_TRACKING

    REQUIRE_NOTHROW([&] {
        using restart_handle_type = typename io::parallel_transfer_engine<SourceStreamFactory, SinkStreamFactory>::restart_handle_type;

        restart_handle_type restart_handle;

        {
            // Cannot use temporary builder with Clang right now.
            // See clang bug 41450 for details.
            io::parallel_transfer_engine_builder builder{_source_stream_factory,
                                                         _sink_stream_factory,
                                                         io::make_stream<SourceStreamFactory>,
                                                         io::make_stream<SinkStreamFactory>,
                                                         io::close_stream<typename SinkStreamFactory::stream_type>,
                                                         _total_bytes_to_transfer};

            auto transfer = builder.number_of_streams(_number_of_streams)
                                   .offset(_offset)
                                   .build();

            restart_handle = transfer.restart_handle();

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(250ms);

            transfer.stop_and_wait();

#ifdef IRODS_ENABLE_TIME_TRACKING
            std::cout << "time elapsed (stopped): " << duration<float>(system_clock::now() - start).count() << "s\n";
#endif // IRODS_ENABLE_TIME_TRACKING
        }

#ifdef IRODS_ENABLE_TIME_TRACKING
        start = system_clock::now();
#endif // IRODS_ENABLE_TIME_TRACKING

        io::parallel_transfer_engine transfer{restart_handle,
                                              _source_stream_factory,
                                              _sink_stream_factory,
                                              io::make_stream<SourceStreamFactory>,
                                              io::make_stream<SinkStreamFactory>,
                                              io::close_stream<typename SinkStreamFactory::stream_type>};

        transfer.wait();

#ifdef IRODS_ENABLE_TIME_TRACKING
        std::cout << "time elapsed (restarted): " << duration<float>(system_clock::now() - start).count() << "s\n";
#endif // IRODS_ENABLE_TIME_TRACKING

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
    io::native_transport_factory xport_fac{*conn_pool};

    // Create the sandbox collection.
    const auto sandbox = ix::filesystem::path{env.rodsHome} / "unit_testing_sandbox";

    if (!ix::filesystem::client::exists(conn_pool->get_connection(), sandbox)) {
        ix::filesystem::client::create_collection(conn_pool->get_connection(), sandbox);
    }

    irods::at_scope_exit remove_sandbox{[conn_pool, &sandbox] {
        namespace fs = irods::experimental::filesystem;
        fs::client::remove_all(conn_pool->get_connection(), sandbox, fs::remove_options::no_trash);
    }};

    // Define the path for the data object that will be written to and read from.
    const auto data_object = sandbox / local_file.filename().c_str();

    const auto offset = 0;

    SECTION("no restart")
    {
        const auto local_file_size = boost::filesystem::file_size(local_file);

        // Push file from the local filesystem into iRODS.
        {
            namespace fs = boost::filesystem;

            io::fstream_factory src_fac{local_file.c_str()};
            io::dstream_factory sink_fac{xport_fac, data_object};

            parallel_transfer(src_fac,
                              sink_fac,
                              static_cast<std::int64_t>(local_file_size),
                              stream_count, 
                              offset);

            // Verify the size of the sink file.
            auto conn = conn_pool->get_connection();
            REQUIRE(ix::filesystem::client::data_object_size(conn, sink_fac.object_name()) == local_file_size);
        }

        // Pull file from iRODS to the local filesystem.
        {
            namespace fs = irods::experimental::filesystem;

            io::dstream_factory src_fac{xport_fac, data_object};
            io::fstream_factory sink_fac{file_from_iRODS.c_str()};

            std::int64_t data_object_size = 0;

            {
                auto conn = conn_pool->get_connection();
                data_object_size = static_cast<std::int64_t>(fs::client::data_object_size(conn, src_fac.object_name()));
            }

            parallel_transfer(src_fac,
                              sink_fac,
                              data_object_size,
                              stream_count, 
                              offset);

            // Verify the size of the sink file.
            REQUIRE(boost::filesystem::file_size(local_file) == local_file_size);
        }
    }

    SECTION("with restart")
    {
        namespace fs = boost::filesystem;

        io::fstream_factory src_fac{local_file.c_str()};
        io::dstream_factory sink_fac{xport_fac, data_object};
        const auto local_file_size = fs::file_size(src_fac.object_name());

        parallel_transfer_restart(src_fac,
                                  sink_fac,
                                  static_cast<std::int64_t>(local_file_size),
                                  stream_count, 
                                  offset);

        // Verify the size of the sink file.
        auto conn = conn_pool->get_connection();
        REQUIRE(ix::filesystem::client::data_object_size(conn, sink_fac.object_name()) == local_file_size);
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

