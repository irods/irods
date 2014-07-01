

// =-=-=-=-=-=-=-
//
#include "irods_resource_plugin_impostor.hpp"
#include "irods_hierarchy_parser.hpp"

extern "C" {
    irods::error impostor_resource_registered_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    }

    irods::error impostor_resource_unregistered_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    }

    irods::error impostor_resource_modified_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    }

    irods::error impostor_resource_notify_plugin(
        irods::resource_plugin_context& _ctx,
        const std::string*               _opr ) {
        return irods::impostor_resource::report_error( _ctx );
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
        void*                           _buf,
        int                             _len ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_read_plugin

    irods::error impostor_resource_write_plugin(
        irods::resource_plugin_context& _ctx,
        void*                           _buf,
        int                             _len ) {
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
        struct stat*                    _statbuf ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_stat_plugin

    irods::error impostor_resource_lseek_plugin(
        irods::resource_plugin_context& _ctx,
        long long                       _offset,
        int                             _whence ) {
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
        struct rodsDirent**             _dirent_ptr ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_readdir_plugin

    irods::error impostor_resource_rename_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                     _new_file_name ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_rename_plugin

    irods::error impostor_resource_truncate_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_truncate_plugin

    irods::error impostor_resource_stagetocache_plugin(
        irods::resource_plugin_context& _ctx,
        const char*                     _cache_file_name ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_stagetocache_plugin

    irods::error impostor_resource_synctoarch_plugin(
        irods::resource_plugin_context& _ctx,
        char*                           _cache_file_name ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_synctoarch_plugin

    irods::error impostor_resource_redirect_plugin(
        irods::resource_plugin_context& _ctx,
        const std::string*              _opr,
        const std::string*              _curr_host,
        irods::hierarchy_parser*        _out_parser,
        float*                          _out_vote ) {
        return irods::impostor_resource::report_error( _ctx );
    } // impostor_resource_redirect_plugin

    irods::error impostor_resource_rebalance_plugin(
        irods::resource_plugin_context& _ctx ) {
        return irods::impostor_resource::report_error( _ctx );
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

    error impostor_resource::post_disconnect_maintenance_operation( pdmo_type& _op ) {
        return ERROR( -1, "nop" );
    }

    error impostor_resource::report_error(
        resource_plugin_context& _ctx ) {
        std::string resc_name;
        error ret = _ctx.prop_map().get< std::string >( RESOURCE_NAME, resc_name );
        if( !ret.ok() ){
            return PASS( ret );
        }

        std::string resc_type;
        ret = _ctx.prop_map().get< std::string >( RESOURCE_TYPE, resc_type );
        if( !ret.ok() ){
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

