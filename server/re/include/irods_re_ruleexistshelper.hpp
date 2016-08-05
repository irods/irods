#ifndef IRODS_RE_RULEEXISTSHELPER_HPP
#define IRODS_RE_RULEEXISTSHELPER_HPP

#include "irods_error.hpp"

#include <vector>
#include <string>

#include "boost/regex.hpp"

class RuleExistsHelper {
public:
    static RuleExistsHelper* Instance();
    void registerRuleRegex( const std::string& _regex );
    bool checkOperation( const std::string& _op_name );
    bool checkPrePep( const std::string& _ns, const std::string& _op_name );
    bool checkPostPep( const std::string& _ns, const std::string& _op_name );
    bool checkDynPeps( const std::string& _ns, const std::string& _op_name );
protected:
private:
    RuleExistsHelper(){};
    static RuleExistsHelper* _instance;
    std::vector<boost::regex> ruleRegexes;
};

#endif
