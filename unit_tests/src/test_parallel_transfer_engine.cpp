#include "catch.hpp"

#include "rodsClient.h"

#include "connection_pool.hpp"
#include "dstream_factory.hpp"
#include "fstream_factory.hpp"
#include "stream_factory_utility.hpp"
#include "parallel_transfer_engine.hpp"
#include "filesystem.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_query.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <memory>

namespace io = irods::experimental::io;

template <typename SourceStreamFactory,
          typename SinkStreamFactory>
void parallel_transfer(SourceStreamFactory& _source_stream_factory,
                       SinkStreamFactory& _sink_stream_factory,
                       std::int64_t _total_bytes_to_transfer,
                       std::int8_t _number_of_streams,
                       std::int64_t _offset,
                       std::function<void()> _handler)
{
    using std::chrono::system_clock;

    // Used for tracking how long transfers take.
    const auto start = system_clock::now();

    try {
        // TODO Cannot use temporary builder with Clang right now.
        // See clang bug 41450 for details.
        io::parallel_transfer_engine_builder builder{_source_stream_factory,
                                                     _sink_stream_factory,
                                                     io::make_stream<SourceStreamFactory>,
                                                     io::make_stream<SinkStreamFactory>,
                                                     _total_bytes_to_transfer};

        auto transfer = builder.number_of_streams(_number_of_streams)
                               .offset(_offset)
                               .build();

        transfer.wait(); // Wait for the transfer to complete or fail.

        using std::chrono::duration;

        std::cout << "time elapsed: " << duration<float>(system_clock::now() - start).count() << "s\n";

        if (!transfer.success()) {
            for (auto&& [error, msg] : transfer.errors()) {
                std::cerr << msg << '\n';
            }

            return;
        }

        _handler();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}

TEST_CASE("parallel transfer engine")
{
    auto& api_table = irods::get_client_api_table();
    auto& pck_table = irods::get_pack_table();
    init_api_table(api_table, pck_table);

    namespace io = irods::experimental::io;

    const auto stream_count = 5;
    const auto refresh_time = 600;

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) >= 0);

    // Spin up some connections to iRODS.
    irods::connection_pool conn_pool{stream_count,
                                     env.rodsHost,
                                     env.rodsPort,
                                     env.rodsUserName,
                                     env.rodsZone,
                                     refresh_time};

    const auto on_transfer_successful = [] { std::cout << "Transfer finished!\n"; };
    const auto offset = 0;

    {
        namespace fs = boost::filesystem;

        io::fstream_factory src_fac{"/home/kory/Downloads/CLion-2019.2.tar.gz"};
        io::dstream_factory sink_fac{conn_pool, "/tempZone/home/kory/CLion-2019.2.tar.gz"};

        parallel_transfer(src_fac,
                          sink_fac,
                          static_cast<std::int64_t>(fs::file_size(src_fac.object_name())),
                          stream_count, 
                          offset,
                          on_transfer_successful);
    }

    {
        namespace fs = irods::experimental::filesystem;

        io::dstream_factory src_fac{conn_pool, "/tempZone/home/kory/CLion-2019.2.tar.gz"};
        io::fstream_factory sink_fac{"/home/kory/Downloads/CLion-2019.2-from_iRODS.tar.gz"};

        std::int64_t data_object_size = 0;

        {
            auto conn = conn_pool.get_connection();
            data_object_size = static_cast<std::int64_t>(fs::client::data_object_size(conn, src_fac.object_name()));
        }

        parallel_transfer(src_fac,
                          sink_fac,
                          data_object_size,
                          stream_count, 
                          offset,
                          on_transfer_successful);
    }
}

