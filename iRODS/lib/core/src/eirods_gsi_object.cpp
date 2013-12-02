/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */


#include "eirods_gsi_object.hpp"
#include "eirods_auth_manager.hpp"

namespace eirods {

    gsi_object::gsi_object(
        rError_t* _r_error) : auth_object(_r_error)
    {
        creds_ = GSS_C_NO_CREDENTIAL;
    }

    gsi_object::~gsi_object() {
        // TODO - stub
    }

    error gsi_object::resolve(
        const std::string& _interface,
        plugin_ptr& _ptr)
    {
        error result = SUCCESS();
        if((result = ASSERT_ERROR(_interface == AUTH_INTERFACE, SYS_INVALID_INPUT_PARAM,
                                  "gsi_object does not support a \"%s\" plugin interface.",
                                  _interface.c_str())).ok()) {
            auth_ptr ath;
            error ret = auth_mgr.resolve(GSI_AUTH_PLUGIN, ath);
            if(!(result = ASSERT_PASS(ret, "Failed to resolve the GSI auth plugin.")).ok()) {

                // Attempt to load the plugin.
                std::string empty_context("");
                ret = auth_mgr.init_from_type(GSI_AUTH_PLUGIN, GSI_AUTH_PLUGIN, GSI_AUTH_PLUGIN, empty_context, ath);
                result = ASSERT_PASS(ret, "Failed to load the GSI auth plugin.");
            }

            if(result.ok()) {
                _ptr = boost::dynamic_pointer_cast<plugin_base>(ath);
            }
        }
        return result;
    }
    
    bool gsi_object::operator==(
        const gsi_object& _rhs) const
    {
        return creds_ == _rhs.creds_;
    }

}; // namespace eirods
