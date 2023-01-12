#include "catch.hpp"

#include "client_connection.hpp"
#include "getRodsEnv.h"
#include "get_delay_rule_info.h"
#include "irods_at_scope_exit.hpp"
#include "irods_query.hpp"
#include "rodsClient.h"
#include "rodsDef.h"
#include "rodsErrorTable.h"
#include "user_administration.hpp"

#include <json.hpp>

#include <cstdlib>
#include <string>
#include <utility>

TEST_CASE("rc_get_delay_rule_info")
{
    using json = nlohmann::json;

    load_client_api_plugins();

    irods::experimental::client_connection conn;
    auto* conn_ptr = static_cast<RcComm*>(conn);

    SECTION("delay rule exists")
    {
        // Schedule a delay rule and get its ID.
        REQUIRE(std::system(R"_(irule -r irods_rule_engine_plugin-irods_rule_language-instance 'delay("<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME><PLUSET>3600s</PLUSET>") { writeLine("serverLog", "Testing API plugin."); }' null ruleExecOut)_") == 0);

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
        const auto info = json::parse(p, p + bbuf->len);

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
        CHECK(rc_get_delay_rule_info(conn_ptr,     nullptr,   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,     "00000", nullptr) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,          "",   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr, "bad_input",   &bbuf) == SYS_INVALID_INPUT_PARAM);
        CHECK(rc_get_delay_rule_info(conn_ptr,        "-1",   &bbuf) == SYS_INVALID_INPUT_PARAM);
    }

    SECTION("only invocable by rodsadmins")
    {
        namespace adm = irods::experimental::administration;

        rodsEnv env;
        _getRodsEnv(env);

        const auto test_cases = {
            std::make_pair(adm::user_type::rodsuser, "rodsuser"),
            std::make_pair(adm::user_type::groupadmin, "groupadmin")
        };

        for (auto&& test_case : test_cases) {
            DYNAMIC_SECTION("fails as " << test_case.second)
            {
                // Create a new user that is not a rodsadmin.
                adm::user alice{"alice"};
                REQUIRE(adm::client::add_user(conn, alice, test_case.first).value() == 0);
                irods::at_scope_exit remove_user{[&] { adm::client::remove_user(conn, alice); }};

                // Give alice a password so that we can authenticate as them.
                REQUIRE(adm::client::set_user_password(conn, alice, "rods").value() == 0);

                // Connect to the server as the test user.
                irods::experimental::client_connection alice_conn{env.rodsHost, env.rodsPort, alice.name.c_str(), env.rodsZone};
                REQUIRE(alice_conn);

                // Show that the non-admin user is not allowed to invoke the API endpoint.
                BytesBuf* bbuf{};
                REQUIRE(rc_get_delay_rule_info(static_cast<RcComm*>(alice_conn), "not_important", &bbuf) == SYS_NO_API_PRIV);
            }
        }
    }
}
