#include "irods/irods_resource_plugin.hpp"

#include <fmt/format.h>

namespace irods
{
    // clang-format off
    const char* const RESC_CHILD_MAP_PROP = "resource_child_map_property";
    const char* const RESC_PARENT_PROP    = "resource_parent_property";
    // clang-format on

    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    error resource::add_child(const std::string& _name, const std::string& _data, resource_ptr _resc)
    {
        if (_name.empty()) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "empty resource name");
        }

        if (!_resc) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "null resource pointer");
        }

        children_[_name] = std::make_pair(_data, _resc);

        return SUCCESS();
    } // add_child

    error resource::remove_child(const std::string& _name)
    {
        // if an entry exists, erase it otherwise issue a warning.
        if (children_.has_entry(_name)) {
            children_.erase(_name);
            return SUCCESS();
        }

        return ERROR(SYS_INVALID_INPUT_PARAM, fmt::format("resource has no child named [{}]", _name));
    } // remove_child

    error resource::set_parent(const resource_ptr& _resc)
    {
        parent_ = _resc;
        return SUCCESS();
    } // set_parent

    error resource::get_parent(resource_ptr& _resc)
    {
        _resc = parent_;

        if (!_resc) {
            return ERROR(SYS_INVALID_INPUT_PARAM, "null resource parent pointer");
        }

        return SUCCESS();
    } // get_parent

    void resource::children(std::vector<std::string>& _out)
    {
        for (auto&& c : children_) {
            _out.push_back(c.first);
        }
    } // children

    auto get_resource_name(plugin_context& ctx) -> std::string
    {
        std::string resc_name;

        if (error err = ctx.prop_map().get<std::string>(RESOURCE_NAME, resc_name); !err.ok()) {
            THROW(err.code(), err.result()); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }

        return resc_name;
    } // get_resource_name

    auto get_resource_status(plugin_context& ctx) -> int
    {
        int resc_status; // NOLINT(cppcoreguidelines-init-variables)

        if (error err = ctx.prop_map().get<int>(RESOURCE_STATUS, resc_status); !err.ok()) {
            const irods::error ret = PASSMSG("Failed to get resource status property.", err);
            THROW(ret.code(), ret.result()); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }

        return resc_status;
    } // get_resource_status

    auto get_resource_location(plugin_context& ctx) -> std::string
    {
        std::string host_name;

        if (error err = ctx.prop_map().get<std::string>(RESOURCE_LOCATION, host_name); !err.ok()) {
            const irods::error ret = PASSMSG("Failed to get resource location property.", err);
            THROW(ret.code(), ret.result()); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }

        return host_name;
    } // get_resource_location
} // namespace irods

