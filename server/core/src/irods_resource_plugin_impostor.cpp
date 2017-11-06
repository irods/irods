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


irods::error impostor_file_registered(
    irods::plugin_context& ) {
    return SUCCESS();
}

irods::error impostor_file_unregistered(
    irods::plugin_context& ) {
    return SUCCESS();
}

irods::error impostor_file_modified(
    irods::plugin_context& ) {
    return SUCCESS();
}

irods::error impostor_file_notify(
    irods::plugin_context&,
    const std::string* ) {
    return SUCCESS();
}

irods::error impostor_file_getfs_freespace(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_getfs_freespace

irods::error impostor_file_create(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_create

irods::error impostor_file_open(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_open

irods::error impostor_file_read(
    irods::plugin_context& _ctx,
    void*,
    const int ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_read

irods::error impostor_file_write(
    irods::plugin_context& _ctx,
    const void*,
    const int ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_write

irods::error impostor_file_close(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_close

irods::error impostor_file_unlink(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_unlink

irods::error impostor_file_stat(
    irods::plugin_context& _ctx,
    struct stat* ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_stat

irods::error impostor_file_lseek(
    irods::plugin_context& _ctx,
    const long long ,
    const int ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_lseek

irods::error impostor_file_mkdir(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_mkdir

irods::error impostor_file_rmdir(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_rmdir

irods::error impostor_file_opendir(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_opendir

irods::error impostor_file_closedir(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_closedir

irods::error impostor_file_readdir(
    irods::plugin_context& _ctx,
    struct rodsDirent** ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_readdir

irods::error impostor_file_rename(
    irods::plugin_context& _ctx,
    const char* ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_rename

irods::error impostor_file_truncate(
    irods::plugin_context& _ctx ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_truncate

irods::error impostor_file_stage_to_cache(
    irods::plugin_context& _ctx,
    const char* ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_stage_to_cache

irods::error impostor_file_sync_to_arch(
    irods::plugin_context& _ctx,
    const char* ) {
    return irods::impostor_resource::report_error( _ctx );
} // impostor_file_sync_to_arch

// =-=-=-=-=-=-=-
// redirect_create - code to determine redirection for create operation
irods::error impostor_file_resolve_hierarchy_create(
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

} // impostor_file_resolve_hierarchy_create

// =-=-=-=-=-=-=-
// resolve_hierarchy_open - code to determine redirection for open operation
irods::error impostor_file_resolve_hierarchy_open(
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

} // impostor_file_resolve_hierarchy_open

// =-=-=-=-=-=-=-
// used to allow the resource to determine which host
// should provide the requested operation
irods::error impostor_file_resolve_hierarchy(
    irods::plugin_context& _ctx,
    const std::string*                  _opr,
    const std::string*                  _curr_host,
    irods::hierarchy_parser*           _out_parser,
    float*                              _out_vote ) {

    // =-=-=-=-=-=-=-
    // check the context validity
    irods::error ret = _ctx.valid< irods::file_object >();
    if ( !ret.ok() ) {
        return PASSMSG( "Invalid resource context.", ret );
    }

    // =-=-=-=-=-=-=-
    // check incoming parameters
    if( NULL == _opr || NULL == _curr_host || NULL == _out_parser || NULL == _out_vote ) {
        return ERROR( SYS_INVALID_INPUT_PARAM, "Invalid input parameter." );
    }

    // =-=-=-=-=-=-=-
    // get the name of this resource
    std::string resc_name;
    ret = _ctx.prop_map().get< std::string >( irods::RESOURCE_NAME, resc_name );
    if ( !ret.ok() ) {
        return PASSMSG( "Failed in get property for name.", ret );
    }

    // =-=-=-=-=-=-=-
    // add ourselves to the hierarchy parser by default
    _out_parser->add_child( resc_name );

    // =-=-=-=-=-=-=-
    // test the operation to determine which choices to make
    if ( irods::OPEN_OPERATION  == ( *_opr ) ||
            irods::WRITE_OPERATION == ( *_opr ) ) {
        // =-=-=-=-=-=-=-
        // cast down the chain to our understood object type
        irods::file_object_ptr file_obj = boost::dynamic_pointer_cast< irods::file_object >( _ctx.fco() );

        // =-=-=-=-=-=-=-
        // call redirect determination for 'open' operation
        ret = impostor_file_resolve_hierarchy_open( _ctx.prop_map(), file_obj, resc_name, ( *_curr_host ), ( *_out_vote ) );
        if ( !ret.ok() ) {
            ret = PASSMSG( "Failed redirecting for open.", ret );
        }
    }
    else if ( irods::CREATE_OPERATION == ( *_opr ) ) {
        // =-=-=-=-=-=-=-
        // call redirect determination for 'create' operation
        ret = impostor_file_resolve_hierarchy_create( _ctx.prop_map(), resc_name, ( *_curr_host ), ( *_out_vote ) );
        if ( !ret.ok() ) {
            ret = PASSMSG( "Failed redirecting for create.", ret );
        }
    }
    else {
        // =-=-=-=-=-=-=-
        // must have been passed a bad operation
        ret = ERROR( INVALID_OPERATION, "Operation not supported." );
    }

    return ret;

} // impostor_file_resolve_hierarchy

irods::error impostor_file_rebalance(
    irods::plugin_context& _ctx ) {
    return SUCCESS();
} // impostor_file_rebalance_plugin

namespace irods {

    impostor_resource::impostor_resource(
        const std::string& _inst_name,
        const std::string& _context ) :
        resource(
            _inst_name,
            _context ) {
        using namespace irods;
        using namespace std;
        add_operation(
            RESOURCE_OP_CREATE,
            function<error(plugin_context&)>(
                impostor_file_create ) );

        add_operation(
            irods::RESOURCE_OP_OPEN,
            function<error(plugin_context&)>(
                impostor_file_open ) );

        add_operation<void*,const int>(
            irods::RESOURCE_OP_READ,
            std::function<
                error(irods::plugin_context&,void*,const int)>(
                    impostor_file_read ) );

        add_operation<const void*,const int>(
            irods::RESOURCE_OP_WRITE,
            function<error(plugin_context&,const void*,const int)>(
                impostor_file_write ) );

        add_operation(
            RESOURCE_OP_CLOSE,
            function<error(plugin_context&)>(
                impostor_file_close ) );

        add_operation(
            irods::RESOURCE_OP_UNLINK,
            function<error(plugin_context&)>(
                impostor_file_unlink ) );

        add_operation<struct stat*>(
            irods::RESOURCE_OP_STAT,
            function<error(plugin_context&, struct stat*)>(
                impostor_file_stat ) );

        add_operation(
            irods::RESOURCE_OP_MKDIR,
            function<error(plugin_context&)>(
                impostor_file_mkdir ) );

        add_operation(
            irods::RESOURCE_OP_OPENDIR,
            function<error(plugin_context&)>(
                impostor_file_opendir ) );

        add_operation<struct rodsDirent**>(
            irods::RESOURCE_OP_READDIR,
            function<error(plugin_context&,struct rodsDirent**)>(
                impostor_file_readdir ) );

        add_operation<const char*>(
            irods::RESOURCE_OP_RENAME,
            function<error(plugin_context&, const char*)>(
                impostor_file_rename ) );

        add_operation(
            irods::RESOURCE_OP_FREESPACE,
            function<error(plugin_context&)>(
                impostor_file_getfs_freespace ) );

        add_operation<const long long, const int>(
            irods::RESOURCE_OP_LSEEK,
            function<error(plugin_context&, long long, int)>(
                impostor_file_lseek ) );

        add_operation(
            irods::RESOURCE_OP_RMDIR,
            function<error(plugin_context&)>(
                impostor_file_rmdir ) );

        add_operation(
            irods::RESOURCE_OP_CLOSEDIR,
            function<error(plugin_context&)>(
                impostor_file_closedir ) );

        add_operation<const char*>(
            irods::RESOURCE_OP_STAGETOCACHE,
            function<error(plugin_context&, const char*)>(
                impostor_file_stage_to_cache ) );

        add_operation<const char*>(
            irods::RESOURCE_OP_SYNCTOARCH,
            function<error(plugin_context&, const char*)>(
                impostor_file_sync_to_arch ) );

        add_operation(
            irods::RESOURCE_OP_REGISTERED,
            function<error(plugin_context&)>(
                impostor_file_registered ) );

        add_operation(
            irods::RESOURCE_OP_UNREGISTERED,
            function<error(plugin_context&)>(
                impostor_file_unregistered ) );

        add_operation(
            irods::RESOURCE_OP_MODIFIED,
            function<error(plugin_context&)>(
                impostor_file_modified ) );

        add_operation<const std::string*>(
            irods::RESOURCE_OP_NOTIFY,
            function<error(plugin_context&, const std::string*)>(
                impostor_file_notify ) );

        add_operation(
            irods::RESOURCE_OP_TRUNCATE,
            function<error(plugin_context&)>(
                impostor_file_truncate ) );

        add_operation<const std::string*, const std::string*, irods::hierarchy_parser*, float*>(
            irods::RESOURCE_OP_RESOLVE_RESC_HIER,
            function<error(plugin_context&,const std::string*, const std::string*, irods::hierarchy_parser*, float*)>(
                impostor_file_resolve_hierarchy ) );

        add_operation(
            irods::RESOURCE_OP_REBALANCE,
            function<error(plugin_context&)>(
                impostor_file_rebalance ) );

        // =-=-=-=-=-=-=-
        // set some properties necessary for backporting to iRODS legacy code
        set_property< int >( RESOURCE_CHECK_PATH_PERM, 2 );//DO_CHK_PATH_PERM );
        set_property< int >( RESOURCE_CREATE_PATH,     1 );//CREATE_PATH );

    } // ctor

    error impostor_resource::need_post_disconnect_maintenance_operation( bool& _b ) {
        _b = false;
        return SUCCESS();
    }

    error impostor_resource::post_disconnect_maintenance_operation( pdmo_type& ) {
        return ERROR( -1, "nop" );
    }

    error impostor_resource::report_error(
        plugin_context& _ctx ) {
        std::string resc_name;
        error ret = _ctx.prop_map().get< std::string >( RESOURCE_NAME, resc_name );
        if ( !ret.ok() ) {
            return PASS( ret );
        }

        std::string resc_type;
        ret = _ctx.prop_map().get< std::string >( RESOURCE_TYPE, resc_type );
        if ( !ret.ok() ) {
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

