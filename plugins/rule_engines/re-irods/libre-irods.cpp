// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
#include "reGlobalsExtern.hpp"
#include "generalAdmin.h"
#include "miscServerFunct.hpp"
#include "execMyRule.h" 

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
#include "irods_re_serialization.hpp"

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

//int applyRuleArg331( const char *action, const char *args[MAX_NUM_OF_ARGS_IN_ACTION], int argc, ruleExecInfo_t *rei, int reiSaveFlag );

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
        _ret );
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
        "applying rule %s, params %ld",
        _rn.c_str(),
        _ps.size());
        
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

    } ar;

    std::stringstream expr;
    expr << _rn << "(";
    int i = 0;

    for ( auto itr = begin(_ps);itr!=end(_ps);++itr ) {
        char arg[10];
        snprintf(arg, 10, "*ARG%d", i);
        if(i!=0) expr << ",";
        expr << arg;

        // serialize to the map then bind to a ms param
        irods::re_serialization::serialized_parameter_t param;
        irods::error ret = irods::re_serialization::serialize_parameter(*itr,param);

        if(!ret.ok()) {
             rodsLog(LOG_ERROR, "unsupported argument for calling re rules from the rule language");
             addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup("<unconvertible>"), NULL);
        }
        else {
            if( 0 == param.size() ) {
                rodsLog( LOG_DEBUG, "empty serialized map for parameter %s", arg );
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup("<unconvertible>"), NULL);
            }
            else if( 1 == param.size() ) {
                // only one key-value in them map, bind it as a string
                addMsParam(&(ar.msParamArray), strdup(arg), STR_MS_T, strdup(param.begin()->second.c_str()), NULL);
            }
            else {
                keyValPair_t* kvp = (keyValPair_t*)malloc(sizeof(keyValPair_t));
                memset( kvp, 0, sizeof( keyValPair_t ) );
                for( auto i : param ) {
                    addKeyVal( kvp, i.first.c_str(), i.second.c_str() );
                }
                addMsParam(&(ar.msParamArray), strdup(arg), KeyValPair_MS_T, kvp, NULL );
            }
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
            rodsLog(LOG_DEBUG, "only string arguments are supported for returning from xre rules to the calling rule");
        }
        i++;
    }

    rmMsParamByLabel(&(ar.msParamArray), "ruleExecOut", 0);

    rodsLog(
        LOG_DEBUG,
        "rule engine return %d", ret);

    // clear client-side errors for dynamic PEPs as we expect failures
    // from the pre-PEPs to control access to operations
    if( _rn.substr(0,4) == "pep_" ) {
        if( rei && rei->rsComm ) {
            freeRErrorContent( &rei->rsComm->rError );
        }
    }

    return ret == 0 ? SUCCESS() : ERROR(ret,"applyRuleUpdateParams failed");

}

irods::error exec_rule_text(
    irods::default_re_ctx&,
    std::string            _rt,
    std::list<boost::any>& _ps,
    irods::callback        _eff_hdlr) {

    if(ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
        rodsLog(
            LOG_DEBUG,
            "rule engine xre has not initialized, skip rule [%s]",
            _rt.c_str());
        return SUCCESS();
    }

    rodsLog(
        LOG_DEBUG,
        "applying rule %s, params %ld",
        _rt.c_str(),
        _ps.size());

    ruleExecInfo_t* rei = nullptr;
    irods::error err;
    if(!(err = _eff_hdlr("unsafe_ms_ctx", &rei)).ok()) {
        return err;
    }

    auto itr = begin(_ps);
    ++itr; // skip tuple
    ++itr; // skip callback
    msParamArray_t* ms_params = boost::any_cast<msParamArray_t*>(*itr);

    ++itr; // skip msparam
    std::string out_desc = boost::any_cast<std::string>(*itr);

    int status = execMyRule(
                     (char*)_rt.c_str(),
                     ms_params,
                     (char*)out_desc.c_str(),
                     rei );
    return SUCCESS();
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
 
    re->add_operation<irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback>(
            "exec_rule_text",
            std::function<irods::error(irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback)>( exec_rule_text ) );


    return re;

}

