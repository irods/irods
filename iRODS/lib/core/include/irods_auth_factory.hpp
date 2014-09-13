#ifndef __IRODS_AUTH_FACTORY_HPP__
#define __IRODS_AUTH_FACTORY_HPP__

// =-=-=-=-=-=-=-
#include "irods_stacktrace.hpp"
#include "irods_auth_object.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief super basic free factory function to create an auth object
///        given the requested authentication scheme
    irods::error auth_factory(
        const std::string&,         // auth scheme
        rError_t*,                  // error object for auth ctor
        irods::auth_object_ptr& ); // auth object pointer

}; // namespace irods


#endif // __IRODS_AUTH_FACTORY_HPP__



