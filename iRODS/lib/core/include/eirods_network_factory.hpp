


#ifndef __EIRODS_NETWORK_FACTORY_H__
#define __EIRODS_NETWORK_FACTORY_H__

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_tcp_object.hpp"
#include "eirods_ssl_object.hpp"
#include "eirods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace eirods {
   /// =-=-=-=-=-=-=-
   /// @brief super basic free factory function to create either a tcp
   ///        object or an ssl object based on wether ssl has been enabled
   eirods::error network_factory( 
       rcComm_t*,                     // irods client comm ptr
       eirods::network_object_ptr& ); // network object
    
   /// =-=-=-=-=-=-=-
   /// @brief version for server connection as well
   eirods::error network_factory( 
       rsComm_t*,                    // irods client comm ptr
       eirods::network_object_ptr&); // network object

}; // namespace eirods


#endif // __EIRODS_NETWORK_FACTORY_H__



