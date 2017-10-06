#ifndef __IRODS_DATABASE_FACTORY_HPP__
#define __IRODS_DATABASE_FACTORY_HPP__

// =-=-=-=-=-=-=-
// irods includes
#include "irods_postgres_object.hpp"
#include "irods_generic_database_object.hpp"
#include "irods_mysql_object.hpp"
#include "irods_oracle_object.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief super basic free factory function to create an appropriate
///        database object given the type requested
    irods::error database_factory(
        const std::string&,            // object type
        irods::database_object_ptr& ); // database object

}; // namespace irods


#endif // __IRODS_DATABASE_FACTORY_HPP__



