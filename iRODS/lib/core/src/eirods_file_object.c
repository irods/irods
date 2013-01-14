/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_file_object.h"
#include "eirods_resource_manager.h"
#include "eirods_hierarchy_parser.h"
#include "eirods_log.h"
#include "eirods_stacktrace.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // public - ctor
    file_object::file_object() :
        first_class_object(),
        size_(0) {
    } // file_object

    // =-=-=-=-=-=-=-
    // public - cctor
    file_object::file_object(
        const file_object& _rhs ) : 
        first_class_object( _rhs ) {
        size_  = _rhs.size_;

    } // cctor 

    // =-=-=-=-=-=-=-
    // public - ctor
    file_object::file_object(
        rsComm_t* _c,
        const std::string& _fn,
        const std::string& _resc_hier,
        int _fd,
        int _m,
        int _f ) :
        first_class_object(),
        size_( -1 ) {
        comm_            = _c;
        physical_path_   = _fn;
        resc_hier_       = _resc_hier;
        file_descriptor_ = _fd;
        mode_            = _m;
        flags_           = _f;
    } // file_object

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

        size_  = _rhs.size_;

        return *this;

    }  // operator=

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
            msg << __FUNCTION__ << " - ERROR parsing resource hierarchy \"" << resc_hier() << "\"";
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
                    //msg << __FUNCTION__ << " - there is no resource specified in the resource hierarchy.";
                    //log(LOG_NOTICE, msg.str());
                } else if(resc.empty()) {
                    return ERROR(-1, "ERROR: Hierarchy string is not empty but first resource is!");
                } else {
                    std::stringstream msg;
                    log(LOG_NOTICE, msg.str());
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
    
}; // namespace eirods



