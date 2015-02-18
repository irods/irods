// =-=-=-=-=-=-=-
// irods includes
#include "msParam.hpp"
#include "reGlobalsExtern.hpp"
#include "generalAdmin.hpp"
#include "miscServerFunct.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_re_plugin.hpp"

// =-=-=-=-=-=-=-
// stl includes
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>

#include "configuration.hpp"
#include "irods_server_properties.hpp"
#include "rules.hpp"
#include "reFuncDefs.hpp"
#include "region.hpp"

    int
    applyRuleArg331( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, ruleExecInfo_t *rei, int reiSaveFlag );

extern "C" {
    irods::error start(irods::default_re_ctx& _u) {
        (void) _u;
        return SUCCESS();
    }
    
    irods::error stop(irods::default_re_ctx& _u) {
        (void) _u;
        return SUCCESS();
    }
    irods::error rule_exists(irods::default_re_ctx&, std::string _rn, bool& _ret) {
        if(ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
            rodsLog(
                LOG_NOTICE,
                "rule engine xre has not initialized, skip rule"
            );
            _ret = false;
            return SUCCESS();
        }
        _ret = lookupFromEnv(ruleEngineConfig.extFuncDescIndex, const_cast<char*>(_rn.c_str())) != NULL;
        rodsLog(
            LOG_DEBUG,
            "looking up rule name %s, found = %d",
            _rn.c_str(),
            _ret
        );
        return SUCCESS();
    }
    
    irods::error exec_rule(irods::default_re_ctx&, std::string _rn, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {

        if(ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
            rodsLog(
                LOG_NOTICE,
                "rule engine xre has not initialized, skip rule"
            );
            return SUCCESS();
        }
        
        rodsLog(
                LOG_NOTICE,
                "applying rule %s", _rn.c_str()
            );
        ruleExecInfo_t * rei;
        irods::error err;
        if(!(err = _eff_hdlr("unsafe_ms_ctx", &rei)).ok()) {
            return err;
        }

        struct all_resources{
            all_resources() {
                memset(&msParamArray, 0, sizeof(msParamArray_t));
        
            }
            ~all_resources() {
                clearMsParamArray(&msParamArray, 1);
            }
            msParamArray_t msParamArray;
        
        }ar;
        
        std::stringstream expr;
        expr << _rn << "(";
        int i = 0;
        for ( auto itr = begin(_ps);itr!=end(_ps);++itr ) {
            char arg[10];
            snprintf(arg, 10, "*ARG%d", i);
            if(i!=0) expr << ",";
            expr << arg;
            if(itr->type() == typeid(std::string)) {
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup(boost::any_cast<std::string>(*itr).c_str()), NULL);
            } else if(itr->type() == typeid(std::string*)) {
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup((*boost::any_cast<std::string*>(*itr)).c_str()), NULL);
            } else {
                rodsLog(LOG_ERROR, "only string arguments are supported for calling xre rules from outside");
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup("<unconvertible>"), NULL);
            }
            i++;
        }
        expr << ")";
        
        rodsLog(
                LOG_NOTICE,
                "applying rule %s", _rn.c_str()
            );
        int ret = applyRuleUpdateParams(const_cast<char *>(expr.str().c_str()), &(ar.msParamArray), rei, 0);
        
        i = 0;
        for ( auto itr = begin(_ps);itr!=end(_ps);++itr ) {
            char arg[10];
            snprintf(arg, 10, "*ARG%d", i);
            if(itr->type() == typeid(std::string)) {
            } else if(itr->type() == typeid(std::string*)) {
                msParam_t *msParam = getMsParamByLabel(&(ar.msParamArray), arg);
                if(msParam != NULL) {
                    *boost::any_cast<std::string*>(*itr) = reinterpret_cast<char *>(msParam->inOutStruct);
                } else {
                    rodsLog(LOG_ERROR, "no output parameter");
                }
                
            } else {
                rodsLog(LOG_ERROR, "only string arguments are supported for returning from xre rules to outside");
            }
            i++;
        }

        rmMsParamByLabel(&(ar.msParamArray), "ruleExecOut", 0);
        
        rodsLog(
                LOG_NOTICE,
                "rule engine return %d", ret
            );

        return ret == 0 ? SUCCESS() : CODE(ret);
    
        

    }
    
    irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory( const std::string& _inst_name,
                                     const std::string& _context ) {
        irods::pluggable_rule_engine<irods::default_re_ctx>* re = new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
        return re;

    }

}; // extern "C"



