


#ifndef __EIRODS_RESOURCE_BACKPORT_H_
#define __EIRODS_RESOURCE_BACKPORT_H_

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_resource_manager.h"

// =-=-=-=-=-=-=-
// irods includes
#include "rsGlobalExtern.h"

namespace eirods {

    // =-=-=-=-=-=-=-
    // helper functions for knitting back into legacy irods code
    error resource_to_resc_info( rescInfo_t&, resource_ptr& );
    error resource_to_resc_grp_info( rescGrpInfo_t&, resource_ptr& );
    error set_default_resource( rsComm_t*, std::string, std::string, keyValPair_t*, rescGrpInfo_t& );
    error resolve_resource_name( std::string, keyValPair_t*, std::string& );
     
    error get_host_status_by_host_info( rodsServerHost_t* );
    error get_resc_info( std::string, rescInfo_t& );
    error get_resc_grp_info( std::string, rescGrpInfo_t& );
    error get_host_for_hier_string( 
              const std::string&,   // hier string
              int&,                 // local flag
              rodsServerHost_t*& ); // server host

    template< typename T >
    error get_resource_property( std::string _name, std::string _prop_name, T& _prop ) {
        // =-=-=-=-=-=-=-
        // resolve the resource by name
        resource_ptr resc;
        error res_err = resc_mgr.resolve( _name, resc );
        if( !res_err.ok() ) {
            std::stringstream msg;
            msg << "get_resource_property - failed to resolve resource [";
            msg << _prop_name;
            msg << "]";
            return PASS( false, -1, msg.str(), res_err );
        }

        // =-=-=-=-=-=-=-
        // get the resource property 
        error get_err = resc->get_property< T >( _prop_name, _prop );
        if( !get_err.ok() ) {
            std::stringstream msg;
            msg << "get_resource_property - failed to get property [";
            msg << _prop_name;
            msg << "]";
            return PASS( false, -1, msg.str(), get_err );
        }

        return SUCCESS();

    } // get_resource_property

}; // namespace eirods

#endif // __EIRODS_RESOURCE_BACKPORT_H_




