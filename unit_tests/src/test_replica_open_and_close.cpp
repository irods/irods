#include "catch.hpp"

#include "rodsClient.h"
#include "connection_pool.hpp"
#include "filesystem.hpp"
#include "replica_open.h"
#include "replica_close.h"
#include "dataObjWrite.h"
#include "irods_query.hpp"
#include "irods_at_scope_exit.hpp"
#include "key_value_proxy.hpp"
#include "modDataObjMeta.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <string_view>

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
}

