#include "irods_auth_plugin.hpp"
#include "irods_load_plugin.hpp"

namespace irods {

    auth::auth(
        const std::string& _inst,
        const std::string& _ctx ) :
        plugin_base( _inst, _ctx ),
        start_operation_( irods::auth::default_start_operation ),
        stop_operation_( irods::auth::default_stop_operation ) {

    }

    auth::~auth() {
        // TODO - stub
    }

    auth::auth(
        const auth& _rhs ) :
        plugin_base( _rhs ) {
        start_operation_ = _rhs.start_operation_;
        stop_operation_ = _rhs.stop_operation_;
        operations_ = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tauth cctor - properties map is not empty." << __FILE__ << ":" << __LINE__ << std::endl;
        }

        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers
    }

    auth& auth::operator=(
        const auth& _rhs ) {
        if ( &_rhs == this ) {
            return *this;
        }

        plugin_base::operator=( _rhs );

        operations_ = _rhs.operations_;
        ops_for_delay_load_ = _rhs.ops_for_delay_load_;

        if ( properties_.size() > 0 ) {
            std::cout << "[!]\tauth assignment operator - properties map is not empty."
                      << __FILE__ << ":" << __LINE__ << std::endl;
        }

        properties_ = _rhs.properties_; // NOTE:: memory leak repaving old containers

        return *this;
    }

    error auth::delay_load(
        void* _handle ) {
        error result = SUCCESS();
        if ( ( result = ASSERT_ERROR( _handle != NULL, SYS_INVALID_INPUT_PARAM, "Void handle pointer." ) ).ok() ) {

            if ( ( result = ASSERT_ERROR( !ops_for_delay_load_.empty(), SYS_INVALID_INPUT_PARAM, "Empty operations list." ) ).ok() ) {

                // Check if we need to load a start function
                if ( !start_opr_name_.empty() ) {
                    dlerror();
                    auth_maintenance_operation start_op = reinterpret_cast<auth_maintenance_operation>( dlsym( _handle, start_opr_name_.c_str() ) );
                    if ( ( result = ASSERT_ERROR( start_op, SYS_INVALID_INPUT_PARAM, "Failed to load start function: \"%s\" - %s.",
                                                  start_opr_name_.c_str(), dlerror() ) ).ok() ) {
                        start_operation_ = start_op;
                    }
                }

                // Check if we need to load a stop function
                if ( result.ok() && !stop_opr_name_.empty() ) {
                    dlerror();
                    auth_maintenance_operation stop_op = reinterpret_cast<auth_maintenance_operation>( dlsym( _handle, stop_opr_name_.c_str() ) );
                    if ( ( result = ASSERT_ERROR( stop_op, SYS_INVALID_INPUT_PARAM, "Failed to load stop function: \"%s\" - %s.",
                                                  stop_opr_name_.c_str(), dlerror() ) ).ok() ) {
                        stop_operation_ = stop_op;
                    }
                }

                // Iterate over the list of operation names, load the functions and add it to the map via the wrapper function
                std::vector<std::pair<std::string, std::string> >::iterator itr = ops_for_delay_load_.begin();
                for ( ; result.ok() && itr != ops_for_delay_load_.end(); ++itr ) {

                    std::string key = itr->first;
                    std::string fcn = itr->second;

                    // load the function
                    dlerror();  // reset error messages
                    plugin_operation res_op_ptr = reinterpret_cast<plugin_operation>( dlsym( _handle, fcn.c_str() ) );
                    if ( ( result = ASSERT_ERROR( res_op_ptr, SYS_INVALID_INPUT_PARAM, "Failed to load function: \"%s\" for operation: \"%s\" - %s.",
                                                  fcn.c_str(), key.c_str(), dlerror() ) ).ok() ) {

#ifdef RODS_SERVER
                        oper_rule_exec_mgr_ptr rex_mgr(
                            new operation_rule_execution_manager( instance_name_, key ) );
#else
                        oper_rule_exec_mgr_ptr rex_mgr(
                            new operation_rule_execution_manager_no_op( instance_name_, key ) );
#endif
                        // Add the operation via a wrapper to the operation map
                        operations_[key] = operation_wrapper( rex_mgr, instance_name_, key, res_op_ptr );
                    }
                }
            }
        }
        return result;
    }

    error auth::set_start_operation(
        const std::string& _name ) {
        error result = SUCCESS();
        start_opr_name_ = _name;
        return result;
    }

    error auth::set_stop_operation(
        const std::string& _name ) {
        error result = SUCCESS();
        stop_opr_name_ = _name;
        return result;
    }

// =-=-=-=-=-=-=-
// function to load and return an initialized auth plugin
    error load_auth_plugin(
        auth_ptr&       _plugin,
        const std::string& _plugin_name,
        const std::string& _inst_name,
        const std::string& _context ) {
        error result = SUCCESS();
        // =-=-=-=-=-=-=-
        // resolve plugin directory
        std::string plugin_home;
        error ret = resolve_plugin_path( PLUGIN_TYPE_AUTHENTICATION, plugin_home );
        if( !ret.ok() ) {
            return PASS( ret );
        }

        // =-=-=-=-=-=-=-
        // call generic plugin loader
        auth* ath = 0;
        ret = load_plugin< auth >( ath, _plugin_name, plugin_home, _inst_name, _context );
        if ( ( result = ASSERT_PASS( ret, "Failed to load plugin: \"%s\".", _plugin_name.c_str() ) ).ok() ) {
            if ( ( result = ASSERT_ERROR( ath, SYS_INVALID_INPUT_PARAM, "Invalid auth plugin." ) ).ok() ) {
                _plugin.reset( ath );
            }
        }

        return result;

    } // load_auth_plugin

}; // namespace irods
