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



irods::configuration_parser::array_t get_re_configs() {
    irods::server_properties& props = irods::server_properties::getInstance();
    
    props.capture_if_needed();
    irods::configuration_parser::array_t re_plugin_configs;
    props.get_property <irods::configuration_parser::array_t> (std::string("re_plugins"), re_plugin_configs);
    return re_plugin_configs;
}

std::vector<std::string> ns;
    
irods::error start(irods::default_re_ctx& _u) {
    (void) _u;
    bool found = false;
    irods::configuration_parser::array_t re_plugin_configs = get_re_configs();
    for(auto itr = begin(re_plugin_configs); itr != end(re_plugin_configs); ++itr) {
        boost::any pn = (*itr)["plugin_name"];
        if(pn.type() == typeid(std::string)) {
            if(boost::any_cast< std::string> (pn) == std::string("re")) {
                if(found) {
                    return ERROR(-1, "duplicate instances of re");
                }
                found = true;
                boost::any nss = (*itr)["namespaces"];
                if(nss.type() == typeid(irods::configuration_parser::array_t)) {
                    auto s = boost::any_cast< irods::configuration_parser::array_t> (nss);
                    for(auto itr = begin(s); itr != end(s); ++itr) {
                        auto n = (*itr)["namespace"];
                        if(n.type() == typeid(std::string)) {
                            auto ss = boost::any_cast< std::string> (n);
                            if(std::find(ns.begin(), ns.end(), ss) != ns.end()) {
                                return ERROR(-1, "duplicate namespaces");
                            }
                            ns.push_back(ss);
                        } else {
                            return ERROR(-1, "configuration error");
                        }
                    }
                } else {           
                    return ERROR(-1, "configuration error");
                }
            }
        } else {
            return ERROR(-1, "configuration error");
        }
    }     
    return SUCCESS();
}

irods::error stop(irods::default_re_ctx& _u) {
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
        irods::rule_exists_manager<irods::default_re_ctx, irods::default_ms_ctx> rexist_mgr(irods::re_plugin_globals.global_re_mgr);
        
        
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




