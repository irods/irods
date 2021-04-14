#include "catch.hpp"

#include "client_connection.hpp"
#include "dataObjChksum.h"
#include "dataObjInpOut.h"
#include "dstream.hpp"
#include "filesystem.hpp"
#include "get_file_descriptor_info.h"
#include "irods_at_scope_exit.hpp"
#include "irods_error_enum_matcher.hpp"
#include "irods_exception.hpp"
#include "replica.hpp"
#include "replica_proxy.hpp"
#include "resource_administration.hpp"
#include "rodsClient.h"
#include "rodsErrorTable.h"
#include "transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include "fmt/format.h"

#include <iostream>
#include <thread>

// clang-format off
namespace adm   = irods::experimental::administration;
namespace fs    = irods::experimental::filesystem;
namespace io    = irods::experimental::io;
namespace ir    = irods::experimental::replica;
// clang-format on

TEST_CASE("basic write lock")
{
    using namespace std::string_literals;

    load_client_api_plugins();

    // create two resources onto which a data object can be written
    const std::string resc_0 = "logical_locking_resc_0";
    const std::string resc_1 = "logical_locking_resc_1";

    irods::at_scope_exit remove_resources{[&resc_0, &resc_1] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        adm::client::remove_resource(comm, resc_0);
        adm::client::remove_resource(comm, resc_1);
    }};

    const auto mkresc = [](std::string_view _name)
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        if (const auto [ec, exists] = adm::client::resource_exists(comm, _name); exists) {
            REQUIRE(adm::client::remove_resource(comm, _name));
        }

        REQUIRE(unit_test_utils::add_ufs_resource(comm, _name, "vault_for_"s + _name.data()));
    };

    mkresc(resc_0);
    mkresc(resc_1);

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // create data object on one resource
    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_logical_locking";

    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    const auto target_object = sandbox / "target_object";

    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }

    REQUIRE(fs::client::exists(comm, target_object));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, 0));

    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), resc_1));

    SECTION("attempt open")
    {
        constexpr int opened_replica_number = 0;
        constexpr int other_replica_number = 1;

        REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_number));
        REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_number));

        dataObjInp_t open_inp{};
        auto cond_input = irods::experimental::make_key_value_proxy(open_inp.condInput);
        std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", target_object.c_str());
        cond_input[REPL_NUM_KW] = std::to_string(opened_replica_number);
        open_inp.openFlags = O_WRONLY;

        const auto fd = rcDataObjOpen(&comm, &open_inp);
        REQUIRE(fd > 2);

        REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_number));
        REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_number));

        {
            irods::experimental::client_connection new_conn;
            RcComm& new_comm = static_cast<RcComm&>(new_conn);

            dataObjInp_t fail_open_inp{};
            std::snprintf(fail_open_inp.objPath, sizeof(fail_open_inp.objPath), "%s", target_object.c_str());

            fail_open_inp.openFlags = O_RDONLY;
            cond_input[REPL_NUM_KW] = std::to_string(opened_replica_number);
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
            cond_input[REPL_NUM_KW] = std::to_string(other_replica_number);
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

            fail_open_inp.openFlags = O_WRONLY;
            cond_input[REPL_NUM_KW] = std::to_string(opened_replica_number);
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
            cond_input[REPL_NUM_KW] = std::to_string(other_replica_number);
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
        }

        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

        // Make sure everything unlocked properly
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_number));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_number));
    }

#if 0
    SECTION("open without close")
    {
        {
            irods::experimental::client_connection new_conn;
            RcComm& new_comm = static_cast<RcComm&>(new_conn);

            dataObjInp_t open_inp{};
            auto cond_input = irods::experimental::make_key_value_proxy(open_inp.condInput);
            std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", target_object.c_str());
            cond_input[REPL_NUM_KW] = std::to_string(0);
            open_inp.openFlags = O_WRONLY;
            const auto fd = rcDataObjOpen(&new_comm, &open_inp);
            REQUIRE(fd > 2);

            CHECK(INTERMEDIATE_REPLICA == ir::replica_status(new_comm, target_object, 0));
            CHECK(WRITE_LOCKED == ir::replica_status(new_comm, target_object, 1));

            // no close
        }

        // Make sure everything unlocked properly on agent teardown
        CHECK(STALE_REPLICA == ir::replica_status(comm, target_object, 0));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, 1));
    }
#endif
}

