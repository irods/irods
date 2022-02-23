// =-=-=-=-=-=-=-
// My Includes
#include "irods/irods_resource_plugin.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/irods_load_plugin.hpp"

// =-=-=-=-=-=-=-
// STL Includes
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>

#include <boost/lexical_cast.hpp>

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

auto get_resource_name(plugin_context& ctx) -> std::string
{
    std::string resc_name{};
    if (error err = ctx.prop_map().get<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
        THROW(err.code(), err.result());
    }
    return resc_name;
} // get_resource_name

auto get_resource_status(plugin_context& ctx) -> int
{
    int resc_status{}; 
    if (error err = ctx.prop_map().get<int>(RESOURCE_STATUS, resc_status); !err.ok()) {
        const irods::error ret = PASSMSG("Failed to get \"status\" property.", err);
        THROW(ret.code(), ret.result());
    }
    return resc_status;
} // get_resource_status

auto get_resource_location(plugin_context& ctx) -> std::string
{
    std::string host_name{};
    if (error err = ctx.prop_map().get<std::string>(RESOURCE_LOCATION, host_name); !err.ok()) {
        const irods::error ret = PASSMSG("Failed to get \"location\" property.", err);
        THROW(ret.code(), ret.result());
    }
    return host_name;
} // get_resource_location

} // namespace irods

