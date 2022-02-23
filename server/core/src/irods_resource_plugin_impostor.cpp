// =-=-=-=-=-=-=-
//
#include "irods/irods_resource_plugin_impostor.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_physical_object.hpp"
#include "irods/irods_collection_object.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_hierarchy_parser.hpp"
#include "irods/irods_logger.hpp"
#include "irods/voting.hpp"

#include "irods/miscServerFunct.hpp"


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
// used to allow the resource to determine which host
// should provide the requested operation
irods::error impostor_file_resolve_hierarchy(
    irods::plugin_context&   _ctx,
    const std::string*       _opr,
    const std::string*       _curr_host,
    irods::hierarchy_parser* _out_parser,
    float*                   _out_vote ) {
    *_out_vote = irods::experimental::resource::voting::vote::zero;
    _out_parser->add_child(irods::get_resource_name(_ctx));
    return SUCCESS();
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

        add_operation(
            irods::RESOURCE_OP_READ,
            std::function<
                error(irods::plugin_context&,void*,const int)>(
                    impostor_file_read ) );

        add_operation(
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

        add_operation(
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

        add_operation(
            irods::RESOURCE_OP_READDIR,
            function<error(plugin_context&,struct rodsDirent**)>(
                impostor_file_readdir ) );

        add_operation(
            irods::RESOURCE_OP_RENAME,
            function<error(plugin_context&, const char*)>(
                impostor_file_rename ) );

        add_operation(
            irods::RESOURCE_OP_FREESPACE,
            function<error(plugin_context&)>(
                impostor_file_getfs_freespace ) );

        add_operation(
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

        add_operation(
            irods::RESOURCE_OP_STAGETOCACHE,
            function<error(plugin_context&, const char*)>(
                impostor_file_stage_to_cache ) );

        add_operation(
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

        add_operation(
            irods::RESOURCE_OP_NOTIFY,
            function<error(plugin_context&, const std::string*)>(
                impostor_file_notify ) );

        add_operation(
            irods::RESOURCE_OP_TRUNCATE,
            function<error(plugin_context&)>(
                impostor_file_truncate ) );

        add_operation(
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

