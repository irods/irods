#ifndef __IRODS_NETWORK_TYPES_HPP__

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
// network plugin pointer type
    class network;
    typedef boost::shared_ptr< network > network_ptr;

// =-=-=-=-=-=-=-
// fwd decl of network manager for fco resolve
    class network_manager;

}; // namespace

#define __IRODS_NETWORK_TYPES_HPP__
#endif // __IRODS_NETWORK_TYPES_HPP__



