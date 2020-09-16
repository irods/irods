#include "catch.hpp"

#include "client_connection.hpp"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "irods_at_scope_exit.hpp"
#include "replica.hpp"
#include "resource_administration.hpp"
#include "rodsClient.h"
#include "transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;
namespace replica = irods::experimental::replica;

// IMPORTANT NOTE REGARDING THE CLIENT_CONNECTION OBJECTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Connection Pools do not work well with the resource administration library
// because the resource manager within the agents do not see any changes to the
// resource hierarchies. The only way around this is to spawn a new agent by
// creating a new connection to the iRODS server.

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

    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
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

        const std::string_view resc_name = "unit_test_ufs";

        {
            // create resource.
            irods::experimental::client_connection conn;
            REQUIRE(unit_test_utils::add_ufs_resource(conn, resc_name, "unit_test_vault"));
        }

        {
            // replicate replica.
            irods::experimental::client_connection conn;
            REQUIRE(unit_test_utils::replicate_data_object(conn, target_object.c_str(), resc_name));
        }

        irods::at_scope_exit clean_up{[&target_object, &resc_name] {
            namespace ix = irods::experimental;
            ix::client_connection conn;
            fs::client::remove(conn, target_object, fs::remove_options::no_trash);
            ix::administration::client::remove_resource(conn, resc_name);
        }};

        irods::experimental::client_connection conn;

        // the replica number of the second replica.
        const auto second_replica = 1;

        // show that the mtime of each replica is different.
        REQUIRE(old_mtime != replica::last_write_time<RcComm>(conn, target_object, second_replica));

        // mtime (modification)
        replica::last_write_time<RcComm>(conn, target_object, second_replica, old_mtime);
        REQUIRE(old_mtime == replica::last_write_time<RcComm>(conn, target_object, second_replica));
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

