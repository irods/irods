/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_AUTH_TYPES_H__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_plugin_base.h"
#include "eirods_lookup_table.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    // auth plugin pointer type
    class auth;
    typedef boost::shared_ptr< auth > auth_ptr;

    // =-=-=-=-=-=-=-
    // fwd decl of network manager for fco resolve
    class auth_manager;
                                         
}; // namespace

#define __EIRODS_AUTH_TYPES_H__
#endif // __EIRODS_AUTH_TYPES_H__



