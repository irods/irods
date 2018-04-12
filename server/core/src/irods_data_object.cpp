// =-=-=-=-=-=-=-
#include "irods_data_object.hpp"
#include "irods_resource_manager.hpp"

extern irods::resource_manager resc_mgr;

namespace irods {

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object() :
        physical_path_( "" ),
        resc_hier_( "" ),
        id_( 0 ),
        mode_( 0 ),
        flags_( 0 ),
        resc_id_(0) {
        memset( &cond_input_, 0, sizeof( keyValPair_t ) );
    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        rodsLong_t           _resc_id,
        int                _mode,
        int                _flags ) :
        physical_path_( _phy_path ),
        id_( 0 ),
        mode_( _mode ),
        flags_( _flags ),
        resc_id_( _resc_id ) {
        memset( &cond_input_, 0, sizeof( keyValPair_t ) );

        std::string resc_hier;
        resc_mgr.leaf_id_to_hier( resc_id_, resc_hier );
        resc_hier_ = resc_hier;
    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        rodsLong_t           _resc_id,
        int                _mode,
        int                _flags,
        const keyValPair_t& _cond_input ) :
        physical_path_( _phy_path ),
        id_( 0 ),
        mode_( _mode ),
        flags_( _flags ),
        resc_id_( _resc_id ) {
        replKeyVal( &_cond_input, &cond_input_ );

        std::string resc_hier;
        resc_mgr.leaf_id_to_hier( resc_id_, resc_hier );
        resc_hier_ = resc_hier;
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
        id_( 0 ),
        mode_( _mode ),
        flags_( _flags ) {
        memset( &cond_input_, 0, sizeof( keyValPair_t ) );

        rodsLong_t resc_id;
        resc_mgr.hier_to_leaf_id( resc_hier_, resc_id );
        resc_id_ = resc_id;
    } // ctor

// =-=-=-=-=-=-=-
// public - ctor
    data_object::data_object(
        const std::string& _phy_path,
        const std::string& _resc_hier,
        int                _mode,
        int                _flags,
        const keyValPair_t& _cond_input ) :
        physical_path_( _phy_path ),
        resc_hier_( _resc_hier ),
        id_( 0 ),
        mode_( _mode ),
        flags_( _flags ) {
        replKeyVal( &_cond_input, &cond_input_ );

        rodsLong_t resc_id;
        resc_mgr.hier_to_leaf_id( resc_hier_, resc_id );
        resc_id_ = resc_id;
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
        flags_( _rhs.flags_ ),
        resc_id_( _rhs.resc_id_ ) {
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
        resc_id_       = _rhs.resc_id_;
        id_            = _rhs.id_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;
        replKeyVal( &_rhs.cond_input_, &cond_input_ );

        return *this;
    } // operator=

// =-=-=-=-=-=-=-
// public - copy attributes to irods kvp
    error data_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {

        _kvp[PHYSICAL_PATH_KW] = physical_path_.c_str();
        _kvp[RESC_HIER_STR_KW] = resc_hier_.c_str();

        std::stringstream id_str;
        id_str << id_;
        _kvp[DATA_ID_KW] = id_str.str().c_str();

        std::stringstream mode_str;
        mode_str << mode_;
        _kvp[MODE_KW] = mode_str.str().c_str();

        std::stringstream flags_str;
        flags_str << flags_;
        _kvp[FLAGS_KW] = flags_str.str().c_str();

        // copy contents of cond_input
        if(cond_input_.len > 0) {
            for(int i = 0; i < cond_input_.len; ++i) {
                if(cond_input_.keyWord && cond_input_.keyWord[i]) {
                    if(cond_input_.value && cond_input_.value[i]) {
                        _kvp[cond_input_.keyWord[i]] = cond_input_.value[i];
                    }
                    else {
                        _kvp[cond_input_.keyWord[i]] = "empty_value";
                    }
                }
            } // for
        } // if
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
