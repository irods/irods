


#ifndef __EIRODS_DATABASE_FACTORY_H__
#define __EIRODS_DATABASE_FACTORY_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "irods_postgres_object.hpp"
#include "eirods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
   /// =-=-=-=-=-=-=-
   /// @brief super basic free factory function to create an appropriate
   ///        database object given the type requested
   eirods::error database_factory( 
       const std::string&,             // object type
       eirods::database_object_ptr& ); // database object
    
}; // namespace irods


#endif // __EIRODS_DATABASE_FACTORY_H__



