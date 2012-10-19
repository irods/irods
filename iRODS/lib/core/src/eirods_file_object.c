


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"
#include "eirods_resource_manager.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    file_object::file_object() :
	             first_class_object(),
				 size_(0) {
	} // file_object

    // =-=-=-=-=-=-=-
	// public - cctor
	file_object::file_object( const file_object& _rhs ) : 
	             first_class_object( _rhs ) {
        size_  = _rhs.size_;

	} // cctor 

    // =-=-=-=-=-=-=-
	// public - ctor
    file_object::file_object( rsComm_t* _c, std::string _fn, int _fd, int _m, int _f ) :
	             first_class_object(),
				 size_( -1 ) {
        comm_            = _c;
	    physical_path_   = _fn;
		file_descriptor_ = _fd;
		mode_            = _m;
		flags_           = _f;
	} // file_object

    // =-=-=-=-=-=-=-
	// public - dtor
	file_object::~file_object() {
	} // dtor

	// =-=-=-=-=-=-=-
	// public - assignment operator
	file_object& file_object::operator=( const file_object& _rhs ) {
		// =-=-=-=-=-=-=-
		// call base class assignment first
        first_class_object::operator=( _rhs );

        size_  = _rhs.size_;

		return *this;

	}  // operator=

    // =-=-=-=-=-=-=-
    // plugin - resolve resource plugin for this object
    error file_object::resolve( resource_manager& _mgr, resource_ptr& _ptr ) {
        return _mgr.resolve( *this, _ptr ); 

    } // resolve
    
}; // namespace eirods



