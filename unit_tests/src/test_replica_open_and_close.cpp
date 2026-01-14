#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/dataObjWrite.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_query.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/modDataObjMeta.h"
#include "irods/replica.hpp"
#include "irods/replica_close.h"
#include "irods/replica_open.h"
#include "irods/replica_proxy.hpp"
#include "irods/rodsClient.h"
#include "irods/transport/default_transport.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <string_view>
#include <thread>

TEST_CASE("replica_open and replica_close")
{
    // clang-format off
    namespace fs = irods::experimental::filesystem;
    using json   = nlohmann::json;
    // clang-format on

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();
    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&conn, &sandbox] {
        REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash));
    }};

    const auto path = sandbox / "test_data_object";

    dataObjInp_t input{};
    input.createMode = 0600;
    input.openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    std::strcpy(input.objPath, path.c_str());

    auto* conn_ptr = static_cast<RcComm*>(conn);
    char* json_output = nullptr;

    const auto fd = rc_replica_open(conn_ptr, &input, &json_output);
    REQUIRE(fd >= 3);

    const auto unlock_replica = irods::at_scope_exit{[&]
    {
        ModDataObjMetaInp mod_meta_inp{};

        DataObjInfo info{};
        std::snprintf(info.objPath, MAX_NAME_LEN, "%s", path.c_str());
        info.replNum = 0;

        auto [kvp, lm] = irods::experimental::make_key_value_proxy();
        kvp[REPL_STATUS_KW] = std::to_string(STALE_REPLICA);
        kvp[ADMIN_KW] = "";

        mod_meta_inp.regParam = kvp.get();
        mod_meta_inp.dataObjInfo = &info;

        REQUIRE(0 == rcModDataObjMeta(conn_ptr, &mod_meta_inp));
    }};

    SECTION("replica size")
    {
        std::string_view contents = "Hello, iRODS!";

        {
            openedDataObjInp_t opened_fd{};
            opened_fd.l1descInx = fd;

            bytesBuf_t bbuf{};
            bbuf.buf = const_cast<char*>(contents.data());
            bbuf.len = contents.size() + 1;

            REQUIRE(rcDataObjWrite(conn_ptr, &opened_fd, &bbuf) == bbuf.len);
        }

        const auto gql = fmt::format("select DATA_SIZE where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                     path.parent_path().c_str(),
                                     path.object_name().c_str());

        SECTION("do not update")
        {
            const auto close_input = json{
                {"fd", fd},
                {"update_size", false}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                REQUIRE("0" == row[0]);
            }
        }

        SECTION("do update")
        {
            const auto close_input = json{
                {"fd", fd},
                {"update_size", true}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                REQUIRE(std::to_string(contents.size() + 1) == row[0]);
            }
        }
    }

    SECTION("replica status")
    {
        const auto gql = fmt::format("select DATA_REPL_STATUS where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                     path.parent_path().c_str(),
                                     path.object_name().c_str());

        SECTION("do not update")
        {
            const auto close_input = json{
                {"fd", fd},
                {"update_status", false}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                std::string_view replica_status_intermediate = "2";
                REQUIRE(replica_status_intermediate == row[0]);
            }
        }

        SECTION("do update")
        {
            const auto close_input = json{
                {"fd", fd},
                {"update_status", true}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                std::string_view replica_status_good = "1";
                REQUIRE(replica_status_good == row[0]);
            }
        }
    }

    SECTION("checksum")
    {
        const auto gql = fmt::format("select DATA_CHECKSUM where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                     path.parent_path().c_str(),
                                     path.object_name().c_str());

        SECTION("do not compute")
        {
            const auto close_input = json{
                {"fd", fd}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                REQUIRE(row[0].empty());
            }
        }

        SECTION("compute")
        {
            const auto close_input = json{
                {"fd", fd},
                {"compute_checksum", true}
            }.dump();

            REQUIRE(rc_replica_close(conn_ptr, close_input.data()) == 0);

            for (auto&& row : irods::query{conn_ptr, gql}) {
                REQUIRE_FALSE(row[0].empty());
            }
        }
    }

    SECTION("do compute checksum, do not update status")
    {
        SECTION("update size")
        {
            const auto close_input =
                json{{"fd", fd}, {"update_status", false}, {"update_size", true}, {"compute_checksum", true}}.dump();
            REQUIRE(rc_replica_close(conn_ptr, close_input.c_str()) == USER_INCOMPATIBLE_PARAMS);
            // Try again, and the close should succeed.
            REQUIRE(rc_replica_close(conn_ptr, json{{"fd", fd}}.dump().c_str()) == 0);
        }
        SECTION("do not update size")
        {
            const auto close_input =
                json{{"fd", fd}, {"update_status", false}, {"update_size", false}, {"compute_checksum", true}}.dump();
            REQUIRE(rc_replica_close(conn_ptr, close_input.c_str()) == USER_INCOMPATIBLE_PARAMS);
            // Try again, and the close should succeed.
            REQUIRE(rc_replica_close(conn_ptr, json{{"fd", fd}}.dump().c_str()) == 0);
        }
    }
}

TEST_CASE("test mtime changes on open and close replica without writing to it")
{
    namespace fs = irods::experimental::filesystem;
    namespace replica = irods::experimental::replica;
    using json = nlohmann::json;
    using namespace std::chrono_literals;

    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    auto conn_pool = irods::make_connection_pool();
    auto conn = conn_pool->get_connection();
    auto* comm = static_cast<RcComm*>(conn);
    const auto sandbox = fs::path{static_cast<char*>(env.rodsHome)} / "unit_testing_sandbox";

    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    const irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    const auto path = sandbox / "test_data_object";

    // Ensure that the replica is unlocked so that it can be removed. This must happen before remove_sandbox above.
    const auto unlock_replica = irods::at_scope_exit{[&] {
        ModDataObjMetaInp mod_meta_inp{};

        DataObjInfo info{};
        std::strncpy(static_cast<char*>(info.objPath), path.c_str(), MAX_NAME_LEN - 1);
        info.replNum = 0;

        KeyValPair kvp{};
        addKeyVal(&kvp, REPL_STATUS_KW, std::to_string(STALE_REPLICA).c_str());
        addKeyVal(&kvp, ADMIN_KW, "");

        mod_meta_inp.regParam = &kvp;
        mod_meta_inp.dataObjInfo = &info;

        static_cast<void>(rcModDataObjMeta(comm, &mod_meta_inp));

        clearKeyVal(&kvp);
    }};

    // Create an empty data object.
    REQUIRE(!fs::client::exists(conn, path));
    {
        irods::experimental::io::client::native_transport transport{conn};
        irods::experimental::io::odstream{transport, path} << "";
    }

    // Get the original mtime so we have something against which to compare.
    const auto original_mtime = replica::last_write_time(*comm, path, 0);

    // Sleep for a second so mtime has the possibility to be updated on the next open.
    std::this_thread::sleep_for(1s);

    dataObjInp_t input{};
    std::strncpy(static_cast<char*>(input.objPath), path.c_str(), MAX_NAME_LEN - 1);

    char* json_output = nullptr;

    SECTION("O_WRONLY alone does not update mtime")
    {
        // Open the replica with only O_WRONLY flag. Specify that we are opening for write only (no truncate).
        input.openFlags = O_WRONLY;
        // NOLINTNEXTLINE(readability-identifier-length)
        const auto fd = rc_replica_open(comm, &input, &json_output);
        REQUIRE(fd >= 3);

        // Get the mtime just after open so we have something against which to compare.
        const auto post_open_mtime = replica::last_write_time(*comm, path, 0);

        // mtime should not be updated on open because replica was not truncated.
        REQUIRE(original_mtime == post_open_mtime);

        // Sleep for a second so mtime has the possibility to be updated on close.
        std::this_thread::sleep_for(1s);

        SECTION("do update status")
        {
            SECTION("do update size")
            {
                SECTION("do compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", true}, {"compute_checksum", true}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(original_mtime == replica::last_write_time(*comm, path, 0));
                }
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", true}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
            SECTION("do not update size")
            {
                SECTION("do compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", false}, {"compute_checksum", true}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", false}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
        }
        SECTION("do not update status")
        {
            SECTION("do update size")
            {
                // "do compute checksum" is not compatible with "do not update status", so the mtime assertions are
                // immaterial. So, the test case is omitted here.
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", false}, {"update_size", true}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
            SECTION("do not update size")
            {
                // "do compute checksum" is not compatible with "do not update status", so the mtime assertions are
                // immaterial. So, the test case is omitted here.
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", false}, {"update_size", false}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
        }
    }

    SECTION("O_WRONLY with O_TRUNC updates mtime")
    {
        // Open replica with O_WRONLY and O_TRUNC flags so that the replica gets truncated.
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        input.openFlags = O_WRONLY | O_TRUNC;
        // NOLINTNEXTLINE(readability-identifier-length)
        const auto fd = rc_replica_open(comm, &input, &json_output);
        REQUIRE(fd >= 3);

        // Get the mtime just after open so we have something against which to compare.
        const auto post_open_mtime = replica::last_write_time(*comm, path, 0);

        // mtime should be updated on open because replica was truncated.
        REQUIRE(original_mtime < post_open_mtime);

        // Sleep for a second so mtime has the possibility to be updated on close.
        std::this_thread::sleep_for(1s);

        SECTION("do update status")
        {
            SECTION("do update size")
            {
                SECTION("do compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", true}, {"compute_checksum", true}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", true}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
            SECTION("do not update size")
            {
                SECTION("do compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", false}, {"compute_checksum", true}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", true}, {"update_size", false}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
        }
        SECTION("do not update status")
        {
            SECTION("do update size")
            {
                // "do compute checksum" is not compatible with "do not update status", so the mtime assertions are
                // immaterial. So, the test case is omitted here.
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", false}, {"update_size", true}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
            SECTION("do not update size")
            {
                // "do compute checksum" is not compatible with "do not update status", so the mtime assertions are
                // immaterial. So, the test case is omitted here.
                SECTION("do not compute checksum")
                {
                    const auto close_input =
                        json{{"fd", fd}, {"update_status", false}, {"update_size", false}, {"compute_checksum", false}}
                            .dump();
                    REQUIRE(rc_replica_close(comm, close_input.c_str()) == 0);
                    CHECK(post_open_mtime == replica::last_write_time(*comm, path, 0));
                }
            }
        }
    }
}

TEST_CASE("#7338")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn{irods::experimental::defer_authentication};

    DataObjInp input{};
    char* json_output{};

    CHECK(rc_replica_open(static_cast<RcComm*>(conn), &input, &json_output) == SYS_NO_API_PRIV);
    CHECK(json_output == nullptr);

    CHECK(rc_replica_close(static_cast<RcComm*>(conn), "") == SYS_NO_API_PRIV);
}
