// =-=-=-=-=-=-=-
#include "irods_osauth_auth_object.hpp"
#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"

extern int ProcessType;

// =-=-=-=-=-=-=-
// irods includes
#include "rcMisc.h"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    osauth_auth_object::osauth_auth_object(
        rError_t* _r_error ) :
        auth_object( _r_error ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - dtor
    osauth_auth_object::~osauth_auth_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - cctor
    osauth_auth_object::osauth_auth_object(
        const osauth_auth_object& _rhs ) :
        auth_object( _rhs ) {
        user_name_ = _rhs.user_name_;
        zone_name_ = _rhs.zone_name_;
        digest_    = _rhs.digest_;
    } // cctor

// =-=-=-=-=-=-=-
// public - assignment operator
    osauth_auth_object& osauth_auth_object::operator=(
        const osauth_auth_object& _rhs ) {
        auth_object::operator=( _rhs );
        user_name_ = _rhs.user_name_;
        zone_name_ = _rhs.zone_name_;
        digest_    = _rhs.digest_;
        return *this;
    }

// =-=-=-=-=-=-=-
// public - equality operator
    bool osauth_auth_object::operator==(
        const osauth_auth_object& ) const {
        return false;
    }

// =-=-=-=-=-=-=-
// public - resolve a plugin given an interface
    error osauth_auth_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check the interface type and error out if it
        // isnt a auth interface
        if ( AUTH_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "osauth_auth_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // ask the auth manager for a osauth auth plugin
        auth_ptr a_ptr;
        error ret = auth_mgr.resolve(
                        AUTH_OSAUTH_SCHEME,
                        a_ptr );
        if ( !ret.ok() ) {
            // =-=-=-=-=-=-=-
            // attempt to load the plugin, in this case the type,
            // instance name, key etc are all osauth as there is only
            // the need for one instance of a osauth object, etc.
            std::string empty_context( "" );
            ret = auth_mgr.init_from_type(
                      ProcessType,
                      AUTH_OSAUTH_SCHEME,
                      AUTH_OSAUTH_SCHEME,
                      AUTH_OSAUTH_SCHEME,
                      empty_context,
                      a_ptr );
            if ( !ret.ok() ) {
                return PASS( ret );

            }
            else {
                // =-=-=-=-=-=-=-
                // upcast for out variable
                _ptr = boost::dynamic_pointer_cast< plugin_base >( a_ptr );
                return SUCCESS();

            }

        } // if !ok

        // =-=-=-=-=-=-=-
        // upcast for out variable
        _ptr = boost::dynamic_pointer_cast< plugin_base >( a_ptr );

        return SUCCESS();

    } // resolve

// =-=-=-=-=-=-=-
// public - serialize object to kvp
    error osauth_auth_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        // =-=-=-=-=-=-=-
        // all we have in this object is the auth results
        _kvp["zone_name"] = zone_name_.c_str();
        _kvp["user_name"] = user_name_.c_str();
        _kvp["digest"]    = digest_.c_str();

        return SUCCESS();

    } // get_re_vars

}; // namespace irods
