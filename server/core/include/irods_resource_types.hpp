#ifndef __IRODS_RESOURCE_TYPES_HPP__
#define __IRODS_RESOURCE_TYPES_HPP__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>

namespace irods {
// =-=-=-=-=-=-=-
// resource plugin pointer type
    class resource;
    typedef boost::shared_ptr< resource > resource_ptr;

// =-=-=-=-=-=-=-
// fwd decl of resource manager for fco etc
    class resource_manager;

}; // namespace

#endif // __IRODS_RESOURCE_TYPES_HPP__



