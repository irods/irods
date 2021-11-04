#include "irods/irods_krb_object.hpp"

#include "irods/irods_auth_manager.hpp"
#include "irods/irods_auth_plugin.hpp"
#include "irods/rcMisc.h"

#include <fmt/format.h>

extern int ProcessType;

namespace irods {

    krb_auth_object::krb_auth_object(
        rError_t* _r_error ) :
        auth_object( _r_error ),
        sock_( 0 ) {
        creds_ = GSS_C_NO_CREDENTIAL;
    }

    krb_auth_object::~krb_auth_object() {
        // TODO - stub
    }

    error krb_auth_object::resolve(const std::string& _interface, plugin_ptr& _ptr)
    {
        if (AUTH_INTERFACE != _interface) {
            return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format(
                "krb_auth_object does not support a \"{}\" plugin interface.",
                _interface.c_str()));
        }

        auth_ptr auth;
        if (const auto err = auth_mgr.resolve(irods::AUTH_KRB_SCHEME, auth); !err.ok()) {
            return PASSMSG("Failed to resolve the KRB auth plugin.", err);
        }

        // Attempt to load the plugin.
        std::string unused;
        const auto& type = irods::AUTH_KRB_SCHEME;
        if (const auto err = auth_mgr.init_from_type(ProcessType, type, type, type, unused, auth); !err.ok()) {
            return PASSMSG("Failed to load the KRB auth plugin.", err);
        }

        _ptr = boost::dynamic_pointer_cast<plugin_base>(auth);

        return SUCCESS();
    }

    bool krb_auth_object::operator==(
        const krb_auth_object& _rhs ) const {
        return creds_ == _rhs.creds_;
    }

// =-=-=-=-=-=-=-
// public - serialize object to kvp
    error krb_auth_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        irods::error result = SUCCESS();

        // =-=-=-=-=-=-=-
        // all we have in this object is the auth results
        std::stringstream sock_msg;
        sock_msg << sock_;
        _kvp["socket"] = sock_msg.str().c_str();
        _kvp["serviceName"] = service_name_.c_str();
        _kvp["digest"] = digest_.c_str();

        return result;

    } // get_re_vars

}; // namespace irods
