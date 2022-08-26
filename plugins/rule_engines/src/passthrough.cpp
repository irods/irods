#include "irods/irods_plugin_context.hpp"
#include "irods/irods_re_plugin.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_state_table.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <regex>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <initializer_list>

#include <nlohmann/json.hpp>

namespace
{
    //
    // Rule Engine Plugin
    //

    namespace log = irods::experimental::log;

    struct pep_config
    {
        std::regex regex;
        int code;
    };

    std::unordered_map<std::string, std::vector<pep_config>> pep_configs;
    int matched_code = 0;

    template <typename ...Args>
    using operation = std::function<irods::error(irods::default_re_ctx&, Args...)>;

    irods::error start(irods::default_re_ctx&, const std::string& _instance_name)
    {
        std::string config_path;

        if (auto error = irods::get_full_path_for_config_file("server_config.json", config_path);
            !error.ok())
        {
            std::string msg = "Server configuration not found [path => ";
            msg += config_path;
            msg += ']';

            // clang-format off
            log::rule_engine::error({{"rule_engine_plugin", "passthrough"},
                                     {"rule_engine_plugin_function", __func__},
                                     {"log_message", msg}});
            // clang-format on

            return ERROR(SYS_CONFIG_FILE_ERR, msg.c_str());
        }

        // clang-format off
        log::rule_engine::trace({{"rule_engine_plugin", "passthrough"},
                                 {"rule_engine_plugin_function", __func__},
                                 {"log_message", "Reading plugin configuration ..."}});
        // clang-format on

        nlohmann::json config;

        {
            std::ifstream config_file{config_path};
            config_file >> config;
        }

        try {
            // Iterate over the list of rule engine plugins until the Passthrough REP is found.
            for (const auto& re : config.at(irods::KW_CFG_PLUGIN_CONFIGURATION).at(irods::KW_CFG_PLUGIN_TYPE_RULE_ENGINE)) {
                if (_instance_name == re.at(irods::KW_CFG_INSTANCE_NAME).get<std::string>()) {
                    // Fill the "pep_configs" plugin variable with objects containing the values
                    // defined in the "return_codes_for_peps" configuration. Each object in the list
                    // will contain a regular expression and a code.
                    for (const auto& e : re.at(irods::KW_CFG_PLUGIN_SPECIFIC_CONFIGURATION).at("return_codes_for_peps")) {
                        pep_configs[_instance_name].push_back({std::regex{e.at("regex").get<std::string>()}, e.at("code").get<int>()});
                    }

                    return SUCCESS();
                }
            }
        }
        catch (const std::exception& e) {
            // clang-format off
            log::rule_engine::error({{"rule_engine_plugin", "passthrough"},
                                     {"rule_engine_plugin_function", __func__},
                                     {"log_message", e.what()}});
            // clang-format on

            return ERROR(SYS_CONFIG_FILE_ERR, e.what());
        }

        return ERROR(SYS_CONFIG_FILE_ERR, "[passthrough] Bad rule engine plugin configuration");
    }

    irods::error rule_exists(const std::string& _instance_name,
                             irods::default_re_ctx&,
                             const std::string& _rule_name,
                             bool& _exists)
    {
        for (const auto& pc : pep_configs[_instance_name]) {
            if (_exists = std::regex_search(_rule_name, pc.regex); _exists) {
                matched_code = pc.code;
                break;
            }
        }

        // clang-format off
        log::rule_engine::trace({{"rule_engine_plugin", "passthrough"},
                                 {"rule_engine_plugin_function", __func__},
                                 {"rule_engine_plugin_instance_name", _instance_name},
                                 {"rule", _rule_name},
                                 {"rule_exists", _exists ? "true" : "false"}});
        // clang-format on

        return SUCCESS();
    }

    irods::error list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules)
    {
        log::rule_engine::trace({{"rule_engine_plugin", "passthrough"}, {"rule_engine_plugin_function", __func__}});
        return SUCCESS();
    }

    irods::error exec_rule(const std::string& _instance_name,
                           irods::default_re_ctx&,
                           const std::string& _rule_name,
                           std::list<boost::any>& _rule_arguments,
                           irods::callback _effect_handler)
    {
        std::string msg = "Returned '";
        msg += std::to_string(matched_code);
        msg += "' to REPF.";

        // clang-format off
        log::rule_engine::trace({{"log_message", msg},
                                 {"rule_engine_plugin", "passthrough"},
                                 {"rule_engine_plugin_function", __func__},
                                 {"rule_engine_plugin_instance_name", _instance_name},
                                 {"rule", _rule_name}});
        // clang-format on

        const std::initializer_list<int> continuation_codes = {SYS_NOT_SUPPORTED, RULE_ENGINE_CONTINUE};
        const auto pred = [ec = matched_code](const auto _ec) { return _ec == ec; };

        if (std::any_of(std::begin(continuation_codes), std::end(continuation_codes), pred)) {
            return CODE(matched_code);
        }

        return ERROR(matched_code, "");
    }
} // namespace (anonymous)

//
// Plugin Factory
//

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

extern "C"
pluggable_rule_engine* plugin_factory(const std::string& _instance_name,
                                      const std::string& _context)
{
    // clang-format off
    const auto no_op         = [](auto&&...) { return SUCCESS(); };
    const auto not_supported = [](auto&&...) { return CODE(SYS_NOT_SUPPORTED); };
    // clang-format on

    const auto rule_exists_wrapper = [_instance_name](irods::default_re_ctx& _ctx,
                                                      const std::string& _rule_name,
                                                      bool& _exists)
    {
        return rule_exists(_instance_name, _ctx, _rule_name, _exists);
    };

    const auto exec_rule_wrapper = [_instance_name](irods::default_re_ctx& _ctx,
                                                    const std::string& _rule_name,
                                                    std::list<boost::any>& _rule_arguments,
                                                    irods::callback _effect_handler)
    {
        return exec_rule(_instance_name, _ctx, _rule_name, _rule_arguments, _effect_handler);
    };

    auto* re = new pluggable_rule_engine{_instance_name, _context};

    re->add_operation("start", operation<const std::string&>{start});
    re->add_operation("stop", operation<const std::string&>{no_op});
    re->add_operation("rule_exists", operation<const std::string&, bool&>{rule_exists_wrapper});
    re->add_operation("list_rules", operation<std::vector<std::string>&>{list_rules});
    re->add_operation("exec_rule", operation<const std::string&, std::list<boost::any>&, irods::callback>{exec_rule_wrapper});
    re->add_operation("exec_rule_text", operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{not_supported});
    re->add_operation("exec_rule_expression", operation<const std::string&, msParamArray_t*, irods::callback>{not_supported});

    return re;
}

