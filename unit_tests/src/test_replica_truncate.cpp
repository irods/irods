#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/dataObjInpOut.h"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_exception.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/objInfo.h"
#include "irods/rcMisc.h"
#include "irods/replica.hpp"
#include "irods/replica_proxy.hpp"
#include "irods/replica_truncate.h"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/system_error.hpp"
#include "irods/transport/default_transport.hpp"
#include "unit_test_utils.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstring>
#include <string>
#include <string_view>
#include <thread>

// NOTE: These tests must be run by a rodsadmin as operations requiring elevated privilege are executed.

using namespace std::chrono_literals;

// clang-format off
namespace adm     = irods::experimental::administration;
namespace fs      = irods::experimental::filesystem;
namespace replica = irods::experimental::replica;
// clang-format on

namespace
{
    auto create_replication_resource(RcComm& _comm, const std::string_view _resc_name) -> void
    {
        adm::resource_registration_info resc_info;
        resc_info.resource_name = _resc_name.data();
        resc_info.resource_type = adm::resource_type::replication;

        adm::client::add_resource(_comm, resc_info);
    } // create_replication_resource
} // anonymous namespace

TEST_CASE("basic_two_standalone_resources")
{
    load_client_api_plugins();

    const std::string test_resc = "test_resc";
    const std::string vault_name = "test_resc_vault";

    // Create new resources.
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);
        unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
    }

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_replica_truncate";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox, &test_resc] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

        adm::client::remove_resource(comm, test_resc);
    }};

    const auto target_object = sandbox / "target_object";

    static constexpr auto contents = std::string_view{"content!"};

    // Create a new data object and show that the replica is in a good state.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

    // Replicate the data object and show that the replica is in a good state.
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), test_resc));
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 1));

    // Sleep here so that the mtime on the replica we truncate can be different.
    const auto original_mtime_replica_0 = replica::last_write_time(comm, target_object, 0);
    const auto original_mtime_replica_1 = replica::last_write_time(comm, target_object, 1);
    std::this_thread::sleep_for(2s);

    // Target replica 0 for truncate so that the tests are guaranteed consistent.
    DataObjInp truncate_doi{};
    const auto clear_kvp = irods::at_scope_exit{[&truncate_doi] { clearKeyVal(&truncate_doi.condInput); }};
    std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);
    addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "0");

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("same size")
    {
        truncate_doi.dataSize = contents.size();

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that none of the replicas were updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
    }

    SECTION("larger size")
    {
        constexpr auto new_size = contents.size() + 1;
        truncate_doi.dataSize = new_size;

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that the replica on the target resource was updated and the other replica was not updated,
        // but marked stale.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(new_size == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 < replica::last_write_time(comm, target_object, 0));
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
    }

    SECTION("smaller size")
    {
        constexpr auto new_size = contents.size() - 1;
        truncate_doi.dataSize = new_size;

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that the replica on the target resource was updated and the other replica was not updated,
        // but marked stale.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(new_size == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 < replica::last_write_time(comm, target_object, 0));
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
    }
} // basic_two_standalone_resources_no_explicit_target

TEST_CASE("two_replicas_in_replication_resource")
{
    load_client_api_plugins();

    const std::string default_resc = "demoResc";
    const std::string replication_resc = "replication_resc";
    const std::string test_resc = "test_resc";
    const std::string other_resc = "other_resc";
    const std::string vault_name = "test_resc_vault";
    const std::string other_vault_name = "other_resc_vault";

    // Create new resources.
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);
        unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
        unit_test_utils::add_ufs_resource(comm, other_resc, other_vault_name);
        create_replication_resource(comm, replication_resc);
    }

    // Construct resource hierarchy.
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);
        adm::client::add_child_resource(comm, replication_resc, test_resc);
        adm::client::add_child_resource(comm, replication_resc, other_resc);
    }

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_replica_truncate";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    const auto remove_sandbox = irods::at_scope_exit{[&] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

        adm::client::remove_child_resource(comm, replication_resc, test_resc);
        adm::client::remove_child_resource(comm, replication_resc, other_resc);
        adm::client::remove_resource(comm, replication_resc);
        adm::client::remove_resource(comm, other_resc);
        adm::client::remove_resource(comm, test_resc);
    }};

    const auto target_object = sandbox / "target_object";

    static constexpr auto contents = std::string_view{"content!"};

    // Create a new data object and show that the replica is in a good state.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

    // Replicate the data object and show that the replica is in a good state.
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), replication_resc));
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 1));
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 2));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 2));

    // Sleep here so that the mtime on the replica we truncate can be different.
    const auto original_mtime_replica_0 = replica::last_write_time(comm, target_object, 0);
    const auto original_mtime_replica_1 = replica::last_write_time(comm, target_object, 1);
    const auto original_mtime_replica_2 = replica::last_write_time(comm, target_object, 2);
    std::this_thread::sleep_for(2s);

    DataObjInp truncate_doi{};
    const auto clear_kvp = irods::at_scope_exit{[&truncate_doi] { clearKeyVal(&truncate_doi.condInput); }};
    std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("same_size_with_different_replicas_in_replication_hierarchy")
    {
        // Target the replica outside of the replication hierarchy.
        addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "0");

        constexpr auto different_size = contents.size() + 1;
        truncate_doi.dataSize = different_size;

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that the replica outside of the replication hierarchy is updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(different_size == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 < replica::last_write_time(comm, target_object, 0));

        // ...but not the replicas inside the replication hierarchy.
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 2));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 2));
        CHECK(original_mtime_replica_2 == replica::last_write_time(comm, target_object, 2));

        // Free the output string because the API is going to be called again.
        std::free(output_str);

        // Put the resource into the replication hierarchy and make sure it gets removed again at the end.
        REQUIRE_NOTHROW(adm::client::add_child_resource(comm, replication_resc, default_resc));
        const auto remove_resource_from_hierarchy = irods::at_scope_exit{[&replication_resc, &default_resc] {
            irods::experimental::client_connection conn2;
            RcComm& comm2 = static_cast<RcComm&>(conn2);
            REQUIRE_NOTHROW(adm::client::remove_child_resource(comm2, replication_resc, default_resc));
        }};

        // Sleep here so that the mtime on the replica we truncate CAN be different (but we don't expect it to be).
        const auto new_mtime_replica_0 = replica::last_write_time(comm, target_object, 0);
        std::this_thread::sleep_for(2s);

        // Attempt to truncate the object again, targeting the same replica with the same size. The idea is that
        // the truncate should not occur and it should not propagate to sibling replicas.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that none of the replicas were updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(different_size == replica::replica_size(comm, target_object, 0));
        CHECK(new_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));

        // ...including the other replicas inside the replication hierarchy.
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 2));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 2));
        CHECK(original_mtime_replica_2 == replica::last_write_time(comm, target_object, 2));
    }

    SECTION("same_size")
    {
        // Target the replication resource so that the replicas therein may be targeted for truncate.
        addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, replication_resc.c_str());

        truncate_doi.dataSize = contents.size();

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that none of the replicas were updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 2));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 2));
        CHECK(original_mtime_replica_2 == replica::last_write_time(comm, target_object, 2));

        // ...including the replica outside of the replication resource.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));
    }

    SECTION("larger_size")
    {
        // Target the replication resource so that the replicas therein may be targeted for truncate.
        addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, replication_resc.c_str());

        constexpr auto new_size = contents.size() + 1;
        //constexpr auto new_contents = std::string_view{contents.data(), new_size};
        truncate_doi.dataSize = new_size;

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that the replicas in the replication resource hierarchy were updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(new_size == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 < replica::last_write_time(comm, target_object, 1));
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 2));
        CHECK(new_size == replica::replica_size(comm, target_object, 2));
        CHECK(original_mtime_replica_2 < replica::last_write_time(comm, target_object, 2));

        // Ensure that the replica outside of the replication resource hierarchy was not updated and is now stale.
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));
    }

    SECTION("smaller_size")
    {
        // Target the replication resource so that the replicas therein may be targeted for truncate.
        addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, replication_resc.c_str());

        constexpr auto new_size = contents.size() - 1;
        //constexpr auto new_contents = std::string_view{contents.data(), new_size};
        truncate_doi.dataSize = new_size;

        // Attempt to truncate the object.
        REQUIRE(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));

        // Ensure that the replicas in the replication resource hierarchy were updated.
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
        CHECK(new_size == replica::replica_size(comm, target_object, 1));
        CHECK(original_mtime_replica_1 < replica::last_write_time(comm, target_object, 1));
        CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 2));
        CHECK(new_size == replica::replica_size(comm, target_object, 2));
        CHECK(original_mtime_replica_2 < replica::last_write_time(comm, target_object, 2));

        // Ensure that the replica outside of the replication resource hierarchy was not updated and is now stale.
        CHECK(STALE_REPLICA == replica::replica_status(comm, target_object, 0));
        CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
        CHECK(original_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));
    }
} // two_replicas_in_replication_resource

TEST_CASE("truncate_locked_data_object__issue_7104")
{
    load_client_api_plugins();

    const std::string test_resc = "test_resc";
    const std::string vault_name = "test_resc_vault";

    // Create a new resource
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);
        unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
    }

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_replica_truncate";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox, &test_resc] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

        adm::client::remove_resource(comm, test_resc);
    }};

    const auto target_object = sandbox / "target_object";

    static constexpr auto contents = std::string_view{"content!"};

    // Create a new data object.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }

    // Show that the replica is in a good state.
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

    // Replicate data object and ensure the replica is good.
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), test_resc));
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 1));

    // Sleep here so that the mtime on the replica we truncate CAN be different (but we don't expect it to be).
    const auto original_mtime_replica_0 = replica::last_write_time(comm, target_object, 0);
    const auto original_mtime_replica_1 = replica::last_write_time(comm, target_object, 1);
    std::this_thread::sleep_for(2s);

    // Open the object in read-write mode in order to lock the object.
    DataObjInp open_doi{};
    const auto clear_open_kvp = irods::at_scope_exit{[&open_doi] { clearKeyVal(&open_doi.condInput); }};
    addKeyVal(&open_doi.condInput, REPL_NUM_KW, "0");
    std::strncpy(open_doi.objPath, target_object.c_str(), MAX_NAME_LEN);
    open_doi.openFlags = O_RDWR;

    // Open replica and ensure that the status is updated appropriately.
    const auto fd = rcDataObjOpen(&comm, &open_doi);
    REQUIRE(fd > 2);
    // If any REQUIRE assertions fail, we need to ensure that the object is closed no matter what.
    const auto close_fd = irods::at_scope_exit{[&] {
        OpenedDataObjInp close_inp{};
        close_inp.l1descInx = fd;
        rcDataObjClose(&comm, &close_inp);
    }};
    REQUIRE(INTERMEDIATE_REPLICA == replica::replica_status(comm, target_object, 0));

    // This input for the truncate API will be common among all of the SECTIONS.
    DataObjInp truncate_doi{};
    const auto clear_kvp = irods::at_scope_exit{[&truncate_doi] { clearKeyVal(&truncate_doi.condInput); }};
    std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("target_intermediate_replica")
    {
        irods::experimental::client_connection conn2;
        RcComm& comm2 = static_cast<RcComm&>(conn2);

        const std::string default_resc = "demoResc";

        SECTION("by_replica_number")
        {
            addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "0");
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }

        SECTION("by_resource_name")
        {
            addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, default_resc.c_str());
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }

        SECTION("by_resource_hierarchy")
        {
            // This will skip voting.
            addKeyVal(&truncate_doi.condInput, RESC_HIER_STR_KW, default_resc.c_str());
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(INTERMEDIATE_REPLICA_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(INTERMEDIATE_REPLICA_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(INTERMEDIATE_REPLICA_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }
    }

    SECTION("target_write_locked_replica")
    {
        irods::experimental::client_connection conn2;
        RcComm& comm2 = static_cast<RcComm&>(conn2);

        SECTION("by_replica_number")
        {
            addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "1");
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }

        SECTION("by_resource_name")
        {
            addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, test_resc.c_str());
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }

        SECTION("by_resource_hierarchy")
        {
            // This will skip voting.
            addKeyVal(&truncate_doi.condInput, RESC_HIER_STR_KW, test_resc.c_str());
            // Attempt to truncate the object using the size specified for each section, and fail.
            // The assertion occurs inside the sections despite being identical for easier identification.
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(LOCKED_DATA_OBJECT_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(LOCKED_DATA_OBJECT_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(LOCKED_DATA_OBJECT_ACCESS == rc_replica_truncate(&comm2, &truncate_doi, &output_str));
            }
        }
    }

    SECTION("no_specific_target")
    {
        // Attempt to truncate the object using the size specified for each section, and fail.
        // The assertion occurs inside the sections despite being identical for easier identification.
        SECTION("same_size")
        {
            truncate_doi.dataSize = contents.size();
            CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm, &truncate_doi, &output_str));
        }
        SECTION("larger_size")
        {
            truncate_doi.dataSize = contents.size() + 1;
            CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm, &truncate_doi, &output_str));
        }
        SECTION("smaller_size")
        {
            truncate_doi.dataSize = contents.size() - 1;
            CHECK(HIERARCHY_ERROR == rc_replica_truncate(&comm, &truncate_doi, &output_str));
        }
    }

    // Close the open data object so that it is back at rest.
    OpenedDataObjInp close_inp{};
    close_inp.l1descInx = fd;
    REQUIRE(0 == rcDataObjClose(&comm, &close_inp));

    // Ensure that the object was not updated on either replica.
    CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
    CHECK(original_mtime_replica_0 == replica::last_write_time(comm, target_object, 0));
    CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 1));
    CHECK(contents.size() == replica::replica_size(comm, target_object, 1));
    CHECK(original_mtime_replica_1 == replica::last_write_time(comm, target_object, 1));
} // truncate_locked_data_object__issue_7104

TEST_CASE("inputs_that_will_not_work")
{
    load_client_api_plugins();

    const std::string default_resc = "demoResc";
    const std::string test_resc = "test_resc";
    const std::string vault_name = "test_resc_vault";

    // Create a new resource
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);
        unit_test_utils::add_ufs_resource(comm, test_resc, vault_name);
    }

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_replica_truncate";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox, &test_resc] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));

        adm::client::remove_resource(comm, test_resc);
    }};

    const auto target_object = sandbox / "target_object";

    static constexpr auto contents = std::string_view{"content!"};

    // Create a new data object.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }

    // Show that the replica is in a good state. Only 1 replica for this test.
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

    // Sleep here so that the mtime on the replica we truncate CAN be different (but we don't expect it to be).
    const auto original_mtime = replica::last_write_time(comm, target_object, 0);
    std::this_thread::sleep_for(2s);

    DataObjInp truncate_doi{};
    const auto clear_kvp = irods::at_scope_exit{[&truncate_doi] { clearKeyVal(&truncate_doi.condInput); }};
    std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);
    truncate_doi.dataSize = 0;

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("DEST_RESC_NAME_KW_not_allowed")
    {
        addKeyVal(&truncate_doi.condInput, DEST_RESC_NAME_KW, default_resc.c_str());
        CHECK(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("DEST_RESC_HIER_STR_KW_not_allowed")
    {
        addKeyVal(&truncate_doi.condInput, DEST_RESC_HIER_STR_KW, default_resc.c_str());
        CHECK(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("REPL_NUM_KW_and_RESC_NAME_KW_not_allowed_together")
    {
        addKeyVal(&truncate_doi.condInput, RESC_NAME_KW, test_resc.c_str());
        addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "1");
        CHECK(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("replica_number_that_does_not_exist")
    {
        addKeyVal(&truncate_doi.condInput, REPL_NUM_KW, "1");
        CHECK(SYS_REPLICA_DOES_NOT_EXIST == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("resource_name_with_no_replica")
    {
        addKeyVal(&truncate_doi.condInput, RESC_HIER_STR_KW, test_resc.c_str());
        CHECK(SYS_REPLICA_DOES_NOT_EXIST == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("hierarchy_with_no_replica")
    {
        addKeyVal(&truncate_doi.condInput, RESC_HIER_STR_KW, test_resc.c_str());
        CHECK(SYS_REPLICA_DOES_NOT_EXIST == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("negative_dataSize")
    {
        truncate_doi.dataSize = -1;
        const auto ec = irods::experimental::make_error_code(rc_replica_truncate(&comm, &truncate_doi, &output_str));
        CHECK(UNIX_FILE_TRUNCATE_ERR == irods::experimental::get_irods_error_code(ec));
        CHECK(EINVAL == irods::experimental::get_errno(ec));
    }

    // Ensure that the object was not updated on either replica.
    CHECK(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    CHECK(contents.size() == replica::replica_size(comm, target_object, 0));
    CHECK(original_mtime == replica::last_write_time(comm, target_object, 0));
} // inputs_that_will_not_work

TEST_CASE("really_bad_inputs")
{
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    DataObjInp truncate_doi{};

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("nullptr_comm")
    {
        REQUIRE(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(nullptr, nullptr, nullptr));
    }

    SECTION("nullptr_input_struct")
    {
        REQUIRE(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(&comm, nullptr, nullptr));
    }

    SECTION("nullptr_output_pointer")
    {
        REQUIRE(SYS_INVALID_INPUT_PARAM == rc_replica_truncate(&comm, &truncate_doi, nullptr));
    }

    SECTION("empty_input_struct")
    {
        REQUIRE(OBJ_PATH_DOES_NOT_EXIST == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }

    SECTION("not_absolute_logical_path")
    {
        std::strncpy(truncate_doi.objPath, "not_absolute_path", MAX_NAME_LEN);
        REQUIRE(SYS_INVALID_FILE_PATH == rc_replica_truncate(&comm, &truncate_doi, &output_str));
    }
} // really_bad_inputs

TEST_CASE("checksum_tests")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "test_rc_replica_truncate";
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    const auto target_object = sandbox / "target_object";

    static constexpr auto contents = std::string_view{"content!"};

    // Create a new data object and show that the replica is in a good state.
    {
        irods::experimental::io::client::native_transport tp{conn};
        irods::experimental::io::odstream{tp, target_object} << contents;
    }
    REQUIRE(GOOD_REPLICA == replica::replica_status(comm, target_object, 0));
    REQUIRE(contents.size() == replica::replica_size(comm, target_object, 0));

    const auto checksum_query = fmt::format("select DATA_CHECKSUM where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                            target_object.parent_path().c_str(),
                                            target_object.object_name().c_str());
    const auto original_checksum =
        replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
    CHECK(!irods::query{&comm, checksum_query}.front()[0].empty());

    DataObjInp truncate_doi{};
    const auto clear_kvp = irods::at_scope_exit{[&truncate_doi] { clearKeyVal(&truncate_doi.condInput); }};
    std::strncpy(truncate_doi.objPath, target_object.c_str(), MAX_NAME_LEN);

    char* output_str{};
    const auto free_output_str = irods::at_scope_exit{[&output_str] { std::free(output_str); }};

    SECTION("default_clear_checksum")
    {
        SECTION("same_size")
        {
            truncate_doi.dataSize = contents.size();
            CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
            // Ensure that the checksum was not cleared and that re-calculating it results in the same checksum.
            CHECK(!irods::query{&comm, checksum_query}.front()[0].empty());
            const auto new_checksum =
                replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
            CHECK(original_checksum == new_checksum);
        }
        SECTION("larger_size")
        {
            truncate_doi.dataSize = contents.size() + 1;
            CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
            // Ensure that the checksum was cleared after the truncate and that re-calculating it results in a
            // different checksum.
            CHECK(irods::query{&comm, checksum_query}.front()[0].empty());
            const auto new_checksum =
                replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
            CHECK(original_checksum != new_checksum);
        }
        SECTION("smaller_size")
        {
            truncate_doi.dataSize = contents.size() - 1;
            CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
            // Ensure that the checksum was cleared after the truncate and that re-calculating it results in a
            // different checksum.
            CHECK(irods::query{&comm, checksum_query}.front()[0].empty());
            const auto new_checksum =
                replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
            CHECK(original_checksum != new_checksum);
        }
    }

#if 0
        // TODO #7561: Calculating the checksum is not currently supported, but this test exists for when it becomes supported.
        SECTION("recalculate_checksum")
        {
            addKeyVal(&truncate_doi, REG_CHKSUM_KW, "");
            SECTION("same_size")
            {
                truncate_doi.dataSize = contents.size();
                CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
                // Ensure that the checksum was not cleared and that re-calculating it results in the same checksum.
                CHECK(!irods::query{&comm, checksum_query}.front()[0].empty());
                const auto new_checksum = replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
                CHECK(original_checksum == new_checksum);
            }
            SECTION("larger_size")
            {
                truncate_doi.dataSize = contents.size() + 1;
                CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
                // Ensure that the checksum was re-calculated and is identical.
                CHECK(!irods::query{&comm, checksum_query}.front()[0].empty());
                const auto new_checksum = replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
                CHECK(original_checksum == new_checksum);
            }
            SECTION("smaller_size")
            {
                truncate_doi.dataSize = contents.size() - 1;
                CHECK(0 == rc_replica_truncate(&comm, &truncate_doi, &output_str));
                // Ensure that the checksum was re-calculated and is identical.
                CHECK(!irods::query{&comm, checksum_query}.front()[0].empty());
                const auto new_checksum = replica::replica_checksum(comm, target_object, 0, replica::verification_calculation::always);
                CHECK(original_checksum == new_checksum);
            }
        }
#endif
} // checksum_tests
