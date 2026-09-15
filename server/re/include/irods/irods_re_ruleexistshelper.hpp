#ifndef IRODS_RE_RULEEXISTSHELPER_HPP
#define IRODS_RE_RULEEXISTSHELPER_HPP

#include <boost/regex.hpp>

#include <string>
#include <vector>

class RuleExistsHelper
{
  public:
    static RuleExistsHelper* Instance();

    void registerRuleRegex(const std::string& _regex);
    bool checkOperation(const std::string& _op_name);
    bool checkPrePep(const std::string& _ns, const std::string& _op_name);
    bool checkPostPep(const std::string& _ns, const std::string& _op_name);
    bool checkDynPeps(const std::string& _ns, const std::string& _op_name);

  private:
    RuleExistsHelper() = default;
    std::vector<boost::regex> ruleRegexes;
}; // class RuleExistsHelper

#endif // IRODS_RE_RULEEXISTSHELPER_HPP
