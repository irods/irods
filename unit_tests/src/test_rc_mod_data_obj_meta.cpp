#include <catch2/catch_all.hpp>

#include "irods_error_enum_matcher.hpp"
#include "unit_test_utils.hpp"

#include "irods/client_connection.hpp"
#include "irods/dataObjInpOut.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_query.hpp"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"
#include "irods/replica.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/transport/default_transport.hpp"

#include <fmt/format.h>

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>

// clang-format off
namespace fs      = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

TEST_CASE("using ADMIN_KW with rcModDataObjMeta")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_mod_data_obj_meta";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    const auto target_object = sandbox / "target_object";

    const std::string contents = "content!";

    irods::at_scope_exit remove_sandbox{[&sandbox, &target_object] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        // Grant ownership to self again and remove the test data.
        fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::own);
        fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash);
    }};

    // Create a new data object.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }

    // Show that the replica is in a good state.
    CHECK(replica::replica_status<RcComm>(conn, target_object, 0) == GOOD_REPLICA);
    CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());

    // Prepare input structure for rsModDataObjMeta. All the test sections will use these as a base.
    DataObjInfo info{};
    KeyValPair reg_param{};
    std::strncpy(info.objPath, target_object.c_str(), MAX_NAME_LEN - 1);
    constexpr auto new_data_size = 1024;
    addKeyVal(&reg_param, DATA_SIZE_KW, std::to_string(new_data_size).c_str());
    ModDataObjMetaInp inp{&info, &reg_param};

    // Remove all permissions from self on the data object so that we are forced to use ADMIN_KW.
    REQUIRE_NOTHROW(fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::null));

    SECTION("using ALL_KW")
    {
        // Use the ALL_KW to exercise the codepath that affects all replicas. This test is for the ADMIN_KW, not
        // the functionality of the ALL_KW, so there are no other replicas to modify.
        addKeyVal(inp.regParam, ALL_KW, "");

        SECTION("no ADMIN_KW results in failure")
        {
            // This should fail because the admin has no permissions on the data object and did not use the ADMIN_KW.
            CHECK(rcModDataObjMeta(&comm, &inp) < 0);

            // Restore permissions so that we can see what happened.
            REQUIRE_NOTHROW(
                fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::own));

            // Ensure that the size did not change.
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());
        }

        SECTION("using ADMIN_KW results in success")
        {
            // Use the ADMIN_KW so that the system metadata can be updated even with no permissions.
            addKeyVal(inp.regParam, ADMIN_KW, "");

            // This should succeed because the admin has no permissions on the data object but used the ADMIN_KW.
            CHECK(0 == rcModDataObjMeta(&comm, &inp));

            // Restore permissions so that we can see what happened.
            REQUIRE_NOTHROW(
                fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::own));

            // Ensure that the size was updated.
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == new_data_size);
        }
    }

    SECTION("targeting a specific replica")
    {
        // Use the REPL_NUM_KW so that a specific replica is targeted for update.
        addKeyVal(inp.regParam, REPL_NUM_KW, "0");

        SECTION("no ADMIN_KW results in failure")
        {
            // This should fail because the admin has no permissions on the data object and did not use the ADMIN_KW.
            CHECK(rcModDataObjMeta(&comm, &inp) < 0);

            // Restore permissions so that we can see what happened.
            REQUIRE_NOTHROW(
                fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::own));

            // Ensure that the size did not change.
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == contents.size());
        }

        SECTION("using ADMIN_KW results in success")
        {
            // Use the ADMIN_KW so that the system metadata can be updated even with no permissions.
            addKeyVal(inp.regParam, ADMIN_KW, "");

            // This should succeed because the admin has no permissions on the data object but used the ADMIN_KW.
            CHECK(0 == rcModDataObjMeta(&comm, &inp));

            // Restore permissions so that we can see what happened.
            REQUIRE_NOTHROW(
                fs::client::permissions(fs::admin, comm, target_object, comm.clientUser.userName, fs::perms::own));

            // Ensure that the size was updated.
            CHECK(replica::replica_size<RcComm>(conn, target_object, 0) == new_data_size);
        }
    }
} // rc_data_obj_repl

TEST_CASE("#8999: rsModDataObjMeta formats string as timestamp for DATA_CREATE_KW")
{
    load_client_api_plugins();

    rodsEnv env;
    _getRodsEnv(env);

    irods::experimental::client_connection conn;

    const auto sandbox = fs::path{env.rodsHome} / "issue_8999";
    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    const auto data_object = sandbox / "data_object";

    const irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash); }};

    // Create a new data object.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, data_object};
    }

    // Configure the input structures to pass invalid arguments to the server.
    DataObjInfo info{};
    std::strncpy(info.objPath, data_object.c_str(), sizeof(DataObjInfo::objPath) - 1);

    // The strings MUST be less than 11 bytes in length for the API to convert
    // and format them as timestamps.
    KeyValPair reg_param{};
    addKeyVal(&reg_param, DATA_CREATE_KW, "bad_ts_0");
    addKeyVal(&reg_param, DATA_MODIFY_KW, "bad_ts_1");
    addKeyVal(&reg_param, DATA_ACCESS_TIME_KW, "bad_ts_2");

    ModDataObjMetaInp input{&info, &reg_param};
    REQUIRE(rcModDataObjMeta(static_cast<RcComm*>(conn), &input) == 0);

    // Show that the server handles create time just like the other time fields.
    const auto query_str = fmt::format(
        "select DATA_CREATE_TIME, DATA_MODIFY_TIME, DATA_ACCESS_TIME where COLL_NAME = '{}' and DATA_NAME = '{}'",
        sandbox.c_str(),
        data_object.object_name().c_str());
    constexpr const char* expected_ts = "00000000000";
    for (auto&& row : irods::query{static_cast<RcComm*>(conn), query_str}) {
        CHECK(expected_ts == row[0]);
        CHECK(expected_ts == row[1]);
        CHECK(expected_ts == row[2]);
    }
}
