// =-=-=-=-=-=-=-
#include "irods_auth_factory.hpp"
#include "irods_native_auth_object.hpp"
#include "irods_pam_auth_object.hpp"
#include "irods_osauth_auth_object.hpp"
#include "irods_gsi_object.hpp"
#include "irods_krb_object.hpp"
#include "irods_generic_auth_object.hpp"
#include "rodsErrorTable.h"
#include <boost/algorithm/string.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief super basic free factory function to create an auth object
///        given the requested authentication scheme
    error auth_factory(
        const std::string& _scheme,
        rError_t*          _r_error,
        auth_object_ptr&   _ptr ) {
        // ensure scheme is lower case for comparison
        std::string scheme = boost::algorithm::to_lower_copy( _scheme );

        if ( scheme.empty() || AUTH_NATIVE_SCHEME == scheme ) {
            _ptr.reset( new native_auth_object( _r_error ) );
        }
        else if ( AUTH_PAM_SCHEME == scheme ) {
            _ptr.reset( new pam_auth_object( _r_error ) );
        }
        else if ( AUTH_OSAUTH_SCHEME == scheme ) {
            _ptr.reset( new osauth_auth_object( _r_error ) );
        }
        else if ( AUTH_GSI_SCHEME == scheme ) {
            _ptr.reset( new gsi_auth_object( _r_error ) );
        }
        else if ( AUTH_KRB_SCHEME == scheme ) {
            _ptr.reset( new krb_auth_object( _r_error ) );
        }
        else {
            _ptr.reset( new irods::generic_auth_object( scheme, _r_error ) );
        }

        return SUCCESS();

    } // auth_factory

}; // namespace irods



