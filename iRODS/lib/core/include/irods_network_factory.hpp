#ifndef __IRODS_NETWORK_FACTORY_HPP__
#define __IRODS_NETWORK_FACTORY_HPP__

// =-=-=-=-=-=-=-
#include "irods_tcp_object.hpp"
#include "irods_ssl_object.hpp"
#include "irods_stacktrace.hpp"

// =-=-=-=-=-=-=-
// boost includes
#include <boost/shared_ptr.hpp>

namespace irods {
/// =-=-=-=-=-=-=-
/// @brief super basic free factory function to create either a tcp
///        object or an ssl object based on wether ssl has been enabled
    irods::error network_factory(
        rcComm_t*,                     // irods client comm ptr
        irods::network_object_ptr& ); // network object

/// =-=-=-=-=-=-=-
/// @brief version for server connection as well
    irods::error network_factory(
        rsComm_t*,                    // irods client comm ptr
        irods::network_object_ptr& ); // network object

}; // namespace irods


#endif // __IRODS_NETWORK_FACTORY_HPP__



