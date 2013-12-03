/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
#include "irods_data_object.hpp"
#include "irods_resource_manager.hpp"

namespace irods {

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

    // =-=-=-=-=-=-=-
    // public - copy attributes to irods kvp
    error data_object::get_re_vars( 
        keyValPair_t& _kvp ) {

        addKeyVal( &_kvp, PHYSICAL_PATH_KW, physical_path_.c_str() );
        addKeyVal( &_kvp, RESC_HIER_STR_KW, resc_hier_.c_str() );

        std::stringstream mode_str;
        mode_str << mode_;
        addKeyVal( &_kvp, MODE_KW, mode_str.str().c_str() );

        std::stringstream flags_str;
        flags_str << flags_;
        addKeyVal( &_kvp, FLAGS_KW, flags_str.str().c_str() );

        return SUCCESS();

    } // get_re_vars

}; // namespace irods



