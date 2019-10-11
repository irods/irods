


#include "irods_re_serialization.hpp"
#include "irods_plugin_context.hpp"
#include "rodsErrorTable.h"
#include "boost/lexical_cast.hpp"

namespace irods {
    namespace re_serialization {

        static void serialize_keyValPair(
            const keyValPair_t&     _kvp,
            serialized_parameter_t& _out) {
            if(_kvp.len > 0) {
                for(int i = 0; i < _kvp.len; ++i) {
                   if(_kvp.keyWord && _kvp.keyWord[i]) {
                        if(_kvp.value && _kvp.value[i]) {
                            _out[_kvp.keyWord[i]] = _kvp.value[i];
                        }
                        else {
                            _out[_kvp.keyWord[i]] = "empty_value";
                        }
                    }
                }
            } else {
                _out["keyValPair_t"] = "nullptr";
            }
        }

        static error serialize_float_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                 float* f = boost::any_cast<float*>(_p);
                 _out["float_pointer"] = boost::lexical_cast<std::string>(*f);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast float*" );
            }

            return SUCCESS();
        } // serialize_float_ptr

        static error serialize_const_std_string_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                 const std::string* s = boost::any_cast<const std::string*>(_p);
                 _out["const_std_string_ptr"] = *s;
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast const_std_string*" );
            }

            return SUCCESS();
        } // serialize_const_std_string_ptr

        static error serialize_std_string_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                 std::string* s = boost::any_cast<std::string*>(_p);
                 _out["std_string_ptr"] = *s;
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast std_string_ptr" );
            }

            return SUCCESS();
        } // serialize_std_string_ptr

        static error serialize_std_string(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                 _out["std_string"] = boost::any_cast<std::string>(_p);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast std_string*" );
            }

            return SUCCESS();
        } // serialize_std_string

        static error serialize_hierarchy_parser_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                hierarchy_parser* p = boost::any_cast<hierarchy_parser*>(_p);
                std::string hier;
                p->str(hier);
                 _out["hierarchy_parser_ptr"] = hier;
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast hierarchy_parser_ptr*" );
            }

            return SUCCESS();
        } // serialize_hierarchy_parser_ptr

        static error serialize_rodslong(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                rodsLong_t l = boost::any_cast<rodsLong_t>(_p);
                _out["rodslong"] = boost::lexical_cast<std::string>(l);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast rodslong" );
            }

            return SUCCESS();
        } // serialize_rodslong

        static error serialize_rodslong_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                rodsLong_t* l = boost::any_cast<rodsLong_t*>(_p);
                _out["rodslong_ptr"] = boost::lexical_cast<std::string>(*l);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast rodslong" );
            }

            return SUCCESS();
        } // serialize_rodslong_ptr

        static error serialize_sizet(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                size_t l = boost::any_cast<size_t>(_p);
                _out["sizet"] = boost::lexical_cast<std::string>(l);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast sizet" );
            }

            return SUCCESS();
        } // serialize_sizet

        static error serialize_int(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                int l = boost::any_cast<int>(_p);
                _out["int"] = boost::lexical_cast<std::string>(l);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast int" );
            }

            return SUCCESS();
        } // serialize_int

        static error serialize_int_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                int* l = boost::any_cast<int*>(_p);
                _out["int_ptr"] = boost::lexical_cast<std::string>(*l);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast int ptr" );
            }

            return SUCCESS();
        } // serialize_rodslong_ptr

        static error serialize_char_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                char* l = boost::any_cast<char*>(_p);
                if(l) {
                    _out["char_ptr"] = l;
                }
                else {
                    _out["char_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast char ptr" );
            }

            return SUCCESS();
        } // serialize_char_ptr

        static error serialize_const_char_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                const char* l = boost::any_cast<const char*>(_p);
                if(l) {
                    _out["const_char_ptr"] = l;
                }
                else {
                    _out["char_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast const char ptr" );
            }

            return SUCCESS();
        } // serialize_const_char_ptr

        static error serialize_rsComm_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                rsComm_t* l = boost::any_cast<rsComm_t*>(_p);
                if (l) {
                    _out["client_addr"] = l->clientAddr;

                    if(l->auth_scheme) {_out["auth_scheme"] = l->auth_scheme;}

                    _out["proxy_user_name"] = l->proxyUser.userName;
                    _out["proxy_rods_zone"] = l->proxyUser.rodsZone;
                    _out["proxy_user_type"] = l->proxyUser.userType;
                    _out["proxy_sys_uid"] = boost::lexical_cast<std::string>(l->proxyUser.sysUid);
                    _out["proxy_auth_info_auth_scheme"] = l->proxyUser.authInfo.authScheme;
                    _out["proxy_auth_info_auth_flag"] = boost::lexical_cast<std::string>(l->proxyUser.authInfo.authFlag);
                    _out["proxy_auth_info_flag"] = boost::lexical_cast<std::string>(l->proxyUser.authInfo.flag);
                    _out["proxy_auth_info_ppid"] = boost::lexical_cast<std::string>(l->proxyUser.authInfo.ppid);
                    _out["proxy_auth_info_host"] = l->proxyUser.authInfo.host;
                    _out["proxy_auth_info_auth_str"] = l->proxyUser.authInfo.authStr;
                    _out["proxy_user_other_info_user_info"] = l->proxyUser.userOtherInfo.userInfo;
                    _out["proxy_user_other_info_user_comments"] = l->proxyUser.userOtherInfo.userComments;
                    _out["proxy_user_other_info_user_create"] = l->proxyUser.userOtherInfo.userCreate;
                    _out["proxy_user_other_info_user_modify"] = l->proxyUser.userOtherInfo.userModify;

                    _out["user_user_name"] = l->clientUser.userName;
                    _out["user_rods_zone"] = l->clientUser.rodsZone;
                    _out["user_user_type"] = l->clientUser.userType;
                    _out["user_sys_uid"] = boost::lexical_cast<std::string>(l->clientUser.sysUid);
                    _out["user_auth_info_auth_scheme"] = l->clientUser.authInfo.authScheme;
                    _out["user_auth_info_auth_flag"] = boost::lexical_cast<std::string>(l->clientUser.authInfo.authFlag);
                    _out["user_auth_info_flag"] = boost::lexical_cast<std::string>(l->clientUser.authInfo.flag);
                    _out["user_auth_info_ppid"] = boost::lexical_cast<std::string>(l->clientUser.authInfo.ppid);
                    _out["user_auth_info_host"] = l->clientUser.authInfo.host;
                    _out["user_auth_info_auth_str"] = l->clientUser.authInfo.authStr;
                    _out["user_user_other_info_user_info"] = l->clientUser.userOtherInfo.userInfo;
                    _out["user_user_other_info_user_comments"] = l->clientUser.userOtherInfo.userComments;
                    _out["user_user_other_info_user_create"] = l->clientUser.userOtherInfo.userCreate;
                    _out["user_user_other_info_user_modify"] = l->clientUser.userOtherInfo.userModify;
                } else {
                    _out["rsComm_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast rsComm ptr" );
            }

            return SUCCESS();
        } // serialize_rsComm_ptr

        static error serialize_plugin_context(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                plugin_context l = boost::any_cast<plugin_context>(_p);
                if( l.fco().get() ) {
                    l.fco()->get_re_vars( _out );
                }

                if( l.comm() ) {
                    error ret = serialize_rsComm_ptr( l.comm(), _out ); 
                    if(!ret.ok()) {
                        return PASS(ret);
                    }
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast plugin_context" );
            }

            return SUCCESS();
        } // serialize_plugin_context

        static void serialize_spec_coll_info_ptr(
                specColl_t* _sc,
                serialized_parameter_t& _out) {
            if( _sc ) {
                _out["coll_class"] = boost::lexical_cast<std::string>(_sc->collClass);
                _out["type"] = boost::lexical_cast<std::string>(_sc->type);
                _out["collection"] = _sc->collection;
                _out["obj_path"] = _sc->objPath;
                _out["resource"] = _sc->resource;
                _out["resc_hier"] = _sc->rescHier;
                _out["phy_path"] = _sc->phyPath;
                _out["cache_dir"] = _sc->cacheDir;
                try {
                    _out["cache_dirty"] = boost::lexical_cast<std::string>(_sc->cacheDirty);
                }
                catch( boost::bad_lexical_cast& ) {
                    _out["cache_dirty"] = "<unconvertable>";
                }
                try {
                    _out["repl_num"] = boost::lexical_cast<std::string>(_sc->replNum);
                }
                catch( boost::bad_lexical_cast& ) {
                    _out["repl_num"] = "<unconvertable>";
                }
            }
        } // serialize_spec_coll_info_ptr

        static error serialize_dataObjInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                dataObjInp_t* l = boost::any_cast<dataObjInp_t*>(_p);

                if (l) {
                    _out["obj_path"]    = l->objPath;
                    _out["create_mode"] = boost::lexical_cast<std::string>(l->createMode);
                    _out["open_flags"]  = boost::lexical_cast<std::string>(l->openFlags);
                    _out["offset"]      = boost::lexical_cast<std::string>(l->offset);
                    _out["data_size"]   = boost::lexical_cast<std::string>(l->dataSize);
                    _out["num_threads"] = boost::lexical_cast<std::string>(l->numThreads);
                    _out["opr_type"]    = boost::lexical_cast<std::string>(l->oprType);
                    if(l->specColl) {
                        serialize_spec_coll_info_ptr(
                                l->specColl,
                                _out );
                    }

                    serialize_keyValPair(l->condInput, _out);

                } else {
                    _out["dataObjInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast dataObjInp ptr" );
            }

            return SUCCESS();
        } // serialize_dataObjInp_ptr

        static error serialize_authResponseInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                authResponseInp_t* l = boost::any_cast<authResponseInp_t*>(_p);
                if (l) {
                    if(l->response) {
                        _out["response"] = l->response;
                    }
                    if(l->username) {
                        _out["username"]  = l->username;
                    }
                } else {
                    _out["authResponseInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast authResponseInp*" );
            }

            return SUCCESS();
        } // serialize_authResponseInp_ptr

        static error serialize_dataObjInfo_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                dataObjInfo_t* l = boost::any_cast<dataObjInfo_t*>(_p);

                if (l) {
                    _out["logical_path"] = l->objPath;
                    _out["resc_hier"] = l->rescHier;
                    _out["data_type"] = l->dataType;
                    _out["data_size"] = boost::lexical_cast<std::string>(l->dataSize);
                    _out["checksum"] = l->chksum;
                    _out["version"] = l->version;
                    _out["physical_path"] = l->filePath;
                    _out["data_owner_name"] = l->dataOwnerName;
                    _out["data_owner_zone"] = l->dataOwnerZone;
                    _out["replica_number"] = boost::lexical_cast<std::string>(l->replNum);
                    _out["replica_status"] = boost::lexical_cast<std::string>(l->replStatus);
                    _out["data_id"] = boost::lexical_cast<std::string>(l->dataId);
                    _out["coll_id"] = boost::lexical_cast<std::string>(l->collId);
                    _out["data_map_id"] = boost::lexical_cast<std::string>(l->dataMapId);
                    _out["flags"] = boost::lexical_cast<std::string>(l->flags);
                    _out["data_comments"] = l->dataComments;
                    _out["data_mode"] = l->dataMode;
                    _out["data_expiry"] = l->dataExpiry;
                    _out["data_create"] = l->dataCreate;
                    _out["data_modify"] = l->dataModify;
                    _out["data_access"] = l->dataAccess;
                    _out["data_access_index"] = boost::lexical_cast<std::string>(l->dataAccessInx);
                    _out["write_flag"] = boost::lexical_cast<std::string>(l->writeFlag);
                    _out["dest_resc_name"] = l->destRescName;
                    _out["backup_resc_name"] = l->backupRescName;
                    _out["sub_path"] = l->subPath;
                    _out["reg_uid"] = boost::lexical_cast<std::string>(l->regUid);
                    _out["resc_id"] = l->rescId;

                    if(l->specColl) {
                        serialize_spec_coll_info_ptr(
                                l->specColl,
                                _out );
                    }

                    serialize_keyValPair(l->condInput, _out);

                } else {
                    _out["dataObjInfo_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast dataObjInfo ptr" );
            }

            return SUCCESS();
        } // serialize_dataObjInfo_ptr

        static error serialize_keyValPair_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                keyValPair_t* l = boost::any_cast<keyValPair_t*>(_p);

                if (l) {
                    for(int i = 0; i < l->len; ++i) {
                        _out[l->keyWord[i]] = l->value[i];
                    }
                } else {
                    _out["keyValPair_ptr"] = "nullptr";
                }

            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast keyValPair ptr" );
            }


            return SUCCESS();
        } // serialize_keyValPair_ptr

        static error serialize_userInfo_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                userInfo_t* l = boost::any_cast<userInfo_t*>(_p);

                if (l) {
                    _out["user_name"] = l->userName;
                    _out["rods_zone"] = l->rodsZone;
                    _out["user_type"] = l->userType;
                    _out["sys_uid"] = boost::lexical_cast<std::string>(l->sysUid);
                    _out["auth_info_auth_scheme"] = l->authInfo.authScheme;
                    _out["auth_info_auth_flag"] = boost::lexical_cast<std::string>(l->authInfo.authFlag);
                    _out["auth_info_flag"] = boost::lexical_cast<std::string>(l->authInfo.flag);
                    _out["auth_info_ppid"] = boost::lexical_cast<std::string>(l->authInfo.ppid);
                    _out["auth_info_host"] = l->authInfo.host;
                    _out["auth_info_auth_str"] = l->authInfo.authStr;
                    _out["user_other_info_user_info"] = l->userOtherInfo.userInfo;
                    _out["user_other_info_user_comments"] = l->userOtherInfo.userComments;
                    _out["user_other_info_user_create"] = l->userOtherInfo.userCreate;
                    _out["user_other_info_user_modify"] = l->userOtherInfo.userModify;
                } else {
                    _out["userInfo_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast userInfo ptr" );
            }


            return SUCCESS();
        } // serialize_userInfo_ptr

        static error serialize_collInfo_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                collInfo_t* l = boost::any_cast<collInfo_t*>(_p);

                if (l) {
                    _out["coll_id"] = boost::lexical_cast<std::string>(l->collId);
                    _out["coll_name"] = l->collName;
                    _out["coll_parent_name"] = l->collParentName;
                    _out["coll_owner_name"] = l->collOwnerName;
                    _out["coll_owner_zone"] = l->collOwnerZone;
                    _out["coll_map_id"] = boost::lexical_cast<std::string>(l->collMapId);
                    _out["coll_access_index"] = boost::lexical_cast<std::string>(l->collAccessInx);
                    _out["coll_comments"] = l->collComments;
                    _out["coll_inheritance"] = l->collInheritance;
                    _out["coll_expiry"] = l->collExpiry;
                    _out["coll_create"] = l->collCreate;
                    _out["coll_modify"] = l->collModify;
                    _out["coll_access"] = l->collAccess;
                    _out["coll_type"] = l->collType;
                    _out["coll_info1"] = l->collInfo1;
                    _out["coll_info2"] = l->collInfo2;

                    serialize_keyValPair(l->condInput, _out);

                } else {
                    _out["collInfo_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast collInfo ptr" );
            }

            return SUCCESS();
        } // serialize_collInfo_ptr

        static error serialize_collInp_ptr(
                boost::any              _p,
                serialized_parameter_t& _out) { 
            try {
                collInp_t* l = boost::any_cast<collInp_t*>(_p);

                if (l) {
                    _out["coll_name"] = l->collName;
                    _out["flags"] = l->flags;
                    _out["opr_type"] = l->oprType;

                    serialize_keyValPair(l->condInput, _out);

                } else {
                    _out["collInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast collInp ptr" );
            }

            return SUCCESS();
        } // serialize_collInp_ptr

        static error serialize_modAVUMetaInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                modAVUMetadataInp_t* l = boost::any_cast<modAVUMetadataInp_t*>(_p);

                if (l) {
                    if( l->arg0 ) { _out["arg0"] = l->arg0; }
                    if( l->arg1 ) { _out["arg1"] = l->arg1; }
                    if( l->arg2 ) { _out["arg2"] = l->arg2; }
                    if( l->arg3 ) { _out["arg3"] = l->arg3; }
                    if( l->arg4 ) { _out["arg4"] = l->arg4; }
                    if( l->arg5 ) { _out["arg5"] = l->arg5; }
                    if( l->arg6 ) { _out["arg6"] = l->arg6; }
                    if( l->arg7 ) { _out["arg7"] = l->arg7; }
                    if( l->arg8 ) { _out["arg8"] = l->arg8; }
                    if( l->arg9 ) { _out["arg9"] = l->arg9; }
                } else {
                    _out["modAVUMetaInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast modAVUMetaInp ptr" );
            }

            return SUCCESS();
        } // serialize_modAVUMetaInp_ptr

        static error serialize_modAccessControlInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                modAccessControlInp_t* l = boost::any_cast<modAccessControlInp_t*>(_p);

                if (l) {
                    _out["recursive_flag"] = boost::lexical_cast<std::string>(l->recursiveFlag);
                    if( l->accessLevel ) { _out["access_level"] = l->accessLevel; }
                    if( l->userName ) { _out["user_name"] = l->userName; }
                    if( l->zone ) { _out["zone"] = l->zone; }
                    if( l->path ) { _out["path"] = l->path; }
                } else {
                    _out["modAccessControlInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast modAccessControlInp ptr" );
            }

            return SUCCESS();
        } // serialize_modAccessControlInp_ptr

        static error serialize_modDataObjMeta_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                modDataObjMeta_t* l = boost::any_cast<modDataObjMeta_t*>(_p);

                if (l) {
                    error ret = serialize_dataObjInfo_ptr(
                            l->dataObjInfo,
                            _out);
                    if(!ret.ok()) {
                        irods::log(PASS(ret));
                    }

                    if( l->regParam ) {
                        for(int i = 0; i < l->regParam->len; ++i) {
                            _out[l->regParam->keyWord[i]] = l->regParam->value[i];
                        }
                    }
                } else {
                    _out["modDataObjMeta_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast modDataObjMeta ptr" );
            }

            return SUCCESS();
        } // serialize_modDataObjMeta_ptr

        static error serialize_ruleExecSubmitInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                ruleExecSubmitInp_t* l = boost::any_cast<ruleExecSubmitInp_t*>(_p);

                if (l) {
                    _out["rule_name"] = l->ruleName; 
                    _out["rei_file_path"] = l->reiFilePath; 
                    _out["user_name"] = l->userName; 
                    _out["exe_address"] = l->exeAddress; 
                    _out["exe_time"] = l->exeTime; 
                    _out["exe_frequency"] = l->exeFrequency; 
                    _out["priority"] = l->priority; 
                    _out["last_exec_time"] = l->lastExecTime; 
                    _out["exe_status"] = l->exeStatus; 
                    _out["estimate_exe_time"] = l->estimateExeTime; 
                    _out["notification_addr"] = l->notificationAddr; 
                    _out["rule_exec_id"] = l->ruleExecId; 

                    for(int i = 0; i < l->condInput.len; ++i) {
                        _out[l->condInput.keyWord[i]] = l->condInput.value[i];
                    }
                } else {
                    _out["ruleExecSubmitInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                        INVALID_ANY_CAST,
                        "failed to cast ruleExecSubmitInp ptr" );
            }

            return SUCCESS();
        } // serialize_ruleExecSubmitInp_ptr

        static error serialize_dataObjCopyInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                dataObjCopyInp_t* l = boost::any_cast<dataObjCopyInp_t*>(_p);

                if (l) {
                    serialized_parameter_t src;
                    error ret = serialize_dataObjInp_ptr(
                            &l->srcDataObjInp,
                            src );
                    if(!ret.ok()) {
                        irods::log(PASS(ret));
                    }
                    else {
                        for( auto p : src ) {
                            _out["src_"+p.first] = p.second;
                        }
                    }


                    serialized_parameter_t dst;
                    ret = serialize_dataObjInp_ptr(
                            &l->destDataObjInp,
                            dst );
                    if(!ret.ok()) {
                        irods::log(PASS(ret));
                    }
                    else {
                        for( auto p : dst ) {
                            _out["dst_"+p.first] = p.second;
                        }
                    }
                } else {
                    _out["dataObjCopyInp_ptr"] = "nullptr";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast dataObjCopyInp ptr" );
            }

            return SUCCESS();
        } // serialize_dataObjCopyInp_ptr

        static error serialize_rodsObjStat_ptr_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                rodsObjStat_t** tmp = boost::any_cast<rodsObjStat_t**>(_p);
                if(tmp && *tmp) {
                    rodsObjStat_t* l = *tmp;

                    _out["obj_size"] = boost::lexical_cast<std::string>(l->objSize);
                    _out["obj_type"] = boost::lexical_cast<std::string>((int)l->objType);
                    _out["data_mode"] = boost::lexical_cast<std::string>(l->dataMode);
                    
                    _out["data_id"] = boost::lexical_cast<std::string>(l->dataId);
                    _out["checksum"] = boost::lexical_cast<std::string>(l->chksum);
                    _out["ownerName"] = boost::lexical_cast<std::string>(l->ownerName);
                    _out["owner_zone"] = boost::lexical_cast<std::string>(l->ownerZone);
                    _out["create_time"] = boost::lexical_cast<std::string>(l->createTime);
                    _out["modify_time"] = boost::lexical_cast<std::string>(l->modifyTime);
                    _out["resc_hier"] = boost::lexical_cast<std::string>(l->rescHier);

                    if(l->specColl) {
                        serialize_spec_coll_info_ptr(
                            l->specColl,
                            _out );
                    }
                }
                else {
                    _out["null_value"] = "null_value";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast rodsObjStat ptr ptr" );
            }

            return SUCCESS();
        } // serialize_rodsObjStat_ptr_ptr

        static error serialize_rodsObjStat_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                rodsObjStat_t* tmp = boost::any_cast<rodsObjStat_t*>(_p);
                if(tmp) {
                    rodsObjStat_t* l = tmp;

                    _out["obj_size"] = boost::lexical_cast<std::string>(l->objSize);
                    _out["obj_type"] = boost::lexical_cast<std::string>((int)l->objType);
                    _out["data_mode"] = boost::lexical_cast<std::string>(l->dataMode);
                    
                    _out["data_id"] = boost::lexical_cast<std::string>(l->dataId);
                    _out["checksum"] = boost::lexical_cast<std::string>(l->chksum);
                    _out["ownerName"] = boost::lexical_cast<std::string>(l->ownerName);
                    _out["owner_zone"] = boost::lexical_cast<std::string>(l->ownerZone);
                    _out["create_time"] = boost::lexical_cast<std::string>(l->createTime);
                    _out["modify_time"] = boost::lexical_cast<std::string>(l->modifyTime);
                    _out["resc_hier"] = boost::lexical_cast<std::string>(l->rescHier);

                    if(l->specColl) {
                        serialize_spec_coll_info_ptr(
                            l->specColl,
                            _out );
                    }
                }
                else {
                    _out["null_value"] = "null_value";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast rodsObjStat ptr" );
            }

            return SUCCESS();
        } // serialize_rodsObjStat_ptr

        static error serialize_genQueryInp_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                genQueryInp_t* tmp = boost::any_cast<genQueryInp_t*>(_p);
                if(tmp) {
                    genQueryInp_t* l = tmp;

                    _out["maxRows"] = boost::lexical_cast<std::string>(l->maxRows);
                    _out["continueInx"] = boost::lexical_cast<std::string>(l->continueInx);
                    _out["rowOffset"] = boost::lexical_cast<std::string>(l->rowOffset);
                    _out["options"] = boost::lexical_cast<std::string>(l->options);

                    for(int i = 0; i < l->condInput.len; ++i) {
                        _out[l->condInput.keyWord[i]] = l->condInput.value[i];
                    }

                    for (int i = 0; i < l->selectInp.len; ++i) {
                        std::string index = boost::lexical_cast<std::string>(l->selectInp.inx[i]);

                        std::string value = boost::lexical_cast<std::string>(l->selectInp.value[i]);
                        _out["select_" + index] = value;
                    }

                    for (int i = 0; i < l->sqlCondInp.len; ++i) {
                        std::string index = boost::lexical_cast<std::string>(l->sqlCondInp.inx[i]);

                        _out["where_" + index] = l->sqlCondInp.value[i];
                    }
                }
                else {
                    _out["null_value"] = "null_value";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast genQueryInp ptr" );
            }

            return SUCCESS();
        } // serialize_genQueryInp_ptr

        static error serialize_genQueryOut_ptr(
               boost::any                  _p,
               serialized_parameter_t&  _out) {
            try {
                genQueryOut_t* tmp = boost::any_cast<genQueryOut_t*>(_p);
                if (tmp) {
                    genQueryOut_t* l = tmp;

                    _out["rowCnt"] = boost::lexical_cast<std::string>(l->rowCnt);
                    _out["attriCnt"] = boost::lexical_cast<std::string>(l->attriCnt);
                    _out["continueInx"] = boost::lexical_cast<std::string>(l->continueInx);
                    _out["totalRowCount"] = boost::lexical_cast<std::string>(l->totalRowCount);

                    for (int i = 0; i < l->attriCnt; ++i) {
                        for (int j = 0; j < l->rowCnt; ++j) {
                            std::string i_str = boost::lexical_cast<std::string>(i);
                            std::string j_str = boost::lexical_cast<std::string>(j);
                            _out["attriInx_" + i_str] = boost::lexical_cast<std::string>(l->sqlResult[i].attriInx);
                            _out["len_" + i_str] = boost::lexical_cast<std::string>(l->sqlResult[i].len);
                            _out["value_" + j_str + "_" + i_str] = boost::lexical_cast<std::string>(l->sqlResult[i].value+j*l->sqlResult[i].len);
                        }
                    }
                } else {
                    _out["null_value"] = "null_value";
                }
            } catch ( std::exception& ) {
                return ERROR(
                        INVALID_ANY_CAST,
                        "failed to cast genQueryOut ptr" );
            }

            return SUCCESS();
        } //serialize_genQueryOut_ptr

 
        static error serialize_char_ptr_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                char** tmp = boost::any_cast<char**>(_p);
                if(tmp && *tmp ) {
                    _out["value"] = *tmp;
                }
                else {
                    _out["null_value"] = "null_value";
                }
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast char ptr ptr" );
            }

            return SUCCESS();
        } // serialize_char_ptr_ptr
#if 0
        static error serialize_XXXX_ptr(
                boost::any               _p,
                serialized_parameter_t& _out) { 
            try {
                XXXX_t* l = boost::any_cast<XXXX_t*>(_p);
            }
            catch ( std::exception& ) {
                return ERROR(
                         INVALID_ANY_CAST,
                         "failed to cast XXXX ptr" );
            }

            return SUCCESS();
        } // serialize_XXXX_ptr
#endif

        serialization_map_t& get_serialization_map() {
            static serialization_map_t the_map {
                { std::type_index(typeid(float*)), serialize_float_ptr },
                { std::type_index(typeid(const std::string*)), serialize_const_std_string_ptr },
                { std::type_index(typeid(std::string*)), serialize_std_string_ptr },
                { std::type_index(typeid(std::string)), serialize_std_string },
                { std::type_index(typeid(hierarchy_parser*)), serialize_hierarchy_parser_ptr },
                { std::type_index(typeid(rodsLong_t)), serialize_rodslong },
                { std::type_index(typeid(rodsLong_t*)), serialize_rodslong_ptr },
                { std::type_index(typeid(size_t)), serialize_sizet },
                { std::type_index(typeid(int)), serialize_int },
                { std::type_index(typeid(int*)), serialize_int_ptr },
                { std::type_index(typeid(char*)), serialize_char_ptr },
                { std::type_index(typeid(const char*)), serialize_const_char_ptr },
                { std::type_index(typeid(rsComm_t*)), serialize_rsComm_ptr },
                { std::type_index(typeid(plugin_context)), serialize_plugin_context },
                { std::type_index(typeid(dataObjInp_t*)), serialize_dataObjInp_ptr },
                { std::type_index(typeid(authResponseInp_t*)), serialize_authResponseInp_ptr },
                { std::type_index(typeid(dataObjInfo_t*)), serialize_dataObjInfo_ptr },
                { std::type_index(typeid(keyValPair_t*)), serialize_keyValPair_ptr },
                { std::type_index(typeid(userInfo_t*)), serialize_userInfo_ptr },
                { std::type_index(typeid(collInfo_t*)), serialize_collInfo_ptr },
                { std::type_index(typeid(collInp_t*)), serialize_collInp_ptr },
                { std::type_index(typeid(modAVUMetadataInp_t*)), serialize_modAVUMetaInp_ptr },
                { std::type_index(typeid(modAccessControlInp_t*)), serialize_modAccessControlInp_ptr },
                { std::type_index(typeid(modDataObjMeta_t*)), serialize_modDataObjMeta_ptr },
                { std::type_index(typeid(ruleExecSubmitInp_t*)), serialize_ruleExecSubmitInp_ptr },
                { std::type_index(typeid(dataObjCopyInp_t*)), serialize_dataObjCopyInp_ptr },
                { std::type_index(typeid(rodsObjStat_t**)), serialize_rodsObjStat_ptr_ptr },
                { std::type_index(typeid(rodsObjStat_t*)), serialize_rodsObjStat_ptr },
                { std::type_index(typeid(genQueryInp_t*)), serialize_genQueryInp_ptr },
                { std::type_index(typeid(genQueryOut_t*)), serialize_genQueryOut_ptr },
                { std::type_index(typeid(char**)), serialize_char_ptr_ptr }
            };
            return the_map;

        } // get_serialization_map

        error add_operation(
            const index_t& _index,
            operation_t    _operation ) {
            
            serialization_map_t& the_map = get_serialization_map();
            if(the_map.find(_index) != the_map.end() ) {
                return ERROR(
                           KEY_NOT_FOUND,
                           "type_index exists");
            }

            the_map[ _index ] = _operation;

            return SUCCESS();

        } // add_operation

        static std::string demangle(const char* name) {
            int status = -4; // some arbitrary value to eliminate the compiler warning
            std::unique_ptr<char, void(*)(void*)> res {
                abi::__cxa_demangle(name, NULL, NULL, &status),
                    std::free
            };
            return (status==0) ? res.get() : name ;
        }

        error serialize_parameter(
            boost::any               _in_param,
            serialized_parameter_t&  _out_param ) {
            index_t idx = std::type_index(_in_param.type());
            serialization_map_t& the_map = get_serialization_map();
            if(the_map.find(idx) == the_map.end() ) {
                std::string err = "[";
                err += demangle( _in_param.type().name() );
                err += "] not supported";
                _out_param["ERROR"] = err;
                return SUCCESS();
            }
            
            return the_map[idx](_in_param, _out_param);

        } // serialize_parameter

    }; // re_serialization

}; // namespace irods



