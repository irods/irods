
#ifndef __IRODS_CLIENT_API_TABLE_HPP__
#define __IRODS_CLIENT_API_TABLE_HPP__

#include "apiHandler.hpp"

namespace irods {

#ifndef RODS_SERVER
    api_entry_table& get_client_api_table();
#endif

}; // namespace irods

#endif // __IRODS_CLIENT_API_TABLE_HPP__


