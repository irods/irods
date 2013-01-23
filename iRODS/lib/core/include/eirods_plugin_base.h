


#ifndef __EIRODS_PLUGIN_BASE_H__
#define __EIRODS_PLUGIN_BASE_H__

// =-=-=-=-=-=-=-
// stl includes
#include <string>

// =-=-=-=-=-=-=-
// boost includes
#include <boost/function.hpp>

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_error.h"

namespace eirods {
    // =-=-=-=-=-=-=-
    /// @brief abstraction for post disconnect functor
    typedef boost::function< void(void) > pdmo_type;
    
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
		virtual error post_disconnect_maintenance_operation( pdmo_type& );

        protected:
        std::string context_;

	}; // class plugin_base

}; // namespace eirods

#endif // __EIRODS_PLUGIN_BASE_H__



