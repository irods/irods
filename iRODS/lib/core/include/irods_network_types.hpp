/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_NETWORK_TYPES_H__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.hpp"
#include "eirods_lookup_table.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.hpp"

namespace eirods {
    // =-=-=-=-=-=-=-
    // network plugin pointer type
    class network;
    typedef boost::shared_ptr< network > network_ptr;

    // =-=-=-=-=-=-=-
    // fwd decl of network manager for fco resolve
    class network_manager;
                                         
}; // namespace

#define __EIRODS_NETWORK_TYPES_H__
#endif // __EIRODS_NETWORK_TYPES_H__



