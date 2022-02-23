#ifndef PLUGIN_LIFETIME_MANAGER
#define PLUGIN_LIFETIME_MANAGER

#include "irods/experimental_plugin_framework.hpp"

#include <map>
#include <string>

namespace irods::experimental::api {

    class plugin_lifetime_manager
    {
    public:
        using map_type = std::map<std::string, std::unique_ptr<irods::experimental::api::base>>;
        static auto instance() -> plugin_lifetime_manager&;
        static void destroy();
        auto plugins() -> map_type&;

    private:
        map_type p;

    }; // plugin_lifetime_manager

} // namespace irods::experimental::api

#endif // PLUGIN_LIFETIME_MANAGER
