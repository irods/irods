#include "rcMisc.h"

#include "irods_krb_object.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"

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

    error krb_auth_object::resolve(
        const std::string& _interface,
        plugin_ptr& _ptr ) {
        error result = SUCCESS();
        if ( ( result = ASSERT_ERROR( _interface == AUTH_INTERFACE, SYS_INVALID_INPUT_PARAM,
                                      "krb_auth_object does not support a \"%s\" plugin interface.",
                                      _interface.c_str() ) ).ok() ) {
            auth_ptr ath;
            error ret = auth_mgr.resolve( AUTH_KRB_SCHEME, ath );
            if ( !( result = ASSERT_PASS( ret, "Failed to resolve the KRB auth plugin." ) ).ok() ) {

                // Attempt to load the plugin.
                std::string empty_context( "" );
                ret = auth_mgr.init_from_type( ProcessType, AUTH_KRB_SCHEME, AUTH_KRB_SCHEME, AUTH_KRB_SCHEME, empty_context, ath );
                result = ASSERT_PASS( ret, "Failed to load the KRB auth plugin." );
            }

            if ( result.ok() ) {
                _ptr = boost::dynamic_pointer_cast<plugin_base>( ath );
            }
        }
        return result;
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
