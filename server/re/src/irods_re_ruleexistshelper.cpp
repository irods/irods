#include "irods_re_ruleexistshelper.hpp"
#include "irods_log.hpp"

RuleExistsHelper* RuleExistsHelper::_instance = 0;

RuleExistsHelper* RuleExistsHelper::Instance() {
    if (!_instance) {
        _instance = new RuleExistsHelper;
    }

    return _instance;
}

void RuleExistsHelper::registerRuleRegex( const std::string& _regex ) {
    boost::regex expr(_regex);
    ruleRegexes.push_back(expr);
}

bool RuleExistsHelper::checkOperation( const std::string& _op_name ) {
    for (auto& expr : ruleRegexes) {
        if (boost::regex_match(_op_name, expr)) {
            return true;
        }
    }

    return false;
}

bool RuleExistsHelper::checkPrePep( const std::string& _ns, const std::string& _op_name ) {
    const std::string& preString = _ns + "pep_" + _op_name + "_pre";

    return checkOperation(preString);
}

bool RuleExistsHelper::checkPostPep( const std::string& _ns, const std::string& _op_name ) {
    const std::string& postString = _ns + "pep_" + _op_name + "_post";

    return checkOperation(postString);
}

bool RuleExistsHelper::checkDynPeps( const std::string& _ns, const std::string& _op_name ) {
    return checkPrePep(_ns, _op_name) || checkPostPep(_ns, _op_name); 
}
