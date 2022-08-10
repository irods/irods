#include "irods/private/irods_is_in_host_list.hpp"

#include "irods/irods_error.hpp"
#include "irods/irods_string_tokenize.hpp"
#include "irods/filesystem/path.hpp"

// boost includes
#include <boost/algorithm/string/predicate.hpp>

// stl includes
#include <vector>
#include <string_view>

namespace irods
{
    const std::string HOST_LIST("host_list");

    // returns true if there is no host_list or if the current host is in
    // the list
    auto is_host_in_host_list(irods::plugin_property_map& prop_map, const std::string_view& resource_hostname) -> bool
    {
        std::string host_list_str;

        if (!prop_map.get<std::string>(HOST_LIST, host_list_str).ok()) {
            // no HOST_LIST parameter, all hosts assumed to be able to handle request
            // and original vault path used for all hosts
            return true;
        }

        // There is a host_list=x,y,z in the resource's context string.
        // Check to see if the resource_hostname is in the comma delimited list.

        // split parameter by delimiter (,)
        const std::string delimiter = ",";
        std::vector<std::string> tokens;
        irods::string_tokenize(host_list_str, delimiter, tokens);

        for (const std::string& token : tokens) {
            if (boost::iequals(token, resource_hostname)) {
                return true;
            }
        }
        return false;
    } // is_host_in_host_list
} // namespace irods
