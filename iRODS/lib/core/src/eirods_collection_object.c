/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_collection_object.h"
#include "eirods_resource_manager.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_stacktrace.h"

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
    collection_object::collection_object(
        const std::string& _fn,
        const std::string& _resc_hier,
        int _m,
        int _f ) :
        first_class_object(),
        directory_pointer_(0)
    {
        physical_path_   = _fn;
        mode_            = _m;
        flags_           = _f;
        resc_hier_       = _resc_hier;
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
    error collection_object::resolve(
        resource_manager& _mgr,
        resource_ptr& _ptr )
    {
        error result = SUCCESS();
        error ret;
    
        hierarchy_parser hparse;
        ret = hparse.set_string(resc_hier());
    
        if(!ret.ok()) {
            std::stringstream msg;
            msg << __FUNCTION__ << " - ";
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
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - No resource hierarchy or resource specified.";
                    return ERROR(EIRODS_HIERARCHY_ERROR, msg.str());
                } else if(resc.empty()) {
                    return ERROR( EIRODS_HIERARCHY_ERROR, "Hierarchy string is not empty but first resource is!");
                }
    
                ret = _mgr.resolve(resc, _ptr);
                if(!ret.ok()) {
                    std::stringstream msg;
                    msg << __FUNCTION__ << " - ERROR resolving resource \"" << resc << "\"";
                    result = PASSMSG(msg.str(), ret);

                    if(true) {
                        eirods::stacktrace st;
                        st.trace();
                        st.dump();
                    }

                } 
            }
        }
        return result;
    } // resolve

}; // namespace eirods



