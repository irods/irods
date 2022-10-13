#include "irods/irods_resource_plugin.hpp"

#include <fmt/format.h>

namespace
{
    template <typename T>
    auto get_resource_property(const irods::resource_ptr _rptr, const std::string& _property_name) -> T
    {
        T value;

        if (const auto err = _rptr->get_property<T>(_property_name, value); !err.ok()) {
            THROW(err.code(), err.result()); // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        }

        return value;
    } // get_resource_property
} // anonymous namespace

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

    auto resource_id(const resource_ptr _resc_ptr) -> rodsLong_t
    {
        return get_resource_property<rodsLong_t>(_resc_ptr, RESOURCE_ID);
    } // resource_id

    auto resource_name(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_NAME);
    } // resource_name

    auto resource_host(const resource_ptr _resc_ptr) -> RodsHostAddress*
    {
        return get_resource_property<RodsHostAddress*>(_resc_ptr, RESOURCE_HOST);
    } // resource_host

    auto resource_quota(const resource_ptr _resc_ptr) -> long
    {
        return get_resource_property<long>(_resc_ptr, RESOURCE_QUOTA);
    } // resource_quota

    auto resource_status(const resource_ptr _resc_ptr) -> int
    {
        return get_resource_property<int>(_resc_ptr, RESOURCE_STATUS);
    } // resource_status

    auto resource_zone(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_ZONE);
    } // resource_zone

    auto resource_location(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_LOCATION);
    } // resource_location

    auto resource_type(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_TYPE);
    } // resource_type

    auto resource_class(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_CLASS);
    } // resource_class

    auto resource_vault_path(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_PATH);
    } // resource_vault_path

    auto resource_info(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_INFO);
    } // resource_info

    auto resource_comments(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_COMMENTS);
    } // resource_comments

    auto resource_create_time(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_CREATE_TS);
    } // resource_create_time

    auto resource_modify_time(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_MODIFY_TS);
    } // resource_modify_time

    auto resource_children(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_CHILDREN);
    } // resource_children

    auto resource_context(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_CONTEXT);
    } // resource_context

    auto resource_parent_id(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_PARENT);
    } // resource_parent_id

    auto resource_parent_context(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_PARENT_CONTEXT);
    } // resource_parent_context

    auto resource_free_space(const resource_ptr _resc_ptr) -> std::string
    {
        return get_resource_property<std::string>(_resc_ptr, RESOURCE_FREESPACE);
    } // resource_free_space
} // namespace irods

