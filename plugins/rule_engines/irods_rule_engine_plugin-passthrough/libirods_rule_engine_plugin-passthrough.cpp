#include "irods_plugin_context.hpp"
#include "irods_re_plugin.hpp"
#include "irods_configuration_keywords.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_logger.hpp"
#include "irods_state_table.h"
#include "rodsError.h"

#include <string>
#include <functional>
#include <vector>
#include <regex>
#include <fstream>

#include "json.hpp"

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

namespace {

//
// Rule Engine Plugin
//

using log = irods::experimental::log;

struct pep_config
{
    std::regex regex;
    int code;
};

std::vector<pep_config> pep_configs;
int matched_code = 0;

template <typename ...Args>
using operation = std::function<irods::error(irods::default_re_ctx&, Args...)>;

irods::error start(irods::default_re_ctx&, const std::string& _instance_name)
{
    std::string config_path;

    if (auto error = irods::get_full_path_for_config_file("server_config.json", config_path);
        !error.ok())
    {
        std::string msg = "server configuration not found [path => ";
        msg += config_path;
        msg += ']';

        // clang-format off
        log::rule_engine::error({{"rep", "passthrough"},
                                 {"function", __func__},
                                 {"msg", msg}});
        // clang-format on

        return ERROR(SYS_CONFIG_FILE_ERR, msg.c_str());
    }

    // clang-format off
    log::rule_engine::trace({{"rep", "passthrough"},
                             {"function", __func__},
                             {"msg", "reading plugin configuration ..."}});
    // clang-format on

    nlohmann::json config;

    {
        std::ifstream config_file{config_path};
        config_file >> config;
    }

    try {
        // Iterate over the list of rule engine plugins until the Passthrough REP is found.
        for (const auto& re : config.at(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_RULE_ENGINE)) {
            if (_instance_name == re.at(irods::CFG_INSTANCE_NAME_KW).get<std::string>()) {
                // Fill the "pep_configs" plugin variable with objects containing the values
                // defined in the "return_codes_for_peps" configuration. Each object in the list
                // will contain a regular expression and a code.
                for (const auto& e : re.at(irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW).at("return_codes_for_peps")) {
                    pep_configs.push_back({std::regex{e.at("regex").get<std::string>()}, e.at("code").get<int>()});
                }

                return SUCCESS();
            }
        }
    }
    catch (const std::exception& e) {
        // clang-format off
        log::rule_engine::error({{"rep", "passthrough"},
                                 {"function", __func__},
                                 {"msg", e.what()}});
        // clang-format on

        return ERROR(SYS_CONFIG_FILE_ERR, e.what());
    }

    return ERROR(SYS_CONFIG_FILE_ERR, "[passthrough] bad rule engine plugin configuration");
}

irods::error rule_exists(irods::default_re_ctx&, const std::string& _rule_name, bool& _exists)
{
    for (const auto& pc : pep_configs) {
        if (_exists = std::regex_search(_rule_name, pc.regex); _exists) {
            matched_code = pc.code;
            break;
        }
    }

    // clang-format off
    log::rule_engine::trace({{"rep", "passthrough"},
                             {"function", __func__},
                             {"rule", _rule_name},
                             {"exists", _exists ? "true" : "false"}});
    // clang-format on

    return SUCCESS();
}

irods::error list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules)
{
    log::rule_engine::trace({{"rep", "passthrough"}, {"function", __func__}});
    return SUCCESS();
}

irods::error exec_rule(irods::default_re_ctx&,
                       const std::string& _rule_name,
                       std::list<boost::any>& _rule_arguments,
                       irods::callback _effect_handler)
{
    // clang-format off
    log::rule_engine::trace({{"rep", "passthrough"},
                             {"function", __func__},
                             {"rule", _rule_name},
                             {"msg", "returned 'RULE_ENGINE_CONTINUE' to REPF."}});
    // clang-format on

    return ERROR(matched_code, "");
}

} // namespace (anonymous)

//
// Plugin Factory
//

extern "C"
pluggable_rule_engine* plugin_factory(const std::string& _instance_name,
                                      const std::string& _context)
{
    // clang-format off
    const auto no_op         = [] { return SUCCESS(); };
    const auto not_supported = [] { return ERROR(SYS_NOT_SUPPORTED, "not supported"); };
    // clang-format on

    auto* re = new pluggable_rule_engine{_instance_name, _context};

    re->add_operation("start", operation<const std::string&>{start});
    re->add_operation("stop", {no_op});
    re->add_operation("rule_exists", operation<const std::string&, bool&>{rule_exists});
    re->add_operation("list_rules", operation<std::vector<std::string>&>{list_rules});
    re->add_operation("exec_rule", operation<const std::string&, std::list<boost::any>&, irods::callback>{exec_rule});
    re->add_operation("exec_rule_text", {not_supported});
    re->add_operation("exec_rule_expression", {not_supported});

    return re;
}

