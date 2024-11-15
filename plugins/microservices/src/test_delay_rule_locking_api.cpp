/// \file

#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_ms_plugin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/msParam.h"
#include "irods/rcConnect.h"
#include "irods/rcMisc.h"
#include "irods/rs_delay_rule_lock.hpp"
#include "irods/rs_delay_rule_unlock.hpp"
#include "irods/rs_genquery2.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/irods_query.hpp"

#include "msi_assertion_macros.hpp"

#include <boost/asio/ip/host_name.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

namespace
{
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

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto test_lock_and_unlock_delay_rule([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("lock and unlock delay rules")

        // Delete all existing delay rules to avoid issues with the tests.
        IRODS_MSI_ASSERT(delete_all_delay_rules() == 0);

        // Delete the delay rule regardless of the test results.
        irods::at_scope_exit_unsafe delete_delay_rules{[] { delete_all_delay_rules(); }};

        // Schedule a delay rule for execution in the distant future. The delay rule
        // MUST NOT be executed by the delay server. It is just a placeholder to test
        // the delay rule locking API.
        constexpr const char* rule = R"___(writeLine("serverLog", "SHOULD NOT EXECUTE BEFORE TEST COMPLETES"))___";
        IRODS_MSI_ASSERT(create_delay_rule(rule) == 0);

        std::string rule_id;
        {
            // Show the delay rule is not locked using GenQuery1.
            irods::query query{_rei->rsComm, "select RULE_EXEC_ID where RULE_EXEC_LOCK_HOST = ''"};
            IRODS_MSI_ASSERT(!query.empty());
            // Capture the delay rule's ID so we can prove the locking API works as intended.
            rule_id = std::move(query.front()[0]);
            IRODS_MSI_ASSERT(!rule_id.empty());

            // Show the delay rule is not locked using GenQuery2.
            Genquery2Input input{};
            irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
            input.query_string = strdup("select DELAY_RULE_ID where DELAY_RULE_LOCK_HOST = ''");

            char* output{};
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

            IRODS_MSI_ASSERT(rs_genquery2(_rei->rsComm, &input, &output) == 0);
            const auto rows = nlohmann::json::parse(output);
            IRODS_MSI_ASSERT(!rows.empty());
            IRODS_MSI_ASSERT(rows[0][0].get_ref<const std::string&>() == rule_id);
        }

        // Lock the delay rule.
        DelayRuleLockInput lock_input{};
        rule_id.copy(lock_input.rule_id, sizeof(DelayRuleLockInput::rule_id) - 1);
        boost::asio::ip::host_name().copy(lock_input.lock_host, sizeof(DelayRuleLockInput::lock_host) - 1);
        lock_input.lock_host_pid = getpid();
        IRODS_MSI_ASSERT(rs_delay_rule_lock(_rei->rsComm, &lock_input) == 0);

        {
            // Show the delay rule is now locked using GenQuery1.
            const auto query_string = fmt::format("select RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
            irods::query query{_rei->rsComm, query_string};
            IRODS_MSI_ASSERT(!query.empty());
            const auto row = query.front();
            IRODS_MSI_ASSERT(row[0] == boost::asio::ip::host_name());
            IRODS_MSI_ASSERT(row[1] == std::to_string(getpid()));
            IRODS_MSI_ASSERT(!row[2].empty());

            // Show the delay rule is now locked using GenQuery2.
            Genquery2Input input{};
            irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
            input.query_string = strdup(fmt::format("select DELAY_RULE_LOCK_HOST, DELAY_RULE_LOCK_HOST_PID, DELAY_RULE_LOCK_TIME where DELAY_RULE_ID = '{}'", rule_id).c_str());

            char* output{};
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

            IRODS_MSI_ASSERT(rs_genquery2(_rei->rsComm, &input, &output) == 0);
            const auto rows = nlohmann::json::parse(output);
            IRODS_MSI_ASSERT(!rows.empty());
            IRODS_MSI_ASSERT(rows[0][0].get_ref<const std::string&>() == boost::asio::ip::host_name());
            IRODS_MSI_ASSERT(rows[0][1].get_ref<const std::string&>() == std::to_string(getpid()));
            IRODS_MSI_ASSERT(!rows[0][2].get_ref<const std::string&>().empty());
        }

        // Unlock the delay rule.
        DelayRuleUnlockInput unlock_input{};
        irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};
        unlock_input.rule_ids = strdup(nlohmann::json::array({rule_id}).dump().c_str());
        IRODS_MSI_ASSERT(rs_delay_rule_unlock(_rei->rsComm, &unlock_input) == 0);

        {
            // Show the delay rule is now unlocked using GenQuery1.
            const auto query_string = fmt::format("select RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
            irods::query query{_rei->rsComm, query_string};
            IRODS_MSI_ASSERT(!query.empty());
            const auto row = query.front();
            IRODS_MSI_ASSERT(row[0].empty());
            IRODS_MSI_ASSERT(row[1].empty());
            IRODS_MSI_ASSERT(row[2].empty());

            // Show the delay rule is now unlocked using GenQuery2.
            Genquery2Input input{};
            irods::at_scope_exit_unsafe free_gq2_string{[&input] { clearGenquery2Input(&input); }};
            input.query_string = strdup(fmt::format("select DELAY_RULE_LOCK_HOST, DELAY_RULE_LOCK_HOST_PID, DELAY_RULE_LOCK_TIME where DELAY_RULE_ID = '{}'", rule_id).c_str());

            char* output{};
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc)
            irods::at_scope_exit_unsafe free_output{[&output] { std::free(output); }};

            IRODS_MSI_ASSERT(rs_genquery2(_rei->rsComm, &input, &output) == 0);
            const auto rows = nlohmann::json::parse(output);
            IRODS_MSI_ASSERT(!rows.empty());
            IRODS_MSI_ASSERT(rows[0][0].get_ref<const std::string&>().empty());
            IRODS_MSI_ASSERT(rows[0][1].get_ref<const std::string&>().empty());
            IRODS_MSI_ASSERT(rows[0][2].get_ref<const std::string&>().empty());
        }

        IRODS_MSI_TEST_END
    } // test_lock_and_unlock_delay_rule

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto test_unlock_multiple_delay_rules([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("unlock multiple delay rules")

        // Delete all existing delay rules to avoid issues with the tests.
        IRODS_MSI_ASSERT(delete_all_delay_rules() == 0);

        // Delete the delay rule regardless of the test results.
        irods::at_scope_exit_unsafe delete_delay_rules{[] { delete_all_delay_rules(); }};

        // Schedule five delay rules for execution in the distant future. The delay rules
        // MUST NOT be executed by the delay server. They are just placeholders to test the API.
        constexpr const char* rule = R"___(writeLine("serverLog", "SHOULD NOT EXECUTE BEFORE TEST COMPLETES"))___";
        constexpr auto delay_rule_count = 5;
        for (int i = 0; i < delay_rule_count; ++i) {
            IRODS_MSI_ASSERT(create_delay_rule(rule) == 0);
        }

        std::vector<std::string> rule_ids;
        {
            DelayRuleLockInput lock_input{};
            boost::asio::ip::host_name().copy(lock_input.lock_host, sizeof(DelayRuleLockInput::lock_host) - 1);
            lock_input.lock_host_pid = getpid();

            // Capture the IDs of the delay rules and lock them.
            for (auto&& row : irods::query{_rei->rsComm, "select RULE_EXEC_ID where RULE_EXEC_LOCK_HOST = ''"}) {
                rule_ids.push_back(row[0]);

                // Lock the delay rule.
                row[0].copy(lock_input.rule_id, sizeof(DelayRuleLockInput::rule_id) - 1);
                IRODS_MSI_ASSERT(rs_delay_rule_lock(_rei->rsComm, &lock_input) == 0);
            }
        }
        IRODS_MSI_ASSERT(rule_ids.size() == delay_rule_count);

        // Show the delay rules are now locked.
        for (auto&& rule_id : rule_ids) {
            const auto query_string = fmt::format("select RULE_EXEC_ID, RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
            irods::query query{_rei->rsComm, query_string};
            IRODS_MSI_ASSERT(!query.empty());
            const auto row = query.front();
            IRODS_MSI_ASSERT(row[0] == rule_id);
            IRODS_MSI_ASSERT(row[1] == boost::asio::ip::host_name());
            IRODS_MSI_ASSERT(row[2] == std::to_string(getpid()));
            IRODS_MSI_ASSERT(!row[3].empty());
        }

        // Unlock the delay rules.
        DelayRuleUnlockInput unlock_input{};
        irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};
        unlock_input.rule_ids = strdup(nlohmann::json(rule_ids).dump().c_str());
        IRODS_MSI_ASSERT(rs_delay_rule_unlock(_rei->rsComm, &unlock_input) == 0);

        // Show the delay rules are now unlocked.
        for (auto&& rule_id : rule_ids) {
            const auto query_string = fmt::format("select RULE_EXEC_ID, RULE_EXEC_LOCK_HOST, RULE_EXEC_LOCK_HOST_PID, RULE_EXEC_LOCK_TIME where RULE_EXEC_ID = '{}'", rule_id);
            irods::query query{_rei->rsComm, query_string};
            IRODS_MSI_ASSERT(!query.empty());
            const auto row = query.front();
            IRODS_MSI_ASSERT(row[0] == rule_id);
            IRODS_MSI_ASSERT(row[1].empty());
            IRODS_MSI_ASSERT(row[2].empty());
            IRODS_MSI_ASSERT(row[3].empty());
        }

        // Show that invoking the unlock API with the same IDs is safe and does not
        // affect the delay rules.
        IRODS_MSI_ASSERT(rs_delay_rule_unlock(_rei->rsComm, &unlock_input) == 0);

        IRODS_MSI_TEST_END
    } // test_unlock_multiple_delay_rules

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    auto test_invalid_delay_rule_ids([[maybe_unused]] RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_BEGIN("invalid delay rule ids")

        // Unlock a nonexistent delay rule.
        DelayRuleUnlockInput unlock_input{};
        irods::at_scope_exit_unsafe clear_unlock_input{[&unlock_input] { clearDelayRuleUnlockInput(&unlock_input); }};

        // An integer which does not identify a delay rule.
        unlock_input.rule_ids = strdup(R"(["10"])");
        IRODS_MSI_ASSERT(rs_delay_rule_unlock(_rei->rsComm, &unlock_input) == 0);

        // Not an integer.
        unlock_input.rule_ids = strdup(R"(["xyz"])");
        IRODS_MSI_ASSERT(rs_delay_rule_unlock(_rei->rsComm, &unlock_input) == SYS_LIBRARY_ERROR);

        IRODS_MSI_TEST_END
    } // test_unlock_multiple_delay_rules

    auto msi_impl(RuleExecInfo* _rei) -> int
    {
        IRODS_MSI_TEST_CASE(test_lock_and_unlock_delay_rule, _rei);
        IRODS_MSI_TEST_CASE(test_unlock_multiple_delay_rules, _rei);
        IRODS_MSI_TEST_CASE(test_invalid_delay_rule_ids, _rei);

        return 0;
    } // msi_impl

    template <typename... Args, typename Function>
    auto make_msi(const std::string& _name, Function _func) -> irods::ms_table_entry*
    {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        auto* msi = new irods::ms_table_entry{sizeof...(Args)};
        msi->add_operation(_name, std::function<int(Args..., ruleExecInfo_t*)>(_func));
        return msi;
    } // make_msi
} // anonymous namespace

extern "C" auto plugin_factory() -> irods::ms_table_entry*
{
    return make_msi<>("msi_test_delay_rule_locking_api", msi_impl);
} // plugin_factory
