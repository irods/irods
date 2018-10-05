#include "irods_plugin_context.hpp"
#include "irods_re_plugin.hpp"
#include "irods_server_properties.hpp"
#include "irods_state_table.h"
#include "rodsError.h"
#include "rodsLog.h"

#include <string>
#include <functional>
#include <vector>
#include <regex>

using pluggable_rule_engine = irods::pluggable_rule_engine<irods::default_re_ctx>;

namespace {

//
// Rule Engine Plugin
//

template <typename ...Args>
using operation = std::function<irods::error(irods::default_re_ctx&, Args...)>;

struct pep_config
{
    std::regex regex;
    int code;
};

std::vector<pep_config> pep_configs;
int matched_code = 0;

irods::error start(irods::default_re_ctx&, const std::string& _instance_name)
{
    try {
        const auto& rule_engines = irods::get_server_property<const std::vector<boost::any>&>(
            std::vector<std::string>{irods::CFG_PLUGIN_CONFIGURATION_KW, irods::PLUGIN_TYPE_RULE_ENGINE});

        for (const auto& element : rule_engines) {
            const auto& rule_engine = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(element);
            const auto& instance_name = boost::any_cast<const std::string&>(rule_engine.at(irods::CFG_INSTANCE_NAME_KW));

            if (instance_name == _instance_name) {
                const auto& plugin_spec_cfg = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(
                    rule_engine.at(irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW));

                for (const auto& e : boost::any_cast<const std::vector<boost::any>&>(plugin_spec_cfg.at("return_codes_for_peps"))) {
                    const auto& config = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(e);
                    const auto& regex = boost::any_cast<const std::string&>(config.at("regex"));
                    const auto code = boost::any_cast<int>(config.at("code"));
                    pep_configs.push_back({std::regex{regex}, code});
                }

                return SUCCESS();
            }
        }
    }
    catch (const boost::bad_any_cast& e) {
        return ERROR(INVALID_ANY_CAST, e.what());
    }
    catch (const std::exception& e) {
        return ERROR(SYS_CONFIG_FILE_ERR, e.what());
    }

    return ERROR(SYS_CONFIG_FILE_ERR, "[passthrough] bad rule engine plugin configuration.");
}

irods::error rule_exists(irods::default_re_ctx&, const std::string& _rule_name, bool& _exists)
{
    for (const auto& pc : pep_configs) {
        _exists = std::regex_search(_rule_name, pc.regex);

        if (_exists) {
            matched_code = pc.code;
            break;
        }
    }

    const char* msg = "[passthrough] in rule_exists. rule = %s, exists = %s.";
    rodsLog(LOG_DEBUG, msg, _rule_name.c_str(), (_exists ? "true" : "false"));

    return SUCCESS();
}

irods::error list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules)
{
    rodsLog(LOG_DEBUG, "[passthrough] in list_rules.");
    return SUCCESS();
}

irods::error exec_rule(irods::default_re_ctx&,
                       const std::string& _rule_name,
                       std::list<boost::any>& _rule_arguments,
                       irods::callback _effect_handler)
{
    const char* msg = "[passthrough] in exec_rule. rule = %s, returned 'RULE_ENGINE_CONTINUE' to REPF.";
    rodsLog(LOG_DEBUG, msg, _rule_name.c_str());
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

