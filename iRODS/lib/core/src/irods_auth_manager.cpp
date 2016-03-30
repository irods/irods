#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"

namespace irods {
// =-=-=-=-=-=-=-
// globally available auth manager
    auth_manager auth_mgr;

    auth_manager::auth_manager( void ) {
        // TODO - stub
    }

    auth_manager::auth_manager(
        const auth_manager& _rhs ) {
        plugins_ = _rhs.plugins_;
    }

    auth_manager::~auth_manager() {
        // TODO - stub
    }

    error auth_manager::resolve(
        const std::string& _key,
        auth_ptr& _value ) {
        error result = SUCCESS();

        if ( ( result = ASSERT_ERROR( !_key.empty(), SYS_INVALID_INPUT_PARAM, "Empty plugin name." ) ).ok() ) {

            if ( ( result = ASSERT_ERROR( plugins_.has_entry( _key ), SYS_INVALID_INPUT_PARAM, "No auth plugin found for name: \"%s\".",
                                          _key.c_str() ) ).ok() ) {
                _value = plugins_[_key];
            }
        }
        return result;
    }

    // =-=-=-=-=-=-=-
    // function to load and return an initialized auth plugin
    static error load_auth_plugin(
            auth_ptr&       _plugin,
            const std::string& _plugin_name,
            const std::string& _inst_name,
            const std::string& _context ) {
        error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // call generic plugin loader
        auth* ath = 0;
        error ret = load_plugin< auth >(
                        ath,
                        _plugin_name,
                        PLUGIN_TYPE_AUTHENTICATION,
                        _inst_name,
                        _context );
        if ( ( result = ASSERT_PASS( ret, "Failed to load plugin: \"%s\".", _plugin_name.c_str() ) ).ok() ) {
            if ( ( result = ASSERT_ERROR( ath, SYS_INVALID_INPUT_PARAM, "Invalid auth plugin." ) ).ok() ) {
                _plugin.reset( ath );
            }
        }

        return result;

    } // load_auth_plugin

    error auth_manager::init_from_type(
        const int&         _proc_type,
        const std::string& _type,
        const std::string& _key,
        const std::string& _inst,
        const std::string& _ctx,
        auth_ptr& _rtn_auth ) {
        error result = SUCCESS();
        error ret;
        auth_ptr auth;

        std::string type = _type;
        if( CLIENT_PT == _proc_type ) {        
            type += "_client";
        }
        else {
            type += "_server";
        }

        ret = load_auth_plugin( auth, type, _inst, _ctx );
        if ( ( result = ASSERT_PASS( ret, "Failed to load auth plugin." ) ).ok() ) {
            plugins_[_key] = auth;
            _rtn_auth = plugins_[_key];
        }

        return result;
    }

}; // namespace irods
