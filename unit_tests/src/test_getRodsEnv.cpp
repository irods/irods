#include <catch2/catch.hpp>

#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/rcGlobalExtern.h"
#include "irods/rodsDef.h"

#include <fmt/format.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <utility>

TEST_CASE("#8012: getRodsEnv() loads data from irods_environment.json when called from client")
{
    const auto test_file = std::filesystem::current_path() / "irods_unit_test_issue_8012.client.json";
    const std::string_view encryption_algo = "enc_algo_8012.client";
    const std::string_view host = "host_8012.client";
    const int port = 8012;

    // Create a file which contains the information found in an irods_environment.json file.
    // This is to avoid corrupting the local environment file.
    // clang-format off
    std::ofstream{test_file} << fmt::format(fmt::runtime(R"_({{
        "irods_client_server_policy": "CS_NEG_REFUSE",
        "irods_connection_pool_refresh_time_in_seconds": 300,
        "irods_cwd": "/tempZone/home/rods",
        "irods_default_hash_scheme": "SHA256",
        "irods_default_number_of_transfer_threads": 4,
        "irods_default_resource": "demoResc",
        "irods_encryption_algorithm": "{}",
        "irods_encryption_key_size": 32,
        "irods_encryption_num_hash_rounds": 16,
        "irods_encryption_salt_size": 8,
        "irods_home": "/tempZone/home/rods",
        "irods_host": "{}",
        "irods_match_hash_policy": "compatible",
        "irods_maximum_size_for_single_buffer_in_megabytes": 32,
        "irods_port": {},
        "irods_transfer_buffer_size_for_parallel_transfer_in_megabytes": 4,
        "irods_user_name": "rods",
        "irods_zone_name": "tempZone"
    }})_"), encryption_algo, host, port);
    // clang-format on
    irods::at_scope_exit delete_test_file{[&test_file] { remove(test_file); }};

    ProcessType = CLIENT_PT; // ProcessType defaults to this.

    // Force getRodsEnv() to use our test environment file.
    REQUIRE(std::filesystem::exists(test_file));
    REQUIRE(setenv("IRODS_ENVIRONMENT_FILE", test_file.c_str(), 1) == 0);

    // Show that the data was read from the test environment file.
    rodsEnv env;
    CHECK(getRodsEnv(&env) == 0);
    CHECK(encryption_algo == env.rodsEncryptionAlgorithm);
    CHECK(host == env.rodsHost);
    CHECK(port == env.rodsPort);
}

TEST_CASE("#8012: getRodsEnv() loads data from server_properties when called from server")
{
    const auto test_file = std::filesystem::current_path() / "irods_unit_test_issue_8012.server.json";
    const std::string_view encryption_algo = "enc_algo_8012.server";
    const std::string_view host = "host_8012.server";
    const int port = 8012;
    const int transfer_threads = 5;

    // Create a file which contains some of the information found in a server_config.json
    // file. We only provide enough information to guarantee that getRodsEnv() updates the
    // members of RodsEnvironment we're interested in. We're only interested in proving that
    // getRodsEnv() loads information from the in-memory representation of server_config.json.
    // clang-format off
    std::ofstream{test_file} << fmt::format(fmt::runtime(R"_({{
        "advanced_settings": {{
            "default_number_of_transfer_threads": {},
            "maximum_size_for_single_buffer_in_megabytes": 32,
            "transfer_buffer_size_for_parallel_transfer_in_megabytes": 4
        }},
        "client_server_policy": "CS_NEG_REFUSE",
        "connection_pool_refresh_time_in_seconds": 300,
        "default_hash_scheme": "SHA256",
        "default_number_of_transfer_threads": 4,
        "default_resource_name": "demoResc",
        "encryption": {{
            "algorithm": "{}",
            "key_size": 32,
            "num_hash_rounds": 16,
            "salt_size": 8
        }},
        "home": "/tempZone/home/rods",
        "host": "{}",
        "match_hash_policy": "compatible",
        "maximum_size_for_single_buffer_in_megabytes": 32,
        "zone_auth_scheme": "native",
        "zone_name": "tempZone",
        "zone_port": {},
        "zone_user": "rods"
    }})_"), transfer_threads, encryption_algo, host, port);
    // clang-format on
    irods::at_scope_exit delete_test_file{[&test_file] { remove(test_file); }};

    // Tell server_properties which file to load.
    irods::server_properties::instance().init(test_file);

    // These are the various process types managed by the main server process.
    // It's important that we test all of them since all iRODS server processes rely on
    // the getRodsEnv() function.
    // clang-format off
    const auto test_cases = {
        std::pair{"SERVER_PT", SERVER_PT},
        std::pair{"AGENT_PT", AGENT_PT},
        std::pair{"DELAY_SERVER_PT", DELAY_SERVER_PT}
    };
    // clang-format on

    for (auto&& tc : test_cases) {
        DYNAMIC_SECTION("ProcessType is [" << tc.first << ']')
        {
            // Setting this value to something other than CLIENT_PT results in getRodsEnv()
            // loading data from server_properties.
            ProcessType = tc.second;

            // Show that the data was read from the test environment file.
            rodsEnv env;
            CHECK(getRodsEnv(&env) == 0);
            CHECK(transfer_threads == env.irodsDefaultNumberTransferThreads);
            CHECK(encryption_algo == env.rodsEncryptionAlgorithm);
            CHECK(host == env.rodsHost);
            CHECK(port == env.rodsPort);
        }
    }
}
