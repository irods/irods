


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_collection_object.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    collection_object::collection_object() :
	             first_class_object(),
				 directory_pointer_(0) {
	} // collection_object

    // =-=-=-=-=-=-=-
	// public - cctor
	collection_object::collection_object( const collection_object& _rhs ) : 
	             first_class_object( _rhs ) {
		directory_pointer_ = _rhs.directory_pointer_;

	} // cctor 

    // =-=-=-=-=-=-=-
	// public - ctor
    collection_object::collection_object( std::string _fn, int _m, int _f ) :
	             first_class_object(),
				 directory_pointer_(0) {
	    physical_path_   = _fn;
		mode_            = _m;
		flags_           = _f;
	} // collection_object

    // =-=-=-=-=-=-=-
	// public - dtor
	collection_object::~collection_object() {
	} // dtor

	// =-=-=-=-=-=-=-
	// public - assignment operator
	collection_object& collection_object::operator=( const collection_object& _rhs ) {
		// =-=-=-=-=-=-=-
		// call base class assignment first
        first_class_object::operator=( _rhs );

		directory_pointer_  = _rhs.directory_pointer_;

	}  // operator=

}; // namespace eirods



