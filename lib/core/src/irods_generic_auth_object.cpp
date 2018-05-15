#include "irods_auth_manager.hpp"
#include "irods_auth_plugin.hpp"
#include "irods_generic_auth_object.hpp"

extern int ProcessType;

namespace irods {

    generic_auth_object::generic_auth_object(
        const std::string& _type,
        rError_t* _r_error ) :
        auth_object( _r_error ),
        type_( _type ),
        sock_( 0 ) {

    } // constructor

    generic_auth_object::generic_auth_object(
        const generic_auth_object& _rhs ) :
        auth_object( _rhs ), type_( _rhs.type_ ) {

    } // constructor

    generic_auth_object::~generic_auth_object() {

    } // destructor


    error generic_auth_object::resolve(
        const std::string& _interface,
        plugin_ptr& _plugin ) {
        
        if ( AUTH_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "generic_auth_object does not support a [";
            msg << _interface;
            msg << "] plugin interface";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        auth_ptr auth_p;
        error ret = auth_mgr.resolve( type_, auth_p );
        if ( !ret.ok() ) {
            std::string empty_context("");
            ret = auth_mgr.init_from_type(
                    ProcessType,
                    type_,
                    type_,
                    type_,
                    empty_context,
                    auth_p );

            if ( !ret.ok() ) {
                return PASS( ret );
            }
            else {
                _plugin = boost::dynamic_pointer_cast<plugin_base>( auth_p );
                return SUCCESS();
            }
        }

        _plugin = boost::dynamic_pointer_cast<plugin_base>( auth_p );
        return SUCCESS();
    }

    bool generic_auth_object::operator==(
        const generic_auth_object& _rhs ) const {
        return false;
    }

    generic_auth_object& generic_auth_object::operator=(
        const generic_auth_object& _rhs ) {
        return *this;
    }

    error generic_auth_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        
        return SUCCESS(); 
    }
}
