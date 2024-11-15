#include <catch2/catch.hpp>

#include "irods/client_connection.hpp"
#include "irods/delay_rule_lock.h"
#include "irods/delay_rule_unlock.h"
#include "irods/genquery2.h"
#include "irods/getRodsEnv.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_query.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsClient.h"
#include "irods/rodsDef.h"
#include "irods/rodsErrorTable.h"

#include <boost/asio/ip/host_name.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

auto delete_all_delay_rules() -> int;
auto create_delay_rule(const std::string& _rule) -> int;

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("lock and unlock delay rule")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;

    // Delete all existing delay rules to avoid issues with the tests.
    REQUIRE(delete_all_delay_rules() == 0);

    // Delete the delay rule regardless of the test results.
    irods::at_scope_exit_unsafe delete_delay_rules{[] { delete_all_delay_rules(); }};

    // Schedule a delay rule for execution in the distant future. The delay rule
    // MUST NOT be executed by the delay server. It is just a placeholder to test
    // the delay rule locking API.
    constexpr const char* rule = R"___(writeLine("serverLog", "SHOULD NOT EXECUTE BEFORE TEST COMPLETES"))___";
    REQUIRE(create_delay_rule(rule) == 0);

    std::string rule_id;
    {
        // Show the delay rule is not locked using GenQuery1.
        irods::query query{static_cast<RcComm*>(conn), "select RULE_EXEC_ID where RULE_EXEC_LOCK_HOST = ''"};
        REQUIRE_FALSE(query.empty());
        // Capture the delay rule's ID so we can prove the locking API works as intended.
        rule_id = std::move(query.front()[0]);
        CHECK_FALSE(rule_id.empty());

        // Show the delay rule is not locked using GenQuery2.
        Genquery2Input input{};
        irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
        input.query_string = strdup("select DELAY_RULE_ID where DELAY_RULE_LOCK_HOST = ''");

        char* output{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

        CHECK(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        const auto rows = nlohmann::json::parse(output);
        REQUIRE_FALSE(rows.empty());
        CHECK(rows[0][0].get_ref<const std::string&>() == rule_id);
    }

    // Lock the delay rule.
    DelayRuleLockInput lock_input{};
    rule_id.copy(lock_input.rule_id, sizeof(DelayRuleLockInput::rule_id) - 1);
    boost::asio::ip::host_name().copy(lock_input.lock_host, sizeof(DelayRuleLockInput::lock_host) - 1);
    lock_input.lock_host_pid = getpid();
    REQUIRE(rc_delay_rule_lock(static_cast<RcComm*>(conn), &lock_input) == 0);

    {
        // Show the delay rule is now locked using GenQuery1.
        const auto query_string = fmt::format("select RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
        irods::query query{static_cast<RcComm*>(conn), query_string};
        REQUIRE_FALSE(query.empty());
        const auto row = query.front();
        CHECK(row[0] == boost::asio::ip::host_name());
        CHECK(row[1] == std::to_string(getpid()));
        CHECK_FALSE(row[2].empty());

        // Show the delay rule is now locked using GenQuery2.
        Genquery2Input input{};
        irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
        input.query_string = strdup(fmt::format("select DELAY_RULE_LOCK_HOST, DELAY_RULE_LOCK_HOST_PID, DELAY_RULE_LOCK_TIME where DELAY_RULE_ID = '{}'", rule_id).c_str());

        char* output{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

        CHECK(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        const auto rows = nlohmann::json::parse(output);
        REQUIRE_FALSE(rows.empty());
        CHECK(rows[0][0].get_ref<const std::string&>() == boost::asio::ip::host_name());
        CHECK(rows[0][1].get_ref<const std::string&>() == std::to_string(getpid()));
        CHECK_FALSE(rows[0][2].get_ref<const std::string&>().empty());
    }

    // Unlock the delay rule.
    DelayRuleUnlockInput unlock_input{};
    irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};
    unlock_input.rule_ids = strdup(nlohmann::json::array({rule_id}).dump().c_str());
    REQUIRE(rc_delay_rule_unlock(static_cast<RcComm*>(conn), &unlock_input) == 0);

    {
        // Show the delay rule is now unlocked using GenQuery1.
        const auto query_string = fmt::format("select RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
        irods::query query{static_cast<RcComm*>(conn), query_string};
        REQUIRE_FALSE(query.empty());
        const auto row = query.front();
        CHECK(row[0].empty());
        CHECK(row[1].empty());
        CHECK(row[2].empty());

        // Show the delay rule is now unlocked using GenQuery2.
        Genquery2Input input{};
        irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
        input.query_string = strdup(fmt::format("select DELAY_RULE_LOCK_HOST, DELAY_RULE_LOCK_HOST_PID, DELAY_RULE_LOCK_TIME where DELAY_RULE_ID = '{}'", rule_id).c_str());

        char* output{};
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
        irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

        CHECK(rc_genquery2(static_cast<RcComm*>(conn), &input, &output) == 0);
        const auto rows = nlohmann::json::parse(output);
        REQUIRE_FALSE(rows.empty());
        CHECK(rows[0][0].get_ref<const std::string&>().empty());
        CHECK(rows[0][1].get_ref<const std::string&>().empty());
        CHECK(rows[0][2].get_ref<const std::string&>().empty());
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_CASE("unlock multiple delay rules")
{
    load_client_api_plugins();

    irods::experimental::client_connection conn;

    // Delete all existing delay rules to avoid issues with the tests.
    REQUIRE(delete_all_delay_rules() == 0);

    // Delete the delay rule regardless of the test results.
    irods::at_scope_exit_unsafe delete_delay_rules{[] { delete_all_delay_rules(); }};

    // Schedule five delay rules for execution in the distant future. The delay rules
    // MUST NOT be executed by the delay server. They are just placeholders to test the API.
    constexpr const char* rule = R"___(writeLine("serverLog", "SHOULD NOT EXECUTE BEFORE TEST COMPLETES"))___";
    constexpr auto delay_rule_count = 5;
    for (int i = 0; i < delay_rule_count; ++i) {
        REQUIRE(create_delay_rule(rule) == 0);
    }

    std::vector<std::string> rule_ids;
    {
        DelayRuleLockInput lock_input{};
        boost::asio::ip::host_name().copy(lock_input.lock_host, sizeof(DelayRuleLockInput::lock_host) - 1);
        lock_input.lock_host_pid = getpid();

        // Capture the IDs of the delay rules and lock them.
        for (auto&& row : irods::query{static_cast<RcComm*>(conn), "select RULE_EXEC_ID where RULE_EXEC_LOCK_HOST = ''"}) {
            rule_ids.push_back(row[0]);

            // Lock the delay rule.
            row[0].copy(lock_input.rule_id, sizeof(DelayRuleLockInput::rule_id) - 1);
            REQUIRE(rc_delay_rule_lock(static_cast<RcComm*>(conn), &lock_input) == 0);
        }
    }
    CHECK(rule_ids.size() == delay_rule_count);

    // Show the delay rules are now locked.
    for (auto&& rule_id : rule_ids) {
        const auto query_string = fmt::format("select RULE_EXEC_ID, RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
        irods::query query{static_cast<RcComm*>(conn), query_string};
        REQUIRE_FALSE(query.empty());
        const auto row = query.front();
        CHECK(row[0] == rule_id);
        CHECK(row[1] == boost::asio::ip::host_name());
        CHECK(row[2] == std::to_string(getpid()));
        CHECK_FALSE(row[3].empty());
    }

    // Unlock the delay rules.
    DelayRuleUnlockInput unlock_input{};
    irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};
    unlock_input.rule_ids = strdup(nlohmann::json(rule_ids).dump().c_str());
    REQUIRE(rc_delay_rule_unlock(static_cast<RcComm*>(conn), &unlock_input) == 0);

    // Show the delay rules are now unlocked.
    for (auto&& rule_id : rule_ids) {
        const auto query_string = fmt::format("select RULE_EXEC_ID, RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
        irods::query query{static_cast<RcComm*>(conn), query_string};
        REQUIRE_FALSE(query.empty());
        const auto row = query.front();
        CHECK(row[0] == rule_id);
        CHECK(row[1].empty());
        CHECK(row[2].empty());
        CHECK(row[3].empty());
    }

    // Show that invoking the unlock API with the same IDs is safe and does not
    // affect the delay rules.
    CHECK(rc_delay_rule_unlock(static_cast<RcComm*>(conn), &unlock_input) == 0);
}

TEST_CASE("invalid delay rule ids")
{
    irods::experimental::client_connection conn;

    // Unlock a nonexistent delay rule.
    DelayRuleUnlockInput unlock_input{};
    irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};

    SECTION("an integer which does not identify a delay rule")
    {
        unlock_input.rule_ids = strdup(R"(["10"])");
        REQUIRE(rc_delay_rule_unlock(static_cast<RcComm*>(conn), &unlock_input) == 0);
    }

    SECTION("not an integer")
    {
        unlock_input.rule_ids = strdup(R"(["xyz"])");
        REQUIRE(rc_delay_rule_unlock(static_cast<RcComm*>(conn), &unlock_input) == SYS_LIBRARY_ERROR);
    }
}

auto delete_all_delay_rules() -> int
{
    // NOLINTNEXTLINE(cert-env33-c)
    return std::system("iqdel -a");
} // delete_all_delay_rules

auto create_delay_rule(const std::string& _rule) -> int
{
    // NOLINTNEXTLINE(cert-env33-c)
    return std::system(fmt::format(R"___(irule -r irods_rule_engine_plugin-irods_rule_language-instance 'delay("<PLUSET>100000</PLUSET>") {{ {} }}' null null)___", _rule).c_str());
} // create_delay_rule
