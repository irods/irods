/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_data_object.h"
#include "eirods_resource_manager.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - ctor
    data_object::data_object() :
        physical_path_(""),
        resc_hier_(""),
        mode_(0),
        flags_(0) {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        const std::string& _resc_hier,
        int                _mode,
        int                _flags ) :
        physical_path_( _phy_path ),
        resc_hier_( _resc_hier ),
        mode_( _mode ),
        flags_( _flags ) {
    } // ctor

    // =-=-=-=-=-=-=-
    // public - cctor
    data_object::data_object( 
        const data_object& _rhs ) {
        physical_path_ = _rhs.physical_path_;
        resc_hier_     = _rhs.resc_hier_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;

    } // cctor

    // =-=-=-=-=-=-=-
    // public - dtor
    data_object::~data_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    data_object& data_object::operator=( 
        const data_object& _rhs ) {
        physical_path_ = _rhs.physical_path_;
        resc_hier_     = _rhs.resc_hier_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;

        return *this;
    } // operator=

}; // namespace eirods



