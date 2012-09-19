


#ifndef __EIRODS_DYNAMIC_CAST_H__
#define __EIRODS_DYNAMIC_CAST_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"
#include "eirods_collection_object.h"

namespace eirods {

    // =-=-=-=-=-=-=-=-
	// FIXME :: this is an unholy work around discovered due to an issue
	//       :: with the sharing of type info across dynamic shared objects.
	//       :: this code is unnecessary on a modern compiler but is required
	//       :: to build on eariler centos and ubuntu distributions.  it will
	//       :: be removed when we deprecate support for those platforms.
    void dynamic_cast_hack() {

        file_object       file_obj;
        collection_object coll_obj;
	
	    first_class_object* file_fco = &file_obj;	
	    first_class_object* coll_fco = &coll_obj;	
	
	    file_object* file_ptr       = dynamic_cast< file_object* >( file_fco );
		collection_object* coll_ptr = dynamic_cast< collection_object* >( coll_fco );	
		 
	} // dynamic_cast_hack

}; // namespace eirods

#endif // __EIRODS_DYNAMIC_CAST_H__



