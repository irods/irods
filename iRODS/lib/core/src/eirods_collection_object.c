/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_collection_object.h"
#include "eirods_resource_manager.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - ctor
    collection_object::collection_object() :
        first_class_object(),
        directory_pointer_(0) {
    } // collection_object

    // =-=-=-=-=-=-=-
    // public - cctor
    collection_object::collection_object( const collection_object& _rhs ) : 
        first_class_object( _rhs ) {
        directory_pointer_ = _rhs.directory_pointer_;

    } // cctor 

    // =-=-=-=-=-=-=-
    // public - ctor
    collection_object::collection_object( std::string _fn, int _m, int _f ) :
        first_class_object(),
        directory_pointer_(0) {
        physical_path_   = _fn;
        mode_            = _m;
        flags_           = _f;
    } // collection_object

    // =-=-=-=-=-=-=-
    // public - dtor
    collection_object::~collection_object() {
    } // dtor

    // =-=-=-=-=-=-=-
    // public - assignment operator
    collection_object& collection_object::operator=( const collection_object& _rhs ) {
        // =-=-=-=-=-=-=-
        // call base class assignment first
        first_class_object::operator=( _rhs );

        directory_pointer_  = _rhs.directory_pointer_;

        return *this;

    }  // operator=

    // =-=-=-=-=-=-=-
    // plugin - resolve resource plugin for this object
    error collection_object::resolve( resource_manager& _mgr, resource_ptr& _ptr ) {
        _mgr.resolve( *this, _ptr ); 

        std::string type;
        error ret = _ptr->get_property< std::string >( "type", type );
        if( ret.ok() ) {
            if( "unix file system" != type ) {
                std::stringstream msg;
                msg << "[+]\tcollection_object::resolve - warning :: ";
                msg << " did not resolve a unix file system resource type [";
                msg << type;
                msg << "]";
                eirods::log( LOG_NOTICE, msg.str() );
            }
        } else {
            std::stringstream msg;
            msg << "collection_object::resolve - failed in call to get_property";
            return PASSMSG( msg.str(), ret );
        }

        return SUCCESS();

    } // resolve

}; // namespace eirods



