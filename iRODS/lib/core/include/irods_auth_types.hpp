#ifndef __IRODS_AUTH_TYPES_HPP__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
#include "irods_plugin_base.hpp"
#include "irods_lookup_table.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace irods {
// =-=-=-=-=-=-=-
// auth plugin pointer type
    class auth;
    typedef boost::shared_ptr< auth > auth_ptr;

// =-=-=-=-=-=-=-
// fwd decl of network manager for fco resolve
    class auth_manager;

}; // namespace

#define __IRODS_AUTH_TYPES_HPP__
#endif // __IRODS_AUTH_TYPES_HPP__



