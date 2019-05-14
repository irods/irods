#include "irods_plugin_context.hpp"
#include "irods_re_plugin.hpp"
#include "irods_server_properties.hpp"
#include "irods_state_table.h"
#include "rodsError.h"
#include "rodsErrorTable.h"
#include "rodsLog.h"

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <regex>
#include <iterator>
#include <algorithm>
#include <initializer_list>

namespace
{
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

    std::unordered_map<std::string, std::vector<pep_config>> pep_configs;
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
                        pep_configs[_instance_name].push_back({std::regex{regex}, code});
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

        return ERROR(SYS_CONFIG_FILE_ERR, "[passthrough] Bad rule engine plugin configuration.");
    }

    irods::error rule_exists(const std::string& _instance_name,
                             irods::default_re_ctx&,
                             const std::string& _rule_name,
                             bool& _exists)
    {
        for (const auto& pc : pep_configs[_instance_name]) {
            _exists = std::regex_search(_rule_name, pc.regex);

            if (_exists) {
                matched_code = pc.code;
                break;
            }
        }

        const char* msg = "[passthrough] In rule_exists. instance name = %s, rule = %s, exists = %s.";
        rodsLog(LOG_DEBUG, msg, _instance_name.c_str(), _rule_name.c_str(), (_exists ? "true" : "false"));

        return SUCCESS();
    }

    irods::error list_rules(irods::default_re_ctx&, std::vector<std::string>& _rules)
    {
        rodsLog(LOG_DEBUG, "[passthrough] In list_rules.");
        return SUCCESS();
    }

    irods::error exec_rule(const std::string& _instance_name,
                           irods::default_re_ctx&,
                           const std::string& _rule_name,
                           std::list<boost::any>& _rule_arguments,
                           irods::callback _effect_handler)
    {
        const char* msg = "[passthrough] In exec_rule. instance name = %s, rule = %s, returned '%s' to REPF.";
        const auto matched_code_string = std::to_string(matched_code);
        rodsLog(LOG_DEBUG, msg, _instance_name.c_str(), _rule_name.c_str(), matched_code_string.c_str());

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
    const auto no_op         = [](auto...) { return SUCCESS(); };
    const auto not_supported = [](auto...) { return CODE(SYS_NOT_SUPPORTED); };
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
    re->add_operation("exec_rule_expression", operation<const std::string&, msParamArray_t*, const std::string&, irods::callback>{not_supported});

    return re;
}

