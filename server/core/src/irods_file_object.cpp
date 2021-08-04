#include "miscServerFunct.hpp"
#include "dataObjOpr.hpp"
#include "objDesc.hpp"

#include "irods_file_object.hpp"
#include "irods_hierarchy_parser.hpp"
#include "irods_log.hpp"
#include "irods_resource_backport.hpp"
#include "irods_resource_manager.hpp"
#include "irods_stacktrace.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "filesystem.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica_proxy.hpp"
#include "data_object_proxy.hpp"

#include "fmt/format.h"

namespace {
    namespace id = irods::experimental::data_object;
} // anonymous namespace

namespace irods {
// =-=-=-=-=-=-=-
// public - ctor
    file_object::file_object() :
        data_object(),
        comm_( NULL ),
        file_descriptor_( -1 ),
        l1_desc_idx_( -1 ),
        size_( 0 ),
        repl_requested_( -1 ) {
    } // file_object

// =-=-=-=-=-=-=-
// public - cctor
    file_object::file_object(
        const file_object& _rhs ) :
        data_object( _rhs ) {
        // =-=-=-=-=-=-=-
        // explicit initialization
        comm_           = _rhs.comm_;
        data_id_        = _rhs.data_id_;
        coll_id_        = _rhs.coll_id_;
        logical_path_   = _rhs.logical_path_;
        data_type_      = _rhs.data_type_;
        file_descriptor_ = _rhs.file_descriptor_;
        l1_desc_idx_    = _rhs.l1_desc_idx_;
        size_           = _rhs.size_;
        repl_requested_ = _rhs.repl_requested_;
        replicas_       = _rhs.replicas_;
        in_pdmo_        = _rhs.in_pdmo_;
    } // cctor

// =-=-=-=-=-=-=-
// public - ctor
    file_object::file_object(
        rsComm_t* _c,
        const std::string& _logical_path,
        const std::string& _fn,
        rodsLong_t         _resc_id,
        int _fd,
        int _m,
        int _f ) :
        data_object(
            _fn,
            _resc_id,
            _m,
            _f ),
        comm_( _c ),
        logical_path_( _logical_path ),
        data_type_( "" ),
        file_descriptor_( _fd ),
        l1_desc_idx_( -1 ),
        size_( 0 ),
        repl_requested_( -1 ) {
        // =-=-=-=-=-=-=-
        // explicit initialization
        replicas_.clear();
    } // file_object

// =-=-=-=-=-=-=-
// public - ctor
    file_object::file_object(
        rsComm_t* _c,
        const std::string& _logical_path,
        const std::string& _fn,
        const std::string& _resc_hier,
        int _fd,
        int _m,
        int _f ) :
        data_object(
            _fn,
            _resc_hier,
            _m,
            _f ),
        comm_( _c ),
        logical_path_( _logical_path ),
        data_type_( "" ),
        file_descriptor_( _fd ),
        l1_desc_idx_( -1 ),
        size_( 0 ),
        repl_requested_( -1 ) {
        // =-=-=-=-=-=-=-
        // explicit initialization
        replicas_.clear();
    } // file_object


// from dataObjInfo
    file_object::file_object(
        RsComm*            _rsComm,
        const DataObjInfo* _dataObjInfo)
    {
        data_type_      = _dataObjInfo->dataType;
        comm_           = _rsComm;
        data_id_        = _dataObjInfo->dataId;
        coll_id_        = _dataObjInfo->collId;
        logical_path_   = _dataObjInfo->objPath;
        physical_path_  = _dataObjInfo->filePath;
        resc_hier_      = _dataObjInfo->rescHier;
        resc_id_        = _dataObjInfo->rescId;
        flags_          = _dataObjInfo->flags;
        repl_requested_ = _dataObjInfo->replNum;
        replicas_.clear();
        replKeyVal( &_dataObjInfo->condInput, &cond_input_ );
        size_ = _dataObjInfo->dataSize;
        l1_desc_idx_ = -1;
        file_descriptor_ = -1;
    }

// =-=-=-=-=-=-=-
// public - dtor
    file_object::~file_object() {
        clearKeyVal( &cond_input_ );
    } // dtor

// =-=-=-=-=-=-=-
// public - assignment operator
    file_object& file_object::operator=(
        const file_object& _rhs ) {
        // =-=-=-=-=-=-=-
        // call base class assignment first
        data_object::operator=( _rhs );

        comm_           = _rhs.comm_;
        data_id_        = _rhs.data_id_;
        coll_id_        = _rhs.coll_id_;
        logical_path_   = _rhs.logical_path_;
        data_type_      = _rhs.data_type_;
        file_descriptor_ = _rhs.file_descriptor_;
        l1_desc_idx_    = _rhs.l1_desc_idx_;
        size_           = _rhs.size_;
        repl_requested_ = _rhs.repl_requested_;
        replicas_       = _rhs.replicas_;
        in_pdmo_        = _rhs.in_pdmo_;
        replKeyVal( &_rhs.cond_input_, &cond_input_ );

        return *this;

    }  // operator=

// public comparison operator
    bool file_object::operator==(
        const file_object& _rhs ) const {
        bool result = true;
        if ( this->repl_requested() != _rhs.repl_requested() ||
                this->logical_path() != _rhs.logical_path() ) {
            result = false;
        }
        return result;
    }

// =-=-=-=-=-=-=-
// plugin - resolve resource plugin for this object
    error file_object::resolve(
        const std::string& _interface,
        plugin_ptr&        _ptr ) {
        // =-=-=-=-=-=-=-
        // check to see if this is for a resource plugin
        // resolution, otherwise it is an error
        if ( RESOURCE_INTERFACE != _interface ) {
            std::stringstream msg;
            msg << "file_object does not support a [";
            msg << _interface;
            msg << "] for plugin resolution";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

        error result = SUCCESS();
        error ret;

        hierarchy_parser hparse;
        ret = hparse.set_string( resc_hier() );

        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << "error parsing resource hierarchy \"" << resc_hier() << "\"";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            std::string resc;

            ret = hparse.first_resc( resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - ERROR getting first resource from hierarchy.";
                result = PASSMSG( msg.str(), ret );
            }
            else {

                if ( resc.empty() && resc_hier().empty() ) {
                    //std::stringstream msg;
                    //msg << __FUNCTION__
                    //    << " - there is no resource specified in the resource hierarchy.";
                    //log(LOG_NOTICE, msg.str());
                }
                else if ( resc.empty() ) {
                    return ERROR( HIERARCHY_ERROR, "Hierarchy string is not empty but first resource is!" );
                }

                resource_ptr resc_ptr;
                ret = resc_mgr.resolve( resc, resc_ptr );
                if ( !ret.ok() ) {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - ERROR resolving resource \"" << resc << "\"";
                    result = PASSMSG( msg.str(), ret );
                }

                _ptr = boost::dynamic_pointer_cast< resource >( resc_ptr );
            }
        }

        return result;

    } // resolve

// =-=-=-=-=-=-=-
// public - get vars from object for rule engine
    error file_object::get_re_vars(
        rule_engine_vars_t& _kvp ) {
        data_object::get_re_vars( _kvp );
        _kvp[LOGICAL_PATH_KW] = logical_path_.c_str();
        _kvp[DATA_TYPE_KW]    = data_type_.c_str();

        std::stringstream fd;
        fd << file_descriptor_;
        _kvp[FILE_DESCRIPTOR_KW] = fd.str().c_str();

        std::stringstream idx;
        idx << l1_desc_idx_;
        _kvp[L1_DESC_IDX_KW] = idx.str().c_str();

        std::stringstream sz;
        sz << size_;
        _kvp[SIZE_KW] = sz.str().c_str();

        std::stringstream repl;
        repl << repl_requested_;
        _kvp[REPL_REQUESTED_KW] = repl.str().c_str();

        std::stringstream pdmo;
        pdmo << in_pdmo_;
        _kvp[IN_PDMO_KW] = pdmo.str().c_str();

        // TODO serialize physical objects ?

        return SUCCESS();

    } // get_re_vars

    auto file_object::get_replica(const int _replica_number) -> std::optional<std::reference_wrapper<physical_object>>
    {
        auto itr = std::find_if(
            std::begin(replicas_), std::end(replicas_),
            [&_replica_number](const auto& _r) { return _replica_number == _r.repl_num(); }
        );

        return std::cend(replicas_) == itr ? std::nullopt : std::make_optional(std::ref(*itr));
    } // get_replica

    auto file_object::get_replica(std::string_view _hierarchy) -> std::optional<std::reference_wrapper<physical_object>>
    {
        auto itr = std::find_if(
            std::begin(replicas_), std::end(replicas_),
            [&_hierarchy](const auto& _r) { return _hierarchy == _r.resc_hier(); }
        );

        return std::cend(replicas_) == itr ? std::nullopt : std::make_optional(std::ref(*itr));
    } // get_replica

// =-=-=-=-=-=-=-
// static factory to create file_object from dataObjInfo linked list
    error file_object_factory( rsComm_t*        _comm,
                               dataObjInp_t*    _data_obj_inp,
                               file_object_ptr  _file_obj,
                               dataObjInfo_t**  _data_obj_info)
    {
        if (!_comm || !_data_obj_inp) {
            return ERROR(SYS_INTERNAL_NULL_INPUT_ERR, "some inputs were null");
        }

        // start populating file_object
        _file_obj->comm( _comm );
        _file_obj->logical_path( _data_obj_inp->objPath );
        _file_obj->cond_input( _data_obj_inp->condInput );
        _file_obj->size( _data_obj_inp->dataSize );

        // handle the case where we are being called as part of a pdmo
        const char* in_pdmo = getValByKey( &_data_obj_inp->condInput, IN_PDMO_KW );
        if ( in_pdmo ) {
            _file_obj->in_pdmo( in_pdmo );
        }

        // =-=-=-=-=-=-=-
        // if a repl is requested, cache that fact
        const char* repl_num = getValByKey( &_data_obj_inp->condInput, REPL_NUM_KW );
        if (repl_num) {
            try {
                _file_obj->repl_requested(std::stoi(repl_num));
            }
            catch (const std::invalid_argument& e) {
                return ERROR(USER_INVALID_REPLICA_INPUT, fmt::format("invalid replica number argument:[{}]", repl_num));
            }
            catch (const std::out_of_range& e) {
                return ERROR(USER_INVALID_REPLICA_INPUT, fmt::format("replica number is out of range:[{}]", repl_num));
            }
        }

        // =-=-=-=-=-=-=-
        // make a call to build the linked list
        dataObjInfo_t* head_ptr = nullptr;

        int status = getDataObjInfoIncSpecColl( _comm, _data_obj_inp, &head_ptr );

        if ( !head_ptr || status < 0 ) {
            if ( head_ptr ) {
                freeAllDataObjInfo( head_ptr );
            }

            if (repl_num && status == CAT_NO_ROWS_FOUND) {
                return ERROR(SYS_REPLICA_DOES_NOT_EXIST, "replica does not exist");
            }

            char* sys_error = nullptr;
            const char* rods_error = rodsErrorName( status, &sys_error );

            std::stringstream msg;
            msg << "failed in call to getDataObjInfoIncSpecColl";
            msg << " for [";
            msg << _data_obj_inp->objPath;
            msg << "] ";
            msg << rods_error << " " << sys_error;

            free( sys_error );

            return ERROR( status, msg.str() );
        }

        _file_obj->resc_hier( head_ptr->rescHier );
        _file_obj->data_id(head_ptr->dataId);
        _file_obj->coll_id(head_ptr->collId);

        // If the repl_requested value is less than 0, this means a REPL_NUM_KW was not
        // provided and so finding a specific replica number is not of concern. If the
        // REPL_NUM_KW was provided, we need to make sure that the replica exists before
        // we return the information as it is an error if the requested replica number
        // does not exist.
        bool found_replica_number = _file_obj->repl_requested() < 0 ? true : false;

        // =-=-=-=-=-=-=-
        // iterate over the linked list and populate
        // the physical_object vector in the file_object
        dataObjInfo_t* info_ptr = head_ptr;
        std::vector< physical_object > objects;
        while ( info_ptr ) {
            if (!found_replica_number && info_ptr->replNum == _file_obj->repl_requested()) {
                found_replica_number = true;
            }

            objects.push_back(irods::physical_object{*info_ptr});

            if (info_ptr->dataSize <= 0) {
                auto& obj = objects.back();
                obj.size(_data_obj_inp->dataSize);
            }

            info_ptr = info_ptr->next;
        } // while

        if (!found_replica_number) {
            freeAllDataObjInfo(head_ptr);
            return ERROR(SYS_REPLICA_DOES_NOT_EXIST, "replica does not exist");
        }

        _file_obj->replicas( objects );

        //delete head_ptr->rescInfo;
        //free( head_ptr );
        if (_data_obj_info) {
            *_data_obj_info = head_ptr;
        }
        else {
            freeAllDataObjInfo( head_ptr );
        }
        return SUCCESS();

    } // file_object_factory

    auto file_object_factory(RsComm& _comm, const rodsLong_t _data_id) -> irods::file_object_ptr
    {
        auto [data_obj, obj_lm] = id::make_data_object_proxy(_comm, _data_id);

        irods::file_object_ptr obj{new irods::file_object{&_comm, data_obj.get()}};

        std::vector<physical_object> objects;

        for (const auto& r : data_obj.replicas()) {
            objects.push_back(irods::physical_object{*r.get()});
        }

        obj->replicas( objects );

        return obj;
    } // file_object_factory

    auto file_object_factory(
        RsComm& _comm,
        const std::string_view _logical_path,
        const id::json_repr_t& _replicas) -> irods::file_object_ptr
    {
        auto [data_obj, obj_lm] = id::make_data_object_proxy(_logical_path, _replicas);

        irods::file_object_ptr obj{new irods::file_object{&_comm, data_obj.get()}};

        std::vector<physical_object> objects;

        for (auto& r : data_obj.replicas()) {
            // Hierarchy information is not stored in the catalog
            r.hierarchy(resc_mgr.leaf_id_to_hier(r.resource_id()));
            r.resource(irods::hierarchy_parser{r.hierarchy().data()}.last_resc());

            objects.push_back(irods::physical_object{*r.get()});
        }

        obj->replicas( objects );

        return obj;
    } // file_object_factory

    auto hierarchy_has_replica(const irods::file_object_ptr _obj, std::string_view _hierarchy) -> bool
    {
        return std::any_of(std::cbegin(_obj->replicas()), std::cend(_obj->replicas()),
            [&_hierarchy](const irods::physical_object& _r) {
                return _r.resc_hier() == _hierarchy;
            });
    } // hierarchy_has_replica
} // namespace irods
