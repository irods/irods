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
#include "irods_re_ruleexistshelper.hpp"
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

const std::string STATIC_PEP_RULE_REGEX = "ac[^ ]*";
const std::string DYNAMIC_PEP_RULE_REGEX = "[^ ]*pep_[^ ]*_(pre|post)";
const std::string MICROSERVICE_RULE_REGEX = "msi[^ ]*";
static std::string local_instance_name;

int initRuleEngine( const char*, rsComm_t*, const char*, const char*, const char *);

static std::string get_string_array_from_array( const nlohmann::json& _array ) {
    std::string str_array;
    try {
        for(const auto& el : _array) {
            try {
                str_array += el.get<std::string>();
            }
            catch ( const boost::bad_any_cast& ) {
                rodsLog(LOG_ERROR, "%s - failed to cast rule base file name entry to string", __PRETTY_FUNCTION__);
                continue;
            }
            str_array += ",";

        } // for itr
        str_array = str_array.substr( 0, str_array.size() - 1 );
        return str_array;
    } catch( const boost::bad_any_cast& ) {
        THROW(INVALID_ANY_CAST, "failed to any_cast to vector");
    }

} // get_string_array_from_array

void register_regexes_from_array(
    const boost::any& _array,
    const std::string& _instance_name ) {
    try {
        const auto& arr = boost::any_cast<const std::vector<boost::any>&>( _array );
        for ( const auto& elem : arr ) {
            try {
                const auto& tmp = boost::any_cast<const std::string&>(elem);
                RuleExistsHelper::Instance()->registerRuleRegex( tmp );
            } catch ( boost::bad_any_cast& ) {
                rodsLog(
                        LOG_ERROR,
                        "%s - failed to cast regex to string",
                        __FUNCTION__);
                continue;
            }
        }
    } catch ( const boost::bad_any_cast& ) {
        std::stringstream msg;
        msg << "[" << _instance_name << "] failed to any_cast a std::vector<boost::any>&";
        THROW(INVALID_ANY_CAST, msg.str());
    }
} // register_regexes_from_array

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

    try {
        const auto re_plugin_arr = irods::get_server_property<nlohmann::json>(irods::CFG_PLUGIN_CONFIGURATION_KW).at(irods::PLUGIN_TYPE_RULE_ENGINE);
        for( const auto& plugin_config : re_plugin_arr ) {
            const auto& inst_name = plugin_config.at(irods::CFG_INSTANCE_NAME_KW).get<std::string>();
            if( inst_name == _instance_name) {
                const auto shmem_value = plugin_config.at(irods::CFG_SHARED_MEMORY_INSTANCE_KW).get<std::string>();
                const auto plugin_spec_cfg = plugin_config.at(irods::CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW);
                std::string core_re  = get_string_array_from_array(plugin_spec_cfg.at(irods::CFG_RE_RULEBASE_SET_KW));
                std::string core_fnm = get_string_array_from_array(plugin_spec_cfg.at(irods::CFG_RE_FUNCTION_NAME_MAPPING_SET_KW));
                std::string core_dvm = get_string_array_from_array(plugin_spec_cfg.at(irods::CFG_RE_DATA_VARIABLE_MAPPING_SET_KW));
                int status = initRuleEngine(
                        shmem_value.c_str(),
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

                if (plugin_spec_cfg.count(irods::CFG_RE_PEP_REGEX_SET_KW) > 0) {
                    register_regexes_from_array(
                            plugin_spec_cfg.at(irods::CFG_RE_PEP_REGEX_SET_KW),
                            _instance_name );
                } else {
                    RuleExistsHelper::Instance()->registerRuleRegex( STATIC_PEP_RULE_REGEX );
                    RuleExistsHelper::Instance()->registerRuleRegex( DYNAMIC_PEP_RULE_REGEX );
                    RuleExistsHelper::Instance()->registerRuleRegex( MICROSERVICE_RULE_REGEX );
                    rodsLog(
                            LOG_DEBUG,
                            "No regexes found in server config - using default regexes: [%s], [%s], [%s]",
                            STATIC_PEP_RULE_REGEX.c_str(),
                            DYNAMIC_PEP_RULE_REGEX.c_str(),
                            MICROSERVICE_RULE_REGEX.c_str() );
                }

                return SUCCESS();
            }
        }
    } catch (const irods::exception& e) {
        return irods::error(e);
    } catch (const boost::bad_any_cast& e) {
        return ERROR(INVALID_ANY_CAST, e.what());
    } catch (const std::out_of_range& e) {
        return ERROR(KEY_NOT_FOUND, e.what());
    }

    std::stringstream msg;
    msg << "failed to find configuration for re-irods plugin ["
        << _instance_name << "]";
    rodsLog( LOG_ERROR, "%s", msg.str().c_str() );
    return ERROR(
            SYS_INVALID_INPUT_PARAM,
            msg.str() );

}

irods::error stop(irods::default_re_ctx& _u, const std::string& _instance_name) {
    return SUCCESS();
}

irods::error rule_exists(irods::default_re_ctx&, const std::string& _rn, bool& _ret) {
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
        LOG_DEBUG9,
        "looking up rule name %s, found = %d",
        _rn.c_str(),
        _ret );

    return SUCCESS();
}

irods::error list_rules( irods::default_re_ctx&, std::vector<std::string>& rule_vec ) {
    for ( int i = 0; i < ruleEngineConfig.coreRuleSet->len; ++i ) {
       rule_vec.push_back( ruleEngineConfig.coreRuleSet->rules[i]->node->subtrees[0]->text );
    }
    return SUCCESS();
}

irods::error exec_rule(irods::default_re_ctx&, const std::string& _rn, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {
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

    ruleExecInfo_t* rei;
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
             addMsParam(&(ar.msParamArray), arg, STR_MS_T, (void *) "<unconvertible>", NULL);
        }
        else {
            if( 0 == param.size() ) {
                rodsLog( LOG_DEBUG, "empty serialized map for parameter %s", arg );
                addMsParam(&(ar.msParamArray), arg, STR_MS_T, (void *) "<unconvertible>", NULL);
            }
            else if( 1 == param.size() ) {
                // only one key-value in them map, bind it as a string
                addMsParam(&(ar.msParamArray), arg, STR_MS_T, (void *) param.begin()->second.c_str(), NULL);
            }
            else {
                keyValPair_t* kvp = (keyValPair_t*)malloc(sizeof(keyValPair_t));
                memset( kvp, 0, sizeof( keyValPair_t ) );
                for( auto i : param ) {
                    addKeyVal( kvp, i.first.c_str(), i.second.c_str() );
                }
                addMsParam(&(ar.msParamArray), arg, KeyValPair_MS_T, kvp, NULL );
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

    if (ret < 0) {
        std::stringstream msg;
        msg << "applyRuleUpdateParams failed for rule " << _rn;
        rodsLog(LOG_DEBUG, "%s", msg.str().c_str() );
        return ERROR(ret, msg.str());
    }

    return CODE(ret);
}

irods::error exec_rule_text(
    irods::default_re_ctx&,
    const std::string&     _rt,
    msParamArray_t*        _ms_params,
    const std::string&     _out_desc,
    irods::callback        _eff_hdlr) {

    if(ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
        rodsLog(
            LOG_DEBUG,
            "rule engine xre has not initialized, skip rule [%s]",
            _rt.c_str());
        return SUCCESS();
    }

    ruleExecInfo_t * rei;
    irods::error err;
    if(!(err = _eff_hdlr("unsafe_ms_ctx", &rei)).ok()) {
        return err;
    }

    rodsLog(
        LOG_DEBUG,
        "applying rule %s, params %ld",
        _rt.c_str(),
        _ms_params->len);

    const int status = execMyRule(
                     (char*)_rt.c_str(),
                     _ms_params,
                     _out_desc.c_str(),
                     rei );

     if (status < 0) {
        std::stringstream msg;
        msg << "execMyRule failed for rule " << _rt;
        rodsLog(LOG_DEBUG, "%s", msg.str().c_str() );
        return ERROR(status, msg.str());
    }

    return SUCCESS();
}

irods::error exec_rule_expression(
    irods::default_re_ctx&,
    const std::string&     _rt,
    msParamArray_t*        _ms_params,
    irods::callback        _eff_hdlr) {

    if(ruleEngineConfig.ruleEngineStatus == UNINITIALIZED) {
        rodsLog(
            LOG_DEBUG,
            "rule engine xre has not initialized, skip rule [%s]",
            _rt.c_str());
        return SUCCESS();
    }

    ruleExecInfo_t * rei;
    irods::error err;
    if(!(err = _eff_hdlr("unsafe_ms_ctx", &rei)).ok()) {
        return err;
    }

    rodsLog(
        LOG_DEBUG,
        "applying rule %s, params %ld",
        _rt.c_str(),
        _ms_params->len);

    std::string rule_text = "{" + _rt + "}";

    char res[MAX_COND_LEN];
    int status = computeExpression(
                     (char*)rule_text.c_str(),
                     _ms_params,
                     rei,
                     NO_SAVE_REI,
                     res );

    if (status < 0) {
        std::stringstream msg;
        msg << "computeExpression failed for input " << _rt;
        rodsLog(LOG_DEBUG, "%s", msg.str().c_str() );
        return ERROR(status, msg.str());
    }

    return SUCCESS();
}


extern "C"
irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory( const std::string& _inst_name,
                                 const std::string& _context ) {
    irods::pluggable_rule_engine<irods::default_re_ctx>* re = new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
    re->add_operation( "start",
            std::function<irods::error(irods::default_re_ctx&,const std::string&)>( start ) );

    re->add_operation( "stop",
            std::function<irods::error(irods::default_re_ctx&,const std::string&)>( stop ) );

    re->add_operation( "rule_exists",
            std::function<irods::error(irods::default_re_ctx&, const std::string&, bool&)>( rule_exists ) );

    re->add_operation( "list_rules",
            std::function<irods::error(irods::default_re_ctx&,std::vector<std::string>&)>( list_rules ) );

    re->add_operation( "exec_rule",
            std::function<irods::error(irods::default_re_ctx&,const std::string&,std::list<boost::any>&,irods::callback)>( exec_rule ) );

    re->add_operation( "exec_rule_text",
            std::function<irods::error(irods::default_re_ctx&,const std::string&,msParamArray_t*,const std::string&,irods::callback)>( exec_rule_text ) );

    re->add_operation( "exec_rule_expression",
            std::function<irods::error(irods::default_re_ctx&,const std::string&,msParamArray_t*,irods::callback)>( exec_rule_expression ) );

    return re;

}
