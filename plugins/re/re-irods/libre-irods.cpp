// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.hpp"
#include "generalAdmin.h"
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
#include "region.h"

    int
    applyRuleArg331( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, ruleExecInfo_t *rei, int reiSaveFlag );

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
            LOG_DEBUG,
            "rule engine xre has not initialized, skip rule [%s]",
            _rn.c_str()
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
            LOG_DEBUG,
            "rule engine xre has not initialized, skip rule [%s]",
            _rn.c_str());
        return SUCCESS();
    }

    rodsLog(
        LOG_DEBUG,
        "applying rule %s",
        _rn.c_str());
        
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

    if(_rn.compare(0,4,"pep_") == 0 ) {
        // adapter for 4.0.3 dynamic pep
        // because it only accepts one string param
        // but all operation parameters are passed here
        std::stringstream expr;
        boost::any* arg0 = nullptr, *arg1 = nullptr;
        std::string rule_ctx;
        int i = 0;
        for ( auto itr = begin(_ps);itr!=end(_ps);++itr ) {
            switch(i) {
                case 0:
                    if(itr->type() == typeid(std::string)) {
                        arg0 = &*itr;
                    } else {
                        rodsLog(LOG_ERROR, "only string arguments are supported for instance_name");
                        return ERROR(-1, "only string arguments are supported for instance_name");
                    }
                    break;
                case 1:
                    if(itr->type() == typeid(irods::plugin_context) ) {
                        arg1 = &*itr;
                    } else {
                        rodsLog(
                            LOG_ERROR,
                            "only plugin_context arguments are supported for rule_param [%s]",
                            itr->type().name() );
                        return ERROR(-1, "only plugin_context arguments are supported for rule_param");
                    }
                    break;
            }
            ++i;
        }

        if( !arg1 ) {
            rodsLog(
                LOG_ERROR,
                "%s:%d - did not find plugin_context in parameter list",
                __FUNCTION__,
               __LINE__ );
            return ERROR(
                    SYS_INVALID_INPUT_PARAM,
                    "did not find plugin_context in parameter list" );
        }

        std::string instance_name = boost::any_cast<std::string>(*arg0).c_str();
        irods::plugin_context plugin_ctx = boost::any_cast<irods::plugin_context>(*arg1);
        addMsParam(&(ar.msParamArray), "*OUT", STR_MS_T, (void *) plugin_ctx.rule_results().c_str(), NULL);

        expr << _rn << "(*OUT)";
        snprintf(rei->pluginInstanceName, MAX_NAME_LEN, "%s", instance_name.c_str());
        
        int ret = applyRuleUpdateParams(const_cast<char *>(expr.str().c_str()), &(ar.msParamArray), rei, 0);

        msParam_t *msParam = getMsParamByLabel(&(ar.msParamArray), "*OUT");
        if(msParam != NULL) {
            plugin_ctx.rule_results(reinterpret_cast<char *>(msParam->inOutStruct)); // currently won't work because boost::any doesn't store reference, use std::ref to make this work
        } else {
            rodsLog(LOG_ERROR, "no output parameter");
        }

        rmMsParamByLabel(&(ar.msParamArray), "ruleExecOut", 0);

        rodsLog(
                LOG_DEBUG,
                "rule engine return %d", ret
            );

        return ret == 0 ? SUCCESS() : ERROR(ret,"applyRuleUpdateParams failed");

    } 
    else {
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
                rodsLog(LOG_ERROR, "only string arguments are supported for calling re rules from outside");
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup("<unconvertible>"), NULL);
            }
            i++;
        }
        expr << ")";

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
                LOG_DEBUG,
                "rule engine return %d", ret
            );

        return ret == 0 ? SUCCESS() : ERROR(ret,"applyRuleUpdateParams failed");
    }



}

extern "C"
irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory( const std::string& _inst_name,
                                 const std::string& _context ) {
    irods::pluggable_rule_engine<irods::default_re_ctx>* re = new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
    re->add_operation<irods::default_re_ctx&>(
            "start",
            std::function<irods::error(irods::default_re_ctx&)>( start ) );

    re->add_operation<irods::default_re_ctx&>(
            "stop",
            std::function<irods::error(irods::default_re_ctx&)>( stop ) );

    re->add_operation<irods::default_re_ctx&, std::string, bool&>(
            "rule_exists",
            std::function<irods::error(irods::default_re_ctx&, std::string, bool&)>( rule_exists ) );
            
    re->add_operation<irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback>(
            "exec_rule",
            std::function<irods::error(irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback)>( exec_rule ) );

    return re;

}

