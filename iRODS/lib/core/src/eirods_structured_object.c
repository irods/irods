/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"
#include "eirods_resource_manager.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - ctor
    structured_object::structured_object() :
        file_object(),
        sub_file_path_(""),
        mode_(0),
        flags_(0),
        offset_(0),
        spec_coll_(0),
        data_type_(""),
        opr_type_(0) {
    } // structured_object

    // =-=-=-=-=-=-=-
    // public - cctor
    structured_object::structured_object( const structured_object& _rhs ) : 
        file_object( _rhs ) {
        addr_          = _rhs.addr_;
        sub_file_path_ = _rhs.sub_file_path_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;
        offset_        = _rhs.offset_;
        spec_coll_     = _rhs.spec_coll_;
        data_type_     = _rhs.data_type_;
        opr_type_      = _rhs.opr_type_;

    } // cctor 

    // =-=-=-=-=-=-=-
    // public - cctor
    structured_object::structured_object( subFile_t& _sub ) :
        file_object(),
        sub_file_path_(""),
        mode_(0),
        flags_(0),
        offset_(0),
        spec_coll_(0),
        data_type_(""),
        opr_type_(0) {

        // =-=-=-=-=-=-=-
        // pull out subFile attributes
        addr_          = _sub.addr;
        sub_file_path_ = _sub.subFilePath;
        mode_          = _sub.mode;
        flags_         = _sub.flags;
        offset_        = _sub.offset;
        spec_coll_     = _sub.specColl;

        // =-=-=-=-=-=-=-
        // file* functions will fail with an empty physical_path_
        physical_path_ = _sub.subFilePath;
        logical_path(spec_coll_->objPath);

    } // structured_object

    // =-=-=-=-=-=-=-
    // public - dtor
    structured_object::~structured_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    structured_object& structured_object::operator=( const structured_object& _rhs ) {
        // =-=-=-=-=-=-=-
        // call base class assignment first
        file_object::operator=( _rhs );

        addr_          = _rhs.addr_;
        sub_file_path_ = _rhs.sub_file_path_;
        mode_          = _rhs.mode_;
        flags_         = _rhs.flags_;
        offset_        = _rhs.offset_;
        spec_coll_     = _rhs.spec_coll_;
        data_type_     = _rhs.data_type_;
        opr_type_      = _rhs.opr_type_;

        return *this;

    }  // operator=

    // =-=-=-=-=-=-=-
    // plugin - resolve resource plugin for this object
    error structured_object::resolve( resource_manager& _mgr, resource_ptr& _ptr ) {

        // =-=-=-=-=-=-=-
        // try to find the resource based on the type 
        eirods::error err = _mgr.resolve( "struct file", _ptr );
        if( err.ok() ) {
            return SUCCESS();

        } else {
            // =-=-=-=-=-=-=-
            // otherwise create a resource and add properties from this object
            error init_err = _mgr.init_from_type( "structfile", "struct file", "struct_file_inst", "empty context", _ptr );
            if( !init_err.ok() ) {
                return PASSMSG( "failed to load resource plugin", init_err );
            
            }

        } // if !ok

        // =-=-=-=-=-=-=-
        // found ourselves a plugin, fill in the properties
        rodsServerHost_t* tmpRodsServerHost = 0;
        int status = resolveHost( &addr_, &tmpRodsServerHost );
        if( status < 0 ) {
            std::stringstream msg;
            msg << "resolveHost error for [";
            msg << addr_.hostAddr;
            return ERROR( status, msg.str() );
        }
        
        _ptr->set_property< rodsServerHost_t* >( RESOURCE_HOST, tmpRodsServerHost );

        _ptr->set_property<long>( RESOURCE_ID, -1 );
        _ptr->set_property<long>( RESOURCE_FREESPACE, -1 );
        _ptr->set_property<long>( RESOURCE_QUOTA, -1 );
        
        _ptr->set_property<int>( RESOURCE_STATUS, INT_RESC_STATUS_UP );
        
        _ptr->set_property<std::string>( RESOURCE_ZONE,      addr_.zoneName );
        _ptr->set_property<std::string>( RESOURCE_NAME,      "structfile" );
        _ptr->set_property<std::string>( RESOURCE_LOCATION,  addr_.hostAddr );
        _ptr->set_property<std::string>( RESOURCE_TYPE,      "structfile" );
        _ptr->set_property<std::string>( RESOURCE_CLASS,     "cache" );
        _ptr->set_property<std::string>( RESOURCE_PATH,      physical_path_ );
        _ptr->set_property<std::string>( RESOURCE_INFO,      "blank info" );
        _ptr->set_property<std::string>( RESOURCE_COMMENTS,  "blank comments" );
        _ptr->set_property<std::string>( RESOURCE_CREATE_TS, "create?" );
        _ptr->set_property<std::string>( RESOURCE_MODIFY_TS, "modify?" );

        return SUCCESS();

    } // resolve
    
    // =-=-=-=-=-=-=-
    // public - get vars from object for rule engine 
    error structured_object::get_re_vars( 
        keyValPair_t& _vars ) {
        return SUCCESS();

    } // get_re_vars 

}; // namespace eirods



