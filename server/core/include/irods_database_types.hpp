#ifndef __IRODS_DATABASE_TYPES_HPP__
#define __IRODS_DATABASE_TYPES_HPP__

// =-=-=-=-=-=-=-
// Boost Includes
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

// =-=-=-=-=-=-=-
// irods includes
#include "irods_plugin_base.hpp"
#include "irods_lookup_table.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rcConnect.h"

namespace irods {
// =-=-=-=-=-=-=-
// database plugin pointer type
    class database;
    typedef boost::shared_ptr< database > database_ptr;

// =-=-=-=-=-=-=-
// fwd decl of database manager for fco resolve
    class database_manager;

}; // namespace

#endif // __IRODS_DATABASE_TYPES_HPP__



