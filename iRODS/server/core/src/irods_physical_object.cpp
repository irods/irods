// =-=-=-=-=-=-=-
#include "irods_physical_object.hpp"


namespace irods {

// =-=-=-=-=-=-=-
// public - default constructor
    physical_object::physical_object() : is_dirty_( -1 ),
        repl_num_( -1 ),
        map_id_( -1 ),
        size_( -1 ),
        id_( -1 ),
        coll_id_( -1 ) {
    } // ctor

// =-=-=-=-=-=-=-
// public - copy constructor
    physical_object::physical_object( const physical_object& _rhs ) {
        is_dirty_        = _rhs.is_dirty_;
        repl_num_        = _rhs.repl_num_;
        map_id_          = _rhs.map_id_;
        size_            = _rhs.size_;
        id_              = _rhs.id_;
        coll_id_         = _rhs.coll_id_;
        name_            = _rhs.name_;
        version_         = _rhs.version_;
        type_name_       = _rhs.type_name_;
        resc_name_       = _rhs.resc_name_;
        path_            = _rhs.path_;
        owner_name_      = _rhs.owner_name_;
        owner_zone_      = _rhs.owner_zone_;
        status_          = _rhs.status_;
        checksum_        = _rhs.checksum_;
        expiry_ts_       = _rhs.expiry_ts_;
        mode_            = _rhs.mode_;
        r_comment_       = _rhs.r_comment_;
        create_ts_       = _rhs.create_ts_;
        modify_ts_       = _rhs.modify_ts_;
        resc_hier_       = _rhs.resc_hier_;

    } // cctor

// =-=-=-=-=-=-=-
// public - destructor
    physical_object::~physical_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    physical_object& physical_object::operator=( const physical_object& _rhs ) {
        if ( this != &_rhs ) {
            is_dirty_        = _rhs.is_dirty_;
            repl_num_        = _rhs.repl_num_;
            map_id_          = _rhs.map_id_;
            size_            = _rhs.size_;
            id_              = _rhs.id_;
            coll_id_         = _rhs.coll_id_;
            name_            = _rhs.name_;
            version_         = _rhs.version_;
            type_name_       = _rhs.type_name_;
            resc_name_       = _rhs.resc_name_;
            path_            = _rhs.path_;
            owner_name_      = _rhs.owner_name_;
            owner_zone_      = _rhs.owner_zone_;
            status_          = _rhs.status_;
            checksum_        = _rhs.checksum_;
            expiry_ts_       = _rhs.expiry_ts_;
            mode_            = _rhs.mode_;
            r_comment_       = _rhs.r_comment_;
            create_ts_       = _rhs.create_ts_;
            modify_ts_       = _rhs.modify_ts_;
            resc_hier_       = _rhs.resc_hier_;

        } // if

        return *this;

    } // operator=


}; // namespace irods



