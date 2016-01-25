#include "irods_re_plugin.hpp"
#include "region.h"
#include "irods_hashtable.h"
#include "restructs.hpp"
#include "functions.hpp"
#include "conversion.hpp"
#include "reFuncDefs.hpp"
#include "reGlobalsExtern.hpp"
#include "irods_server_properties.hpp"
int processReturnRes( Res *res );
namespace irods{

    // extern variable for the re plugin globals
    struct global_re_plugin_mgr re_plugin_globals;

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
        server_properties& props = server_properties::getInstance();

        props.capture_if_needed();
        configuration_parser::array_t re_plugin_configs;
        error err;
        if(!(err = props.get_property <configuration_parser::array_t> (std::string("re_plugins"), re_plugin_configs)).ok()) {
            rodsLog(LOG_ERROR, "cannot load re_plugins");
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
                UNIT);
        }
        return ret;
    }


    template class pluggable_rule_engine<default_re_ctx>;

    Region *r = make_region(0, NULL);
    Hashtable *ft = newHashTable2(10, r);
    bool load = false;

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

        if(!load) {
            getSystemFunctions(ft,r);
            load = true;
        }

        unsigned int nargs = l.size();


        error err;
        struct all_resources {
            all_resources() {
                rNew = make_region(0, NULL);
                env = newEnv( NULL, NULL, NULL, rNew );
                memset(msParams,0 ,sizeof(msParam_t[10]));
                memset(args, 0 ,sizeof(Res*[10]));
            }
            ~all_resources() {
                for(auto itr= begin(myArgv);itr != end(myArgv); ++itr) {
                    clearMsParam(*itr, 1);
                }
                region_free(rNew);
            }

            std::vector<msParam_t *> myArgv;
            Region *rNew;
            Env *env;
            msParam_t msParams[10];
            Res* args[10];
        } ar;

        irods::ms_table_entry ms_entry;
        int actionInx;
        // look in the system functions table first
        // sys func table support are limited as they should be roll into the rule engien plugin and microservice plugins
        // currently no return val support
        Node *node = (Node *) lookupFromHashTable(ft, msName.c_str());
        if(node == NULL) {
            // then in ms table
            /* look up the micro service */
            actionInx = actionTableLookUp( ms_entry,const_cast<char*>( msName .c_str()));

            if ( actionInx < 0 ) {
                return ERROR( NO_MICROSERVICE_FOUND_ERR, "default_microservice_manager: no micro service found" + msName);
            }

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

        if (node!=NULL) {
            i = 0;
            for(auto itr = begin(ar.myArgv); itr != end(ar.myArgv); ++itr) {
                ar.args[i] = convertMsParamToRes(*itr, ar.rNew);
                if(processReturnRes(ar.args[i]) != 0) {
                    return ERROR(RES_ERR_CODE(ar.args[i]), "cannot convert parameter");
                }
                i++;
            }

            Res *res = node->func( ar.args, nargs, NULL, rei, NO_SAVE_REI, ar.env, &rei->rsComm->rError, ar.rNew);
            int ret = processReturnRes(res);
            if(ret) {
                return ERROR(ret,"processReturnRes failed");
            } else {
                i = 0;
                for(auto itr = begin(ar.myArgv); itr != end(ar.myArgv); ++itr) {
                    if((ret = convertResToMsParam(*itr, ar.args[i], &rei->rsComm->rError)) !=0) {
                        return ERROR(ret, "cannot convert parameter");
                    }
                    i++;
                }
            }
        } else {
            funcPtr myFunc = NULL;
            unsigned int numOfStrArgs;
            int ii = 0;
            std::vector<msParam_t *> &myArgv = ar.myArgv;

            myFunc       = ms_entry.call_action_;
            numOfStrArgs = ms_entry.num_args_;
            if ( nargs != numOfStrArgs ) {
                return ERROR( ACTION_ARG_COUNT_MISMATCH, "execMicroService3: wrong number of arguments");
            }


            if ( numOfStrArgs == 0 ) {
                ii = ( *( int ( * )( ruleExecInfo_t * ) )myFunc )( rei ) ;
            }
            else if ( numOfStrArgs == 1 ) {
                ii = ( *( int ( * )( msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], rei );
            }
            else if ( numOfStrArgs == 2 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], rei );
            }
            else if ( numOfStrArgs == 3 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], rei );
            }
            else if ( numOfStrArgs == 4 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], rei );
            }
            else if ( numOfStrArgs == 5 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], rei );
            }
            else if ( numOfStrArgs == 6 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], rei );
            }
            else if ( numOfStrArgs == 7 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], rei );
            }
            else if ( numOfStrArgs == 8 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7], rei );
            }
            else if ( numOfStrArgs == 9 ) {
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7], myArgv[8], rei );
            }
            else if ( numOfStrArgs == 10 )
                ii = ( *( int ( * )( msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, msParam_t *, ruleExecInfo_t * ) )myFunc )( myArgv[0], myArgv[1], myArgv[2], myArgv[3], myArgv[4], myArgv[5], myArgv[6], myArgv[7],
                        myArgv[8], myArgv [9], rei );

            if ( ii < 0 ) {
                return ERROR(ii,"exec_microservice_adapter failed");
            }
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
