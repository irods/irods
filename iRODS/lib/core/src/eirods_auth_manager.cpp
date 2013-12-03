/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "eirods_auth_manager.hpp"
#include "eirods_auth_plugin.hpp"

namespace eirods {
    // =-=-=-=-=-=-=-
    // globally available auth manager
    auth_manager auth_mgr;

    auth_manager::auth_manager(void) {
        // TODO - stub
    }

    auth_manager::auth_manager(
        const auth_manager& _rhs)
    {
        plugins_ = _rhs.plugins_;
    }
    
    auth_manager::~auth_manager() {
        // TODO - stub
    }

    error auth_manager::resolve(
        const std::string& _key,
        auth_ptr& _value)
    {
        error result = SUCCESS();

        if((result = ASSERT_ERROR(!_key.empty(), SYS_INVALID_INPUT_PARAM, "Empty plugin name.")).ok()) {

            if((result = ASSERT_ERROR(plugins_.has_entry(_key), SYS_INVALID_INPUT_PARAM, "No auth plugin found for name: \"%s\".",
                                      _key.c_str())).ok()) {
                _value = plugins_[_key];
            }
        }
        return result;
    }

    error auth_manager::init_from_type(
        const std::string& _type,
        const std::string& _key,
        const std::string& _inst,
        const std::string& _ctx,
        auth_ptr& _rtn_auth)
    {
        error result = SUCCESS();
        error ret;
        auth_ptr auth;

        ret = load_auth_plugin(auth, _type, _inst, _ctx);
        if((result = ASSERT_PASS(ret, "Failed to load auth plugin.")).ok()) {
            plugins_[_key] = auth;
            _rtn_auth = plugins_[_key];
        }
        
        return result;
    }

}; // namespace eirods
