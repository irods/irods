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


extern "C" {

    irods::configuration_parser::array_t get_re_configs() {
        irods::server_properties& props = irods::server_properties::getInstance();

        props.capture_if_needed();
        irods::configuration_parser::array_t re_plugin_configs;
        props.get_property <irods::configuration_parser::array_t> (std::string("re_plugins"), re_plugin_configs);
        return re_plugin_configs;
    }

    // http://stackoverflow.com/questions/7724448/simple-json-string-escape-for-c
    std::string escapeJsonString(const std::string& input) {
        std::ostringstream ss;
        ss << "\"";
        for (auto iter = input.cbegin(); iter != input.cend(); iter++) {
        //C++98/03:
        //for (std::string::const_iterator iter = input.begin(); iter != input.end(); iter++) {
            switch (*iter) {
                case '\\': ss << "\\\\"; break;
                case '"': ss << "\\\""; break;
                case '/': ss << "\\/"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default: ss << *iter; break;
            }
        }
        ss << "\"";
        return ss.str();
    }

    std::vector<std::string> ns;

    irods::error start(irods::default_re_ctx& _u) {
        (void) _u;

        return SUCCESS();
    }

    irods::error stop(irods::default_re_ctx& _u) {
        (void) _u;
        return SUCCESS();
    }
    irods::error rule_exists(irods::default_re_ctx&, std::string _rn, bool& _ret) {
        _ret = _rn.compare(0,6,"audit_") == 0;
        return SUCCESS();
    }

    irods::error exec_rule(irods::default_re_ctx&, std::string _rn, std::list<boost::any>& _ps, irods::callback _eff_hdlr) {
        ruleExecInfo_t * rei;
        irods::error err;
        if(!(err = _eff_hdlr("unsafe_ms_ctx", &rei)).ok()) {
            return err;
        }

        std::stringstream ss;
        ss << "{" << escapeJsonString (_rn) << ": { " << std::endl;
        int i = 0;
        std::string instname;
        for (auto itr = begin(_ps); itr != end(_ps); ++itr) {

            switch(i){
                case 0:
                    instname = boost::any_cast<std::string>(*itr);
                    break;
            }
            if (itr->type() == typeid(std::string)) {
                ss << escapeJsonString(std::to_string(i)) << ":" << escapeJsonString(boost::any_cast<std::string>(*itr)) <<","<< std::endl;
            //} else if (itr->type() == typeid(char *)) {
            //    ss << escapeJsonString(std::to_string(i)) << ":" << escapeJsonString(boost::any_cast<char *>(*itr)) <<","<< std::endl;
            //} else if (itr->type() == typeid(int *)) {
            //    ss << escapeJsonString(std::to_string(i)) << ":" << escapeJsonString("pointer to " + std::to_string(*boost::any_cast<int *>(*itr))) <<","<< std::endl;
            //} else if (itr->type() == typeid(const char *)) {
            //    ss << escapeJsonString(std::to_string(i)) << ":" << escapeJsonString(boost::any_cast<const char *>(*itr)) <<","<< std::endl;
            } else if (itr->type() == typeid(int)) {
                ss<< escapeJsonString(std::to_string(i))<<":"<< boost::any_cast<int>(*itr) <<","<< std::endl;
            } else {
                ss<< escapeJsonString(std::to_string(i))<<": "<<escapeJsonString(std::string("type ") + itr->type().name())<<","<< std::endl;
            }
            i++;

        }
        if (rei->condInputData) {
            for (i=0; i< rei->condInputData->len;i++) {
                ss<<escapeJsonString(rei->condInputData->keyWord[i]) << ":" << escapeJsonString(rei->condInputData->value[i]) <<(i==rei->condInputData->len-1?"":",")<< std::endl;
            }
        } else {
            ss<< escapeJsonString("")<<":"<<escapeJsonString("no condInputData")<<std::endl;
        }
        ss<<"}}";
        return _eff_hdlr(std::string("writeLine"), std::string("serverLog"), ss.str());


    }

    irods::pluggable_rule_engine<irods::default_re_ctx>* plugin_factory( const std::string& _inst_name,
                                     const std::string& _context ) {
        irods::pluggable_rule_engine<irods::default_re_ctx>* re = new irods::pluggable_rule_engine<irods::default_re_ctx>( _inst_name , _context);
        return re;

    }

}; // extern "C"
