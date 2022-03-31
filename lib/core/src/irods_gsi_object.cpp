#include "irods/irods_gsi_object.hpp"

#include "irods/irods_auth_manager.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/rcMisc.h"

#include <fmt/format.h>

extern int ProcessType;

namespace irods {

    gsi_auth_object::gsi_auth_object(
        rError_t* _r_error ) :
        auth_object( _r_error ),
        sock_( 0 ) {
        creds_ = GSS_C_NO_CREDENTIAL;
    }

    gsi_auth_object::~gsi_auth_object() {
        // TODO - stub
    }

    error gsi_auth_object::resolve(const std::string& _interface, plugin_ptr& _ptr)
    {
        if (AUTH_INTERFACE != _interface) {
            return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format(
                "gsi_auth_object does not support a \"{}\" plugin interface.",
                _interface.c_str()));
        }

        const auto& type = irods::AUTH_GSI_SCHEME;

        auth_ptr ap;
        if (auto err = auth_mgr.resolve(type, ap); !err.ok()) {
            // Attempt to load the plugin.
            if (err = auth_mgr.init_from_type(ProcessType, type, type, type, std::string{}, ap); !err.ok()) {
                return PASSMSG("Failed to load the GSI auth plugin.", err);
            }
        }

        _ptr = boost::dynamic_pointer_cast<plugin_base>(ap);

        return SUCCESS();
    } // gsi_auth_object::resolve

    bool gsi_auth_object::operator==(
        const gsi_auth_object& _rhs ) const {
        return creds_ == _rhs.creds_;
    }

// =-=-=-=-=-=-=-
// public - serialize object to kvp
    error gsi_auth_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // all we have in this object is the auth results
        std::stringstream sock_msg;
        sock_msg << sock_;
        _kvp["socket"] = sock_msg.str().c_str();
        _kvp["serverDN"] = server_dn_.c_str();
        _kvp["digest"] = digest_.c_str();

        return result;

    } // get_re_vars

}; // namespace irods
