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
#include "irods/transport/default_transport.hpp"

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>
#include <string_view>
#include <tuple>
#include <vector>

#include <unistd.h>
#include <signal.h>

#include "unit_test_utils.hpp"

namespace ix = irods::experimental;
namespace ra = irods::experimental::administration;
namespace fs = irods::experimental::filesystem;
namespace io = irods::experimental::io;

namespace
{
    auto get_hostname() noexcept -> std::string;
    auto create_resource_vault(const std::string& _vault_name) -> boost::filesystem::path;
} //namespace

TEST_CASE("test api resource_info_for_operation")
{
    load_client_api_plugins();
    irods::experimental::client_connection conn;
    RcComm& comm = static_cast<RcComm&>(conn);

    rodsEnv env;
    _getRodsEnv(env);

    const auto sandbox = fs::path{env.rodsHome} / "unit_testing_sandbox";
    if (!fs::client::exists(conn, sandbox)) {
        REQUIRE(fs::client::create_collection(conn, sandbox));
    }

    irods::at_scope_exit remove_sandbox{
        [&conn, &sandbox] { REQUIRE(fs::client::remove_all(conn, sandbox, fs::remove_options::no_trash)); }};

    std::string default_resource{"demoResc"};

    // get host name for default_resource
    auto p = ra::client::resource_info(conn, default_resource);
    REQUIRE(p.has_value());
    auto hostname = p->host_name();

    io::client::native_transport tp{conn};

    const std::string path{sandbox / "data_object.txt"};

#define BUFFER_CLEARING_AND_NULLING_LAMBDA \
  [&char_buffer] {                         \
  std::free(char_buffer);                  \
  char_buffer = nullptr;                   \
  }

    SECTION("For CREATE")
    {
        DataObjInp inp{};
        ix::key_value_proxy kvp{inp.condInput};
        std::strncpy(inp.objPath, path.c_str(), MAX_NAME_LEN);

        irods::at_scope_exit free_memory{[&kvp] { kvp.clear(); }};
        kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = "CREATE";

        char* char_buffer{};
        //irods::at_scope_exit free_json_buffer{free_and_null_buffer}; // doesn't work => needs deduction guide (DWM -
        //why?)

        std::function<void()> free_and_null_buffer{BUFFER_CLEARING_AND_NULLING_LAMBDA};
        irods::at_scope_exit free_json_buffer{BUFFER_CLEARING_AND_NULLING_LAMBDA};

        rc_get_resource_info_for_operation(&comm, &inp, &char_buffer);
        REQUIRE(char_buffer != nullptr);

        auto results = nlohmann::json::parse(char_buffer);
        REQUIRE(results["host"].get<std::string>() == hostname);
        REQUIRE(results["resource_hierarchy"].get<std::string>() == default_resource);
    }

    SECTION("For OPEN")
    {
        // create data object

        io::odstream data_object_out{tp, path, io::leaf_resource_name{default_resource}};
        data_object_out.close();

        DataObjInp inp{};
        ix::key_value_proxy kvp{inp.condInput};
        std::strncpy(inp.objPath, path.c_str(), MAX_NAME_LEN);

        irods::at_scope_exit free_memory{[&kvp] { kvp.clear(); }};
        kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = "OPEN";

        char* char_buffer{};
        rc_get_resource_info_for_operation(&comm, &inp, &char_buffer);
        REQUIRE(char_buffer != nullptr);

        auto results = nlohmann::json::parse(char_buffer);
        REQUIRE(results["host"].get<std::string>() == hostname);
        REQUIRE(results["resource_hierarchy"].get<std::string>() == default_resource);
    }

    SECTION("For WRITE")
    {
        const std::string path_for_write{sandbox / "data_object_for_WRITE.txt"};

        const auto vault = create_resource_vault("irods_get_resc_info_for_op_vault_1");
        const std::string hostname = get_hostname(); //"localhost";//DWM

        // Clean up resource configuration.
        const auto cmd = fmt::format("iadmin mkresc ut_ufs_resc_1 unixfilesystem {}:{}", hostname, vault.c_str());
        REQUIRE(std::system(cmd.c_str()) == 0);
        REQUIRE(std::system("iadmin mkresc ut_pt_resc passthru") == 0);
        REQUIRE(std::system("iadmin addchildtoresc ut_pt_resc ut_ufs_resc_1") == 0);

        // A new connection is required to pick up the test resource configuration.
        irods::experimental::client_connection conn;
        RcComm& comm = static_cast<RcComm&>(conn);

        // RAII clean-up path for the test resource configuration.
        irods::at_scope_exit remove_resources{[&conn, &path_for_write] {
            if (fs::client::exists(conn, path_for_write)) {
                REQUIRE(fs::client::remove(conn, path_for_write, fs::remove_options::no_trash));
            }
            REQUIRE(std::system("iadmin rmchildfromresc ut_pt_resc ut_ufs_resc_1") == 0);
            REQUIRE(std::system("iadmin rmresc ut_ufs_resc_1") == 0);
            REQUIRE(std::system("iadmin rmresc ut_pt_resc") == 0);
        }};

        io::client::native_transport tp{conn};
        io::odstream data_object_out{tp, path_for_write, io::root_resource_name{"ut_pt_resc"}};
        data_object_out.close();

        using tuples = std::vector<std::tuple<std::string, std::string>>;
        auto once = False;
        for (const auto& [resource_keyword, value] : tuples{// test case for one existing replica
                                                            {"", ""},
                                                            // test case for two existing replicas
                                                            {RESC_NAME_KW, "ut_pt_resc"},
                                                            {RESC_HIER_STR_KW, "ut_pt_resc;ut_ufs_resc_1"}})
        {
            DataObjInp inp{};
            ix::key_value_proxy kvp{inp.condInput};
            irods::at_scope_exit free_memory{[&kvp] { kvp.clear(); }};
            std::strncpy(inp.objPath, path_for_write.c_str(), MAX_NAME_LEN);

            kvp[GET_RESOURCE_INFO_OP_TYPE_KW] = "WRITE";

            if (!resource_keyword.empty()) {
                kvp[resource_keyword] = value;
            }

            char* char_buffer{};
            irods::at_scope_exit free_char_buffer{[&char_buffer] { std::free(char_buffer); }};

            rc_get_resource_info_for_operation(&comm, &inp, &char_buffer);
            REQUIRE(char_buffer != nullptr);

            auto results = nlohmann::json::parse(char_buffer);
            REQUIRE(results["host"].get<std::string>() == hostname);
            REQUIRE(results["resource_hierarchy"].get<std::string>() == "ut_pt_resc;ut_ufs_resc_1");

            // Ensure on all but first repetition that there are two replicas.
            if (!once) {
                unit_test_utils::replicate_data_object(conn, path_for_write, "demoResc");
                once = True;
            }
        }
    }
}

namespace
{

    auto get_hostname() noexcept -> std::string
    {
        char hostname[HOST_NAME_MAX + 1]{};
        gethostname(hostname, sizeof(hostname));
        return hostname;
    }

    auto create_resource_vault(const std::string& _vault_name) -> boost::filesystem::path
    {
        namespace fs = boost::filesystem;

        const auto suffix = "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        const auto vault = boost::filesystem::temp_directory_path() / (_vault_name + suffix).data();

        // Create the vault for the resource and allow the iRODS server to
        // read and write to the vault.

        if (!exists(vault)) {
            REQUIRE(fs::create_directory(vault));
        }

        fs::permissions(vault, fs::perms::add_perms | fs::perms::others_read | fs::perms::others_write);

        return vault;
    }
} //namespace
