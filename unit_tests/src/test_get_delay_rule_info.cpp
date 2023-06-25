#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/getRodsEnv.h"
#include "irods/get_delay_rule_info.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_query.hpp"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"
#include "irods/user_administration.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>
#include <utility>

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("rc_get_delay_rule_info")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;
    auto* conn_ptr = static_cast<RcComm*>(conn);

    SECTION("delay rule exists")
    {
        // clang-format off
        // Schedule a delay rule and get its ID.
        REQUIRE(std::system(R"_(irule -r irods_rule_engine_plugin-irods_rule_language-instance 'delay("<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME><PLUSET>3600s</PLUSET>") { writeLine("serverLog", "Testing API plugin."); }' null ruleExecOut)_") == 0); // NOLINT(cert-env33-c, concurrency-mt-unsafe)
        // clang-format on

        std::string rule_id;
        for (auto&& row : irods::query{conn_ptr, "select RULE_EXEC_ID"}) {
            rule_id = row[0];
            break;
        }

        INFO("GenQuery returned rule_id = " << rule_id);

        BytesBuf* bbuf{};
        REQUIRE(rc_get_delay_rule_info(conn_ptr, rule_id.c_str(), &bbuf) == 0);

        CHECK(bbuf->buf);
        CHECK(bbuf->len > 0);

        const auto* p = static_cast<char*>(bbuf->buf);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto info = nlohmann::json::parse(p, p + bbuf->len);

        CHECK(info.size() == 13);
        CHECK(info.at("rule_id").get_ref<const std::string&>() == rule_id);
        CHECK(info.count("rule_name"));
        CHECK(info.count("rei_file_path"));
        CHECK(info.count("user_name"));
        CHECK(info.count("exe_address"));
        CHECK(info.count("exe_time"));
        CHECK(info.count("exe_frequency"));
        CHECK(info.count("priority"));
        CHECK(info.count("last_exe_time"));
        CHECK(info.count("exe_status"));
        CHECK(info.count("estimated_exe_time"));
        CHECK(info.count("notification_addr"));
        CHECK(info.count("exe_context"));
    }

    SECTION("invalid input")
    {
        BytesBuf* bbuf{};

        // clang-format off
        CHECK(rc_get_delay_rule_info(conn_ptr,     nullptr,   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,     "00000", nullptr) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,          "",   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr, "bad_input",   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,        "-1",   &bbuf) == SYS_INVALID_INPUT_PARAM);
        // clang-format on
    }

    SECTION("only invocable by rodsadmins")
    {
        namespace adm = irods::experimental::administration;

        rodsEnv env;
        _getRodsEnv(env);

        // clang-format off
        const auto test_cases = {
            std::make_pair(adm::user_type::rodsuser, "rodsuser"),
            std::make_pair(adm::user_type::groupadmin, "groupadmin")
        };
        // clang-format on

        for (auto&& test_case : test_cases) {
            DYNAMIC_SECTION("fails as " << test_case.second)
            {
                // Create a new user that is not a rodsadmin.
                adm::user alice{"alice"};
                REQUIRE_NOTHROW(adm::client::add_user(conn, alice, test_case.first));
                irods::at_scope_exit remove_user{[&] { adm::client::remove_user(conn, alice); }};

                // Give alice a password so that we can authenticate as them.
                REQUIRE_NOTHROW(adm::client::modify_user(conn, alice, adm::user_password_property{"rods"}));

                // Connect to the server as the test user.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                irods::experimental::client_connection alice_conn{
                    env.rodsHost, env.rodsPort, {alice.name, env.rodsZone}};
                REQUIRE(alice_conn);

                // Show that the non-admin user is not allowed to invoke the API endpoint.
                BytesBuf* bbuf{};
                REQUIRE(rc_get_delay_rule_info(static_cast<RcComm*>(alice_conn), "bogus", &bbuf) == SYS_NO_API_PRIV);
            }
        }
    }
}
