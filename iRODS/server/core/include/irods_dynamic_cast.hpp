#ifndef __IRODS_DYNAMIC_CAST_HPP__
#define __IRODS_DYNAMIC_CAST_HPP__

// =-=-=-=-=-=-=-
#include "irods_file_object.hpp"
#include "irods_collection_object.hpp"
#include "irods_structured_object.hpp"

namespace irods {

// =-=-=-=-=-=-=-=-
// FIXME :: this is an unholy work around discovered due to an issue
//       :: with the sharing of type info across dynamic shared objects.
//       :: this code is unnecessary on a modern compiler but is required
//       :: to build on eariler centos and ubuntu distributions.  it will
//       :: be removed when we deprecate support for those platforms.
    void dynamic_cast_hack() {
        file_object       file_obj;
        collection_object coll_obj;
        structured_object struct_obj;

        first_class_object* file_fco   = &file_obj;
        first_class_object* coll_fco   = &coll_obj;
        first_class_object* struct_fco = &struct_obj;

        file_object*       file_ptr   = dynamic_cast< file_object* >( file_fco );
        if ( !file_ptr ) {
            rodsLog( LOG_NOTICE, "dynamic_cast_hack - failed to cast file_object" );
        }
        collection_object* coll_ptr   = dynamic_cast< collection_object* >( coll_fco );
        if ( !coll_ptr ) {
            rodsLog( LOG_NOTICE, "dynamic_cast_hack - failed to cast collection_object" );
        }
        structured_object* struct_ptr = dynamic_cast< structured_object* >( struct_fco );
        if ( !struct_ptr ) {
            rodsLog( LOG_NOTICE, "dynamic_cast_hack - failed to cast structured_object" );
        }

    } // dynamic_cast_hack

}; // namespace irods

#endif // __IRODS_DYNAMIC_CAST_HPP__

