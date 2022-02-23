#include "irods/irods_physical_object.hpp"

namespace irods
{
// =-=-=-=-=-=-=-
// public - default constructor
    physical_object::physical_object()
        : replica_status_{-1}
        , repl_num_{-1}
        , map_id_{-1}
        , size_{-1}
        , id_{-1}
        , coll_id_{-1}
    {
    } // ctor

// =-=-=-=-=-=-=-
// public - copy constructor
    physical_object::physical_object( const physical_object& _rhs )
        : replica_status_{_rhs.replica_status_}
        , repl_num_{_rhs.repl_num_}
        , map_id_{_rhs.map_id_}
        , size_{_rhs.size_}
        , id_{_rhs.id_}
        , coll_id_{_rhs.coll_id_}
        , name_{_rhs.name_}
        , version_{_rhs.version_}
        , type_name_{_rhs.type_name_}
        , resc_name_{_rhs.resc_name_}
        , path_{_rhs.path_}
        , owner_name_{_rhs.owner_name_}
        , owner_zone_{_rhs.owner_zone_}
        , status_{_rhs.status_}
        , checksum_{_rhs.checksum_}
        , expiry_ts_{_rhs.expiry_ts_}
        , mode_{_rhs.mode_}
        , r_comment_{_rhs.r_comment_}
        , create_ts_{_rhs.create_ts_}
        , modify_ts_{_rhs.modify_ts_}
        , resc_hier_{_rhs.resc_hier_}
        , resc_id_{_rhs.resc_id_}
        , vote_{_rhs.vote_}
    {
    } // cctor

    physical_object::physical_object(const DataObjInfo& _rhs)
        : replica_status_{_rhs.replStatus}
        , repl_num_{_rhs.replNum}
        , map_id_{_rhs.dataMapId}
        , size_{_rhs.dataSize}
        , id_{_rhs.dataId}
        , coll_id_{_rhs.collId}
        , name_{_rhs.objPath}
        , version_{_rhs.version}
        , type_name_{_rhs.dataType}
        , resc_name_{_rhs.rescName}
        , path_{_rhs.filePath}
        , owner_name_{_rhs.dataOwnerName}
        , owner_zone_{_rhs.dataOwnerZone}
        , status_{_rhs.statusString}
        , checksum_{_rhs.chksum}
        , expiry_ts_{_rhs.dataExpiry}
        , mode_{_rhs.dataMode}
        , r_comment_{_rhs.dataComments}
        , create_ts_{_rhs.dataCreate}
        , modify_ts_{_rhs.dataModify}
        , resc_hier_{_rhs.rescHier}
        , resc_id_{_rhs.rescId}
        , vote_{-1}
    {
    }

// =-=-=-=-=-=-=-
// public - destructor
    physical_object::~physical_object() {
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    physical_object& physical_object::operator=( const physical_object& _rhs ) {
        if ( this != &_rhs ) {
            replica_status_  = _rhs.replica_status_;
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
            resc_id_         = _rhs.resc_id_;
            vote_            = _rhs.vote_;

        } // if

        return *this;

    } // operator=


}; // namespace irods



