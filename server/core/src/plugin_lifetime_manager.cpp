
#include "irods/plugin_lifetime_manager.hpp"

namespace irods::experimental::api {

    auto plugin_lifetime_manager::instance() -> plugin_lifetime_manager&
    {
        static plugin_lifetime_manager i;
        return i;
    } // instance

    void plugin_lifetime_manager::destroy()
    {
        instance().plugins().clear();
    }

    auto plugin_lifetime_manager::plugins() -> map_type&
    {
        return p;
    }

} // namespace irods::experimental::api
