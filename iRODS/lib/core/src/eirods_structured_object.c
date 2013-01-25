


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_structured_object.h"
#include "eirods_resource_manager.h"

namespace eirods {

    // =-=-=-=-=-=-=-
	// public - ctor
    structured_object::structured_object() :
                       first_class_object(),
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
                       first_class_object( _rhs ) {
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
	             first_class_object(),
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
        first_class_object::operator=( _rhs );

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
            error init_err = _mgr.init_from_type( "structfile", "struct file", "empty context", _ptr );
            if( !init_err.ok() ) {
                return PASS( false, -1, "structured_object::resolve - failed to load resource plugin", init_err );
            
            }

        } // if !ok

        // =-=-=-=-=-=-=-
        // found ourselves a plugin, fill in the properties
        rodsServerHost_t* tmpRodsServerHost = 0;
        if( resolveHost( &addr_, &tmpRodsServerHost ) < 0 ) {
            std::string msg(  "structured_object::resolve - resolveHost error for " );
            msg += addr_.hostAddr;
            return ERROR( -1, msg );
        }
        
        _ptr->set_property< rodsServerHost_t* >( "host", tmpRodsServerHost );

        _ptr->set_property<long>( "id", -1 );
        _ptr->set_property<long>( "freespace", -1 );
        _ptr->set_property<long>( "quota", -1 );
        
        _ptr->set_property<std::string>( "zone",     addr_.zoneName );
        _ptr->set_property<std::string>( "name",     "structfile" );
        _ptr->set_property<std::string>( "location", addr_.hostAddr );
        _ptr->set_property<std::string>( "type",     "structfile" );
        _ptr->set_property<std::string>( "class",    "cache" );
        _ptr->set_property<std::string>( "path",     physical_path_ );
        _ptr->set_property<std::string>( "info",     "blank info" );
        _ptr->set_property<std::string>( "comments", "blank comments" );
        _ptr->set_property<std::string>( "create",   "create?" );
        _ptr->set_property<std::string>( "modify",   "modify?" );
        _ptr->set_property<int>( "status", INT_RESC_STATUS_UP );

        return SUCCESS();

    } // resolve
    

}; // namespace eirods



