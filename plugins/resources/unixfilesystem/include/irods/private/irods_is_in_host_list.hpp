#ifndef IRODS_IS_IN_HOST_LIST_HPP
#define IRODS_IS_IN_HOST_LIST_HPP

#include "irods/irods_lookup_table.hpp"

#include <string_view>

namespace irods
{
    auto is_host_in_host_list(irods::plugin_property_map& prop_map, std::string_view resource_hostname) -> bool;
} // namespace irods

#endif // IRODS_IS_IN_HOST_LIST_HPP
