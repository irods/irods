#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_plugin_context.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_re_ruleexistshelper.hpp"
#include "irods/irods_re_serialization.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/irods_state_table.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"
#include "irods/scoped_privileged_client.hpp"

#define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#include "irods/irods_query.hpp"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/any.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <unordered_map>

namespace
{
    // clang-format off
    using log_re = irods::experimental::log::rule_engine;
    using json   = nlohmann::json;
    // clang-format on

    auto get_rei(irods::callback& _effect_handler) -> ruleExecInfo_t&
    {
        ruleExecInfo_t* rei{};
        irods::error result{_effect_handler("unsafe_ms_ctx", &rei)};

        if (!result.ok()) {
            THROW(result.code(), "Failed to get rule execution info");
        }

        return *rei;
    }

    auto load_plugin_config(ruleExecInfo_t& _rei) -> std::optional<json>
    {
        // Must elevate privileges so that the configuration can be retrieved.
        // Users who aren't administrators cannot retrieve metadata they don't own.
        irods::experimental::scoped_privileged_client spc{*_rei.rsComm};

        const auto gql = fmt::format("select META_COLL_ATTR_VALUE "
                                     "where META_COLL_ATTR_NAME = 'irods::metadata_guard' and COLL_NAME = '/{}'",
                                     _rei.rsComm->myEnv.rodsZone);

        if (irods::query q{_rei.rsComm, gql}; q.size() > 0) {
            try {
                return json::parse(q.front()[0]);
            }
            catch (const json::exception&) {
                const char* msg = "Cannot parse Rule Engine Plugin configuration";
                log_re::error({{"log_message", fmt::format("{}.", msg)}, {"rule_engine_plugin", "metadata_guard"}});
                THROW(SYS_CONFIG_FILE_ERR, msg);
            }
        }

        return std::nullopt;
    }

    auto user_is_administrator(rsComm_t& conn) -> irods::error
    {
        if (irods::is_privileged_client(conn)) {
            return CODE(RULE_ENGINE_CONTINUE);
        }

        // clang-format off
        log_re::error({{"log_message", "User is not allowed to modify metadata."},
                       {"rule_engine_plugin", "metadata_guard"}});
        // clang-format on

        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "User must be an admininstrator to modify metadata");
    }

    //
    // Rule Engine Plugin
    //

    template <typename... Args>
    using operation = std::function<irods::error(irods::default_re_ctx&, Args...)>;

    auto rule_exists(irods::default_re_ctx&, const std::string& _rule_name, bool& _exists) -> irods::error
    {
        _exists = (_rule_name == "pep_api_mod_avu_metadata_pre") ||
                  (_rule_name == "pep_api_atomic_apply_metadata_operations_pre");
        return SUCCESS();
    }

    auto list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules) -> irods::error
    {
        _rules.push_back("pep_api_mod_avu_metadata_pre");
        _rules.push_back("pep_api_atomic_apply_metadata_operations_pre");
        return SUCCESS();
    }

    auto check_operation_for_violations(const std::string& _attribute,
                                        ruleExecInfo_t& rei,
                                        const json& config,
                                        const json& prefixes,
                                        const bool admin_only) -> irods::error
    {
        const auto prefix_matched = std::any_of(prefixes.cbegin(), prefixes.cend(), [&_attribute](const json& prefix) {
            return boost::starts_with(_attribute, prefix.get_ref<const std::string&>());
        });
        if (!prefix_matched) {
            return CODE(RULE_ENGINE_CONTINUE);
        }

        // If admin_only, success was already checked outside this call
        // Therefore, always fail.
        if (admin_only) {
            return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "User must be an admininstrator to modify metadata");
        }

        namespace adm = irods::experimental::administration;
        const adm::user user{rei.uoic->userName, rei.uoic->rodsZone};
        const auto& editors = config.at("editors");
        const auto editor_matched = std::any_of(editors.cbegin(), editors.cend(), [&rei, &user](const json& editor) {
            const auto& type = editor.at("type").get_ref<const std::string&>();
            const auto& name{editor.at("name").get_ref<const std::string&>()};
            if (type == "user") {
                return adm::server::local_unique_name(*rei.rsComm, user) == name;
            }
            if (type == "group") {
                const adm::group group{name};
                return adm::server::user_is_member_of_group(*rei.rsComm, group, user);
            }
            return false;
        });
        if (editor_matched) {
            return CODE(RULE_ENGINE_CONTINUE);
        }

        return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "User must be an authorized editor to modify metadata.");
    }

    auto handle_pep_api_mod_avu_metadata_pre(std::list<boost::any>& _rule_arguments, irods::callback _effect_handler)
        -> irods::error
    {
        try {
            auto* input = boost::any_cast<modAVUMetadataInp_t*>(*std::next(std::begin(_rule_arguments), 2));

            if (strcmp(input->arg0, "rmw") == 0) {
                log_re::error({{"log_message", "rmw is deprecated and its usage is disallowed by metadata_guard"},
                               {"rule_engine_plugin", "metadata_guard"}});
                return ERROR(SYS_NOT_ALLOWED, "rmw is deprecated and its usage is disallowed by metadata_guard");
            }

            auto& rei = get_rei(_effect_handler);
            const auto config = load_plugin_config(rei);

            if (!config) {
                return CODE(RULE_ENGINE_CONTINUE);
            }

            // JSON Configuration structure:
            // {
            //   "prefixes": ["irods::"],
            //   "admin_only": true,
            //   "editors": [
            //     {"type": "group", "name": "rodsadmin"},
            //     {"type": "user",  "name": "kory"},
            //     {"type": "user",  "name": "jane#otherZone"}
            //   ]
            // }

            // Per the API, any of arg5 to arg8 is eligible to be the new attribute
            // (depending on which one starts with "n:")
            // However, only one will be considered (multiples will throw CAT_INVALID_ARGUMENT)
            // Therefore, find the first one that starts with "n:"
            const auto candidates = {input->arg5, input->arg6, input->arg7, input->arg8};
            const auto mod_attr_ptr = std::find_if(std::begin(candidates), std::end(candidates), [](char* arg) {
                return arg && boost::starts_with(arg, "n:");
            });
            const char* mod_attr_name = nullptr;
            if (mod_attr_ptr != std::end(candidates)) {
                mod_attr_name = (*mod_attr_ptr) + 2;
            }
            for (auto&& prefix : config->at("prefixes")) {
                // If the metadata attribute starts with the prefix, then verify that the user
                // can modify the metadata attribute.
                if (boost::starts_with(input->arg3, prefix.get_ref<const std::string&>()) ||
                    (mod_attr_name && boost::starts_with(mod_attr_name, prefix.get_ref<const std::string&>())))
                {
                    // The "admin_only" flag supersedes the "editors" configuration option.
                    const auto& admin_iter = config->find("admin_only");
                    if (admin_iter != config->cend() && admin_iter->get<bool>()) {
                        return user_is_administrator(*rei.rsComm);
                    }

                    namespace adm = irods::experimental::administration;

                    const adm::user user{rei.uoic->userName, rei.uoic->rodsZone};

                    for (auto&& editor : config->at("editors")) {
                        if (const auto& type = editor.at("type").get_ref<const std::string&>(); type == "group") {
                            const adm::group group{editor.at("name").get_ref<const std::string&>()};

                            if (adm::server::user_is_member_of_group(*rei.rsComm, group, user)) {
                                return CODE(RULE_ENGINE_CONTINUE);
                            }
                        }
                        else if (type == "user") {
                            if (editor.at("name").get_ref<const std::string&>() ==
                                adm::server::local_unique_name(*rei.rsComm, user)) {
                                return CODE(RULE_ENGINE_CONTINUE);
                            }
                        }
                    }

                    // At this point, the user is not an administrator and they aren't a member of
                    // the editors list. Therefore, we return an error because the user is attempting to
                    // modify metadata in a guarded namespace.
                    return ERROR(CAT_INSUFFICIENT_PRIVILEGE_LEVEL, "User is not allowed to modify metadata");
                }
            }
        }
        catch (const json::exception&) {
            // clang-format off
            log_re::error({{"log_message", "Unexpected JSON access or type error."},
                           {"rule_engine_plugin", "metadata_guard"}});
            // clang-format on
        }
        catch (const std::exception& e) {
            log_re::error({{"log_message", e.what()}, {"rule_engine_plugin", "metadata_guard"}});
        }

        return CODE(RULE_ENGINE_CONTINUE);
    }

    auto handle_pep_api_atomic_apply_metadata_operations_pre(std::list<boost::any>& _rule_arguments,
                                                             irods::callback _effect_handler) -> irods::error
    {
        auto is_input_valid = [](const bytesBuf_t* _input) -> std::tuple<bool, std::string> {
            if (!_input) {
                return {false, "Missing JSON input"};
            }

            if (_input->len <= 0) {
                return {false, "Length of buffer must be greater than zero"};
            }

            if (!_input->buf) {
                return {false, "Missing input buffer"};
            }

            return {true, ""};
        };

        try {
            auto* input_bb = boost::any_cast<bytesBuf_t*>(*std::next(std::begin(_rule_arguments), 2));

            if (const auto [valid, msg] = is_input_valid(input_bb); !valid) {
                log_re::error(msg);
                return ERROR(INPUT_ARG_NOT_WELL_FORMED_ERR, msg);
            }

            const auto input_bb_casted = static_cast<const char*>(input_bb->buf);
            json input = json::parse(input_bb_casted, input_bb_casted + input_bb->len);
            const auto& operations = input.at("operations");

            auto& rei = get_rei(_effect_handler);
            const auto config = load_plugin_config(rei);
            if (config == std::nullopt) {
                return CODE(RULE_ENGINE_CONTINUE);
            }
            const auto prefixes_iter = config->find("prefixes");
            if (prefixes_iter == config->cend()) {
                log_re::error({{"log_message", "Required property \"prefixes\": [\"...\"] not found"},
                               {"rule_engine_plugin", "metadata_guard"}});
                return CODE(RULE_ENGINE_CONTINUE);
            }

            const auto admin_iter = config->find("admin_only");
            const bool admin_check = admin_iter != config->cend() && admin_iter->get<bool>();

            if (admin_check && user_is_administrator(*rei.rsComm).ok()) {
                return CODE(RULE_ENGINE_CONTINUE);
            }

            for (auto&& op : operations) {
                const auto err_code = check_operation_for_violations(
                    op.at("attribute").get_ref<const std::string&>(), rei, *config, *prefixes_iter, admin_check);
                if (!err_code.ok()) {
                    log_re::error({{"log_message", err_code.result()}, {"rule_engine_plugin", "metadata_guard"}});
                    return err_code;
                }
            }

            return CODE(RULE_ENGINE_CONTINUE);
        }
        catch (const json::exception& e) {
            log_re::error({{"log_message", "Failed to parse input into JSON"},
                           {"error_message", e.what()},
                           {"rule_engine_plugin", "metadata_guard"}});
        }
        catch (const std::exception& e) {
            log_re::error({{"log_message", e.what()}, {"rule_engine_plugin", "metadata_guard"}});
        }

        return CODE(RULE_ENGINE_CONTINUE);
    }

    auto exec_rule(irods::default_re_ctx&,
                   const std::string& _rule_name,
                   std::list<boost::any>& _rule_arguments,
                   irods::callback _effect_handler) -> irods::error
    {
        static const std::unordered_map<std::string,
                                        std::function<irods::error(std::list<boost::any>&, irods::callback)>>
            lookup_table{
                {"pep_api_mod_avu_metadata_pre", handle_pep_api_mod_avu_metadata_pre},
                {"pep_api_atomic_apply_metadata_operations_pre", handle_pep_api_atomic_apply_metadata_operations_pre}};

        static const auto rule_iterator = lookup_table.find(_rule_name);

        if (rule_iterator != lookup_table.end()) {
            return std::get<1>(*rule_iterator)(_rule_arguments, _effect_handler);
        }

        return CODE(RULE_ENGINE_CONTINUE);
    }
} //namespace

//
// Plugin Factory
//

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

extern "C" auto plugin_factory(const std::string& _instance_name, const std::string& _context) -> pluggable_rule_engine*
{
    // clang-format off
    const auto no_op         = [](auto&&...) { return SUCCESS(); };
    const auto not_supported = [](auto&&...) { return CODE(SYS_NOT_SUPPORTED); };
    // clang-format on

    auto* re = new pluggable_rule_engine{_instance_name, _context};

    re->add_operation("start", operation<const std::string&>{no_op});
    re->add_operation("stop", operation<const std::string&>{no_op});
    re->add_operation("rule_exists", operation<const std::string&, bool&>{rule_exists});
    re->add_operation("list_rules", operation<std::vector<std::string>&>{list_rules});
    re->add_operation("exec_rule", operation<const std::string&, std::list<boost::any>&, irods::callback>{exec_rule});
    re->add_operation(
        "exec_rule_text",
        operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{not_supported});
    re->add_operation(
        "exec_rule_expression", operation<const std::string&, msParamArray_t*, irods::callback>{not_supported});

    return re;
}
