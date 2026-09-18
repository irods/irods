#include <catch2/catch_all.hpp>

#include "irods/client_connection.hpp"
#include "irods/data_object_modify_info.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_client_api_table.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_pack_table.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/modDataObjMeta.h"
#include "irods/objInfo.h"
#include "irods/objStat.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/replica.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/rodsKeyWdDef.h"
#include "irods/rodsType.h"
#include "irods/touch.h"
#include "irods/transport/default_transport.hpp"

#include <boost/asio/ip/host_name.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <array>
#include <climits>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;
namespace adm = irods::experimental::administration;

auto stat(RcComm& _comm, const fs::path& _path) -> std::unique_ptr<rodsObjStat, decltype(freeRodsObjStat)&>
{
    dataObjInp_t input{};
    std::strncpy(static_cast<char*>(input.objPath), _path.c_str(), sizeof(input.objPath) - 1);

    rodsObjStat* output{};

    REQUIRE(rcObjStat(&_comm, &input, &output) >= 0);
    REQUIRE(output != nullptr);
    return {output, freeRodsObjStat};
}

auto set_replica_status(RcComm& _comm,
                        const fs::path& _path,
                        int replica,
                        int status,
                        const std::unordered_map<std::string, std::string>& _additional_kvp_args) -> int
{
    auto [kvp, lm] = irods::experimental::make_key_value_proxy();
    kvp[REPL_STATUS_KW] = std::to_string(status);
    kvp[ADMIN_KW] = "";

    std::for_each(std::cbegin(_additional_kvp_args),
                  std::cend(_additional_kvp_args),
                  [&kvp](const auto& _kv_to_insert) { kvp[_kv_to_insert.first] = _kv_to_insert.second; });

    // Specify the data object we want to mess with
    DataObjInfo info{};
    std::strncpy(static_cast<char*>(info.objPath), _path.c_str(), sizeof(info.objPath) - 1);
    info.replNum = replica;

    // Create the required input
    ModDataObjMetaInp inp{&info, kvp.get()};
    return rcModDataObjMeta(&_comm, &inp);
}

TEST_CASE("rcObjStat on a data object with no heirarchy returns the object status of the data object requested")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;

    const auto sandbox = fs::path{static_cast<char*>(env.rodsHome)} / "irods_unit_tests_sandbox";
    const auto path = sandbox / "dstream_data_object.txt";

    fs::client::create_collection(conn, sandbox);

    irods::at_scope_exit cleanup{[&] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a data object in iRODS.
    // This is used in all future sections.
    {
        io::client::default_transport transport{conn};
        io::odstream out{transport, path};
    }

    auto res{stat(conn, path)};
    REQUIRE(res->objSize == 0);
}

// This structure provides the setup and teardown for tests that
// request its use.
//
// The structure provides:
//  - a connection that may be used throughout the test
//  - a UUID to help prevent naming collisions
//  - a test specific collection (sandbox)
//  - a simple resource heirarchy, replication with two unix resources beneath it
//
// Tests wishing to use the structure may use it by specificing it in the following:
//  - TEST_CASE_PERSISTENT_FIXTURE(classname, ...)
//    - Runs setup and teardown once, regardless of SECTION(...)
//  - TEST_CASE_METHOD(classname, ...)
//    - May run setup multiple times if SECTION(...) is present
struct test_fixture_for_issue_8993
{
    // Ignore member variable complaints for now
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    // UUID for ensuring uniqueness of paths and resources
    rodsEnv env;
    std::string test_uui;

    irods::experimental::client_connection conn;

    // Filesystem paths for the tests
    // Also includes paths for the new resources
    fs::path sandbox;
    fs::path test_data_object;
    std::filesystem::path res_a_path;
    std::filesystem::path res_b_path;

    // Resource info for the heirarchy
    adm::resource_registration_info res_regis_a;
    adm::resource_registration_info res_regis_b;
    adm::resource_registration_info res_regis_repl;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    // Make clang happy with the class
    auto operator=(test_fixture_for_issue_8993&) -> test_fixture_for_issue_8993 = delete;
    test_fixture_for_issue_8993(test_fixture_for_issue_8993&) = delete;
    auto operator=(test_fixture_for_issue_8993&&) -> test_fixture_for_issue_8993 = delete;
    test_fixture_for_issue_8993(test_fixture_for_issue_8993&&) = delete;

    static auto generate_uuid() -> std::string
    {
        static boost::uuids::random_generator gen;
        return to_string(gen());
    }

    test_fixture_for_issue_8993()
        : env{}
        , test_uui{generate_uuid()}
        , conn{irods::experimental::defer_connection}
    {
        // No idea if this is needed per run?
        load_client_api_plugins();
        _getRodsEnv(env);

        conn.connect();

        sandbox = fs::path{static_cast<char*>(env.rodsHome)} / fmt::format("irods_unit_test_sandbox-{}", test_uui);
        fs::client::create_collection(conn, sandbox);

        // Get hostname for unixfilesystem resources
        auto hostname{boost::asio::ip::host_name()};

        auto create_temp_directory{[](std::string_view _dir_name) -> std::filesystem::path {
            const auto temp_path{std::filesystem::temp_directory_path()};
            auto path_to_create{temp_path / _dir_name};
            std::filesystem::create_directory(path_to_create);
            return path_to_create;
        }};

        // Create some temp directories for the resources
        res_a_path = create_temp_directory(fmt::format("cool-{}", test_uui));
        res_b_path = create_temp_directory(fmt::format("thing-{}", test_uui));

        // Create resources
        // Should you even test for no throw here?
        res_regis_a = {.resource_name = fmt::format("cool-{}", test_uui),
                       .resource_type = adm::resource_type::unixfilesystem,
                       .host_name = hostname,
                       .vault_path = res_a_path.string()};
        REQUIRE_NOTHROW(adm::client::add_resource(conn, res_regis_a));
        res_regis_b = {.resource_name = fmt::format("thing-{}", test_uui),
                       .resource_type = adm::resource_type::unixfilesystem,
                       .host_name = hostname,
                       .vault_path = res_b_path.string()};
        REQUIRE_NOTHROW(adm::client::add_resource(conn, res_regis_b));
        res_regis_repl = {
            .resource_name = fmt::format("repl-{}", test_uui), .resource_type = adm::resource_type::replication};
        REQUIRE_NOTHROW(adm::client::add_resource(conn, res_regis_repl));

        // Close connection and create new one to "commit" previous actions
        conn.disconnect();
        conn.connect();

        // Create the hierarchy
        REQUIRE_NOTHROW(adm::client::add_child_resource(conn, res_regis_repl.resource_name, res_regis_a.resource_name));
        REQUIRE_NOTHROW(adm::client::add_child_resource(conn, res_regis_repl.resource_name, res_regis_b.resource_name));

        // Reset connection jic!
        conn.disconnect();
        conn.connect();

        test_data_object = sandbox / "cool-cool-epic.txt";
        {
            io::client::default_transport transport{conn};
            io::odstream out{transport, test_data_object, io::root_resource_name{res_regis_repl.resource_name}};
        }
    }

    ~test_fixture_for_issue_8993() noexcept
    {
        // Reset connection jic!
        conn.disconnect();

        try {
            conn.connect();

            // Cleanup all of the files
            fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash);

            // Unlink the hierarchy
            adm::client::remove_child_resource(conn, res_regis_repl.resource_name, res_regis_b.resource_name);
            adm::client::remove_child_resource(conn, res_regis_repl.resource_name, res_regis_a.resource_name);

            // Cleanup the resources
            adm::client::remove_resource(conn, res_regis_repl.resource_name);
            adm::client::remove_resource(conn, res_regis_b.resource_name);
            adm::client::remove_resource(conn, res_regis_a.resource_name);

            // Cleanup the temp directories
            std::filesystem::remove_all(res_b_path);
            std::filesystem::remove_all(res_a_path);
        }
        catch (const irods::exception& e) {
            WARN("Exception thrown during cleanup: " << e.what() << "\nTest cleanup may be incomplete.");
        }
    }
};

TEST_CASE_METHOD(test_fixture_for_issue_8993, "Stat on data object with only good replicas")
{
    auto& comm{static_cast<RcComm&>(conn)};
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 0) == GOOD_REPLICA);
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 1) == GOOD_REPLICA);

    auto res{stat(conn, test_data_object)};
    REQUIRE(res->objSize == 0);
}

TEST_CASE_METHOD(test_fixture_for_issue_8993, "Stat on data object with mixed stale and good replicas")
{
    constexpr rodsLong_t bad_size{10};
    auto& comm{static_cast<RcComm&>(conn)};
    REQUIRE(set_replica_status(comm, test_data_object, 0, STALE_REPLICA, {{DATA_SIZE_KW, std::to_string(bad_size)}}) >=
            0);

    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 0) == STALE_REPLICA);
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 1) == GOOD_REPLICA);

    auto res{stat(conn, test_data_object)};

    // We expect the good replica size
    REQUIRE(res->objSize == 0);
}

TEST_CASE_METHOD(test_fixture_for_issue_8993, "Stat on data object with only stale replicas")
{
    constexpr rodsLong_t bad_size_one{10};
    auto& comm{static_cast<RcComm&>(conn)};
    REQUIRE(set_replica_status(
                comm, test_data_object, 0, STALE_REPLICA, {{DATA_SIZE_KW, std::to_string(bad_size_one)}}) >= 0);

    constexpr rodsLong_t bad_size_two{20};
    REQUIRE(set_replica_status(
                comm, test_data_object, 1, STALE_REPLICA, {{DATA_SIZE_KW, std::to_string(bad_size_two)}}) >= 0);

    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 0) == STALE_REPLICA);
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 1) == STALE_REPLICA);

    auto res{stat(conn, test_data_object)};

    // We expect the first replica to give the stat when both replicas are stale
    REQUIRE(res->objSize == bad_size_one);
}

TEST_CASE_METHOD(test_fixture_for_issue_8993, "Stat on data object with invalid status")
{
    constexpr rodsLong_t bad_size{10};
    constexpr auto bad_status{42};
    auto& comm{static_cast<RcComm&>(conn)};
    REQUIRE(set_replica_status(comm, test_data_object, 0, bad_status, {{DATA_SIZE_KW, std::to_string(bad_size)}}) >= 0);

    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 0) == bad_status);
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 1) == GOOD_REPLICA);

    auto res{stat(conn, test_data_object)};

    // We expect to have the size of the good replica
    REQUIRE(res->objSize == 0);
}

TEST_CASE_METHOD(test_fixture_for_issue_8993, "Stat on data object with only invalid status")
{
    constexpr rodsLong_t bad_size_one{10};
    constexpr auto bad_status_one{42};
    auto& comm{static_cast<RcComm&>(conn)};
    REQUIRE(set_replica_status(
                comm, test_data_object, 0, bad_status_one, {{DATA_SIZE_KW, std::to_string(bad_size_one)}}) >= 0);

    constexpr rodsLong_t bad_size_two{20};
    constexpr auto bad_status_two{56709};
    REQUIRE(set_replica_status(
                comm, test_data_object, 1, bad_status_two, {{DATA_SIZE_KW, std::to_string(bad_size_two)}}) >= 0);

    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 0) == bad_status_one);
    REQUIRE(irods::experimental::replica::replica_status(comm, test_data_object, 1) == bad_status_two);

    auto res{stat(conn, test_data_object)};

    // We expect the first replica to give the stat when both replicas are stale
    REQUIRE(res->objSize == bad_size_one);
}