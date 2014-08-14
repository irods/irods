// =-=-=-=-=-=-=-
//
#include "irods_resource_plugin_impostor.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_plugin.hpp"
#include "irods_file_object.hpp"
#include "irods_physical_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_string_tokenize.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_resource_redirect.hpp"
#include "irods_stacktrace.hpp"
#include "irods_server_properties.hpp"
#include "irods_hierarchy_parser.hpp"

#include "miscServerFunct.hpp"

extern "C" {
    irods::error impostor_resource_registered_plugin(
        irods::resource_plugin_context& ) {
        return SUCCESS();
    }

    irods::error impostor_resource_unregistered_plugin(
        irods::resource_plugin_context& ) {
        return SUCCESS();
    }

    irods::error impostor_resource_modified_plugin(
        irods::resource_plugin_context& ) {
        return SUCCESS();
    }

    irods::error impostor_resource_notify_plugin(
        irods::resource_plugin_context&,
        const std::string* ) {
        return SUCCESS();
    }

    irods::error impostor_resource_get_fsfreespace_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_get_fsfreespace_plugin

    irods::error impostor_resource_create_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_create_plugin

    irods::error impostor_resource_open_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_open_plugin

    irods::error impostor_resource_read_plugin(
        irods::resource_plugin_context& _ctx,
        void*,
        int ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_read_plugin

    irods::error impostor_resource_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*,
        int ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_write_plugin

    irods::error impostor_resource_close_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_close_plugin

    irods::error impostor_resource_unlink_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_unlink_plugin

    irods::error impostor_resource_stat_plugin(
        irods::resource_plugin_context& _ctx,
        struct stat* ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_stat_plugin

    irods::error impostor_resource_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        long long ,
        int ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_lseek_plugin

    irods::error impostor_resource_mkdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_mkdir_plugin

    irods::error impostor_resource_rmdir_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_rmdir_plugin

    irods::error impostor_resource_opendir_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_opendir_plugin

    irods::error impostor_resource_closedir_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_closedir_plugin

    irods::error impostor_resource_readdir_plugin(
        irods::resource_plugin_context& _ctx,
        struct rodsDirent** ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_readdir_plugin

    irods::error impostor_resource_rename_plugin(
        irods::resource_plugin_context& _ctx,
        const char* ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_rename_plugin

    irods::error impostor_resource_truncate_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_truncate_plugin

    irods::error impostor_resource_stagetocache_plugin(
        irods::resource_plugin_context& _ctx,
        const char* ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_stagetocache_plugin

    irods::error impostor_resource_synctoarch_plugin(
        irods::resource_plugin_context& _ctx,
        char* ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_synctoarch_plugin

    // =-=-=-=-=-=-=-
    // redirect_create - code to determine redirection for create operation
    irods::error impostor_resource_redirect_create(
        irods::plugin_property_map&   _prop_map,
        const std::string&             _resc_name,
        const std::string&             _curr_host,
        float&                         _out_vote ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // determine if the resource is down
        int resc_status = 0;
        irods::error get_ret = _prop_map.get< int >( irods::RESOURCE_STATUS, resc_status );
        if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"status\" property." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // if the status is down, vote no.
            if ( INT_RESC_STATUS_DOWN == resc_status ) {
                _out_vote = 0.0;
                result.code( SYS_RESC_IS_DOWN );
                // result = PASS( result );
            }
            else {

                // =-=-=-=-=-=-=-
                // get the resource host for comparison to curr host
                std::string host_name;
                get_ret = _prop_map.get< std::string >( irods::RESOURCE_LOCATION, host_name );
                if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"location\" property." ) ).ok() ) {

                    // =-=-=-=-=-=-=-
                    // vote higher if we are on the same host
                    if ( _curr_host == host_name ) {
                        _out_vote = 1.0;
                    }
                    else {
                        _out_vote = 0.5;
                    }
                }

                rodsLog(
                    LOG_DEBUG,
                    "create :: resc name [%s] curr host [%s] resc host [%s] vote [%f]",
                    _resc_name.c_str(),
                    _curr_host.c_str(),
                    host_name.c_str(),
                    _out_vote );

            }
        }
        return result;

    } // impostor_resource_redirect_create

    // =-=-=-=-=-=-=-
    // redirect_open - code to determine redirection for open operation
    irods::error impostor_resource_redirect_open(
        irods::plugin_property_map&   _prop_map,
        irods::file_object_ptr        _file_obj,
        const std::string&             _resc_name,
        const std::string&             _curr_host,
        float&                         _out_vote ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // initially set a good default
        _out_vote = 0.0;

        // =-=-=-=-=-=-=-
        // determine if the resource is down
        int resc_status = 0;
        irods::error get_ret = _prop_map.get< int >( irods::RESOURCE_STATUS, resc_status );
        if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"status\" property." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // if the status is down, vote no.
            if ( INT_RESC_STATUS_DOWN != resc_status ) {

                // =-=-=-=-=-=-=-
                // get the resource host for comparison to curr host
                std::string host_name;
                get_ret = _prop_map.get< std::string >( irods::RESOURCE_LOCATION, host_name );
                if ( ( result = ASSERT_PASS( get_ret, "Failed to get \"location\" property." ) ).ok() ) {

                    // =-=-=-=-=-=-=-
                    // set a flag to test if were at the curr host, if so we vote higher
                    bool curr_host = ( _curr_host == host_name );

                    // =-=-=-=-=-=-=-
                    // make some flags to clarify decision making
                    bool need_repl = ( _file_obj->repl_requested() > -1 );

                    // =-=-=-=-=-=-=-
                    // set up variables for iteration
                    irods::error final_ret = SUCCESS();
                    std::vector< irods::physical_object > objs = _file_obj->replicas();
                    std::vector< irods::physical_object >::iterator itr = objs.begin();

                    // =-=-=-=-=-=-=-
                    // check to see if the replica is in this resource, if one is requested
                    for ( ; itr != objs.end(); ++itr ) {
                        // =-=-=-=-=-=-=-
                        // run the hier string through the parser and get the last
                        // entry.
                        std::string last_resc;
                        irods::hierarchy_parser parser;
                        parser.set_string( itr->resc_hier() );
                        parser.last_resc( last_resc );

                        // =-=-=-=-=-=-=-
                        // more flags to simplify decision making
                        bool repl_us  = ( _file_obj->repl_requested() == itr->repl_num() );
                        bool resc_us  = ( _resc_name == last_resc );
                        bool is_dirty = ( itr->is_dirty() != 1 );

                        // =-=-=-=-=-=-=-
                        // success - correct resource and don't need a specific
                        //           replication, or the repl nums match
                        if ( resc_us ) {
                            // =-=-=-=-=-=-=-
                            // if a specific replica is requested then we
                            // ignore all other criteria
                            if ( need_repl ) {
                                if ( repl_us ) {
                                    _out_vote = 1.0;
                                }
                                else {
                                    // =-=-=-=-=-=-=-
                                    // repl requested and we are not it, vote
                                    // very low
                                    _out_vote = 0.25;
                                }
                            }
                            else {
                                // =-=-=-=-=-=-=-
                                // if no repl is requested consider dirty flag
                                if ( is_dirty ) {
                                    // =-=-=-=-=-=-=-
                                    // repl is dirty, vote very low
                                    _out_vote = 0.25;
                                }
                                else {
                                    // =-=-=-=-=-=-=-
                                    // if our repl is not dirty then a local copy
                                    // wins, otherwise vote middle of the road
                                    if ( curr_host ) {
                                        _out_vote = 1.0;
                                    }
                                    else {
                                        _out_vote = 0.5;
                                    }
                                }
                            }

                            rodsLog(
                                LOG_DEBUG,
                                "open :: resc name [%s] curr host [%s] resc host [%s] vote [%f]",
                                _resc_name.c_str(),
                                _curr_host.c_str(),
                                host_name.c_str(),
                                _out_vote );

                            break;

                        } // if resc_us

                    } // for itr
                }
            }
            else {
                result.code( SYS_RESC_IS_DOWN );
                result = PASS( result );
            }
        }

        return result;

    } // impostor_resource_redirect_open

    // =-=-=-=-=-=-=-
    // used to allow the resource to determine which host
    // should provide the requested operation
    irods::error impostor_resource_redirect_plugin(
        irods::resource_plugin_context& _ctx,
        const std::string*                  _opr,
        const std::string*                  _curr_host,
        irods::hierarchy_parser*           _out_parser,
        float*                              _out_vote ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // check the context validity
        irods::error ret = _ctx.valid< irods::file_object >();
        if ( ( result = ASSERT_PASS( ret, "Invalid resource context." ) ).ok() ) {

            // =-=-=-=-=-=-=-
            // check incoming parameters
            if ( ( result = ASSERT_ERROR( _opr && _curr_host && _out_parser && _out_vote, SYS_INVALID_INPUT_PARAM, "Invalid input parameter." ) ).ok() ) {
                // =-=-=-=-=-=-=-
                // cast down the chain to our understood object type
                irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

                // =-=-=-=-=-=-=-
                // get the name of this resource
                std::string resc_name;
                ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
                if ( ( result = ASSERT_PASS( ret, "Failed in get property for name." ) ).ok() ) {
                    // =-=-=-=-=-=-=-
                    // add ourselves to the hierarchy parser by default
                    _out_parser->add_child( resc_name );

                    // =-=-=-=-=-=-=-
                    // test the operation to determine which choices to make
                    if ( irods::OPEN_OPERATION  == ( *_opr ) ||
                            irods::WRITE_OPERATION == ( *_opr ) ) {
                        // =-=-=-=-=-=-=-
                        // call redirect determination for 'get' operation
                        ret = impostor_resource_redirect_open( _ctx.prop_map(), file_obj, resc_name, ( *_curr_host ), ( *_out_vote ) );
                        result = ASSERT_PASS( ret, "Failed redirecting for open." );

                    }
                    else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
                        // =-=-=-=-=-=-=-
                        // call redirect determination for 'create' operation
                        ret = impostor_resource_redirect_create( _ctx.prop_map(), resc_name, ( *_curr_host ), ( *_out_vote ) );
                        result = ASSERT_PASS( ret, "Failed redirecting for create." );
                    }

                    else {
                        // =-=-=-=-=-=-=-
                        // must have been passed a bad operation
                        result = ASSERT_ERROR( false, INVALID_OPERATION, "Operation not supported." );
                    }
                }
            }
        }

        return result;

    } // impostor_resource_redirect_plugin

    irods::error impostor_resource_rebalance_plugin(
        irods::resource_plugin_context& _ctx ) {
        return update_resource_object_count(
                   _ctx.comm(),
                   _ctx.prop_map() );
    } // impostor_resource_rebalance_plugin

}; // extern "C"

namespace irods {

impostor_resource::impostor_resource(
    const std::string& _inst_name,
    const std::string& _context ) :
    resource(
        _inst_name,
        _context ) {
    wire_op( RESOURCE_OP_CREATE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_create_plugin ) );
    wire_op( RESOURCE_OP_OPEN,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_open_plugin ) );
    wire_op( RESOURCE_OP_READ,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_read_plugin ) );
    wire_op( RESOURCE_OP_WRITE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_write_plugin) );
    wire_op( RESOURCE_OP_CLOSE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_close_plugin) );
    wire_op( RESOURCE_OP_UNLINK,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_unlink_plugin) );
    wire_op( RESOURCE_OP_STAT,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_stat_plugin) );
    wire_op( RESOURCE_OP_LSEEK,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_lseek_plugin) );
    wire_op( RESOURCE_OP_MKDIR,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_mkdir_plugin) );
    wire_op( RESOURCE_OP_RMDIR,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_rmdir_plugin) );
    wire_op( RESOURCE_OP_OPENDIR,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_opendir_plugin) );
    wire_op( RESOURCE_OP_CLOSEDIR,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_closedir_plugin) );
    wire_op( RESOURCE_OP_READDIR,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_readdir_plugin) );
    wire_op( RESOURCE_OP_RENAME,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_rename_plugin) );
    wire_op( RESOURCE_OP_TRUNCATE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_truncate_plugin) );
    wire_op( RESOURCE_OP_FREESPACE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_get_fsfreespace_plugin) );
    wire_op( RESOURCE_OP_STAGETOCACHE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_stagetocache_plugin) );
    wire_op( RESOURCE_OP_SYNCTOARCH,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_synctoarch_plugin) );
    wire_op( RESOURCE_OP_REGISTERED,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_registered_plugin ) );
    wire_op( RESOURCE_OP_UNREGISTERED,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_unregistered_plugin ) );
    wire_op( RESOURCE_OP_MODIFIED,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_modified_plugin ) );
    wire_op( RESOURCE_OP_NOTIFY,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_notify_plugin ) );
    wire_op( RESOURCE_OP_REBALANCE,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_rebalance_plugin ) );
    wire_op( RESOURCE_OP_RESOLVE_RESC_HIER,
             reinterpret_cast< plugin_operation >(
                 impostor_resource_redirect_plugin ) );

    // =-=-=-=-=-=-=-
    // set some properties necessary for backporting to iRODS legacy code
    set_property< int >( RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
    set_property< int >( RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

} // ctor

error impostor_resource::wire_op(
    const std::string& _key,
    plugin_operation   _op ) {
    // =-=-=-=-=-=-=-
    // add the operation via a wrapper to the operation map
    oper_rule_exec_mgr_ptr rex_mgr(
        new operation_rule_execution_manager(
            "impostor_resource", _key ) );
    operations_[ _key ] = operation_wrapper(
                              rex_mgr,
                              "impostor_resource",
                              _key,
                              _op );
    return SUCCESS();
} // wire_op

error impostor_resource::need_post_disconnect_maintenance_operation( bool& _b ) {
    _b = false;
    return SUCCESS();
}

error impostor_resource::post_disconnect_maintenance_operation( pdmo_type& ) {
    return ERROR( -1, "nop" );
}

error impostor_resource::report_error(
    resource_plugin_context& _ctx ) {
    std::string resc_name;
    error ret = _ctx.prop_map().get< std::string >( RESOURCE_NAME, resc_name );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    std::string resc_type;
    ret = _ctx.prop_map().get< std::string >( RESOURCE_TYPE, resc_type );
    if( !ret.ok() ) {
        return PASS( ret );
    }

    std::string msg( "NOTE :: Direct Access of Impostor Resource [" );
    msg += resc_name + "] of Given Type [" + resc_type + "]";

    addRErrorMsg( &_ctx.comm()->rError, STDOUT_STATUS, msg.c_str() );

    return ERROR(
               INVALID_ACCESS_TO_IMPOSTOR_RESOURCE,
               msg );

}


}; // namespace irods

