#include "catch.hpp"

#include "connection_pool.hpp"
#include "dstream.hpp"
#include "irods_at_scope_exit.hpp"
#include "replica.hpp"
#include "rodsClient.h"
#include "transport/default_transport.hpp"

#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;
namespace replica = irods::experimental::replica;

TEST_CASE("replica", "[replica]")
{
    load_client_api_plugins();

    auto conn_pool = irods::make_connection_pool();
    auto conn_proxy = conn_pool->get_connection();
    rcComm_t& conn = conn_proxy;

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_replica";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    const auto target_object = sandbox / "target_object";

    SECTION("library operations")
    {
        using namespace std::chrono_literals;

        std::string_view expected_checksum = "sha2:47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=";

        {
            io::client::default_transport tp{conn};
            io::odstream{tp, target_object};
        }

        // size
        REQUIRE(replica::is_replica_empty(conn, target_object, 0));

        // checksum
        REQUIRE(replica::replica_checksum(conn, target_object, 0) == expected_checksum);

        // mtime (access)
        const auto old_mtime = replica::last_write_time(conn, target_object, 0);
        std::this_thread::sleep_for(2s);

        // mtime (modification)
        using clock_type = fs::object_time_type::clock;
        using duration_type = fs::object_time_type::duration;

        const auto now = std::chrono::time_point_cast<duration_type>(clock_type::now());
        REQUIRE(old_mtime != now);

        replica::last_write_time(conn, target_object, 0, now);
        const auto updated = replica::last_write_time(conn, target_object, 0);
        REQUIRE(updated == now);
    }

    SECTION("replica size")
    {
        std::string_view s = "testing";

        {
            io::client::default_transport tp{conn};
            io::odstream{tp, target_object} << s;
        }

        REQUIRE(!replica::is_replica_empty(conn, target_object, 0));
        REQUIRE(s.length() == replica::replica_size(conn, target_object, 0));
    }
}

