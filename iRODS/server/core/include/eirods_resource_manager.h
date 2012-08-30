


#ifndef __EIRODS_RESOURCE_MANAGER_H__
#define __EIRODS_RESOURCE_MANAGER_H__

// =-=-=-=-=-=-=-
// My Includes
#include "eirods_resource_plugin.h"

namespace eirods {

    class resource_manager {
        public:
		
		// =-=-=-=-=-=-=-
		// constructor
        resource_manager();
        resource_manager( const resource_manager& );
		
		// =-=-=-=-=-=-=-
		// destructor
        virtual ~resource_manager();

        // =-=-=-=-=-=-=-
		// interface to get a resource given a name
		error resolve( std::string, resource_ptr& );
        
        // =-=-=-=-=-=-=-
		// resolve a resource from a given vault path
		error resolve_from_path( std::string, resource_ptr& );

        // =-=-=-=-=-=-=-
		// populate resource table from icat database
		error init_from_catalog( rsComm_t* );

        private:
        // =-=-=-=-=-=-=-
		// take results from genQuery, extract values and create resources
        error process_init_results( genQueryOut_t* );

        // =-=-=-=-=-=-=-
		// Attributes
		lookup_table< boost::shared_ptr< resource > > resources_;

	}; // class resource_manager

}; // namespace eirods


#endif // __EIRODS_RESOURCE_MANAGER_H__



