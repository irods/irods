// =-=-=-=-=-=-=-
// irods includes
#include "msParam.h"
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


std::vector<std::string> ns;

//irods::configuration_parser::array_t get_re_configs(
irods::error get_re_configs(
    const std::string& _instance_name ) {

    typedef irods::configuration_parser::object_t object_t;
    typedef irods::configuration_parser::array_t  array_t;

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

    try {
        array_t namespaces = boost::any_cast<array_t>( plugin_spec_cfg["namespaces"] );
        for( auto itr : namespaces ) {
            std::string n = boost::any_cast<std::string>( itr["namespace"] );
            ns.push_back( n );
        }
    }
    catch( boost::bad_any_cast& ) {
        return ERROR( INVALID_ANY_CAST, "failed." );
    }

    return SUCCESS();
}

irods::error start(irods::default_re_ctx& _u, const std::string& _instance_name) {
    return get_re_configs( _instance_name );
}

irods::error stop(irods::default_re_ctx& _u, const std::string& _instance_name) {
    (void) _u;
    return SUCCESS();
}
irods::error rule_exists(irods::default_re_ctx&, std::string _rn, bool& _ret) {
    _ret = _rn == "pep";
    return SUCCESS();
}

irods::error exec_rule(irods::default_re_ctx&, std::string _rn, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {
//              rodsLog(
//                  LOG_NOTICE,
//                  "callback type \n[%s]\n[%s]\n[%s]",
//                  _ps.back().type().name(), typeid(irods::callback<irods::rule_engine_context_manager<irods::unit, ruleExecInfo_t*, irods::AUDIT_RULE> >).name() ,typeid(irods::callback<irods::rule_engine_context_manager<irods::unit, ruleExecInfo_t*, irods::DONT_AUDIT_RULE> >).name());
//                  return SUCCESS();
    std::stringstream ss;
    ss << _rn << ":";
    int i = 0;
    std::function<irods::error()> go;
    bool testGo = _rn.compare("pep") == 0;
    bool foundGo = false;
    boost::any op;
    std::string opname;
    std::string instname;
    for (auto itr = begin(_ps); itr != end(_ps); ++itr) {
//          rodsLog(
//                   LOG_DEBUG,
//                   "rule %s param %d type [%s]",
//                   _rn.c_str(), i, (*itr).type().name());

        typedef std::function<irods::error()> OP_TYPE;

        if (testGo) {
            switch(i){
            case 0:
                instname = boost::any_cast<std::string>(*itr);
                break;
            case 1:
                opname = boost::any_cast<std::string>(*itr);
                break;
            case 2:
                if( itr->type() != typeid(OP_TYPE)) {
                    return ERROR(-1, "wrong op type");
                } else {
                    go = boost::any_cast<OP_TYPE >(*itr);
                    foundGo = true;
                }
                break;
            case 4:
                op = *itr;
                break;

            }
        }
        if (itr->type() == typeid(std::string)) {
            ss << boost::any_cast<std::string>(*itr) << ";";
        } else {
            ss<< "type "<<itr->type().name()<<";";
        }
        i++;

    }
    // _eff_hdlr(std::string("writeLine"), std::string("serverLog"), ss.str());
    irods::error err = SUCCESS();
    if (foundGo)
    {
        std::list<boost::any> pl(_ps);
        auto itr = pl.begin();
        auto itr2 = pl.begin();
        std::advance(itr, 1);
        std::advance(itr2, 4);
        pl.erase(itr, itr2);

        std::vector<std::string> unwind;
        irods::rule_exists_manager<irods::default_re_ctx, irods::default_ms_ctx> rexist_mgr(irods::re_plugin_globals->global_re_mgr);


        // pre rule
        for(auto itr = begin(ns);itr != end(ns);++itr){
            std::string pre_rule_name = *itr + "pep_" + opname + "_pre";
            bool ret;
            irods::error err1 = rexist_mgr.rule_exists(pre_rule_name, ret);
            if(err1.ok()) {
                if(ret) {
                    err1 = _eff_hdlr(pre_rule_name, irods::unpack(pl));
                    if(!err1.ok()) {
                        err = err1;
                        break;
                    }
                }
            } else {
                err = err1;
                break;
            }

            unwind.push_back(*itr);
        }

        if(err.ok()) {
            // go op
            err = go();
        }

        // post rule
        for(auto itr = unwind.rbegin();itr != unwind.rend();++itr){
            auto post_rule_name = *itr + "pep_" + opname + "_post";
            bool ret;
            irods::error err1 = rexist_mgr.rule_exists(post_rule_name, ret);
            if(err1.ok()) {
                if(ret) {
                    err1 = _eff_hdlr(post_rule_name, irods::unpack(pl));
                    if(!err1.ok()) {
                        err = err1;
                    }
                }
            } else {
                err = err1;
            }
        }

    }

    return err;


}

irods::error exec_rule_text(irods::default_re_ctx&, std::string _rt, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {
    return ERROR(SYS_NOT_SUPPORTED,"not supported");
}

irods::error exec_rule_expression(irods::default_re_ctx&, std::string _rt, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {
    return ERROR(SYS_NOT_SUPPORTED,"not supported");
}

irods::error refresh(irods::default_re_ctx&,const std::string& _instance_name ) {
    return SUCCESS();
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

    re->add_operation<irods::default_re_ctx&,const std::string&>(
            "refresh",
            std::function<irods::error(irods::default_re_ctx&,const std::string&)>( refresh ) );

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
