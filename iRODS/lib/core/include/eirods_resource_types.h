/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef __EIRODS_RESOURCE_TYPES_H__
#define __EIRODS_RESOURCE_TYPES_H__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>

namespace eirods {
    // =-=-=-=-=-=-=-
    // resource plugin pointer type
    class resource;
    typedef boost::shared_ptr< resource > resource_ptr;

    // =-=-=-=-=-=-=-
    // fwd decl of resource manager for fco etc
    class resource_manager;

}; // namespace

#endif // __EIRODS_RESOURCE_TYPES_H__



