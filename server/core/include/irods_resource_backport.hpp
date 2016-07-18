#ifndef __IRODS_RESOURCE_BACKPORT_HPP_
#define __IRODS_RESOURCE_BACKPORT_HPP_

// =-=-=-=-=-=-=-
#include "irods_resource_manager.hpp"
#include "irods_hierarchy_parser.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// helper functions for knitting back into legacy irods code
    error resource_to_kvp(resource_ptr&, keyValPair_t*);
    error get_resc_properties_as_kvp(const std::string&, keyValPair_t*);
    error is_resc_live( rodsLong_t );
    error is_hier_live( const std::string& );
    error set_default_resource( rsComm_t*, const std::string&, const std::string&, keyValPair_t*, std::string& );
    error resolve_resource_name( const std::string&, keyValPair_t*, std::string& );
    error get_host_status_by_host_info( rodsServerHost_t* );
    error get_host_for_hier_string(
        const std::string&,   // hier string
        int&,                 // local flag
        rodsServerHost_t*& ); // server host
    error get_resc_type_for_hier_string( const std::string&, std::string& );

// =-=-=-=-=-=-=-
/// @brief function which returns the host name for a given hier string
    error get_loc_for_hier_string(
        const std::string& _hier, // hier string
        std::string& _loc );// location

    template< typename T >
    error get_resource_property( rodsLong_t _resc_id, const std::string& _prop_name, T& _prop ) {
        // =-=-=-=-=-=-=-
        // resolve the resource by name
        resource_ptr resc;
        error res_err = resc_mgr.resolve( _resc_id, resc );
        if ( !res_err.ok() ) {
            std::stringstream msg;
            msg << "failed to resolve resource [";
            msg << _prop_name;
            msg << "]";
            return PASSMSG( msg.str(), res_err );
        }

        // =-=-=-=-=-=-=-
        // get the resource property
        error get_err = resc->get_property< T >( _prop_name, _prop );
        if ( !get_err.ok() ) {
            std::stringstream msg;
            msg << "failed to get property [";
            msg << _prop_name;
            msg << "]";
            return PASSMSG( msg.str(), get_err );
        }

        return SUCCESS();

    } // get_resource_property

    template< typename T >
    error set_resource_property( std::string _name, const std::string& _prop_name, T& _prop ) {
        // =-=-=-=-=-=-=-
        // resolve the resource by name
        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if ( !res_err.ok() ) {
            std::stringstream msg;
            msg << "failed to resolve resource [";
            msg << _prop_name;
            msg << "]";
            return PASSMSG( msg.str(), res_err );
        }

        // =-=-=-=-=-=-=-
        // set the resource property
        error set_err = resc->set_property< T >( _prop_name, _prop );
        if ( !set_err.ok() ) {
            std::stringstream msg;
            msg << "failed to set property [";
            msg << _prop_name;
            msg << "]";
            return PASSMSG( msg.str(), set_err );
        }

        return SUCCESS();

    } // set_resource_property

    /// @brief Returns a given property of the specified hierarchy string's leaf resource
    template< typename T >
    error get_resc_hier_property(
        const std::string& _hier_string,
        const std::string& _prop_name,
        T& _prop ) {

        error result = SUCCESS();
        error ret;
        hierarchy_parser parser;
        ret = parser.set_string( _hier_string );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to parse hierarchy string: \"" << _hier_string << "\"";
            result = PASSMSG( msg.str(), ret );
        }
        else {
            std::string last_resc;
            ret = parser.last_resc( last_resc );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to get last resource in hierarchy: \"" << _hier_string << "\"";
                result = PASSMSG( msg.str(), ret );
            }
            else {
                ret = get_resource_property<T>( last_resc, _prop_name, _prop );
                if ( !ret.ok() ) {
                    std::stringstream msg;
                    msg << __FUNCTION__;
                    msg << " - Failed to get property: \"" << _prop_name << "\" from resource: \"" << last_resc << "\"";
                    result = PASSMSG( msg.str(), ret );
                }
            }
        }

        return result;
    }

    error get_vault_path_for_hier_string( const std::string& _hier_string, std::string& _rtn_vault_path );
}; // namespace irods

#endif // __IRODS_RESOURCE_BACKPORT_HPP_




