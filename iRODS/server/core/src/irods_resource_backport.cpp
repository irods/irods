// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_stacktrace.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// helper function to convert properties from a resource plugin
// the a standard irods rescInfo_t
    error resource_to_resc_info( rescInfo_t& _info, resource_ptr& _resc ) {
        error err;
        std::string prop_name;
        // =-=-=-=-=-=-=-
        // get the resource property - host
        prop_name = RESOURCE_HOST;
        rodsServerHost_t* host = 0;
        err = _resc->get_property< rodsServerHost_t* >( prop_name, host );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - id
        prop_name = RESOURCE_ID;
        long id = 0;
        err = _resc->get_property< long >( prop_name, id );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - freespace
        prop_name = RESOURCE_FREESPACE;
        long freespace = 0;
        err = _resc->get_property< long >( prop_name, freespace );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - quota
        prop_name = RESOURCE_QUOTA;
        long quota = 0;
        err = _resc->get_property< long >( prop_name, quota );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - zone
        prop_name = RESOURCE_ZONE;
        std::string zone;
        err = _resc->get_property< std::string >( prop_name, zone );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - name
        prop_name = RESOURCE_NAME;
        std::string name;
        err = _resc->get_property< std::string >( prop_name, name );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - location
        prop_name = RESOURCE_LOCATION;
        std::string location;
        err = _resc->get_property< std::string >( prop_name, location );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - type
        prop_name = RESOURCE_TYPE;
        std::string type;
        err = _resc->get_property< std::string >( prop_name, type );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - class
        prop_name = RESOURCE_CLASS;
        std::string rclass;
        err = _resc->get_property< std::string >( prop_name, rclass );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - path
        prop_name = RESOURCE_PATH;
        std::string path;
        err = _resc->get_property< std::string >( prop_name, path );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - info
        prop_name = RESOURCE_INFO;
        std::string info;
        err = _resc->get_property< std::string >( prop_name, info );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - comments
        prop_name = RESOURCE_COMMENTS;
        std::string comments;
        err = _resc->get_property< std::string >( prop_name, comments );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - create
        prop_name = RESOURCE_CREATE_TS;
        std::string create;
        err = _resc->get_property< std::string >( prop_name, create );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - modify
        prop_name = RESOURCE_MODIFY_TS;
        std::string modify;
        err = _resc->get_property< std::string >( prop_name, modify );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // get the resource property - status
        prop_name = RESOURCE_STATUS;
        int status = 0;
        err = _resc->get_property< int >( prop_name, status );
        if ( !err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << prop_name;
            msg << "]";
            return ERROR( UNMATCHED_KEY_OR_INDEX, msg.str() );
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
        // end the linked lists and ensure sane values
        _grp_info.next      = NULL;
        _grp_info.cacheNext = NULL;
        _grp_info.status    = 0;
        _grp_info.dummy     = 0;

        // =-=-=-=-=-=-=-
        // allocate the rescinfo struct if necessary
        if ( !_grp_info.rescInfo ) {
            _grp_info.rescInfo = new rescInfo_t;
        }
        // =-=-=-=-=-=-=-
        // call earlier helper function to fill in the rescInfo_t structure
        error err = resource_to_resc_info( *_grp_info.rescInfo, _resc );
        if ( !err.ok() ) {
            return PASS( err );
        }

        // =-=-=-=-=-=-=-
        // copy the name for the resc group.  since we dont have groups anymore this
        // may cause an issue with legacy code.
        rstrcpy( _grp_info.rescGroupName, _grp_info.rescInfo->rescName, NAME_LEN );

        return SUCCESS();

    } // resource_to_resc_grp_info


/// @brief Given a resource name, checks that the resource exists and is not flagged as down
    error is_resc_live(const std::string& _resc_name) {

    	// Try to get resource status
        int status = 0;
        error resc_err = get_resource_property< int >( _resc_name, RESOURCE_STATUS, status );

        if (resc_err.ok()) {
            if ( status == INT_RESC_STATUS_DOWN ) {
                std::stringstream msg;
                msg << "Resource [";
                msg << _resc_name;
                msg << "] is down";

            	return ERROR( SYS_RESC_IS_DOWN, msg.str() );
            }
        }
        else {
            std::stringstream msg;
            msg << "Failed to get status for resource [";
            msg << _resc_name;
            msg << "]";

            return PASSMSG( msg.str(), resc_err );
        }

        return resc_err;

    } // is_resc_available


// =-=-=-=-=-=-=-
// given a list of resource names from a rule, make some decisions
// about which resource should be set to default and used.  store
// that information in the rescGrpInfo structure
// NOTE :: this is a reimplementation of setDefaultResc in resource.c but
//      :: with the composite resource spin
    error set_default_resource( rsComm_t*      _comm,   std::string   _resc_list,
                                std::string    _option, keyValPair_t* _cond_input,
                                std::string& _resc_name ) {
        // =-=-=-=-=-=-=
        // quick error check
        if ( _resc_list.empty() && NULL == _cond_input ) {
            return ERROR( USER_NO_RESC_INPUT_ERR, "no user input" );
        }

        // =-=-=-=-=-=-=-
        // resource name passed in via conditional input
        std::string cond_input_resc;

        // =-=-=-=-=-=-=-
        // if the resource list is not "null" and the forced flag is set
        // then zero out the conditional input as it is to be ignored
        if ( "null"   != _resc_list &&
                "forced" == _option    &&
                _comm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            _cond_input = NULL;
        }
        else if ( _cond_input ) {
            char* name = NULL;
            if ( ( name = getValByKey( _cond_input, BACKUP_RESC_NAME_KW ) ) == NULL &&
                    ( name = getValByKey( _cond_input, DEST_RESC_NAME_KW ) ) == NULL &&
                    ( name = getValByKey( _cond_input, DEF_RESC_NAME_KW ) ) == NULL &&
                    ( name = getValByKey( _cond_input, RESC_NAME_KW ) ) == NULL ) {
                // =-=-=-=-=-=-=-
                // no conditional input resource
            }
            else {
                cond_input_resc = name;

            }

        } // else if

        // =-=-=-=-=-=-=-
        // split the list into a vector of strings using a % delimiter
        std::vector< std::string > resources;
        string_tokenize( _resc_list, "%", resources );

        // =-=-=-=-=-=-=-
        // pick a good resource which is available out of the list,
        // NOTE :: the legacy code picks a random one first then scans
        //      :: for a valid one if the random resource is down.  i just
        //      :: scan for one.  i don't think this would break anything...
        std::string default_resc_name;

        std::vector< std::string >::iterator itr = resources.begin();
        for ( ; itr != resources.end(); ++itr ) {

        	error resc_err = is_resc_live(*itr);
        	if (resc_err.ok()) {
            	// live resource found
            	default_resc_name = *itr;
        	}
        	else {
        		irods::log(resc_err);
        	}

        } // for itr


        // =-=-=-=-=-=-=-
        // determine that we might need a 'preferred' resource
        if ( "preferred" == _option && !cond_input_resc.empty() ) {
            // =-=-=-=-=-=-=-
            // determine if the resource is live
        	error resc_err = is_resc_live(cond_input_resc);
        	if (resc_err.ok()) {
                // =-=-=-=-=-=-=-
                // we found a live one, we're good to go
        		_resc_name = cond_input_resc;
        		return SUCCESS();
        	}

        }
        else if ( "forced" == _option && _comm->clientUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
            // stick with the default found above, its forced.
        	_resc_name = default_resc_name;
            return SUCCESS();

        }
        else {
            // =-=-=-=-=-=-=-
            // try the conditional input string, if not go back to the default resource
        	error resc_err = is_resc_live(cond_input_resc);
            if ( resc_err.ok() ) {
				// =-=-=-=-=-=-=-
				// we found a live one, go!
            	_resc_name = cond_input_resc;
				return SUCCESS();

            }
            else {
                // =-=-=-=-=-=-=-
                // otherwise go back to the old, default we had before
            	error resc_err = is_resc_live(default_resc_name);
                if ( resc_err.ok() ) {
                	_resc_name = default_resc_name;
                    return SUCCESS();

                }
                else {
                    std::stringstream msg;
                    msg << "set_default_resource - failed to find default resource for list [";
                    msg << _resc_list;
                    msg << "] and option [";
                    msg << _option;
                    msg << "]";
                    return PASSMSG( msg.str(), resc_err );
                }

            } // else

        } // else


        // =-=-=-=-=-=-=-
        // should not reach here
        std::stringstream msg;
        msg << "should not reach here for list [";
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
            if ( name ) {
                _out = std::string( name );
                return SUCCESS();
            }

            name = getValByKey( _cond_input, DEST_RESC_NAME_KW );
            if ( name ) {
                _out = std::string( name );
                return SUCCESS();
            }

            name = getValByKey( _cond_input, DEF_RESC_NAME_KW );
            if ( name ) {
                _out = std::string( name );
                return SUCCESS();
            }

            return ERROR( INT_RESC_STATUS_DOWN, "failed to resolve resource name" );

        }
        else {
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
        if ( !_info ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null pointer" );
        }

        // =-=-=-=-=-=-=-
        // find a matching resource
        resource_ptr resc;
        error err = resc_mgr.resolve_from_property< rodsServerHost_t* >( RESOURCE_HOST, _info, resc );
        if ( !err.ok() ) {
            return PASSMSG( "failed to resolve resource", err );
        }

        // =-=-=-=-=-=-=-
        // get the status property of the resource
        int status = -1;
        err = resc->get_property< int >( RESOURCE_STATUS, status );
        if ( !err.ok() ) {
            return PASSMSG( "failed to get resource property", err );
        }

        return CODE( status );

    } // get_host_status_by_host_info

// =-=-=-=-=-=-=-
// helper function to save on typing - get legacy data struct
// for resource given a resource name
    error get_resc_info( std::string _name, rescInfo_t& _info ) {

        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if ( res_err.ok() ) {
            // =-=-=-=-=-=-=-
            // check to see if the resource is active, if not fail
            int status = 0;
            get_resource_property< int >( _name, RESOURCE_STATUS, status );
            if ( status == INT_RESC_STATUS_DOWN ) {
                return ERROR( SYS_RESC_IS_DOWN, "The Resource is Down" );
            }

            error info_err = resource_to_resc_info( _info, resc );
            if ( info_err.ok() ) {
                return SUCCESS();

            }
            else {
                return PASS( info_err );

            }

        }
        else {
            return PASS( res_err );

        }

    } // get_resc_info

// =-=-=-=-=-=-=-
// helper function to save on typing - get legacy data struct
// for resource group given a resource name
    error get_resc_grp_info( std::string _name, rescGrpInfo_t& _info ) {
        if ( _name.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "empty key" );
        }

        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if ( res_err.ok() ) {
            // =-=-=-=-=-=-=-
            // check to see if the resource is active, if not fail
            int status = 0;
            get_resource_property< int >( _name, RESOURCE_STATUS, status );
            if ( status == INT_RESC_STATUS_DOWN ) {
                return ERROR( SYS_RESC_IS_DOWN, "The Resource is Down" );
            }

            error info_err = resource_to_resc_grp_info( _info, resc );
            if ( info_err.ok() ) {
                return SUCCESS();
            }
            else {
                return PASS( info_err );
            }

        }
        else {
            return PASS( res_err );
        }


    } // get_resc_grp_info

    error get_host_for_hier_string(
        const std::string& _hier_str,      // hier string
        int&               _local_flag,    // local flag
        rodsServerHost_t*& _server_host ) { // server host

        // =-=-=-=-=-=-=-
        // check hier string
        if ( _hier_str.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "hier string is empty" );
        }

        // =-=-=-=-=-=-=-
        // extract the last resource in the hierarchy
        std::string resc_name;
        hierarchy_parser parse;
        parse.set_string( _hier_str );
        parse.last_resc( resc_name );

        // =-=-=-=-=-=-=-
        // check hier string
        if ( resc_name.empty() ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "resc_name string is empty" );
        }

        // =-=-=-=-=-=-=-
        // get the rods server host info for the child resc
        rodsServerHost_t* host = NULL;
        error ret = get_resource_property< rodsServerHost_t* >( resc_name, RESOURCE_HOST, host );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "get_host_for_hier_string - failed to get host property for [";
            msg << resc_name;
            msg << "]";
            return PASSMSG( msg.str(), ret );
        }

        // Check for null host.
        if ( host == NULL ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Host from hierarchy string: \"";
            msg << _hier_str;
            msg << "\" is NULL";
            return ERROR( INVALID_LOCATION, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // set the outgoing variables
        _server_host = host;
        _local_flag  = host->localFlag;

        return SUCCESS();

    } // get_host_for_hier_string

// =-=-=-=-=-=-=-
// function which returns the host name for a given hier string
    error get_loc_for_hier_string(
        const std::string& _hier,
        std::string& _loc ) {
        // =-=-=-=-=-=-=-
        // use the parser to get the leaf resc in the string
        hierarchy_parser parser;
        parser.set_string( _hier );

        std::string last_resc;
        parser.last_resc( last_resc );

        std::string location;
        error ret = get_resource_property< std::string >( last_resc, RESOURCE_LOCATION, location );
        if ( !ret.ok() ) {
            location = "";
            return PASSMSG( "get_loc_for_hier_string - failed in get_resource_property", ret );
        }

        // =-=-=-=-=-=-=-
        // set out variable and return
        _loc = location;

        return SUCCESS();

    } // get_loc_for_hier_string

/// @brief Returns the vault path of the leaf resource of the specified hierarchy string
    error get_vault_path_for_hier_string(
        const std::string& _hier_string,
        std::string& _rtn_vault_path ) {
        error result = SUCCESS();
        error ret;
        hierarchy_parser parser;
        ret = parser.set_string( _hier_string );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to parse the hierarchy string \"" << _hier_string << "\"";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            std::string last_resc;
            ret = parser.last_resc( last_resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get the last resource in the hierarchy: \"" << _hier_string << "\"";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                ret = get_resource_property<std::string>( last_resc, RESOURCE_PATH, _rtn_vault_path );
                if ( !ret.ok() ) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get the fault path property from the resource: \"" << last_resc << "\"";
                    result = PASSMSG( msg.str(), ret );
                }
            }
        }

        return result;
    }

}; // namespace irods














