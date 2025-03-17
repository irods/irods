#include <catch2/catch.hpp>

#include "unit_test_utils.hpp"

#include "irods/client_connection.hpp"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_query.hpp"
#include "irods/modDataObjMeta.h"
#include "irods/rcConnect.h"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/transport/default_transport.hpp"
#include "irods/update_replica_access_time.h"
#include "irods/user_administration.hpp"

#include <boost/asio/ip/host_name.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

namespace fs = irods::experimental::filesystem;
namespace adm = irods::experimental::administration;
namespace io = irods::experimental::io;

using json = nlohmann::json;

// Returns a tuple holding the data id, atime, and mtime of a replica.
auto get_replica_info(RcComm& _comm, const fs::path& _logical_path, int _replica_number)
    -> std::tuple<std::string, std::string, std::string>;

TEST_CASE("#8260: rc_update_replica_access_time returns error on invalid inputs")
{
    load_client_api_plugins();

    SECTION("null inputs")
    {
        RcComm comm{};
        const char* input = "";
        char* output{};

        CHECK(rc_update_replica_access_time(nullptr, input, &output) == INVALID_INPUT_ARGUMENT_NULL_POINTER);
        CHECK(rc_update_replica_access_time(&comm, nullptr, &output) == INVALID_INPUT_ARGUMENT_NULL_POINTER);
        CHECK(rc_update_replica_access_time(&comm, input, nullptr) == INVALID_INPUT_ARGUMENT_NULL_POINTER);
    }

    SECTION("not what the api wants")
    {
        irods::experimental::client_connection conn;
        const char* input = "not json";
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input, &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("empty object")
    {
        irods::experimental::client_connection conn;
        const char* input = "{}";
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input, &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("non-integer value for data_id")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", "not a number"},
                    {"replica_number", 0},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("non-integer value for replica_number")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", 0},
                    {"replica_number", "not a number"},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("missing data_id property")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"replica_number", 0},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("missing replica_number property")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", 0},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("missing atime property")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", 0},
                    {"replica_number", 0}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("null value for data_id")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", nullptr},
                    {"replica_number", 0},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("null value for replica_number")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", 0},
                    {"replica_number", nullptr},
                    {"atime", "00000000000"}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }

    SECTION("null value for atime")
    {
        // clang-format off
        const auto input = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", 0},
                    {"replica_number", 0},
                    {"atime", nullptr}
                }
            }}
        }.dump();
        // clang-format on

        irods::experimental::client_connection conn;
        char* output{};
        CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), input.c_str(), &output) == SYS_LIBRARY_ERROR);
    }
}

TEST_CASE("#8260: rc_update_replica_access_time updates access time with bad access time values")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a data object.
    const auto logical_path = sandbox / "data_object.txt";
    io::client::native_transport tp{conn};
    io::odstream{tp, logical_path};

    const auto [data_id, atime, mtime] = get_replica_info(conn, logical_path, 0);

    // This JSON string contains the new atime for the replica.
    const auto* bad_atime = "bogus";
    // clang-format off
    const auto updates = json{
        {"access_time_updates", json::array_t{
            {
                {"data_id", std::stoull(data_id)},
                {"replica_number", 0},
                {"atime", bad_atime}
            }
        }}
    }.dump();
    // clang-format on

    char* output{};
    CHECK(rc_update_replica_access_time(static_cast<RcComm*>(conn), updates.c_str(), &output) == 0);

    // Show that the replica's atime has been updated to the bad value.
    const auto updated_replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<0>(updated_replica_info) == data_id);
    CHECK(std::get<1>(updated_replica_info) == bad_atime); // atime should be different.
}

TEST_CASE("#8260: rc_update_replica_access_time requires rodsadmin privileges")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;

    rodsEnv env;
    _getRodsEnv(env);

    // Create a rodsuser.
    const adm::user test_user{"issue_8260_user_alice", env.rodsZone};
    REQUIRE_NOTHROW(adm::client::add_user(conn, test_user));
    irods::at_scope_exit remove_test_user{
        [&conn, &test_user] { REQUIRE_NOTHROW(adm::client::remove_user(conn, test_user)); }};
    REQUIRE_NOTHROW(adm::client::modify_user(conn, test_user, adm::user_password_property{"rods"}));

    // Connect to the server as the rodsuser.
    irods::experimental::client_connection test_user_conn{env.rodsHost, env.rodsPort, {test_user.name, env.rodsZone}};

    const char* input = "";
    char* output{};
    REQUIRE(rc_update_replica_access_time(static_cast<RcComm*>(test_user_conn), input, &output) == SYS_NO_API_PRIV);
}

TEST_CASE("#8260: rc_update_replica_access_time can update access time of one replica")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a data object.
    const auto logical_path = sandbox / "data_object.txt";
    io::client::native_transport tp{conn};
    io::odstream out{tp, logical_path};

    {
        // Show that the replica's atime and mtime are identical. It's important that
        // this assertion be made while the replica is in the intermediate state because
        // closing a new replica will result in the server updating the mtime.
        const auto [data_id, atime, mtime] = get_replica_info(conn, logical_path, 0);
        REQUIRE(atime == mtime);
    }

    // Closing the replica will result in the mtime being updated, so fetch the latest
    // mtime of the replica in case it changed.
    out.close();
    const auto [data_id, atime, mtime] = get_replica_info(conn, logical_path, 0);

    // This JSON string contains the new atime for the replica.
    const auto* expected_atime = "01700000000";
    // clang-format off
    const auto updates = json{
        {"access_time_updates", json::array_t{
            {
                {"data_id", std::stoull(data_id)},
                {"replica_number", 0},
                {"atime", expected_atime}
            }
        }}
    }.dump();
    // clang-format on

    // Update the replica's atime.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    char* ignored{};
    CHECK(rc_update_replica_access_time(conn_ptr, updates.c_str(), &ignored) == 0);

    // Show that the replica's atime has been updated to the expected value.
    const auto updated_replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<0>(updated_replica_info) == data_id);
    CHECK(std::get<1>(updated_replica_info) == expected_atime); // atime should be different.
    CHECK(std::get<2>(updated_replica_info) == mtime); // mtime should not have changed.
}

TEST_CASE("#8260: rc_update_replica_access_time can update access time of multiple unrelated replicas")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    // Create an empty directory to serve as the vault for a new unixfilesystem resource.
    const auto vault_path = unit_test_utils::create_resource_vault("issue_8260_atime_unrelated_replicas_vault");
    irods::at_scope_exit remove_vault{[&vault_path] { std::filesystem::remove_all(vault_path); }};

    // Create a new resource so that we can prove the atime update API supports targeting
    // targeting individual replicas.
    adm::resource_registration_info ufs_info;
    ufs_info.resource_name = "ufs_issue_8260";
    ufs_info.resource_type = adm::resource_type::unixfilesystem;
    ufs_info.host_name = boost::asio::ip::host_name();
    ufs_info.vault_path = vault_path;

    irods::experimental::client_connection conn;

    REQUIRE_NOTHROW(adm::client::add_resource(conn, ufs_info));

    irods::at_scope_exit remove_resources{
        [&conn, &ufs_info] { CHECK_NOTHROW(adm::client::remove_resource(conn, ufs_info.resource_name)); }};

    // Reset the connection so the resource is available to future operations.
    conn.disconnect();
    conn.connect();

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create two data objects.
    const auto logical_path_0 = sandbox / "data_object_0.txt";
    io::client::native_transport tp{conn};
    io::odstream{tp, logical_path_0};

    const auto logical_path_1 = sandbox / "data_object_1.txt";
    io::odstream{tp, logical_path_1};

    // Replicate replica 0 of the first data object to the second resource.
    unit_test_utils::replicate_data_object(conn, logical_path_0.c_str(), ufs_info.resource_name);

    // Get the data id, atime, and mtime of each replica. We only care about the
    // replicas that will be modified by the API. Those being replica 1 of the first
    // data object and replica 0 of the second data object.
    const auto replica_info_0 = get_replica_info(conn, logical_path_0, 1);
    const auto replica_info_1 = get_replica_info(conn, logical_path_1, 0);

    // Show that the second replica's atime and mtime are identical.
    // This is unrelated to the test, but proves that the atime logic for iRODS covers
    // replicas created through replication.
    REQUIRE(std::get<1>(replica_info_0) == std::get<2>(replica_info_0));

    // These are the new atimes for the replicas. To guard against false-positives,
    // these two variables must always have unique atime values.
    const auto expected_atime_0 = "01700000000";
    const auto expected_atime_1 = "01711111111";
    // clang-format off
    // This JSON string contains the new atimes for the replicas.
    const auto updates = json{
        {"access_time_updates", json::array_t{
            // Replica 1 of the first data object.
            {
                {"data_id", std::stoull(std::get<0>(replica_info_0))},
                {"replica_number", 1},
                {"atime", expected_atime_0}
            },
            // Replica 0 of the second data object.
            {
                {"data_id", std::stoull(std::get<0>(replica_info_1))},
                {"replica_number", 0},
                {"atime", expected_atime_1}
            }
        }}
    }.dump();
    // clang-format on

    // Update the atimes.
    auto* conn_ptr = static_cast<RcComm*>(conn);
    char* ignored{};
    CHECK(rc_update_replica_access_time(conn_ptr, updates.c_str(), &ignored) == 0);

    // Show that the atimes for each replica have been updated to the expected values.
    // Here, we check replica 1 of the first data object.
    auto updated_replica_info = get_replica_info(conn, logical_path_0, 1);
    CHECK(std::get<0>(updated_replica_info) == std::get<0>(replica_info_0));
    CHECK(std::get<1>(updated_replica_info) == expected_atime_0); // atime should be different.
    CHECK(std::get<2>(updated_replica_info) == std::get<2>(replica_info_0)); // mtime should not have changed.
    // And now we check replica 0 of the second data object.
    updated_replica_info = get_replica_info(conn, logical_path_1, 0);
    CHECK(std::get<0>(updated_replica_info) == std::get<0>(replica_info_1));
    CHECK(std::get<1>(updated_replica_info) == expected_atime_1); // atime should be different.
    CHECK(std::get<2>(updated_replica_info) == std::get<2>(replica_info_1)); // mtime should not have changed.
}

TEST_CASE("#8260: targeting mix of existent and non-existent replicas is not an error")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Show that no data object exists in the catalog with the target data id.
    // Checking the replica number isn't necessary because its existence requires the
    // existence of the data id.
    constexpr auto non_existent_data_id = 9999;
    constexpr auto non_existent_replica_number = 8888;
    auto* conn_ptr = static_cast<RcComm*>(conn);
    {
        auto query = irods::query{conn_ptr, fmt::format("select DATA_ID where DATA_ID = '{}'", non_existent_data_id)};
        REQUIRE(query.empty());
    }

    SECTION("targeting non-existent replica")
    {
        // clang-format off
        const auto updates = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", non_existent_data_id},
                    {"replica_number", non_existent_replica_number},
                    {"atime", "01600000000"}
                }
            }}
        }.dump();
        // clang-format on

        // Show that invoking the API does not result in an error.
        char* ignored{};
        CHECK(rc_update_replica_access_time(conn_ptr, updates.c_str(), &ignored) == 0);

        // Show that no data object exists matching the non-existent data id. This helps
        // prove the post-condition state is what we expect. That is, no new replica is
        // accidentally created.
        {
            auto query =
                irods::query{conn_ptr, fmt::format("select DATA_ID where DATA_ID = '{}'", non_existent_data_id)};
            CHECK(query.empty());
        }
    }

    SECTION("targeting existent and non-existent replicas")
    {
        // Create a data object.
        const auto logical_path = sandbox / "data_object.txt";
        io::client::native_transport tp{conn};
        io::odstream out{tp, logical_path};

        {
            // Show that the replica's atime and mtime are identical. It's important that
            // this assertion be made while the replica is in the intermediate state because
            // closing a new replica will result in the server updating the mtime.
            const auto [data_id, atime, mtime] = get_replica_info(conn, logical_path, 0);
            REQUIRE(atime == mtime);
        }

        // Closing the replica will result in the mtime being updated, so fetch the
        // latest mtime of the replica in case it changed.
        out.close();
        const auto [data_id, atime, mtime] = get_replica_info(conn, logical_path, 0);

        // This JSON string contains the new atime for the replicas. Only the update
        // involving the existent replica should succeed. The other update is effectively
        // a no-op.
        const auto* expected_atime = "01700000000";
        // clang-format off
        const auto updates = json{
            {"access_time_updates", json::array_t{
                {
                    {"data_id", std::stoull(data_id)},
                    {"replica_number", 0},
                    {"atime", expected_atime}
                },
                // The non-existent replica.
                {
                    {"data_id", non_existent_data_id},
                    {"replica_number", non_existent_replica_number},
                    {"atime", "01600000000"}
                }
            }}
        }.dump();
        // clang-format on

        // Show that invoking the API does not result in an error.
        char* ignored{};
        CHECK(rc_update_replica_access_time(conn_ptr, updates.c_str(), &ignored) == 0);

        // Show that no data object exists matching the non-existent data id. This helps
        // prove the post-condition state is what we expect. That is, no new replica is
        // accidentally created.
        {
            auto query =
                irods::query{conn_ptr, fmt::format("select DATA_ID where DATA_ID = '{}'", non_existent_data_id)};
            CHECK(query.empty());
        }

        // Show that the replica's atime has been updated to the expected value.
        const auto updated_replica_info = get_replica_info(conn, logical_path, 0);
        CHECK(std::get<0>(updated_replica_info) == data_id);
        CHECK(std::get<1>(updated_replica_info) == expected_atime); // atime should be different.
        CHECK(std::get<2>(updated_replica_info) == mtime); // mtime should not have changed.
    }
}

TEST_CASE("#8260: opening a replica for reading updates its access time")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a data object.
    const auto logical_path = sandbox / "data_object.txt";
    io::client::native_transport tp{conn};
    io::odstream{tp, logical_path} << "issue #8260";

    // Adjust the replica's atime so that a read operation triggers an atime update.
    auto replica_info = get_replica_info(conn, logical_path, 0);
    DataObjInfo info_input{};
    info_input.dataId = std::stoull(std::get<0>(replica_info));

    KeyValPair reg_params{};
    const auto* configured_atime = "00000000000";
    addKeyVal(&reg_params, DATA_ACCESS_TIME_KW, configured_atime);

    modDataObjMeta_t input{};
    input.dataObjInfo = &info_input;
    input.regParam = &reg_params;

    CHECK(rcModDataObjMeta(static_cast<RcComm*>(conn), &input) == 0);

    // Confirm the replica's atime was set to the target value.
    replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<1>(replica_info) == configured_atime);

    // Open the data object for reading. Reading data from the replica isn't necessary
    // because iRODS only updates the atime during the open API operation.

    SECTION("opening the replica with O_RDONLY triggers an atime update")
    {
        io::idstream{tp, logical_path, std::ios::in};
    }

    SECTION("opening the replica with O_RDWR triggers an atime update")
    {
        io::idstream{tp, logical_path, std::ios::out | std::ios::in};
    }

    // Give the server a few seconds to apply the atime updates.
    std::this_thread::sleep_for(std::chrono::seconds{2});

    // Show that the replica's atime has been updated.
    const auto updated_replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<1>(updated_replica_info) > configured_atime);
}

TEST_CASE("#8260: opening a replica for writing does not update its access time")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox_issue_8260";

    irods::experimental::client_connection conn;

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a data object.
    const auto logical_path = sandbox / "data_object.txt";
    io::client::native_transport tp{conn};
    io::odstream{tp, logical_path} << "issue #8260";

    // Adjust the replica's atime so we can prove that opening for writing does not
    // trigger atime updates.
    auto replica_info = get_replica_info(conn, logical_path, 0);
    DataObjInfo info_input{};
    info_input.dataId = std::stoull(std::get<0>(replica_info));

    KeyValPair reg_params{};
    const auto* configured_atime = "00000000000";
    addKeyVal(&reg_params, DATA_ACCESS_TIME_KW, configured_atime);

    modDataObjMeta_t input{};
    input.dataObjInfo = &info_input;
    input.regParam = &reg_params;

    CHECK(rcModDataObjMeta(static_cast<RcComm*>(conn), &input) == 0);

    // Confirm the replica's atime was set to the target value.
    replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<1>(replica_info) == configured_atime);

    // Open the data object for writing. Writing data to the replica isn't necessary
    // because iRODS only updates the atime during the open API operation.
    io::odstream{tp, logical_path};

    // Give the server a few seconds to apply the atime updates if there are any. If
    // things are working as intended, there shouldn't be any.
    std::this_thread::sleep_for(std::chrono::seconds{2});

    // Show that the replica's atime has not been updated.
    const auto updated_replica_info = get_replica_info(conn, logical_path, 0);
    CHECK(std::get<1>(updated_replica_info) == configured_atime);
}

auto get_replica_info(RcComm& _comm, const fs::path& _logical_path, int _replica_number)
    -> std::tuple<std::string, std::string, std::string>
{
    const auto query_string = fmt::format("select DATA_ID, DATA_ACCESS_TIME, DATA_MODIFY_TIME where COLL_NAME = '{}' "
                                          "and DATA_NAME = '{}' and DATA_REPL_NUM = '{}'",
                                          _logical_path.parent_path().c_str(),
                                          _logical_path.object_name().c_str(),
                                          _replica_number);

    for (auto&& row : irods::query{&_comm, query_string}) {
        return {row[0], row[1], row[2]};
    }

    throw std::logic_error{
        fmt::format("Precondition violation: replica does not exist: logical_path=[{}], replica_number=[{}]",
                    _logical_path.c_str(),
                    _replica_number)};
} // get_replica_info
