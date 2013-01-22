


#ifndef __EIRODS_PLUGIN_BASE_H__
#define __EIRODS_PLUGIN_BASE_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

namespace eirods {
    /**
	  * \class pdmo_base - ABC for maintenance operations
	  * \author Jason M. Coposky 
	  * \date   January 2013
	  * \brief  This class provides the operator interface for defining maintenance operations which will be called after the agent disconnects from the client.  these will be call depth first based on the organization of the resource composition tree
	  **/
    class pdmo_base {
        public:
        virtual error operator()() = 0;

    }; // class pdmo_base

    /**
	  * \class plugin_base - ABC for E-iRODS Plugins
	  * \author Jason M. Coposky 
	  * \date   October 2011
	  * \brief  This class enforces the delay_load interface necessary for the load_plugin call to load any other non-member symbols from the shared object.  Reference server/core/include/eirods_load_plugin.h
	  **/
    class plugin_base {
        public:
		plugin_base( std::string ); // context
		virtual ~plugin_base();
		virtual error delay_load( void* ) = 0;
		virtual error post_disconnect_maintenance_operation( pdmo_base*& );

        protected:
        std::string context_;

	}; // class plugin_base

}; // namespace eirods

#endif // __EIRODS_PLUGIN_BASE_H__



