#include "catch.hpp"

#include "rcConnect.h"
#include "rodsClient.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "connection_pool.hpp"
#include "dstream.hpp"
#include "transport/default_transport.hpp"
#include "filesystem.hpp"
#include "get_file_descriptor_info.h"

#include <json.hpp>

TEST_CASE("get_file_descriptor_info")
{
    namespace fs = irods::experimental::filesystem;
    namespace io = irods::experimental::io;
    using json   = nlohmann::json;

    auto& api_table = irods::get_client_api_table();
    auto& pack_table = irods::get_pack_table();
    init_api_table(api_table, pack_table);

    rodsEnv env;
    REQUIRE(getRodsEnv(&env) == 0);

    irods::connection_pool conn_pool{1, env.rodsHost, env.rodsPort, env.rodsUserName, env.rodsZone, 600};

    auto conn = conn_pool.get_connection();

    const auto sandbox = fs::path{env.rodsHome} / "irods_unit_tests_sandbox";
    const auto data_object_path = sandbox / "dstream_data_object.txt";

    fs::client::create_collection(conn, sandbox);

    char* json_output = nullptr;

    // Guarantees that the stream is closed before clean up.
    {
        io::client::default_transport tp{conn};
        io::odstream out{tp, data_object_path};
        REQUIRE(out.is_open());

        const auto json_input = json{{"fd", out.file_descriptor()}}.dump();

        REQUIRE(rc_get_file_descriptor_info(static_cast<rcComm_t*>(conn), json_input.c_str(), &json_output) == 0);
    }

    // Clean up.
    fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash);

    json info;
    REQUIRE_NOTHROW(info = json::parse(json_output));

    // Verify existence of properties.
    REQUIRE(info.count("l3descInx"));
    REQUIRE(info.count("data_object_input_replica_flag"));
    REQUIRE(info.count("data_object_input"));
    REQUIRE(info.count("data_object_info"));
    REQUIRE(info.count("other_data_object_info"));
    REQUIRE(info.count("data_size"));
}

