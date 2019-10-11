// =-=-=-=-=-=-=
// irods includes
#include "miscServerFunct.hpp"
#include "objInfo.h"
#include "dataObjCreate.h"
#include "specColl.hpp"
#include "collection.hpp"
#include "dataObjOpr.hpp"
#include "getRescQuota.h"
#include "rsDataObjCreate.hpp"
#include "rsGetRescQuota.hpp"

// =-=-=-=-=-=-=
#include "irods_resource_redirect.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_backport.hpp"


namespace irods {
/// =-=-=-=-=-=-=-
/// @brief function to handle collecting a vote from a resource
///        for a given operation and fco
    static
    error request_vote_for_file_object(
        rsComm_t*                _comm,
        const std::string&       _oper,
        const std::string&       _resc_name,
        file_object_ptr          _file_obj,
        std::string&             _out_hier,
        float&                   _out_vote ) {
        // =-=-=-=-=-=-=-
        // request the resource by name
        irods::resource_ptr resc;
        error err = resc_mgr.resolve( _resc_name, resc );
        if ( !err.ok() ) {
            return PASSMSG( "failed in resc_mgr.resolve", err );

        }

        // =-=-=-=-=-=-=-
        // if the resource has a parent, bail as this is a grave, terrible error.
        resource_ptr parent;
        error p_err = resc->get_parent( parent );
        if ( p_err.ok() ) {
            return ERROR(
                       DIRECT_CHILD_ACCESS,
                       "attempt to directly address a child resource" );
        }

        // =-=-=-=-=-=-=-
        // get current hostname, which is also done by init local server host
        char host_name_str[ MAX_NAME_LEN ];
        if ( gethostname( host_name_str, MAX_NAME_LEN ) < 0 ) {
            return ERROR( SYS_GET_HOSTNAME_ERR, "failed in gethostname" );

        }
        std::string host_name( host_name_str );

        // =-=-=-=-=-=-=-
        // query the resc given the operation for a hier string which
        // will determine the host
        hierarchy_parser parser;
        float            vote = 0.0;
        first_class_object_ptr ptr = boost::dynamic_pointer_cast< first_class_object >( _file_obj );
        err = resc->call< const std::string*, const std::string*, hierarchy_parser*, float* >(
                  _comm, RESOURCE_OP_RESOLVE_RESC_HIER, ptr, &_oper, &host_name, &parser, &vote );
        if ( !err.ok() || 0.0 == vote ) {
            std::stringstream msg;
            msg << "failed in call to redirect";
            msg << " host [" << host_name      << "] ";
            msg << " hier [" << _out_hier << "]";
            err.status( false );
            if ( err.code() == 0 ) {
                err.code( HIERARCHY_ERROR );
            }
            return PASSMSG( msg.str(), err );
        }

        // =-=-=-=-=-=-=-
        // extract the hier string from the parser, politely.
        parser.str( _out_hier );
        _out_vote = vote;

        return SUCCESS();

    } // request_vote_for_file_object

/// =-=-=-=-=-=-=-
/// @brief function to handle resolving the hier given votes of the
///        root resources for an open operation
    static
    error resolve_hier_for_open_or_write_without_keyword(
        rsComm_t*                _comm,
        irods::file_object_ptr  _file_obj,
        const std::string&       _oper,
        std::string&             _out_hier ) {
        // =-=-=-=-=-=-=-
        // build a list of root hiers for all
        // the repls we have in the list
        std::map< std::string, float > root_map;

        // =-=-=-=-=-=-=-
        // grind through the list, get the root of the hiers and
        // place it into the map
        std::vector< physical_object > repls = _file_obj->replicas();
        for ( size_t i = 0; i < repls.size(); ++i ) {
            // =-=-=-=-=-=-=-
            // extract the root resource from the hierarchy
            hierarchy_parser parser;
            parser.set_string( repls[ i ].resc_hier() );

            std::string      root_resc;
            parser.first_resc( root_resc );
            root_map[ root_resc ] = 0.0;

        } // for i

        // =-=-=-=-=-=-=-
        // grind through the map and get a vote for each root
        // cache that and keep track of the max
        std::string max_hier;
        float       max_vote = -1.0;
        std::map< std::string, float >::iterator itr = root_map.begin();
        for ( ; itr != root_map.end(); ++itr ) {
            // =-=-=-=-=-=-=-
            // request the vote
            float       vote = 0.0;
            std::string voted_hier;
            irods::error ret = request_vote_for_file_object(
                                   _comm,
                                   _oper.c_str(),
                                   itr->first,
                                   _file_obj,
                                   voted_hier,
                                   vote );
            if ( ret.ok() ) {
                // =-=-=-=-=-=-=-
                // assign the vote to the root
                itr->second = vote;

                // =-=-=-=-=-=-=-
                // keep track of max vote, hier and resc name
                if ( vote > max_vote ) {
                    max_vote = vote;
                    max_hier = voted_hier;
                }
            }

        } // for itr

        // =-=-=-=-=-=-=-
        // if we have a max vote of 0.0 then
        // this is an error
        double diff = ( max_vote - 0.00000001 );
        if ( diff <= 0.0 ) {
            return ERROR(
                       HIERARCHY_ERROR,
                       "no valid resource found for data object" );
        }

        // =-=-=-=-=-=-=-
        // set out variables
        _out_hier = max_hier;

        return SUCCESS();

    } // resolve_hier_for_open_or_write_without_keyword

/// =-=-=-=-=-=-=-
/// @brief function to handle resolving the hier given the fco and
///        resource keyword
    static
    error resolve_hier_for_open_or_write(
        rsComm_t*                _comm,
        irods::file_object_ptr  _file_obj,
        const char*              _key_word,
        const std::string&       _oper,
        std::string&             _out_hier ) {
        // =-=-=-=-=-=-=-
        // regardless we need to resolve the appropriate resource
        // to do the voting so search the repls for the proper resc
        std::vector< physical_object > repls = _file_obj->replicas();

        bool kw_match_found = false;
        if ( _key_word ) {
            // =-=-=-=-=-=-=-
            // we have a kw present, compare against all the repls for a match
            for ( size_t i = 0; i < repls.size(); ++i ) {
                // =-=-=-=-=-=-=-
                // extract the root resource from the hierarchy
                std::string      root_resc;
                hierarchy_parser parser;
                parser.set_string( repls[ i ].resc_hier() );
                parser.first_resc( root_resc );

                // =-=-=-=-=-=-=-
                // if we have a match then set open & break, otherwise continue
                if ( root_resc == _key_word ) {
                    _file_obj->resc_hier( repls[ i ].resc_hier() );
                    kw_match_found = true;
                    break;
                }

            } // for i

            // =-=-=-=-=-=-=-
            // if a match is found, resolve it and get the hier string
            if ( kw_match_found ) {
                float vote = 0.0;
                error ret = request_vote_for_file_object(
                                _comm,
                                _oper.c_str(),
                                _key_word,
                                _file_obj,
                                _out_hier,
                                vote );
                if ( 0.0 == vote ) {
                    if ( ret.code() == 0 ) {
                        ret.code( -1 );
                    }
                    ret.status( false );
                }

                return PASS( ret );

            } // if kw_match_found

            // =-=-=-=-=-=-=-
            // NOTE:: if a kw match is not found is this an
            //        error or is falling through acceptable
        }

        // =-=-=-=-=-=-=-
        // either no kw match or no kw, so pick one...
        return resolve_hier_for_open_or_write_without_keyword(
                   _comm,
                   _file_obj,
                   _oper,
                   _out_hier );

    } // resolve_hier_for_open_or_write

/// =-=-=-=-=-=-=-
/// @brief function to handle resolving the hier given the fco and
///        resource keyword
    static
    error resolve_hier_for_create(
        rsComm_t*                _comm,
        irods::file_object_ptr  _file_obj,
        const char*              _key_word,
        dataObjInp_t*            _data_obj_inp,
        std::string&             _out_hier ) {
        // =-=-=-=-=-=-=-
        // handle the create operation
        // check for incoming requested destination resource first
        std::string resc_name;
        if ( !_key_word ) {
            // =-=-=-=-=-=-=-
            // this is a 'create' operation and no resource is specified,
            // query the server for the default or other resource to use
            int status = getRescForCreate( _comm, _data_obj_inp, resc_name );
            if ( status < 0 || resc_name.empty() ) {
                // =-=-=-=-=-=-=-
                return ERROR( status, "failed in getRescForCreate" );
            }
        }
        else {
            resc_name = _key_word;

        }

        // =-=-=-=-=-=-=-
        // set the resc hier given the root resc name
        _file_obj->resc_hier( resc_name );

        // =-=-=-=-=-=-=-
        // get a vote and hier for the create
        float vote = 0.0;
        error ret = request_vote_for_file_object(
                        _comm,
                        CREATE_OPERATION,
                        resc_name,
                        _file_obj,
                        _out_hier,
                        vote );
        if ( 0.0 == vote ) {
            if ( ret.code() == 0 ) {
                ret.code( HIERARCHY_ERROR );
            }
            ret.status( false );
        }

        return PASS( ret );

    } // resolve_hier_for_create

/// =-=-=-=-=-=-=-
/// @brief function to handle resolving the hier given the fco and
///        resource keyword for create or open depending on the keyword
    static
    error resolve_hier_for_create_or_open(
        rsComm_t*                _comm,
        irods::file_object_ptr  _file_obj,
        const char*              _key_word,
        dataObjInp_t*            _data_obj_inp,
        std::string&             _out_hier ) {
        // =-=-=-=-=-=-=-
        // regardless we need to resolve the appropriate resource
        // to do the voting so search the repls for the proper resc
        std::vector< physical_object > repls = _file_obj->replicas();
        bool kw_match_found = false;
        if ( _key_word ) {
            // =-=-=-=-=-=-=-
            // we have a kw present, compare against all the repls for a match
            for ( size_t i = 0; i < repls.size(); ++i ) {
                // =-=-=-=-=-=-=-
                // extract the root resource from the hierarchy
                std::string      root_resc;
                hierarchy_parser parser;
                parser.set_string( repls[ i ].resc_hier() );
                parser.first_resc( root_resc );

                // =-=-=-=-=-=-=-
                // if we have a match then set open & break, otherwise continue
                if ( root_resc == _key_word ) {
                    _file_obj->resc_hier( repls[ i ].resc_hier() );
                    kw_match_found = true;
                    break;
                }

            } // for i

            // =-=-=-=-=-=-=-
            // if a match is found, resolve it and get the hier string
            if ( kw_match_found ) {
                float vote = 0.0;
                error ret = request_vote_for_file_object(
                                _comm,
                                WRITE_OPERATION,
                                _key_word,
                                _file_obj,
                                _out_hier,
                                vote );
                if ( 0.0 == vote ) {
                    if ( ret.code() == 0 ) {
                        ret.code( -1 );
                    }
                    ret.status( false );
                }

                return PASS( ret );

            } // if kw_match_found

            // =-=-=-=-=-=-=-
            // NOTE:: if a kw match is not found is this an
            //        error or is falling through acceptable
        }

        // =-=-=-=-=-=-=-
        // either no kw match or no kw, so pick one...
        return resolve_hier_for_create(
                   _comm,
                   _file_obj,
                   _key_word,
                   _data_obj_inp,
                   _out_hier );

    } // resolve_hier_for_create_or_open

/// =-=-=-=-=-=-=-
/// @brief determine if a forced write is to be done to a destination resource
///        which does not have an existing replica of the data object
    error determine_force_write_to_new_resource(
        const std::string& _oper,
        file_object_ptr&   _fobj,
        dataObjInp_t*      _data_obj_inp ) {

        char* dst_resc_kw   = getValByKey( &_data_obj_inp->condInput, DEST_RESC_NAME_KW );
        char* force_flag_kw = getValByKey( &_data_obj_inp->condInput, FORCE_FLAG_KW );
        std::vector<physical_object> repls = _fobj->replicas();
        if ( PUT_OPR != _data_obj_inp->oprType ||
                repls.empty()  ||
                !dst_resc_kw   ||
                !force_flag_kw ||
                strlen( dst_resc_kw ) == 0 ||
                !( OPEN_OPERATION  == _oper ||
                   WRITE_OPERATION == _oper ) ) {
            return SUCCESS();
        }

        bool hier_match_flg = false;
        for ( size_t i = 0; i < repls.size(); ++i ) {
            // =-=-=-=-=-=-=-
            // extract the root resource from the hierarchy
            hierarchy_parser parser;
            parser.set_string( repls[ i ].resc_hier() );

            std::string root_resc;
            parser.first_resc( root_resc );
            if ( root_resc == dst_resc_kw ) {
                hier_match_flg = true;
                break;
            }

        } // for i

        if ( !hier_match_flg ) {
            std::stringstream msg;
            msg << "cannot force put ["
                << _data_obj_inp->objPath
                << "] to a different resource ["
                << dst_resc_kw
                << "]";
            return ERROR(
                       HIERARCHY_ERROR,
                       msg.str() );
        }

        return SUCCESS();

    } // determine_force_write_to_new_resource

    // iterate over the linked list and look for a given resource hierarchy
    bool is_hier_in_obj_info_list(
            const std::string&    _hier,
            dataObjInfo_t*        _data_obj_info_head) {

        dataObjInfo_t* data_obj_info = _data_obj_info_head;

        while (data_obj_info) {
            if (!strcmp(_hier.c_str(), data_obj_info->rescHier)) {
                return true;
            }
            data_obj_info = data_obj_info->next;
        }
        return false;
    }

    int apply_policy_for_create_operation(
	rsComm_t*     _comm,
	dataObjInp_t* _obj_inp,
	std::string&  _resc_name ) {

	/* query rcat for resource info and sort it */
	ruleExecInfo_t rei;
	initReiWithDataObjInp( &rei, _comm, _obj_inp );

	int status = 0;
	if ( _obj_inp->oprType == REPLICATE_OPR ) {
	    status = applyRule( "acSetRescSchemeForRepl", NULL, &rei, NO_SAVE_REI );
	}
	else {
	    status = applyRule( "acSetRescSchemeForCreate", NULL, &rei, NO_SAVE_REI );
	}
    clearKeyVal(rei.condInputData);
    free(rei.condInputData);

	    if ( status < 0 ) {
		    if ( rei.status < 0 ) {
			    status = rei.status;
		    }

		    rodsLog(
				    LOG_NOTICE,
				    "getRescForCreate:acSetRescSchemeForCreate error for %s,status=%d",
				    _obj_inp->objPath,
				    status );

		    return status;
	    }

	    // get resource name
	    if ( !strlen( rei.rescName ) ) {
		 irods::error set_err = irods::set_default_resource(
				    _comm,
				    "", "",
				    &_obj_inp->condInput,
				    _resc_name );
		    if ( !set_err.ok() ) {
			    irods::log( PASS( set_err ) );
			    return SYS_INVALID_RESC_INPUT;
		    }
	    }
	    else {
		    _resc_name = rei.rescName;
	    }

	    status = setRescQuota(
			    _comm,
			    _obj_inp->objPath,
			    _resc_name.c_str(),
			    _obj_inp->dataSize );
	    if( status == SYS_RESC_QUOTA_EXCEEDED ) {
		    return SYS_RESC_QUOTA_EXCEEDED;
	    }


	    return 0;
    }

/// =-=-=-=-=-=-=-
/// @brief function to query resource for chosen server to which to redirect
///       for a given operation
    error resolve_resource_hierarchy(
        const std::string&   _oper,
        rsComm_t*            _comm,
        dataObjInp_t*        _data_obj_inp,
        std::string&         _out_hier,
        dataObjInfo_t**      _data_obj_info ) {
        // =-=-=-=-=-=-=-
        // validate incoming parameters
        if ( !_comm ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null comm pointer" );
        }
        else if ( !_data_obj_inp ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null data obj inp pointer" );
        }

        // =-=-=-=-=-=-=-
        // cache the operation, as we may need to modify it
        std::string oper = _oper;

        // =-=-=-=-=-=-=-
        // if this is a put operation then we do not have a first class object
        file_object_ptr file_obj(
            new file_object( ) );


        // =-=-=-=-=-=-=-
        // if this is a special collection then we need to get the hier
        // pass that along and bail as it is not a data object, or if
        // it is just a not-so-special collection then we continue with
        // processing the operation, as this may be a create op
        rodsObjStat_t *rodsObjStatOut = NULL;
        int spec_stat = collStat( _comm, _data_obj_inp, &rodsObjStatOut );
        file_obj->logical_path( _data_obj_inp->objPath );
        if ( spec_stat >= 0 ) {
            if ( rodsObjStatOut->specColl != NULL ) {
                _out_hier = rodsObjStatOut->specColl->rescHier;
                freeRodsObjStat( rodsObjStatOut );
                return SUCCESS();
            }

        }
        freeRodsObjStat( rodsObjStatOut );

        // =-=-=-=-=-=-=-
        // extract the resc name keyword from the conditional input
        char* back_up_resc_name  = getValByKey( &_data_obj_inp->condInput, BACKUP_RESC_NAME_KW );
        char* dest_resc_name     = getValByKey( &_data_obj_inp->condInput, DEST_RESC_NAME_KW );
        char* default_resc_name  = getValByKey( &_data_obj_inp->condInput, DEF_RESC_NAME_KW );
        char* resc_name          = getValByKey( &_data_obj_inp->condInput, RESC_NAME_KW );

        // =-=-=-=-=-=-=-
        // assign the keyword in an order, if it applies
        char* key_word = 0;
        if ( resc_name ) {
            key_word = resc_name;
        }
        else if ( dest_resc_name ) {
            key_word = dest_resc_name;
        }
        else if ( back_up_resc_name ) {
            key_word = back_up_resc_name;
        }


        // =-=-=-=-=-=-=-
        // call factory for given dataObjInp, get a file_object
        error fac_err = file_object_factory( _comm, _data_obj_inp, file_obj, _data_obj_info );

        // =-=-=-=-=-=-=-
        // determine if this is an invalid write
        error ret = determine_force_write_to_new_resource(
                        oper,
                        file_obj,
                        _data_obj_inp );
        if ( !ret.ok() ) {
            return PASS( ret );

        }

        // =-=-=-=-=-=-=-
        // perform an open operation if create is not specified ( thats all we have for now )
        if ( OPEN_OPERATION  == oper ||
                WRITE_OPERATION == oper ||
                UNLINK_OPERATION == oper ) {
            // =-=-=-=-=-=-=-
            // reality check: if the key_word is set, verify that the resource
            // actually exists before moving forward.
            if ( key_word ) {
                resource_ptr resc;
                error ret = resc_mgr.resolve( key_word, resc );
                if ( !ret.ok() ) {
                    return PASS( ret );
                }
            }

            // =-=-=-=-=-=-=-
            // factory has already been called, test for
            // success before proceeding
            if ( !fac_err.ok() ) {
                std::stringstream msg;
                msg << "resolve_resource_hierarchy :: failed in file_object_factory";
                return PASSMSG( msg.str(), fac_err );
            }

            // =-=-=-=-=-=-=-
            // consider force flag - we need to consider the default
            // resc if -f is specified
            char* force_flag = getValByKey( &_data_obj_inp->condInput, FORCE_FLAG_KW );
            if ( force_flag &&
                    !key_word ) {
                key_word = default_resc_name;
            }

            // =-=-=-=-=-=-=-
            // attempt to resolve for an open
            _out_hier = "";
            error ret = resolve_hier_for_open_or_write(
                            _comm,
                            file_obj,
                            key_word,
                            oper,
                            _out_hier );
            if (!ret.ok()) {
                return PASSMSG("Failed to resolve resource hierarchy", ret);
            }

            // make sure the desired hierarchy is in the linked list
            // otherwise get data object info list again,
            // in case object was just staged to cache by above function
            if (_data_obj_info && !is_hier_in_obj_info_list(_out_hier, *_data_obj_info)) {

                // free current list
                freeAllDataObjInfo(*_data_obj_info);
                *_data_obj_info = NULL;

                // get updated list
                int status = getDataObjInfoIncSpecColl(
                        _comm,
                        _data_obj_inp,
                        _data_obj_info);

                // error checks
                if ( status < 0 ) {
                    status = getDataObjInfo( _comm, _data_obj_inp, _data_obj_info, 0, 0 );
                }
                if ( 0 == *_data_obj_info || status < 0 ) {
                    if ( *_data_obj_info ) {
                        freeAllDataObjInfo( *_data_obj_info );
                    }
                    std::stringstream msg;
                    msg << "Failed to retrieve data object info list for ["
                        << _data_obj_inp->objPath
                        << "]";
                    return ERROR( HIERARCHY_ERROR, msg.str() );
                }

                // check that we have found the correct hierarchy this time
                if (!is_hier_in_obj_info_list(_out_hier, *_data_obj_info)) {
                    std::stringstream msg;
                    msg << "Failed to find resource hierarchy ["
                        << _out_hier
                        << "for ["
                        << _data_obj_inp->objPath
                        << "]";
                    return ERROR( HIERARCHY_ERROR, msg.str() );
                }
            }
            /////////////////////////
            /////////////////////////


            return ret;

        }
        else if ( CREATE_OPERATION == oper ) {
            std::string create_resc_name;
            // =-=-=-=-=-=-=-
            // include the default resc name if it applies
            if ( !key_word && default_resc_name ) {
                create_resc_name = default_resc_name;

            }
            else if( key_word ) {
                create_resc_name = key_word;
            }
            int status = apply_policy_for_create_operation(
                             _comm,
                             _data_obj_inp,
                             create_resc_name );
            if( status < 0 ) {
                return ERROR(
                           status,
                           "apply_policy_for_create_operation failed");
            }

            // =-=-=-=-=-=-=-
            // if we have valid data objects then this could
            // be actually an open rather than a pure create
            error ret = SUCCESS();
            if ( fac_err.ok() ) {
                ret = resolve_hier_for_create_or_open(
                          _comm,
                          file_obj,
                          create_resc_name.c_str(),
                          _data_obj_inp,
                          _out_hier );
            }
            else {
                // =-=-=-=-=-=-=-
                // attempt to resolve for a create
                ret = resolve_hier_for_create(
                          _comm,
                          file_obj,
                          create_resc_name.c_str(),
                          _data_obj_inp,
                          _out_hier );
            }
            if (!ret.ok()) {
                return PASSMSG("Failed to resolve resource hierarchy", ret);
            }

            // make sure the desired hierarchy is in the linked list
            // otherwise get data object info list again,
            // in case object was just staged to cache by above function
            if (_data_obj_info && !is_hier_in_obj_info_list(_out_hier, *_data_obj_info)) {

                // free current list
                freeAllDataObjInfo(*_data_obj_info);
                *_data_obj_info = NULL;

                // get updated list
                int status = getDataObjInfoIncSpecColl(
                        _comm,
                        _data_obj_inp,
                        _data_obj_info);

                // error checks
                if ( status < 0 ) {
                    status = getDataObjInfo( _comm, _data_obj_inp, _data_obj_info, 0, 0 );
                }
                if ( 0 == *_data_obj_info || status < 0 ) {
                    if ( *_data_obj_info ) {
                        freeAllDataObjInfo( *_data_obj_info );
                    }
                    std::stringstream msg;
                    msg << "Failed to retrieve data object info list for ["
                        << _data_obj_inp->objPath
                        << "]";
                    return ERROR( HIERARCHY_ERROR, msg.str() );
                }

                // check that we have found the correct hierarchy this time
                if (!is_hier_in_obj_info_list(_out_hier, *_data_obj_info)) {
                    std::stringstream msg;
                    msg << "Failed to find resource hierarchy ["
                        << _out_hier
                        << "for ["
                        << _data_obj_inp->objPath
                        << "]";
                    return ERROR( HIERARCHY_ERROR, msg.str() );
                }
            }
            /////////////////////////
            /////////////////////////

            return ret;

        } // else

        // =-=-=-=-=-=-=-
        // should not get here
        std::stringstream msg;
        msg << "operation not supported ["
            << oper
            << "]";
        return ERROR( -1, msg.str() );

    } // resolve_resource_hierarchy

// =-=-=-=-=-=-=-
// @brief function to query resource for chosen server to which to redirect
// for a given operation
    error resource_redirect( const std::string&   _oper,
                             rsComm_t*            _comm,
                             dataObjInp_t*        _data_obj_inp,
                             std::string&         _out_hier,
                             rodsServerHost_t*&   _out_host,
                             int&                 _out_flag,
                             dataObjInfo_t**      _data_obj_info ) {
        // =-=-=-=-=-=-=-
        // default to local host if there is a failure
        _out_flag = LOCAL_HOST;

        // =-=-=-=-=-=-=-
        // resolve the resource hierarchy for this given operation and dataObjInp
        std::string resc_hier;
        error res_err = resolve_resource_hierarchy(
                            _oper,
                            _comm,
                            _data_obj_inp,
                            resc_hier,
                            _data_obj_info );
        if ( !res_err.ok() ) {
            std::stringstream msg;
            msg << "resource_redirect - failed to resolve resource hierarchy for [";
            msg << _data_obj_inp->objPath;
            msg << "]";
            return PASSMSG( msg.str(), res_err );

        }

        // =-=-=-=-=-=-=-
        // we may have an empty hier due to special collections and other
        // unfortunate cases which we cannot control, check the hier string
        // and if it is empty return success ( for now )
        if ( resc_hier.empty() ) {
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // parse out the leaf resource id for redirection
        rodsLong_t resc_id = 0;
        irods::error ret = resc_mgr.hier_to_leaf_id(resc_hier,resc_id);
        if( !ret.ok() ) {
            return PASS(ret);
        }

        // =-=-=-=-=-=-=-
        // get the host property from the last resc and get the
        // host name from that host
        rodsServerHost_t* last_resc_host = NULL;
        error err = get_resource_property< rodsServerHost_t* >(
                        resc_id,
                        RESOURCE_HOST,
                        last_resc_host );
        if ( !err.ok() || NULL == last_resc_host ) {
            std::stringstream msg;
            msg << "resource_redirect :: failed in get_resource_property call ";
            msg << "for [" << resc_hier << "]";
            return PASSMSG( msg.str(), err );
        }


        // =-=-=-=-=-=-=-
        // get current hostname, which is also done by init local server host
        char host_name_char[ MAX_NAME_LEN ];
        if ( gethostname( host_name_char, MAX_NAME_LEN ) < 0 ) {
            return ERROR( SYS_GET_HOSTNAME_ERR, "failed in gethostname" );

        }

        std::string host_name( host_name_char );

        // =-=-=-=-=-=-=
        // iterate over the list of hostName_t* and see if any match our
        // host name.  if we do, then were local
        bool        match_flg = false;
        hostName_t* tmp_host  = last_resc_host->hostName;
        while ( tmp_host ) {
            std::string name( tmp_host->name );
            if ( name.find( host_name ) != std::string::npos ) {
                match_flg = true;
                break;

            }

            tmp_host = tmp_host->next;

        } // while tmp_host

        // =-=-=-=-=-=-=-
        // are we really, really local?
        if ( match_flg ) {
            _out_hier = resc_hier;
            _out_flag = LOCAL_HOST;
            _out_host = 0;
            return SUCCESS();
        }

        // =-=-=-=-=-=-=-
        // it was not a local resource so then do a svr to svr connection
        int conn_err = svrToSvrConnect( _comm, last_resc_host );
        if ( conn_err < 0 ) {
            return ERROR( conn_err, "failed in svrToSvrConnect" );
        }


        // =-=-=-=-=-=-=-
        // return with a hier string and new connection as remote host
        _out_hier = resc_hier;
        _out_host = last_resc_host;
        _out_flag = REMOTE_HOST;

        return SUCCESS();

    } // resource_redirect

}; // namespace irods
