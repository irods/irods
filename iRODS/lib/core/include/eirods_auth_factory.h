


#ifndef __EIRODS_AUTH_FACTORY_H__
#define __EIRODS_AUTH_FACTORY_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_stacktrace.h"
#include "eirods_auth_object.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {
   /// =-=-=-=-=-=-=-
   /// @brief super basic free factory function to create an auth object
   ///        given the requested authentication scheme
   eirods::error auth_factory( 
       const std::string&,         // auth scheme
       rError_t*,                  // error object for auth ctor
       eirods::auth_object_ptr& ); // auth object pointer
    
}; // namespace eirods


#endif // __EIRODS_AUTH_FACTORY_H__



