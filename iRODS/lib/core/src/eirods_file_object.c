


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    file_object::file_object() :
	             first_class_object(),
				 size_(0) {
	} // file_object

    // =-=-=-=-=-=-=-
	// public - cctor
    file_object::file_object( fileCreateInp_t& _inp ) :
	             first_class_object(),
				 size_( _inp.dataSize ) {
	    physical_path_ = _inp.fileName;
		mode_          = _inp.mode;
		flags_         = _inp.flags;
	} // file_object

    // =-=-=-=-=-=-=-
	// public - cctor
	file_object::file_object( const file_object& _rhs ) : 
	             first_class_object( _rhs ) {
        size_  = _rhs.size_;

	} // cctor 

    // =-=-=-=-=-=-=-
	// public - ctor
    file_object::file_object( std::string _fn, int _fd, int _m, int _f ) :
	             first_class_object(),
				 size_( -1 ) {
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

}; // namespace eirods



