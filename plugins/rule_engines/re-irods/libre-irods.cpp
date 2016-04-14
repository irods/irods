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
#include "irods_configuration_parser.hpp"
#include "irods_server_properties.hpp"
#include "irods_ms_plugin.hpp"

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
#include "rules.hpp"
#include "reFuncDefs.hpp"
#include "region.h"

#include "msiHelper.hpp"

// =-=-=-=-=-=-=-
// from reGlobals.hpp
ruleStruct_t coreRuleStrct;
rulevardef_t coreRuleVarDef;
rulefmapdef_t coreRuleFuncMapDef;
ruleStruct_t appRuleStrct;
rulevardef_t appRuleVarDef;
rulefmapdef_t appRuleFuncMapDef;
msrvcStruct_t coreMsrvcStruct;
msrvcStruct_t appMsrvcStruct;
int reTestFlag = 0;
int reLoopBackFlag = 0;
int GlobalAllRuleExecFlag = 0;
int GlobalREDebugFlag = 0;
int GlobalREAuditFlag = 0;
char *reDebugStackFull[REDEBUG_STACK_SIZE_FULL];
struct reDebugStack reDebugStackCurr[REDEBUG_STACK_SIZE_CURR];
int reDebugStackFullPtr = 0;
int reDebugStackCurrPtr = 0;

static std::string local_instance_name;








int initRuleEngine( const char*, int, rsComm_t*, const char*, const char*, const char*);

typedef irods::configuration_parser::object_t object_t;
typedef irods::configuration_parser::array_t  array_t;


static irods::error get_string_array_from_array( 
    boost::any        _array,
    const std::string _instance_name,
    std::string&      _str_array ) {
    array_t arr;
    try {
        arr = boost::any_cast<array_t>( _array );
    } catch( const boost::bad_any_cast& ) {
        std::stringstream msg;
        msg << "[" << _instance_name << "] failed to any_cast an array_t";
        return ERROR(
                   INVALID_ANY_CAST,
                   msg.str() );
    }

    for( auto itr : arr ) {
        try {
            _str_array += boost::any_cast< std::string >(
                            itr[ irods::CFG_FILENAME_KW ] );
        }
        catch ( boost::bad_any_cast& _e ) {
            rodsLog(
                LOG_ERROR,
                "%s - failed to cast rule base file name entry to string",
                __FUNCTION__);
            continue;
        }
        _str_array += ",";

    } // for itr
        
    _str_array = _str_array.substr( 0, _str_array.size() - 1 );

    return SUCCESS();

} // get_string_array_from_array

irods::ms_table& get_microservice_table();
void initialize_microservice_table() {
    irods::ms_table& table = get_microservice_table();

    table[ "msiGetStdoutInExecCmdOut" ] = new irods::ms_table_entry( "msiGetStdoutInExecCmdOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetStdoutInExecCmdOut ) );
    table[ "msiGetStderrInExecCmdOut" ] = new irods::ms_table_entry( "msiGetStderrInExecCmdOut", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetStderrInExecCmdOut ) );
    table[ "msiAddKeyValToMspStr" ] = new irods::ms_table_entry( "msiAddKeyValToMspStr", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiAddKeyValToMspStr ) );
    table[ "msiWriteRodsLog" ] = new irods::ms_table_entry( "msiWriteRodsLog", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiWriteRodsLog ) );
    table[ "msiSplitPath" ] = new irods::ms_table_entry( "msiSplitPath", 3, std::function<int(msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSplitPath ) );
    table[ "msiSplitPathByKey" ] = new irods::ms_table_entry( "msiSplitPathByKey", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSplitPathByKey ) );
    table[ "msiGetSessionVarValue" ] = new irods::ms_table_entry( "msiGetSessionVarValue", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiGetSessionVarValue ) );
    table[ "msiStrlen" ] = new irods::ms_table_entry( "msiStrlen", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrlen ) );
    table[ "msiStrCat" ] = new irods::ms_table_entry( "msiStrCat", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrCat ) );
    table[ "msiStrchop" ] = new irods::ms_table_entry( "msiStrchop", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiStrchop ) );
    table[ "msiSubstr" ] = new irods::ms_table_entry( "msiSubstr", 4, std::function<int(msParam_t*,msParam_t*,msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiSubstr ) );
    table[ "msiExit" ] = new irods::ms_table_entry( "msiExit", 2, std::function<int(msParam_t*,msParam_t*,ruleExecInfo_t*)>( msiExit ) );

}

extern Cache ruleEngineConfig;

irods::error start(irods::default_re_ctx&,const std::string& _instance_name ) {
    local_instance_name = _instance_name;

    array_t re_plugin_arr;
    irods::error ret = irods::get_server_property<
          array_t > (
              irods::CFG_RULE_ENGINES_KW,
              re_plugin_arr );
    if(!ret.ok()) {
        return PASS(ret);
    }

    bool found_instance = false;
    object_t plugin_config;
    for( auto itr : re_plugin_arr ) {
        try {
            plugin_config = boost::any_cast<object_t>( itr );
        } catch( const boost::bad_any_cast& ) {
            std::stringstream msg;
            msg << "[" << _instance_name << "] failed to any_cast a rule_engines object";
            return ERROR(
                       INVALID_ANY_CAST,
                       msg.str() );
        }

        try {
            const std::string inst_name = boost::any_cast<std::string>(plugin_config[irods::CFG_INSTANCE_NAME_KW]);
            if( inst_name == _instance_name) {
                found_instance = true;
                break;
            }
        }
        catch( const boost::bad_any_cast& ) {
            continue;
        }
    }

    if( !found_instance ) {
        std::stringstream msg;
        msg << "failed to find configuration for re-irods plugin ["
            << _instance_name << "]";
        rodsLog( LOG_ERROR, "%s", msg.str().c_str() );
        return ERROR(
                SYS_INVALID_INPUT_PARAM,
                msg.str() );
    }

    std::string shmem_value;
    try {
        shmem_value = boost::any_cast<std::string>( plugin_config[irods::CFG_SHARED_MEMORY_INSTANCE_KW] );
    } catch( const boost::bad_any_cast& ) {
        std::stringstream msg;
        msg << "[" << _instance_name << "] failed to any_cast " << irods::CFG_SHARED_MEMORY_INSTANCE_KW;
        return ERROR(
                   INVALID_ANY_CAST,
                   msg.str() );
    }

    object_t plugin_spec_cfg;
    try {
        plugin_spec_cfg = boost::any_cast<object_t>( plugin_config[irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW] );
    } catch( const boost::bad_any_cast& ) {
        std::stringstream msg;
        msg << "[" << _instance_name << "] failed to any_cast " << irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW;
        return ERROR(
                   INVALID_ANY_CAST,
                   msg.str() );
    }

    std::string core_re;
    ret = get_string_array_from_array(
              plugin_spec_cfg[irods::CFG_RE_RULEBASE_SET_KW],
              _instance_name,
              core_re );
    if(!ret.ok()) {
        return PASS(ret);
    }
 
    std::string core_fnm;
    ret = get_string_array_from_array(
              plugin_spec_cfg[irods::CFG_RE_FUNCTION_NAME_MAPPING_SET_KW],
              _instance_name,
              core_fnm );
    if(!ret.ok()) {
        return PASS(ret);
    }
  
    std::string core_dvm;
    ret = get_string_array_from_array(
              plugin_spec_cfg[irods::CFG_RE_DATA_VARIABLE_MAPPING_SET_KW],
              _instance_name,
              core_dvm );
    if(!ret.ok()) {
        return PASS(ret);
    }
    int status = initRuleEngine(
                     shmem_value.c_str(),
                     RULE_ENGINE_TRY_CACHE,
                     nullptr,
                     core_re.c_str(),
                     core_dvm.c_str(),
                     core_fnm.c_str() );
    if( status < 0 ) {
        return ERROR(
                   status,
                   "failed to initialize native rule engine" );
    }

    // index locally defined microservices
    initialize_microservice_table();

    return SUCCESS();
}

int finalizeRuleEngine();
irods::error stop(irods::default_re_ctx& _u, const std::string& _instance_name) {
    (void) _u;
    finalizeRuleEngine();
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
    std::string out_desc = *boost::any_cast<std::string*>(*itr);

    const int status = execMyRule(
                     (char*)_rt.c_str(),
                     ms_params,
                     const_cast<char*>(out_desc.data()),
                     rei );

    if (status != 0) {
        return ERROR(status, "execMyRule failed");
    }
    return SUCCESS();
}

irods::error exec_rule_expression(
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
    std::string out_desc = *boost::any_cast<std::string*>(*itr);

    std::string rule_text = "{" + _rt + "}";

    char res[MAX_COND_LEN];
    int status = computeExpression(
                     (char*)rule_text.c_str(),
                     ms_params,
                     rei,
                     NO_SAVE_REI,
                     res );
    if( status < 0 ) {
        return ERROR(
                 status,
                 rule_text );
    }
    else {
        return SUCCESS();
    }
}


extern "C"
irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory( const std::string& _inst_name,
                                 const std::string& _context ) {
    irods::pluggable_rule_engine<irods::default_re_ctx>* re = new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
    re->add_operation<irods::default_re_ctx&,const std::string&>(
            "start",
            std::function<irods::error(irods::default_re_ctx&,const std::string&)>( start ) );

    re->add_operation<irods::default_re_ctx&,const std::string&>(
            "stop",
            std::function<irods::error(irods::default_re_ctx&,const std::string&)>( stop ) );

    re->add_operation<irods::default_re_ctx&, std::string, bool&>(
            "rule_exists",
            std::function<irods::error(irods::default_re_ctx&, std::string, bool&)>( rule_exists ) );

    re->add_operation<irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback>(
            "exec_rule",
            std::function<irods::error(irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback)>( exec_rule ) );

    re->add_operation<irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback>(
            "exec_rule_text",
            std::function<irods::error(irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback)>( exec_rule_text ) );
    re->add_operation<irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback>(
            "exec_rule_expression",
            std::function<irods::error(irods::default_re_ctx&,std::string,std::list<boost::any>&,irods::callback)>( exec_rule_expression ) );

    return re;

}
