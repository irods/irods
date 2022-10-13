#ifndef IRODS_RESOURCE_PLUGIN_HPP
#define IRODS_RESOURCE_PLUGIN_HPP

#include "irods/irods_plugin_base.hpp"
#include "irods/irods_resource_constants.hpp"
#include "irods/irods_resource_types.hpp"

#include <utility>
#include <vector>

namespace irods
{
    extern const char* const RESC_CHILD_MAP_PROP;
    extern const char* const RESC_PARENT_PROP;

    using resource_child_map = lookup_table<std::pair<std::string, resource_ptr>>;

    class resource : public plugin_base // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        resource(const std::string& _inst, const std::string& _ctx)
            : plugin_base(_inst, _ctx)
        {
            properties_.set(RESC_CHILD_MAP_PROP, &children_);
            properties_.set(RESC_PARENT_PROP, parent_);
        }

        ~resource() = default; // NOLINT(modernize-use-override)

        resource(const resource& _rhs) = default;

        resource& operator=(const resource& _rhs)
        {
            if (&_rhs == this) {
                return *this;
            }

            plugin_base::operator=(_rhs);
            children_ = _rhs.children_;
            parent_ = _rhs.parent_;

            return *this;
        }

        virtual error add_child(const std::string& _name, const std::string& _data, resource_ptr _resc);

        virtual error remove_child(const std::string& _name);

        virtual std::size_t num_children()
        {
            return children_.size();
        }

        virtual bool has_child(const std::string& _name)
        {
            return children_.has_entry(_name);
        }

        virtual void children(std::vector<std::string>& _out);

        virtual error set_parent(const resource_ptr& _resc);

        virtual error get_parent(resource_ptr& _resc);

    protected:
        // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
        resource_child_map children_;
        resource_ptr parent_;
        // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
    }; // class resource

    /// \brief Convenience function for getting resource name from plugin context
    ///
    /// \param[in] ctx Plugin context from which resource name will be extracted
    ///
    /// \throws irods::exception thrown if the error object returned by get() is not ok()
    auto get_resource_name(plugin_context& ctx) -> std::string;

    /// \brief Convenience function for getting resource status from plugin context
    ///
    /// \param[in] ctx Plugin context from which resource status will be extracted
    ///
    /// \throws irods::exception thrown if the error object returned by get() is not ok()
    auto get_resource_status(plugin_context& ctx) -> int;

    /// \brief Convenience function for getting resource location from plugin context
    ///
    /// \param[in] ctx Plugin context from which resource location will be extracted
    ///
    /// \throws irods::exception thrown if the error object returned by get() is not ok()
    auto get_resource_location(plugin_context& ctx) -> std::string;
} // namespace irods

#endif // IRODS_RESOURCE_PLUGIN_HPP

