#ifndef __IRODS_RESOURCE_BACKPORT_HPP_
#define __IRODS_RESOURCE_BACKPORT_HPP_

// =-=-=-=-=-=-=-
#include "irods_resource_manager.hpp"

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.hpp"

namespace irods {

// =-=-=-=-=-=-=-
// helper functions for knitting back into legacy irods code
    error resource_to_resc_info( rescInfo_t&, resource_ptr& );
#if 0	// #1472
    error resource_to_resc_grp_info( rescGrpInfo_t&, resource_ptr& );
    error get_resc_grp_info( std::string, rescGrpInfo_t& );
#endif
    error is_resc_live( const std::string& );
    error is_hier_live( const std::string& );
    error set_default_resource( rsComm_t*, std::string, std::string, keyValPair_t*, std::string& );
    error resolve_resource_name( std::string, keyValPair_t*, std::string& );
    error get_host_status_by_host_info( rodsServerHost_t* );
    error get_resc_info( std::string, rescInfo_t& );
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
    error get_resource_property( std::string _name, std::string _prop_name, T& _prop ) {
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

    error get_vault_path_for_hier_string( const std::string& _hier_string, std::string& _rtn_vault_path );
}; // namespace irods

#endif // __IRODS_RESOURCE_BACKPORT_HPP_




