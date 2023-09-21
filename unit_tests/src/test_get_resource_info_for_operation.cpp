#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/connection_pool.hpp"
#include "irods/dstream.hpp"
#include "irods/filesystem.hpp"
#include "irods/get_resource_info_for_operation.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_query.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/replica.hpp"
#include "irods/resource_administration.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsErrorTable.h"
#include "irods/transport/default_transport.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <csignal>
#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <unistd.h>

#include "unit_test_utils.hpp"

namespace ix = irods::experimental;
namespace ra = irods::experimental::administration;
namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;

namespace
{
    auto create_resource_vault_path(const std::string& _vault_name) -> std::string;
} //namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("get_resource_info_for_operation")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";
    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    const std::string_view default_resource{"demoResc"};

    // Get host name for default_resource.
    auto p = ra::client::resource_info(conn, default_resource);
    REQUIRE(p.has_value());
    auto hostname = p->host_name();

    const auto path{sandbox / "data_object.txt"};

    SECTION("operation is CREATE")
    {
        DataObjInp inp{};
        ix::key_value_proxy kvp{inp.condInput};
        std::strncpy(static_cast<char*>(inp.objPath), path.c_str(), MAX_NAME_LEN);

        irods::at_scope_exit clear_kvp{[&kvp] { kvp.clear(); }};
        kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = "CREATE";

        char* char_buffer{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        irods::at_scope_exit free_char_buffer{[&char_buffer] { std::free(char_buffer); }};

        REQUIRE(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &inp, &char_buffer) == 0);
        REQUIRE(nullptr != char_buffer);

        auto results = nlohmann::json::parse(char_buffer);
        CHECK(results.at("host").get_ref<const std::string&>() == hostname);
        CHECK(results.at("resource_hierarchy").get_ref<const std::string&>() == default_resource);
    }

    SECTION("operation is OPEN or UNLINK")
    {
        // Create a data object.
        {
            io::client::native_transport tp{conn};
            io::odstream{tp, path, io::leaf_resource_name{std::string{default_resource}}};
        }

        DataObjInp inp{};
        ix::key_value_proxy kvp{inp.condInput};
        std::strncpy(static_cast<char*>(inp.objPath), path.c_str(), MAX_NAME_LEN);

        // clang-format off
        const auto test_cases = std::to_array<std::pair<const char*, const char*>>({
            {"operation is OPEN", "OPEN"},
            {"operation is UNLINK", "UNLINK"}
        });
        // clang-format on

        for (auto&& [title, keyword_value] : test_cases) {
            DYNAMIC_SECTION(title)
            {
                irods::at_scope_exit clear_kvp{[&kvp] { kvp.clear(); }};
                kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = keyword_value;

                char* char_buffer{};
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
                irods::at_scope_exit free_char_buffer{[&char_buffer] { std::free(char_buffer); }};

                REQUIRE(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &inp, &char_buffer) == 0);
                REQUIRE(nullptr != char_buffer);

                const auto results = nlohmann::json::parse(char_buffer);
                CHECK(results.at("host").get_ref<const std::string&>() == hostname);
                CHECK(results.at("resource_hierarchy").get_ref<const std::string&>() == default_resource);
            }
        }
    }

    SECTION("operation is WRITE")
    {
        const auto path_for_write{sandbox / "data_object_for_WRITE.txt"};

        const auto vault = create_resource_vault_path("irods_get_resc_info_for_op_vault_1");
        const std::string hostname = unit_test_utils::get_hostname();

        // Clean up resource configuration.
        const auto cmd = fmt::format("iadmin mkresc ut_ufs_resc_1 unixfilesystem {}:{}", hostname, vault.c_str());
        REQUIRE(std::system(cmd.c_str()) == 0); // NOLINT(cert-env33-c)
        REQUIRE(std::system("iadmin mkresc ut_pt_resc passthru") == 0); // NOLINT(cert-env33-c)
        REQUIRE(std::system("iadmin addchildtoresc ut_pt_resc ut_ufs_resc_1") == 0); // NOLINT(cert-env33-c)

        // A new connection is required to pick up the test resource configuration.
        irods::experimental::client_connection conn;

        // RAII clean-up path for the test resource configuration.
        irods::at_scope_exit remove_resources{[&conn, &path_for_write] {
            if (fs::client::exists(conn, path_for_write)) {
                REQUIRE(fs::client::remove(conn, path_for_write, fs::remove_options::no_trash));
            }
            REQUIRE(std::system("iadmin rmchildfromresc ut_pt_resc ut_ufs_resc_1") == 0); // NOLINT(cert-env33-c)
            REQUIRE(std::system("iadmin rmresc ut_ufs_resc_1") == 0); // NOLINT(cert-env33-c)
            REQUIRE(std::system("iadmin rmresc ut_pt_resc") == 0); // NOLINT(cert-env33-c)
        }};

        // Create a data object.
        {
            io::client::native_transport tp{conn};
            io::odstream{tp, path_for_write, io::root_resource_name{"ut_pt_resc"}};
        }

        // clang-format off
        const std::vector<std::tuple<std::string, std::string, std::string>> test_cases{
            // Test case for one existing replica.
            {"single existing replica - no keywords", "", ""},
            // Test case for two existing replicas.
            {"multiple replicas - using RESC_NAME_KW", RESC_NAME_KW, "ut_pt_resc"},
            {"multiple replicas - using RESC_HIER_STR_KW", RESC_HIER_STR_KW, "ut_pt_resc;ut_ufs_resc_1"}
        };
        // clang-format on

        auto replicate_data_object = true;

        for (const auto& [title, resource_keyword, value] : test_cases) {
            DYNAMIC_SECTION(title)
            {
                DataObjInp inp{};
                ix::key_value_proxy kvp{inp.condInput};
                irods::at_scope_exit clear_kvp{[&kvp] { kvp.clear(); }};
                std::strncpy(static_cast<char*>(inp.objPath), path_for_write.c_str(), MAX_NAME_LEN);

                kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = "WRITE";

                if (!resource_keyword.empty()) {
                    kvp[resource_keyword] = value;
                }

                char* char_buffer{};
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
                irods::at_scope_exit free_char_buffer{[&char_buffer] { std::free(char_buffer); }};

                REQUIRE(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &inp, &char_buffer) == 0);
                REQUIRE(nullptr != char_buffer);

                const auto results = nlohmann::json::parse(char_buffer);
                CHECK(results.at("host").get_ref<const std::string&>() == hostname);
                CHECK(results.at("resource_hierarchy").get_ref<const std::string&>() == "ut_pt_resc;ut_ufs_resc_1");

                // Ensure on all but first repetition that there are two replicas.
                if (replicate_data_object) {
                    replicate_data_object = false;
                    unit_test_utils::replicate_data_object(conn, path_for_write.c_str(), "demoResc");
                }
            }
        }
    }

    SECTION("returns error on invalid inputs")
    {
        DataObjInp input{};
        char* json_output{};

        SECTION("RcComm is null")
        {
            CHECK(rc_get_resource_info_for_operation(nullptr, &input, &json_output) == SYS_INVALID_INPUT_PARAM);
            CHECK(nullptr == json_output);
        }

        SECTION("DataObjInp is null")
        {
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), nullptr, &json_output) ==
                  SYS_INVALID_INPUT_PARAM);
            CHECK(nullptr == json_output);
        }

        SECTION("Output pointer is null")
        {
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, nullptr) ==
                  SYS_INVALID_INPUT_PARAM);
            CHECK(nullptr == json_output);
        }

        SECTION("GET_RESOURCE_INFO_OP_TYPE_KW not set")
        {
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, &json_output) ==
                  SYS_INVALID_INPUT_PARAM);
            CHECK(nullptr == json_output);
        }

        SECTION("invalid value for GET_RESOURCE_INFO_OP_TYPE_KW")
        {
            addKeyVal(&input.condInput, GET_RESOURCE_INFO_OP_TYPE_KW, "invalid");
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, &json_output) ==
                  INVALID_OPERATION);
            CHECK(nullptr == json_output);
        }

        SECTION("invalid value for RESC_NAME_KW")
        {
            addKeyVal(&input.condInput, GET_RESOURCE_INFO_OP_TYPE_KW, "CREATE");
            addKeyVal(&input.condInput, RESC_NAME_KW, "invalid");
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, &json_output) ==
                  SYS_RESC_DOES_NOT_EXIST);
            CHECK(nullptr == json_output);
        }

        SECTION("invalid value for RESC_HIER_STR_KW")
        {
            addKeyVal(&input.condInput, GET_RESOURCE_INFO_OP_TYPE_KW, "CREATE");
            addKeyVal(&input.condInput, RESC_HIER_STR_KW, "invalid");
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, &json_output) ==
                  SYS_RESC_DOES_NOT_EXIST);
            CHECK(nullptr == json_output);
        }

        SECTION("invalid logical path")
        {
            const auto path = sandbox / "does_not_exist/data_object.txt";
            std::strncpy(static_cast<char*>(input.objPath), path.c_str(), sizeof(DataObjInp::objPath));
            addKeyVal(&input.condInput, GET_RESOURCE_INFO_OP_TYPE_KW, "OPEN");
            CHECK(rc_get_resource_info_for_operation(static_cast<RcComm*>(conn), &input, &json_output) ==
                  CAT_NO_ROWS_FOUND);
            CHECK(nullptr == json_output);
        }
    }
}

namespace
{
    auto create_resource_vault_path(const std::string& _vault_name) -> std::string
    {
        const auto suffix = "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        const auto vault = boost::filesystem::temp_directory_path() / (_vault_name + suffix).data();
        return vault.string();
    } // create_resource_vault_path
} //namespace
