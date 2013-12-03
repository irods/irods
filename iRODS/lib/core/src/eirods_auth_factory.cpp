


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_auth_factory.hpp"
#include "eirods_native_auth_object.hpp"
#include "eirods_pam_auth_object.hpp"
#include "eirods_osauth_auth_object.hpp"

namespace eirods {
   /// =-=-=-=-=-=-=-
   /// @brief super basic free factory function to create an auth object
   ///        given the requested authentication scheme
   error auth_factory( 
       const std::string& _scheme,
       rError_t*          _r_error,
       auth_object_ptr&   _ptr ) {

       // =-=-=-=-=-=-=-
       // ensure scheme is lower case for comparison
       std::string scheme = _scheme;
       std::transform( 
           scheme.begin(), 
           scheme.end(), 
           scheme.begin(), 
           ::tolower );

       // =-=-=-=-=-=-=-
       // currently just support the native scheme
       if( scheme.empty() ||
           AUTH_NATIVE_SCHEME == scheme ) {
           native_auth_object* nat_obj = new native_auth_object( _r_error );
           if( !nat_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "native auth allocation failed" );
           }

           auth_object* auth_obj = dynamic_cast< auth_object* >( nat_obj );
           if( !auth_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "native auth dynamic cast failed" );
           }
           
           _ptr.reset( auth_obj );

       } else if( AUTH_PAM_SCHEME == scheme ) {
           pam_auth_object* pam_obj = new pam_auth_object( _r_error );
           if( !pam_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "pam auth allocation failed" );
           }

           auth_object* auth_obj = dynamic_cast< auth_object* >( pam_obj );
           if( !auth_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "pam auth dynamic cast failed" );
           }
           
           _ptr.reset( auth_obj );

       } else if( AUTH_OSAUTH_SCHEME == scheme ) {
           osauth_auth_object* osauth_obj = new osauth_auth_object( _r_error );
           if( !osauth_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "osauth auth allocation failed" );
           }

           auth_object* auth_obj = dynamic_cast< auth_object* >( osauth_obj );
           if( !auth_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "osauth auth dynamic cast failed" );
           }
           
           _ptr.reset( auth_obj );
       
       } else {
           std::string msg( "auth scheme not supported [" );
           msg += scheme + "]";
           return ERROR( 
                      SYS_INVALID_INPUT_PARAM, 
                      msg );
       }

       return SUCCESS();

   } // auth_factory

}; // namespace eirods



