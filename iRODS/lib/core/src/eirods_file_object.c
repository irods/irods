/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"
#include "eirods_resource_manager.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_log.h"
#include "eirods_stacktrace.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_resource_backport.h"

// =-=-=-=-=-=-=-
// irods includes
#include "miscServerFunct.h"
#include "dataObjOpr.h"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/asio/ip/host_name.hpp>

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - ctor
    file_object::file_object() :
        first_class_object(),
        size_(0),
        repl_requested_( -1 ),
        in_pdmo_(false)
    {
        memset(&cond_input_, 0, sizeof(keyValPair_t));
    } // file_object

    // =-=-=-=-=-=-=-
    // public - cctor
    file_object::file_object(
        const file_object& _rhs ) : 
        first_class_object( _rhs ) {
        size_           = _rhs.size_;
        repl_requested_ = _rhs.repl_requested_;
        replicas_       = _rhs.replicas_;
        in_pdmo_        = _rhs.in_pdmo_;
        memset(&cond_input_, 0, sizeof(keyValPair_t));
    } // cctor 

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
        first_class_object(),
        size_( -1 ),
        in_pdmo_(false)
    {
        logical_path(_logical_path);
        comm_            = _c;
        physical_path_   = _fn;
        resc_hier_       = _resc_hier;
        file_descriptor_ = _fd;
        mode_            = _m;
        flags_           = _f;
        repl_requested_  = -1;
        replicas_.empty();
        memset(&cond_input_, 0, sizeof(keyValPair_t));
    } // file_object

    // from dataObjInfo
    file_object::file_object(
        rsComm_t* _rsComm,
        const dataObjInfo_t* _dataObjInfo)
    {
        comm_ = _rsComm;
        logical_path(_dataObjInfo->objPath);
        physical_path_ = _dataObjInfo->filePath;
        resc_hier_ = _dataObjInfo->rescHier;
        flags_ = _dataObjInfo->flags;
        repl_requested_ = _dataObjInfo->replNum;
        replicas_.empty();
        // should mode be set here? - hcj
        memset(&cond_input_, 0, sizeof(keyValPair_t));
    }
    
    // =-=-=-=-=-=-=-
    // public - dtor
    file_object::~file_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    file_object& file_object::operator=(
        const file_object& _rhs ) {
        // =-=-=-=-=-=-=-
        // call base class assignment first
        first_class_object::operator=( _rhs );

        size_           = _rhs.size_;
        repl_requested_ = _rhs.repl_requested_;
        replicas_       = _rhs.replicas_;
        in_pdmo_        = _rhs.in_pdmo_;
        replKeyVal(&_rhs.cond_input_, &cond_input_);
        
        return *this;

    }  // operator=

    // public comparison operator
    bool file_object::operator==(
        const file_object& _rhs) const
    {
        bool result = true;
        if(this->repl_requested() != _rhs.repl_requested() ||
           this->logical_path() != _rhs.logical_path()) {
            result = false;
        }
        return result;
    }
    
    // =-=-=-=-=-=-=-
    // plugin - resolve resource plugin for this object
    error file_object::resolve(
        resource_manager& _mgr,
        resource_ptr& _ptr ) {
        
        error result = SUCCESS();
        error ret;
    
        hierarchy_parser hparse;
        ret = hparse.set_string(resc_hier());
    
        if(!ret.ok()) {
            std::stringstream msg;
            msg << "error parsing resource hierarchy \"" << resc_hier() << "\"";
            result = PASSMSG(msg.str(), ret);
        } else {
            std::string resc;
    
            ret = hparse.first_resc(resc);
            if(!ret.ok()) {
                std::stringstream msg;
                msg << __FUNCTION__ << " - ERROR getting first resource from hierarchy.";
                result = PASSMSG(msg.str(), ret);
            } else {
    
                if(resc.empty() && resc_hier().empty()) {
                    //std::stringstream msg;
                    //msg << __FUNCTION__ 
                    //    << " - there is no resource specified in the resource hierarchy.";
                    //log(LOG_NOTICE, msg.str());
                } else if(resc.empty()) {
                    return ERROR( EIRODS_HIERARCHY_ERROR, "Hierarchy string is not empty but first resource is!");
                }
    
                ret = _mgr.resolve(resc, _ptr);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - ERROR resolving resource \"" << resc << "\"";
                    result = PASSMSG(msg.str(), ret);
                } 
            }
        }
        return result;
    } // resolve

    // =-=-=-=-=-=-=-
    // static factory to create file_object from dataobjinfo linked list
    error file_object_factory( rsComm_t*     _comm,
                               dataObjInp_t* _data_obj_inp,
                               file_object&  _file_obj ) {
        // =-=-=-=-=-=-=-
        // make a call to build the linked list 
        dataObjInfo_t* head_ptr = 0;
        
        int status = getDataObjInfoIncSpecColl ( _comm, _data_obj_inp, &head_ptr );
        if( status < 0 ) {
            status = getDataObjInfo( _comm, _data_obj_inp, &head_ptr, 0, 0 );
        }
        if( 0 == head_ptr || status < 0 ) {
            char* sys_error;
            char* rods_error = rodsErrorName(status, &sys_error);
            std::stringstream msg;
            msg << "failed in call to getDataObjInfoIncSpecColl";
            msg << " for [";
            msg << _data_obj_inp->objPath;
            msg << "] ";
            msg << rods_error << " " << sys_error;
            free( head_ptr->rescInfo );
            free( head_ptr );
            return ERROR( status, msg.str() );    
        }

        // =-=-=-=-=-=-=-
        // start populating file_object
        _file_obj.comm( _comm );
        _file_obj.logical_path( _data_obj_inp->objPath );
        _file_obj.resc_hier( head_ptr->rescHier );
        _file_obj.cond_input( _data_obj_inp->condInput );
        
        // =-=-=-=-=-=-=-
        // handle requested repl number
        char* repl_num = getValByKey( &_data_obj_inp->condInput, REPL_NUM_KW );
        if( repl_num ) {
            _file_obj.repl_requested( atoi( repl_num ) );
        }

        // handle the case where we are being called as part of a pdmo
        char* in_pdmo = getValByKey(&_data_obj_inp->condInput, IN_PDMO_KW);
        if(in_pdmo) {
            _file_obj.in_pdmo(true);
        }
        
        // =-=-=-=-=-=-=-
        // iterate over the linked list and populate 
        // the physical_object vector in the file_object
        dataObjInfo_t* info_ptr = head_ptr;
        std::vector< physical_object > objects;
        while( info_ptr ) {
            physical_object obj;

            obj.repl_num( info_ptr->replNum );
            obj.map_id( info_ptr->dataMapId );
            obj.size( info_ptr->dataSize );
            obj.id( info_ptr->dataId );
            obj.coll_id( info_ptr->collId );
            obj.name( info_ptr->objPath );
            obj.version( info_ptr->version );
            obj.type_name( info_ptr->dataType );
            obj.resc_group_name( info_ptr->rescGroupName );
            obj.resc_name( info_ptr->rescName );
            obj.path( info_ptr->filePath );
            obj.owner_name( info_ptr->dataOwnerName );
            obj.owner_zone( info_ptr->dataOwnerZone );
            obj.status( info_ptr->statusString );
            obj.checksum( info_ptr->chksum );
            obj.expiry_ts( info_ptr->dataExpiry );
            obj.mode( info_ptr->dataMode );
            obj.r_comment( info_ptr->dataComments );
            obj.create_ts( info_ptr->dataCreate );
            obj.modify_ts( info_ptr->dataModify );
            obj.resc_hier( info_ptr->rescHier );
     
            objects.push_back( obj );

            info_ptr = info_ptr->next;

        } // while

        _file_obj.replicas( objects );

        free( head_ptr->rescInfo );
        free( head_ptr );
        return SUCCESS();

    } // file_object_factory

}; // namespace eirods



