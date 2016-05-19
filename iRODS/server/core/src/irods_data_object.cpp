// =-=-=-=-=-=-=-
#include "irods_data_object.hpp"
#include "irods_resource_manager.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object() :
        physical_path_( "" ),
        resc_hier_( "" ),
        id_ ( 0 ),
        mode_( 0 ),
        flags_( 0 ) {
        memset( &cond_input_, 0, sizeof( keyValPair_t ) );
    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        const std::string& _resc_hier,
        long _id,
        int _mode,
        int _flags ) :
        physical_path_( _phy_path ),
        resc_hier_( _resc_hier ),
        id_( _id ),
        mode_( _mode ),
        flags_( _flags ) {
        memset( &cond_input_, 0, sizeof( keyValPair_t ) );
    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        const std::string& _resc_hier,
        long               _id,
        int                _mode,
        int                _flags,
        const keyValPair_t& _cond_input ) :
        physical_path_( _phy_path ),
        resc_hier_( _resc_hier ),
        id_( _id ),
        mode_( _mode ),
        flags_( _flags ) {
        replKeyVal( &_cond_input, &cond_input_ );
    } // ctor

// =-=-=-=-=-=-=-
// public - cctor
    data_object::data_object(
        const data_object& _rhs ) :
        first_class_object( _rhs ),
        physical_path_( _rhs.physical_path_ ),
        resc_hier_( _rhs.resc_hier_ ),
        id_( _rhs.id_ ),
        mode_( _rhs.mode_ ),
        flags_( _rhs.flags_ ) {
        replKeyVal( &_rhs.cond_input_, &cond_input_ );
    } // cctor

// =-=-=-=-=-=-=-
// public - dtor
    data_object::~data_object() {
        clearKeyVal( &cond_input_ );
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    data_object& data_object::operator=(
        const data_object& _rhs ) {
        physical_path_ = _rhs.physical_path_;
        resc_hier_     = _rhs.resc_hier_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;
        replKeyVal( &_rhs.cond_input_, &cond_input_ );

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

        // copy contents of cond_input
        for ( int i = 0; i < cond_input_.len; i++ ) {
            addKeyVal( &_kvp, cond_input_.keyWord[i], cond_input_.value[i] );
        }

        return SUCCESS();

    } // get_re_vars

    void add_key_val(
        data_object_ptr&   _do,
        const std::string& _k,
        const std::string& _v ) {
        addKeyVal(
            &_do->cond_input_,
            ( char* )_k.c_str(),
            ( char* )_v.c_str() );
    }

    void remove_key_val(
        data_object_ptr&   _do,
        const std::string& _k ) {
        rmKeyVal(
            &_do->cond_input_,
            ( char* )_k.c_str() );

    }
}; // namespace irods



