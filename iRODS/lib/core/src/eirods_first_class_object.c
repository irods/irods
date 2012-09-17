


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_first_class_object.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
	first_class_object::first_class_object() :
	                    physical_path_(""),
						logical_path_(""),
						data_type_(""),
						file_descriptor_(-1),
						l1_desc_idx_(-1),
						mode_(0),
						flags_(0) {
	} // ctor

    // =-=-=-=-=-=-=-
	// public - cctor
	first_class_object::first_class_object( const first_class_object& _rhs ) {
		physical_path_   = _rhs.physical_path_;
		logical_path_    = _rhs.logical_path_;
		data_type_       = _rhs.data_type_;
		file_descriptor_ = _rhs.file_descriptor_;
		l1_desc_idx_     = _rhs.l1_desc_idx_;
		mode_            = _rhs.mode_;
		flags_           = _rhs.flags_;

	} // cctor

    // =-=-=-=-=-=-=-
	// public - dtor
	first_class_object::~first_class_object() {
	} // dtor

    // =-=-=-=-=-=-=-
	// public - assignment operator
	first_class_object& first_class_object::operator=( const first_class_object& _rhs ) {
		physical_path_   = _rhs.physical_path_;
		logical_path_    = _rhs.logical_path_;
		data_type_       = _rhs.data_type_;
		file_descriptor_ = _rhs.file_descriptor_;
		l1_desc_idx_     = _rhs.l1_desc_idx_;
		mode_            = _rhs.mode_;
		flags_           = _rhs.flags_;

        return *this;
	} // operator=

}; // namespace eirods



