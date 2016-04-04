// =-=-=-=-=-=-=-
// My Includes
#include "irods_resource_plugin.hpp"
#include "irods_load_plugin.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

// =-=-=-=-=-=-=-
// dlopen, etc
#include <dlfcn.h>

namespace irods {

    error resource::add_child(
            const std::string& _name,
            const std::string& _data,
            resource_ptr       _resc ) {
        // =-=-=-=-=-=-=-
        // check params
        if ( _name.empty() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "empty name" );
        }

        if ( 0 == _resc.get() ) {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null resource pointer" );
        }

        // =-=-=-=-=-=-=-
        // add resource and data to the table
        children_[ _name ] = std::make_pair( _data, _resc );

        return SUCCESS();

    } // add_child

    error resource::remove_child( const std::string& _name ) {
        // =-=-=-=-=-=-=-
        // if an entry exists, erase it otherwise issue a warning.
        if ( children_.has_entry( _name ) ) {
            children_.erase( _name );
            return SUCCESS();
        }
        else {
            std::stringstream msg;
            msg << "resource has no child named [" << _name << "]";
            return ERROR( SYS_INVALID_INPUT_PARAM, msg.str() );
        }

    } // remove_child

    error resource::set_parent( const resource_ptr& _resc ) {
        parent_ = _resc;
        return SUCCESS();

    } // set_parent

    error resource::get_parent( resource_ptr& _resc ) {
        _resc = parent_;
        if ( _resc.get() ) {
            return SUCCESS();
        }
        else {
            return ERROR(
                       SYS_INVALID_INPUT_PARAM,
                       "null parent pointer" );
        }

    } // get_parent

    void resource::children(
        std::vector<std::string>& _out ) {
        for( auto c_itr : children_ ) {
            _out.push_back( c_itr.first );
        }
    } // children

}; // namespace irods
