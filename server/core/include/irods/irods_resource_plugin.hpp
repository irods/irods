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

    /// Retrieves the id of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return rodsLong_t
    ///
    /// \since 4.3.1
    auto resource_id(const resource_ptr _resc_ptr) -> rodsLong_t;

    /// Retrieves the name of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_name(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the host information of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return RodsHostAddress*
    ///
    /// \since 4.3.1
    auto resource_host(const resource_ptr _resc_ptr) -> RodsHostAddress*;

    /// Retrieves the quota value of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return long
    ///
    /// \since 4.3.1
    auto resource_quota(const resource_ptr _resc_ptr) -> long;

    /// Retrieves the status of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return int
    ///
    /// \since 4.3.1
    auto resource_status(const resource_ptr _resc_ptr) -> int;

    /// Retrieves the zone name of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_zone(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the location of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_location(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the type of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_type(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the classification of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_class(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the vault path of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_vault_path(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the general information of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_info(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the comments associated with the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_comments(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the create time of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_create_time(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the modify time of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_modify_time(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the children of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_children(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the context of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_context(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the parent id of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_parent_id(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the parent conext of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_parent_context(const resource_ptr _resc_ptr) -> std::string;

    /// Retrieves the free space of the resource.
    ///
    /// \param[in] _resc_ptr The handle to the resource plugin object.
    ///
    /// \throw irods::exception If an error occurs.
    ///
    /// \return std::string
    ///
    /// \since 4.3.1
    auto resource_free_space(const resource_ptr _resc_ptr) -> std::string;
} // namespace irods

#endif // IRODS_RESOURCE_PLUGIN_HPP

