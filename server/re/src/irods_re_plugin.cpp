#include "irods_re_plugin.hpp"
#include "region.h"
#include "irods_hashtable.h"
#include "irods_server_properties.hpp"
#include "irods_ms_plugin.hpp"
int actionTableLookUp( irods::ms_table_entry& _entry, char* _action );

namespace irods{

    // extern variable for the re plugin globals
    std::unique_ptr<struct global_re_plugin_mgr> re_plugin_globals;

    void var_arg_to_list(std::list<boost::any>& _l) {
        (void) _l;
    }

    error list_to_var_arg(std::list<boost::any>& _l) {
        if(! _l.empty()) {
            return ERROR(-1, "arg list mismatch");
        } else {
            return SUCCESS();
        }
    }

    unpack::unpack(std::list<boost::any> &_l) : l_(_l) {};

    configuration_parser::array_t get_re_configs() {
        configuration_parser::array_t re_plugin_configs;
        error err;
        if(!(err = irods::get_server_property<configuration_parser::array_t> (std::string("rule_engines"), re_plugin_configs)).ok()) {
            rodsLog(LOG_ERROR, "cannot load re_plugins from server_config.json");
        }
        return re_plugin_configs;
    }

    std::vector<re_pack_inp<default_re_ctx> > init_global_re_packs() {
        std::vector<re_pack_inp<default_re_ctx> > ret;
        configuration_parser::array_t re_plugin_configs = get_re_configs();
        for(auto&& itr : re_plugin_configs ) {
            ret.emplace_back(
                boost::any_cast< std::string> (itr["instance_name"]),
                boost::any_cast< std::string> (itr["plugin_name"]),
                (RuleExecInfo *) UNIT);
        }
        return ret;
    }


    template class pluggable_rule_engine<default_re_ctx>;

    error convertToMsParam(boost::any &itr, msParam_t *t) {
        if(itr.type() == typeid(std::string)) {
            fillStrInMsParam( t, const_cast<char*>( boost::any_cast<std::string>(itr).c_str() ));
            return SUCCESS();
        } else if(itr.type() == typeid(std::string *)) {
            fillStrInMsParam( t, const_cast<char*>( (*(boost::any_cast<std::string *>(itr))).c_str() ));
            return SUCCESS();
        } else {
            return ERROR(-1, "cannot convert parameter");
        }
    }
    error convertFromMsParam(boost::any& itr, msParam_t *t) {
        if(std::string(t->type).compare(STR_MS_T) == 0) {
            if(itr.type() == typeid(std::string *)) {
                *(boost::any_cast<std::string *>(itr)) = std::string(reinterpret_cast<char*>( t->inOutStruct) );
            }
            return SUCCESS();
        } else {
            return ERROR(-1, "cannot convert parameter");
        }
    }

    error default_microservice_manager<default_ms_ctx>:: exec_microservice_adapter( std::string msName, default_ms_ctx rei, std::list<boost::any> &l ) {
        if(msName == std::string("unsafe_ms_ctx")) {
            default_ms_ctx *p;
            error err;
            if(!(err = list_to_var_arg(l, p)).ok()) {
                return err;
            }
            *p = rei;
            return SUCCESS();
        }
        
        unsigned int nargs = l.size();

        error err;
        struct all_resources {
            all_resources() {
                rNew = make_region(0, NULL);
                memset(msParams,0 ,sizeof(msParam_t[10]));
            }
            ~all_resources() {
                for(auto itr= begin(myArgv);itr != end(myArgv); ++itr) {
                    clearMsParam(*itr, 1);
                }
                region_free(rNew);
            }

            std::vector<msParam_t *> myArgv;
            Region *rNew;
            msParam_t msParams[10];
        } ar;

        irods::ms_table_entry ms_entry;
        int actionInx;
        actionInx = actionTableLookUp( ms_entry,const_cast<char*>( msName .c_str()));
        if ( actionInx < 0 ) {
            return ERROR( NO_MICROSERVICE_FOUND_ERR, "default_microservice_manager: no microservice found " + msName);
        }

        int i = 0;
        for(auto itr = begin(l); itr != end(l); ++itr) {
            msParam_t* p = &(ar.msParams[i]);
            if(!(err = convertToMsParam(*itr, p)).ok()) {
                return err;
            }
            ar.myArgv.push_back(p);
            i++;
        }

        unsigned int numOfStrArgs = ms_entry.num_args();
        if ( nargs != numOfStrArgs ) {
            return ERROR( ACTION_ARG_COUNT_MISMATCH, "execMicroService3: wrong number of arguments");
        }

        std::vector<msParam_t *> &myArgv = ar.myArgv;
        int status = ms_entry.call( rei, myArgv );
        if ( status < 0 ) {
            return ERROR(status,"exec_microservice_adapter failed");
        }
    
        i = 0;
        for(auto itr = begin(l); itr != end(l); ++itr) {
            if(!(err = convertFromMsParam(*itr, ar.myArgv[i])).ok()) {
                return err;
            }
            i++;
        }

        return  SUCCESS();

    }

}
