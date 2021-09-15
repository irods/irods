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

using namespace std::string_literals;
// clang-format on

namespace
{
    // global constants
    static const std::string resc_0 = "logical_locking_resc_0";
    static const std::string resc_1 = "logical_locking_resc_1";
    static const std::string resc_2 = "logical_locking_resc_2";
    static const auto resc_names = std::vector{resc_0, resc_1, resc_2};
    static const auto& opened_replica_resc = resc_0;
    static const auto& other_replica_resc = resc_1;

    inline auto get_sandbox_name() -> fs::path
    {
        rodsEnv env;
        _getRodsEnv(env);
        return fs::path{env.rodsHome} / "test_logical_locking";
    } // get_sandbox_name

    auto mkresc(const std::string_view _name) -> void
    {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        if (const auto [ec, exists] = adm::client::resource_exists(comm, _name); exists) {
            REQUIRE(adm::client::remove_resource(comm, _name));
        }

        REQUIRE(unit_test_utils::add_ufs_resource(comm, _name, "vault_for_"s + _name.data()));
    } // mkresc

    auto close_replica(RcComm& _comm, const int _fd)
    {
        const auto close_input = nlohmann::json{
            {"fd", _fd},
            {"update_size", false},
            {"update_status", false},
            {"compute_checksum", false},
            {"send_notifications", false},
            {"preserve_replica_state_table", true}
        }.dump();

        return rc_replica_close(&_comm, close_input.data());
    } // close_replica
} // anonymous namespace

TEST_CASE("open_and_close", "[write_lock]")
{
    load_client_api_plugins();

    // Make sure that the test resources created below are removed upon test completion
    irods::at_scope_exit remove_resources{[] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        for (const auto& r : resc_names) {
            adm::client::remove_resource(comm, r);
        }
    }};

    // Create resources to use for test cases
    for (const auto& r : resc_names) {
        mkresc(r);
    }

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // Construct some paths for use in tests
    const auto sandbox = get_sandbox_name();
    const auto target_object = sandbox / "target_object";

    // Create a target collection for the test data
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    // Clean up test collection after tests complete
    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    // Create a test data object
    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }
    REQUIRE(fs::client::exists(comm, target_object));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));

    // Replicate the test data object so that it has multiple replicas
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), resc_1));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));

    // Open the test data object so that it will be write-locked
    DataObjInp open_inp{};
    auto cond_input = irods::experimental::make_key_value_proxy(open_inp.condInput);
    const auto free_cond_input = irods::at_scope_exit{[&cond_input] { cond_input.clear(); }};
    std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", target_object.c_str());
    cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
    open_inp.openFlags = O_WRONLY;

    // Ensure that open successfully locked data object
    const auto fd = rcDataObjOpen(&comm, &open_inp);
    REQUIRE(fd > 2);
    REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
    REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));

    // This is done in an irods::at_scope_exit because catch will return immediately if
    // a REQUIRE clause fails. This ensures that the data object is properly closed for
    // cleanup purposes.
    const auto close_object = irods::at_scope_exit{[&]
    {
        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

        // Make sure everything unlocked properly
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
    }};

    // Use a separate connection to run the tests from that used in the setup
    irods::experimental::client_connection new_conn;
    RcComm& new_comm = static_cast<RcComm&>(new_conn);

    // Prepare input to rcDataObjOpen for the test cases
    DataObjInp fail_open_inp{};
    auto fail_cond_input = irods::experimental::make_key_value_proxy(fail_open_inp.condInput);
    std::snprintf(fail_open_inp.objPath, sizeof(fail_open_inp.objPath), "%s", target_object.c_str());

    // Get the file descriptor info from the opened replica and extract the replica token.
    char* out = nullptr;
    const auto json_input = nlohmann::json{{"fd", fd}}.dump();
    const auto l1_desc_info = rc_get_file_descriptor_info(&comm, json_input.data(), &out);
    REQUIRE(l1_desc_info == 0);
    const auto token = nlohmann::json::parse(out).at("replica_token").get<std::string>();
    std::free(out);

    SECTION("open_for_read")
    {
        fail_open_inp.openFlags = O_RDONLY;

        // open existing, currently opened replica
        fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
        REQUIRE(INTERMEDIATE_REPLICA_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // open existing, currently locked replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // ...and now, with a replica token
        fail_cond_input[REPLICA_TOKEN_KW] = token;

        // open existing, currently intermediate replica with a token... should not fail
        {
            fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            const auto fd2 = rcDataObjOpen(&new_comm, &fail_open_inp);
            REQUIRE(fd2 > 2);
            REQUIRE(close_replica(new_comm, fd2) >= 0);
        }

        // open existing, currently locked replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
    }

    SECTION("open_for_write")
    {
        fail_open_inp.openFlags = O_WRONLY;

        // open existing, currently opened replica
        fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
        REQUIRE(INTERMEDIATE_REPLICA_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // open existing, currently locked replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // ...and now, with a replica token
        fail_cond_input[REPLICA_TOKEN_KW] = token;

        // open existing, currently opened replica
        {
            fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            const auto fd2 = rcDataObjOpen(&new_comm, &fail_open_inp);
            REQUIRE(fd2 > 2);
            REQUIRE(close_replica(new_comm, fd2) >= 0);
        }

        // open existing, currently locked replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
    }

    SECTION("open_for_create")
    {
        fail_open_inp.openFlags = O_CREAT | O_WRONLY;

        // attempt overwriting existing, currently opened replica with new replica
        fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
        REQUIRE(INTERMEDIATE_REPLICA_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // attempt overwriting existing, currently locked replica with new replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // Attempt opening a new replica on locked object
        fail_cond_input[RESC_HIER_STR_KW] = resc_2;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // ...and now, with a replica token
        fail_cond_input[REPLICA_TOKEN_KW] = token;

        // attempt overwriting existing, currently opened replica with new replica
        {
            fail_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            const auto fd2 = rcDataObjOpen(&new_comm, &fail_open_inp);
            REQUIRE(fd2 > 2);
            REQUIRE(close_replica(new_comm, fd2) >= 0);
        }

        // attempt overwriting existing, currently locked replica with new replica
        fail_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));

        // Attempt opening a new replica on locked object
        fail_cond_input[RESC_HIER_STR_KW] = resc_2;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjOpen(&new_comm, &fail_open_inp));
    }
}

TEST_CASE("unlink", "[write_lock]")
{
    load_client_api_plugins();

    // Make sure that the test resources created below are removed upon test completion
    irods::at_scope_exit remove_resources{[] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        for (const auto& r : resc_names) {
            adm::client::remove_resource(comm, r);
        }
    }};

    // Create resources to use for test cases
    for (const auto& r : resc_names) {
        mkresc(r);
    }

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // Construct some paths for use in tests
    const auto sandbox = get_sandbox_name();
    const auto target_object = sandbox / "target_object";

    // Create a target collection for the test data
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    // Clean up test collection after tests complete
    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    // Create a test data object
    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }
    REQUIRE(fs::client::exists(comm, target_object));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));

    // Replicate the test data object so that it has multiple replicas
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), resc_1));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));

    // Open the test data object so that it will be write-locked
    DataObjInp open_inp{};
    auto cond_input = irods::experimental::make_key_value_proxy(open_inp.condInput);
    const auto free_cond_input = irods::at_scope_exit{[&cond_input] { cond_input.clear(); }};
    std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", target_object.c_str());
    cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
    open_inp.openFlags = O_WRONLY;

    // Ensure that open successfully locked data object
    const auto fd = rcDataObjOpen(&comm, &open_inp);
    REQUIRE(fd > 2);
    REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
    REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));

    // This is done in an irods::at_scope_exit because catch will return immediately if
    // a REQUIRE clause fails. This ensures that the data object is properly closed for
    // cleanup purposes.
    const auto close_object = irods::at_scope_exit{[&]
    {
        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

        // Make sure everything unlocked properly
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
    }};

    // Use a separate connection to run the tests from that used in the setup
    irods::experimental::client_connection new_conn;
    RcComm& new_comm = static_cast<RcComm&>(new_conn);

    // Prepare input to rcDataObjUnlink for the test cases
    DataObjInp unlink_inp{};
    auto unlink_cond_input = irods::experimental::make_key_value_proxy(unlink_inp.condInput);
    const auto free_unlink_cond_input = irods::at_scope_exit{[&unlink_cond_input] { unlink_cond_input.clear(); }};
    std::snprintf(unlink_inp.objPath, sizeof(unlink_inp.objPath), "%s", target_object.c_str());

    // When no flags are passed to rcDataObjUnlink, the default behavior will move the object
    // to the trash. Moving to the trash is a logical operation, so don't provide a target replica.
    SECTION("no flags")
    {
        REQUIRE(HIERARCHY_ERROR == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
    }

    // Providing the force flag skips putting the object in the trash (different code path)
    // Note: In this case, the force flag does not fail in hierarchy resolution but in the
    // actual lock check because hierarchy resolution failure is allowed to continue when
    // the force flag is active for unlinks.
    SECTION("force flag")
    {
        unlink_cond_input[FORCE_FLAG_KW] = "";

        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));

        unlink_cond_input.erase(FORCE_FLAG_KW);
    }

    // Providing REPL_NUM_KW is deprecated in the API, but will act like rsDataObjTrim
    SECTION("target replica number")
    {
        const auto opened_replica_number = ir::to_replica_number(new_comm, target_object.c_str(), opened_replica_resc);
        unlink_cond_input[REPL_NUM_KW] = std::to_string(*opened_replica_number);

        REQUIRE(HIERARCHY_ERROR == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));

        const auto other_replica_number = ir::to_replica_number(new_comm, target_object.c_str(), other_replica_resc);
        unlink_cond_input[REPL_NUM_KW] = std::to_string(*other_replica_number);

        REQUIRE(HIERARCHY_ERROR == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));

        unlink_cond_input.erase(REPL_NUM_KW);
    }

    // The UNREG_OPR type means that the replica(s) should not be physically unlinked. This
    // will take the same code path as the force flag and replica number cases.
    SECTION("unreg")
    {
        unlink_inp.oprType = UNREG_OPR;

        REQUIRE(HIERARCHY_ERROR == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
    }

    // When the full resource hierarchy is provided, voting does not occur so direct errors
    // regarding logical locking are returned. However, the hierarchy keyword does not really
    // impact the behavior of rcDataObjUnlink in any other way than bypassing voting. It
    // operates at the logical level so all replicas will be considered for unlinking.
    SECTION("target hierarchy")
    {
        unlink_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
        // This API endpoint is a logical level operation - it only considers the locked state
        // of the entire object when determining whether unlinking is allowed, not that of the
        // individual replicas. Therefore, we do not expect INTERMEDIATE_REPLICA_ACCESS here.
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));

        unlink_cond_input[RESC_HIER_STR_KW] = other_replica_resc;
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjUnlink(&new_comm, &unlink_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));

        unlink_cond_input.erase(RESC_HIER_STR_KW);
    }
}

TEST_CASE("trim", "[write_lock]")
{
    load_client_api_plugins();

    // Make sure that the test resources created below are removed upon test completion
    irods::at_scope_exit remove_resources{[] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        for (const auto& r : resc_names) {
            adm::client::remove_resource(comm, r);
        }
    }};

    // Create resources to use for test cases
    for (const auto& r : resc_names) {
        mkresc(r);
    }

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // Construct some paths for use in tests
    const auto sandbox = get_sandbox_name();
    const auto target_object = sandbox / "target_object";

    // Create a target collection for the test data
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    // Clean up test collection after tests complete
    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    // Create a test data object
    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }
    REQUIRE(fs::client::exists(comm, target_object));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));

    // Replicate the test data object so that it has multiple replicas
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), other_replica_resc));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));

    const auto& another_replica_resc = resc_2;
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), another_replica_resc));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, another_replica_resc));

    // Open the test data object so that it will be write-locked
    DataObjInp open_inp{};
    auto cond_input = irods::experimental::make_key_value_proxy(open_inp.condInput);
    const auto free_cond_input = irods::at_scope_exit{[&cond_input] { cond_input.clear(); }};
    std::snprintf(open_inp.objPath, sizeof(open_inp.objPath), "%s", target_object.c_str());
    cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
    open_inp.openFlags = O_WRONLY;

    // Ensure that open successfully locked data object
    const auto fd = rcDataObjOpen(&comm, &open_inp);
    REQUIRE(fd > 2);
    REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
    REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));
    REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, another_replica_resc));

    // This is done in an irods::at_scope_exit because catch will return immediately if
    // a REQUIRE clause fails. This ensures that the data object is properly closed for
    // cleanup purposes.
    const auto close_object = irods::at_scope_exit{[&]
    {
        openedDataObjInp_t close_inp{};
        close_inp.l1descInx = fd;
        REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

        // Make sure everything unlocked properly
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
        CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, another_replica_resc));
    }};

    // Use a separate connection to run the tests from that used in the setup
    irods::experimental::client_connection new_conn;
    RcComm& new_comm = static_cast<RcComm&>(new_conn);

    // Prepare input to rcDataObjTrim for the test cases
    DataObjInp trim_inp{};
    auto trim_cond_input = irods::experimental::make_key_value_proxy(trim_inp.condInput);
    const auto free_trim_cond_input = irods::at_scope_exit{[&trim_cond_input] { trim_cond_input.clear(); }};
    std::snprintf(trim_inp.objPath, sizeof(trim_inp.objPath), "%s", target_object.c_str());

    // When no flags are passed to rsDataObjTrim, the default behavior will trim the number of replicas
    // down to the minimum number of good replicas (default: 2). In this case, no replicas should be
    // trimmed because it is locked, but it would also take us below the minimum number of good replicas.
    SECTION("no flags")
    {
        REQUIRE(HIERARCHY_ERROR == rcDataObjTrim(&new_comm, &trim_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), another_replica_resc));
    }

    // Providing the RESC_NAME_KW targets a resource hierarchy from which to trim replicas. Because it
    // could be pointed at the root of a hierarchy, specific errors for trimming intermediate replicas
    // should not be expected.
    SECTION("target resource name")
    {
        // Target the currently open replica
        trim_cond_input[RESC_NAME_KW] = opened_replica_resc;
        REQUIRE(HIERARCHY_ERROR == rcDataObjTrim(&new_comm, &trim_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), another_replica_resc));

        // Target a sibling of the currently open replica
        trim_cond_input[RESC_NAME_KW] = other_replica_resc;
        REQUIRE(HIERARCHY_ERROR == rcDataObjTrim(&new_comm, &trim_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), another_replica_resc));

        trim_cond_input.erase(RESC_NAME_KW);
    }

    // Providing REPL_NUM_KW will target a particular replica number.
    SECTION("target replica number")
    {
        // Target the currently open replica
        const auto opened_replica_number = ir::to_replica_number(new_comm, target_object.c_str(), opened_replica_resc);
        trim_cond_input[REPL_NUM_KW] = std::to_string(*opened_replica_number);
        REQUIRE(HIERARCHY_ERROR == rcDataObjTrim(&new_comm, &trim_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), another_replica_resc));

        // Target a sibling of the currently open replica
        const auto other_replica_number = ir::to_replica_number(new_comm, target_object.c_str(), other_replica_resc);
        trim_cond_input[REPL_NUM_KW] = std::to_string(*other_replica_number);
        REQUIRE(HIERARCHY_ERROR == rcDataObjTrim(&new_comm, &trim_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), another_replica_resc));

        trim_cond_input.erase(REPL_NUM_KW);
    }
}

TEST_CASE("rename", "[write_lock]")
{
    load_client_api_plugins();

    // Make sure that the test resources created below are removed upon test completion
    irods::at_scope_exit remove_resources{[] {
        // reset connection to ensure resource manager is current
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        for (const auto& r : resc_names) {
            adm::client::remove_resource(comm, r);
        }
    }};

    // Create resources to use for test cases
    for (const auto& r : resc_names) {
        mkresc(r);
    }

    // reset connection so resources exist
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    // Construct some paths for use in tests
    const auto sandbox = get_sandbox_name();
    const auto target_object = sandbox / "target_object";

    // Create a target collection for the test data
    if (!fs::client::exists(comm, sandbox)) {
        REQUIRE(fs::client::create_collection(comm, sandbox));
    }

    // Clean up test collection after tests complete
    irods::at_scope_exit remove_sandbox{[&sandbox] {
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        REQUIRE(fs::client::remove_all(comm, sandbox, fs::remove_options::no_trash));
    }};

    // Create a test data object
    {
        io::client::default_transport tp{comm};
        io::odstream{tp, target_object, io::root_resource_name{resc_0}};
    }
    REQUIRE(fs::client::exists(comm, target_object));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));

    // Replicate the test data object so that it has multiple replicas
    REQUIRE(unit_test_utils::replicate_data_object(comm, target_object.c_str(), resc_1));
    REQUIRE(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));

    // Use a separate connection to run the tests from that used in the setup
    irods::experimental::client_connection new_conn;
    RcComm& new_comm = static_cast<RcComm&>(new_conn);

    const auto other_target_object = sandbox / "other_target_object";
    DataObjInp source_inp{};
    DataObjInp dest_inp{};
    std::snprintf(source_inp.objPath, sizeof(source_inp.objPath), "%s", target_object.c_str());
    std::snprintf(dest_inp.objPath, sizeof(dest_inp.objPath), "%s", other_target_object.c_str());

    DataObjCopyInp rename_inp{};
    rename_inp.srcDataObjInp = source_inp;
    rename_inp.destDataObjInp = dest_inp;
    auto source_cond_input = irods::experimental::make_key_value_proxy(rename_inp.srcDataObjInp.condInput);
    const auto free_source_cond_input = irods::at_scope_exit{[&source_cond_input] { source_cond_input.clear(); }};

    SECTION("new data object")
    {
        // Open source object to lock it
        DataObjInp source_open_inp{};
        auto source_open_cond_input = irods::experimental::make_key_value_proxy(source_open_inp.condInput);
        std::snprintf(source_open_inp.objPath, sizeof(source_open_inp.objPath), "%s", target_object.c_str());
        source_open_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
        source_open_inp.openFlags = O_WRONLY;

        const auto fd = rcDataObjOpen(&comm, &source_open_inp);
        REQUIRE(fd > 2);
        REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
        REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));

        // This is done in an irods::at_scope_exit because catch will return immediately if
        // a REQUIRE clause fails. This ensures that the data object is properly closed for
        // cleanup purposes.
        const auto close_source_object = irods::at_scope_exit{[&]
        {
            openedDataObjInp_t close_inp{};
            close_inp.l1descInx = fd;
            REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

            // Make sure everything unlocked properly
            CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
            CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
        }};

        // Attempt renaming source object
        REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjRename(&new_comm, &rename_inp));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), opened_replica_resc));
        REQUIRE(ir::replica_exists(new_comm, target_object.c_str(), other_replica_resc));
        REQUIRE(!fs::client::exists(comm, other_target_object));
    }

    SECTION("existing data object")
    {
        source_cond_input[FORCE_FLAG_KW] = "";

        const std::string contents = "something different";

        {
            io::client::default_transport tp{comm};
            io::odstream{tp, other_target_object, io::root_resource_name{resc_0}} << contents;
        }

        REQUIRE(fs::client::exists(comm, other_target_object));
        REQUIRE(GOOD_REPLICA == ir::replica_status(comm, other_target_object, 0));

        // source locked, destination unlocked
        {
            // Open source object to lock it
            DataObjInp source_open_inp{};
            auto source_open_cond_input = irods::experimental::make_key_value_proxy(source_open_inp.condInput);
            const auto free_source_cond_input = irods::at_scope_exit{[&source_open_cond_input] { source_open_cond_input.clear(); }};
            std::snprintf(source_open_inp.objPath, sizeof(source_open_inp.objPath), "%s", target_object.c_str());
            source_open_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            source_open_inp.openFlags = O_WRONLY;

            const auto fd = rcDataObjOpen(&comm, &source_open_inp);
            REQUIRE(fd > 2);
            REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
            REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));

            // This is done in an irods::at_scope_exit because catch will return immediately if
            // a REQUIRE clause fails. This ensures that the data object is properly closed for
            // cleanup purposes.
            const auto close_source_object = irods::at_scope_exit{[&]
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // Make sure everything unlocked properly
                CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
                CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
            }};

            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjRename(&new_comm, &rename_inp));
            REQUIRE(fs::client::exists(comm, target_object));
            REQUIRE(fs::client::exists(comm, other_target_object));
            REQUIRE(contents.size() == fs::client::data_object_size(new_comm, other_target_object));
        }

        // source unlocked, destination locked
        {
            // Open destination object to lock it
            DataObjInp dest_open_inp{};
            auto dest_open_cond_input = irods::experimental::make_key_value_proxy(dest_open_inp.condInput);
            const auto free_dest_cond_input = irods::at_scope_exit{[&dest_open_cond_input] { dest_open_cond_input.clear(); }};
            std::snprintf(dest_open_inp.objPath, sizeof(dest_open_inp.objPath), "%s", other_target_object.c_str());
            dest_open_inp.openFlags = O_WRONLY;

            const auto dest_fd = rcDataObjOpen(&comm, &dest_open_inp);
            REQUIRE(dest_fd > 2);
            REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, other_target_object, opened_replica_resc));

            // This is done in an irods::at_scope_exit because catch will return immediately if
            // a REQUIRE clause fails. This ensures that the data object is properly closed for
            // cleanup purposes.
            const auto close_dest_object = irods::at_scope_exit{[&]
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = dest_fd;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // Make sure everything unlocked properly and destination was not updated
                CHECK(GOOD_REPLICA == ir::replica_status(comm, other_target_object, opened_replica_resc));
                CHECK(contents.size() == fs::client::data_object_size(comm, other_target_object));
            }};

            // Attempt renaming source object to destination object
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjRename(&new_comm, &rename_inp));
            REQUIRE(fs::client::exists(comm, target_object));
            REQUIRE(fs::client::exists(comm, other_target_object));
        }

        // source locked, destination locked
        {
            // Open source object to lock it
            DataObjInp source_open_inp{};
            auto source_open_cond_input = irods::experimental::make_key_value_proxy(source_open_inp.condInput);
            const auto free_source_cond_input = irods::at_scope_exit{[&source_open_cond_input] { source_open_cond_input.clear(); }};
            std::snprintf(source_open_inp.objPath, sizeof(source_open_inp.objPath), "%s", target_object.c_str());
            source_open_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            source_open_inp.openFlags = O_WRONLY;

            const auto fd = rcDataObjOpen(&comm, &source_open_inp);
            REQUIRE(fd > 2);
            REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
            REQUIRE(WRITE_LOCKED == ir::replica_status(comm, target_object, other_replica_resc));

            // This is done in an irods::at_scope_exit because catch will return immediately if
            // a REQUIRE clause fails. This ensures that the data object is properly closed for
            // cleanup purposes.
            const auto close_source_object = irods::at_scope_exit{[&]
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = fd;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // Make sure everything unlocked properly
                CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, opened_replica_resc));
                CHECK(GOOD_REPLICA == ir::replica_status(comm, target_object, other_replica_resc));
            }};

            // Open destination object to lock it
            DataObjInp dest_open_inp{};
            auto dest_open_cond_input = irods::experimental::make_key_value_proxy(dest_open_inp.condInput);
            const auto free_dest_cond_input = irods::at_scope_exit{[&dest_open_cond_input] { dest_open_cond_input.clear(); }};
            std::snprintf(dest_open_inp.objPath, sizeof(dest_open_inp.objPath), "%s", other_target_object.c_str());
            dest_open_cond_input[RESC_HIER_STR_KW] = opened_replica_resc;
            dest_open_inp.openFlags = O_WRONLY;

            const auto dest_fd = rcDataObjOpen(&comm, &dest_open_inp);
            REQUIRE(dest_fd > 2);
            REQUIRE(INTERMEDIATE_REPLICA == ir::replica_status(comm, other_target_object, opened_replica_resc));

            // This is done in an irods::at_scope_exit because catch will return immediately if
            // a REQUIRE clause fails. This ensures that the data object is properly closed for
            // cleanup purposes.
            const auto close_dest_object = irods::at_scope_exit{[&]
            {
                openedDataObjInp_t close_inp{};
                close_inp.l1descInx = dest_fd;
                REQUIRE(rcDataObjClose(&comm, &close_inp) >= 0);

                // Make sure everything unlocked properly and destination was not updated
                CHECK(GOOD_REPLICA == ir::replica_status(comm, other_target_object, opened_replica_resc));
                REQUIRE(contents.size() == fs::client::data_object_size(comm, other_target_object));
            }};

            // Attempt renaming source object to destination object
            REQUIRE(LOCKED_DATA_OBJECT_ACCESS == rcDataObjRename(&new_comm, &rename_inp));
            REQUIRE(fs::client::exists(comm, target_object));
            REQUIRE(fs::client::exists(comm, other_target_object));
        }
    }
}

