


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_backport.h"
#include "eirods_string_tokenize.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_stacktrace.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // helper function to convert properties from a resource plugin
    // the a standard irods rescInfo_t 
    error resource_to_resc_info( rescInfo_t& _info, resource_ptr& _resc ) {
        error err;
        std::string prop_name;
        // =-=-=-=-=-=-=-
        // get the resource property - host
        prop_name = "host";
        rodsServerHost_t* host = 0;
        err = _resc->get_property< rodsServerHost_t* >( prop_name, host );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - id
        prop_name = "id";
        long id = 0;
        err = _resc->get_property< long >( prop_name, id );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - freespace
        prop_name = "freespace";
        long freespace = 0;
        err = _resc->get_property< long >( prop_name, freespace );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - quota
        prop_name = "quota";
        long quota = 0;
        err = _resc->get_property< long >( prop_name, quota );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - zone
        prop_name = "zone";
        std::string zone;
        err = _resc->get_property< std::string >( prop_name, zone );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - name
        prop_name = "name";
        std::string name;
        err = _resc->get_property< std::string >( prop_name, name );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - location
        prop_name = "location";
        std::string location;
        err = _resc->get_property< std::string >( prop_name, location );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - type
        prop_name = "type";
        std::string type;
        err = _resc->get_property< std::string >( prop_name, type );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - class
        prop_name = "class";
        std::string rclass;
        err = _resc->get_property< std::string >( prop_name, rclass );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - path
        prop_name = "path";
        std::string path;
        err = _resc->get_property< std::string >( prop_name, path );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - info
        prop_name = "info";
        std::string info;
        err = _resc->get_property< std::string >( prop_name, info );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - comments
        prop_name = "comments";
        std::string comments;
        err = _resc->get_property< std::string >( prop_name, comments );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - create
        prop_name = "create";
        std::string create;
        err = _resc->get_property< std::string >( prop_name, create );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - modify
        prop_name = "modify";
        std::string modify;
        err = _resc->get_property< std::string >( prop_name, modify );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - status
        prop_name = "status";
        int status = 0;
        err = _resc->get_property< int >( prop_name, status );
        if( !err.ok() ) {
            std::stringstream msg;
            msg << "resource_to_resc_info - failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( -1, msg.str() );
        }

        _info.rodsServerHost = host;
        _info.rescId         = id;
        _info.freeSpace      = freespace;
        _info.quotaLimit     = quota;
        _info.rescStatus     = status;
        strncpy( _info.zoneName,      zone.c_str(),     NAME_LEN );
        strncpy( _info.rescName,      name.c_str(),     NAME_LEN );
        strncpy( _info.rescLoc,       location.c_str(), NAME_LEN );
        strncpy( _info.rescType,      type.c_str(),     NAME_LEN );
        strncpy( _info.rescClass,     rclass.c_str(),   NAME_LEN );
        strncpy( _info.rescVaultPath, path.c_str(),     NAME_LEN );
        strncpy( _info.rescInfo,      info.c_str(),     NAME_LEN );
        strncpy( _info.rescComments,  comments.c_str(), NAME_LEN );
        strncpy( _info.rescCreate,    create.c_str(),   TIME_LEN );
        strncpy( _info.rescModify,    modify.c_str(),   TIME_LEN );

        return SUCCESS();
         
    } // resource_to_resc_info

    // =-=-=-=-=-=-=-
    // helper function to extract useful bits from a resource plugin and fill in a 
    // resource group info structure.
    error resource_to_resc_grp_info( rescGrpInfo_t& _grp_info, resource_ptr& _resc ) {
        // =-=-=-=-=-=-=-
        // end the linked lists
        _grp_info.next      = NULL;
        _grp_info.cacheNext = NULL;

        // =-=-=-=-=-=-=-
        // allocate the rescinfo struct if necessary
        if( !_grp_info.rescInfo ) {
            _grp_info.rescInfo = new rescInfo_t;
        } else {

        }
         
        // =-=-=-=-=-=-=-
        // call earlier helper function to fill in the rescInfo_t structure 
        error err = resource_to_resc_info( *_grp_info.rescInfo, _resc ); 
        if( !err.ok() ) {
            return PASS( false, -1, "resource_to_resc_info - failed.", err );            
            
        }
       
        // =-=-=-=-=-=-=-
        // copy the name for the resc group.  since we dont have groups anymore this
        // may cause an issue with legacy code. 
        rstrcpy( _grp_info.rescGroupName, _grp_info.rescInfo->rescName, NAME_LEN );

        return SUCCESS();

    } // resource_to_resc_grp_info

    // =-=-=-=-=-=-=-
    // given a list of resource names from a rule, make some decisions
    // about which resource should be set to default and used.  store
    // that information in the rescGrpInfo structure
    // NOTE :: this is a reimplementation of setDefaultResc in resource.c but
    //      :: with the composite resource spin
    error set_default_resource( rsComm_t*      _comm,   std::string   _resc_list,  
                                std::string    _option, keyValPair_t* _cond_input, 
                                rescGrpInfo_t& _resc_grp ) {
        // =-=-=-=-=-=-=
        // quick error check
        if( _resc_list.empty() && NULL == _cond_input ) {
            return ERROR( USER_NO_RESC_INPUT_ERR, "set_default_resource - no user input" );
        }
        
        // =-=-=-=-=-=-=-
        // resource name passed in via conditional input 
        std::string cond_input_resc;

        // =-=-=-=-=-=-=-
        // if the resource list is not "null" and the forced flag is set
        // then zero out the conditional input as it is to be ignored
        if( "null"   != _resc_list && 
            "forced" == _option    &&
            _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            _cond_input = NULL;
        } else if( _cond_input ) {
            char* name = NULL;
            if( ( name = getValByKey( _cond_input, BACKUP_RESC_NAME_KW ) ) == NULL &&
                ( name = getValByKey( _cond_input, DEST_RESC_NAME_KW   ) ) == NULL &&
                ( name = getValByKey( _cond_input, DEF_RESC_NAME_KW    ) ) == NULL &&
                ( name = getValByKey( _cond_input, RESC_NAME_KW        ) ) == NULL ) {
                // =-=-=-=-=-=-=-
                // no conditional input resource
            } else {
                cond_input_resc = name;

            }

        } // else if

        // =-=-=-=-=-=-=-
        // split the list into a vector of strings using a % delimiter
        std::vector< std::string > resources;
        string_tokenize( _resc_list, resources, "%" );

        // =-=-=-=-=-=-=-
        // pick a good resource which is availabe out of the list,
        // NOTE :: the legacy code picks a random one first then scans
        //      :: for a valid one if the random resource is down.  i just
        //      :: scan for one.  i dont think this would break anything...
        std::string default_resc_name;
        
        std::vector< std::string >::iterator itr = resources.begin();
        for( ; itr != resources.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // convert the resource into a legacy group info
            error grp_err = get_resc_grp_info( *itr, _resc_grp ); 
            if( grp_err.ok() ) {
                if( _resc_grp.rescInfo->rescStatus != INT_RESC_STATUS_DOWN ) {
                    default_resc_name = *itr;
                    // =-=-=-=-=-=-=-
                    // we found a live one
                    break;
                }

           } // if grp_err

        } // for itr

        // =-=-=-=-=-=-=-
        // determine that we might need a 'preferred' resource
        if( "preferred" == _option && !cond_input_resc.empty() ) {
            // =-=-=-=-=-=-=-
            // determine if the resource is live
            error grp_err = get_resc_grp_info( cond_input_resc, _resc_grp ); 
            if( grp_err.ok() ) {
                if( _resc_grp.rescInfo->rescStatus != INT_RESC_STATUS_DOWN ) {
                    // =-=-=-=-=-=-=-
                    // we found a live one, were good to go
                    return SUCCESS();
                }

            } // if grp_err 

        } else if( "forced" == _option && _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            // stick with the default found above, its forced.
            return SUCCESS();

        } else {
            // =-=-=-=-=-=-=-
            // try the conditional input string, if not go back to the default resource
            error grp_err = get_resc_grp_info( cond_input_resc, _resc_grp ); 
            if( grp_err.ok() ) {
                if( _resc_grp.rescInfo->rescStatus != INT_RESC_STATUS_DOWN ) {
                    // =-=-=-=-=-=-=-
                    // we found a live one, go!
                    return SUCCESS();
                } else {

                }

            } else {
                // =-=-=-=-=-=-=-
                // otherwise go back to the old, default we had before
                error grp_err = get_resc_grp_info( default_resc_name, _resc_grp ); 
                if( grp_err.ok() ) {
                    return SUCCESS();

                } else {
                    std::stringstream msg;
                    msg << "set_default_resource - failed to find default resource for list [";
                    msg << _resc_list;
                    msg << "] and option [";
                    msg << _option;
                    msg << "]";
                    return ERROR( SYS_RESC_DOES_NOT_EXIST, msg.str() );
                }

            } // else

        } // else
        

        // =-=-=-=-=-=-=-
        // should not reach here
        std::stringstream msg;
        msg << "set_default_resource - should not reach here for list [";
        msg << _resc_list;
        msg << "] and option [";
        msg << _option;
        msg << "]";
        return ERROR( CAT_NO_ROWS_FOUND, msg.str() );

    } // set_default_resource

    // =-=-=-=-=-=-=-
    // function which determines resource name based on
    // keyval pair and a given string
    error resolve_resource_name( std::string _resc_name, keyValPair_t* _cond_input, std::string& _out ) {
        if ( _resc_name.empty() ) {
            char* name = 0;
            name = getValByKey( _cond_input, BACKUP_RESC_NAME_KW );
            if( name ) {
                _out = std::string( name );
                return SUCCESS();
            }
              
            name = getValByKey( _cond_input, DEST_RESC_NAME_KW );
            if( name ) {
                _out = std::string( name );
                return SUCCESS();
            }

            name = getValByKey( _cond_input, DEF_RESC_NAME_KW  );
            if( name ) {
                _out = std::string( name );
                return SUCCESS();
            }   
                
            return ERROR( INT_RESC_STATUS_DOWN, "resolve_resource_name - failed to resolve resource name" );
            
        } else {
            _out = _resc_name;
            return SUCCESS();
        }

    } // resolve_resource_name

    // =-=-=-=-=-=-=-
    // helper function - get the status property of a resource given a 
    // match to the incoming pointer
    error get_host_status_by_host_info( rodsServerHost_t* _info ) {
        // =-=-=-=-=-=-=-
        // idiot check pointer
        if( !_info ) {
            return ERROR( -1, "get_host_status_by_host_info - null pointer" );
        }

        // =-=-=-=-=-=-=-
        // find a matching resource
        resource_ptr resc;
        error err = resc_mgr.resolve_from_property< rodsServerHost_t* >( "host", _info, resc );
        if( !err.ok() ) {
            return ERROR( -1, "get_host_status_by_host_info - failed to resolve resource" );
        }

        // =-=-=-=-=-=-=-
        // get the status property of the resource
        int status = -1;
        err = resc->get_property< int >( "status", status );
        if( !err.ok() ) {
            return ERROR( -1, "get_host_status_by_host_info - failed to get resource property" );
        }

        return CODE( status );

    } // get_host_status_by_host_info

    // =-=-=-=-=-=-=-
    // helper function to save on typing - get legacy data struct
    // for resource given a resource name
    error get_resc_info( std::string _name, rescInfo_t& _info ) {
        
        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if( res_err.ok() ) {
            // =-=-=-=-=-=-=-
            // check to see if the resource is active, if not fail
            int status = 0;
            get_resource_property< int >( _name, "status", status );
            if( status == INT_RESC_STATUS_DOWN ) {
                return ERROR( SYS_RESC_IS_DOWN, "The Resource is Down" );
            }

            error info_err = resource_to_resc_info( _info, resc );
            if( info_err.ok() ) {
                return SUCCESS();

            } else {
                return PASS( false, -1, "get_resc_info - failed.", info_err );

            }

        } else {
                return PASS( false, -1, "get_resc_info - failed.", res_err );

        }

    } // get_resc_info

    // =-=-=-=-=-=-=-
    // helper function to save on typing - get legacy data struct
    // for resource group given a resource name
    error get_resc_grp_info( std::string _name, rescGrpInfo_t& _info ) {
        if( _name.empty() ) {
            return ERROR( -1, "get_resc_grp_info :: empty key" ); 
        }
        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if( res_err.ok() ) {
            // =-=-=-=-=-=-=-
            // check to see if the resource is active, if not fail
            int status = 0;
            get_resource_property< int >( _name, "status", status );
            if( status == INT_RESC_STATUS_DOWN ) {
                return ERROR( SYS_RESC_IS_DOWN, "The Resource is Down" );
            }

            error info_err = resource_to_resc_grp_info( _info, resc );
            if( info_err.ok() ) {
                return SUCCESS();

            } else {
                return PASS( false, -1, "get_resc_grp_info - failed.", info_err );

            }

        } else {
                return PASS( false, -1, "get_resc_grp_info - failed.", res_err );

        }


    } // get_resc_grp_info

    error get_host_for_hier_string( 
              const std::string& _hier_str,      // hier string
              int&               _local_flag,    // local flag
              rodsServerHost_t*& _server_host) { // server host

        // =-=-=-=-=-=-=-
        // check hier string
        if( _hier_str.empty() ) {
eirods::stacktrace st;
st.trace();
st.dump();
            return ERROR( -1, "get_host_for_hier_string - hier string is empty" );
        }

        // =-=-=-=-=-=-=-
        // extract the last resource in the hierarchy
        std::string resc_name;
        hierarchy_parser parse;
        parse.set_string( _hier_str );
        parse.last_resc( resc_name );

        // =-=-=-=-=-=-=-
        // check hier string
        if( resc_name.empty() ) {
            return ERROR( -1, "get_host_for_hier_string - resc_name string is empty" );
        }

        // =-=-=-=-=-=-=-
        // get the rods server host info for the child resc
        rodsServerHost_t* host;
        error ret = get_resource_property< rodsServerHost_t* >( resc_name, "host", host );
        if( !ret.ok() ) {
            std::stringstream msg;
            msg << "get_host_for_hier_string - failed to get host property for [";
            msg << resc_name;
            msg << "]";
            return PASSMSG( msg.str(), ret );
        }

        // =-=-=-=-=-=-=-
        // set the outgoing variables
        _server_host = host;
        _local_flag  = host->localFlag;

        return SUCCESS();

    } // get_host_for_hier_string

}; // namespace eirods














