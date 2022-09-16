#ifndef IRODS_RESOURCE_ADMINISTRATION_HPP
#define IRODS_RESOURCE_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxGeneralAdmin

#ifdef IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    #define NAMESPACE_IMPL server
    #define RxComm         RsComm
    #define rxGeneralAdmin rsGeneralAdmin

    #include "irods/rsGeneralAdmin.hpp"

    struct RsComm;
#else
    #define NAMESPACE_IMPL client
    #define RxComm         RcComm
    #define rxGeneralAdmin rcGeneralAdmin

    #include "irods/generalAdmin.h"

    struct RcComm;
#endif // IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/rodsErrorTable.h"
#include "irods/irods_exception.hpp"

#include <fmt/format.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <chrono>
#include <type_traits>
#include <concepts>

/// \brief A namespace that encapsulates administrative types and operations.
namespace irods::experimental::administration
{
    // clang-format off
    using resource_id_type   = std::string_view;
    using resource_name_type = std::string_view;
    using resource_time_type = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
    // clang-format on

    // Forward declaration for "resource_info" API function.
    class resource_info;

    // Forward declaration for "resource_info" class.
    namespace NAMESPACE_IMPL
    {
        auto resource_info(RxComm&, const resource_name_type) -> class resource_info;
    } // namespace NAMESPACE_IMPL

    /// \brief A namespace defining the default set of resource types.
    namespace resource_type
    {
        // clang-format off
        inline constexpr std::string_view compound       = "compound";
        inline constexpr std::string_view deferred       = "deferred";
        inline constexpr std::string_view load_balanced  = "load_balanced";
        inline constexpr std::string_view mockarchive    = "mockarchive";
        inline constexpr std::string_view nonblocking    = "nonblocking";
        inline constexpr std::string_view passthrough    = "passthru";
        inline constexpr std::string_view random         = "random";
        inline constexpr std::string_view replication    = "replication";
        inline constexpr std::string_view struct_file    = "structfile";
        inline constexpr std::string_view universal_mss  = "univmss";
        inline constexpr std::string_view unixfilesystem = "unixfilesystem";
        // clang-format on
    } // namespace resource_type

    /// \brief An enumeration defining all valid resource status values.
    enum class resource_status
    {
        up,     ///< The resource is available.
        down,   ///< The resource is not available.
        unknown ///< The library does not understand the value.
    }; // enum class resource_status

    /// \brief A type that holds information about an existing resource.
    class resource_info
    {
    public:
        // clang-format off
        [[nodiscard]] auto id() const noexcept -> const std::string&                       { return id_; }
        [[nodiscard]] auto name() const noexcept -> const std::string&                     { return name_; }
        [[nodiscard]] auto type() const noexcept -> const std::string&                     { return type_; }
        [[nodiscard]] auto zone_name() const noexcept -> const std::string&                { return zone_name_; }
        [[nodiscard]] auto host_name() const noexcept -> const std::string&                { return host_name_; }
        [[nodiscard]] auto vault_path() const noexcept -> const std::string&               { return vault_path_; }
        [[nodiscard]] auto status() const noexcept -> resource_status                      { return status_; }
        [[nodiscard]] auto context_string() const noexcept -> const std::string&           { return context_string_; }
        [[nodiscard]] auto comments() const noexcept -> const std::string&                 { return comments_; }
        [[nodiscard]] auto information() const noexcept -> const std::string&              { return info_; }
        [[nodiscard]] auto free_space() const noexcept -> const std::string&               { return free_space_; }
        [[nodiscard]] auto free_space_last_modified() const noexcept -> resource_time_type { return free_space_time_; }
        [[nodiscard]] auto parent_id() const noexcept -> const std::string&                { return parent_id_; }
        [[nodiscard]] auto created() const noexcept -> resource_time_type                  { return ctime_; }
        [[nodiscard]] auto last_modified() const noexcept -> resource_time_type            { return mtime_; }
        // clang-format on

        friend auto NAMESPACE_IMPL::resource_info(RxComm&, const resource_name_type) -> class resource_info;

    private:
        resource_info() = default;

        std::string id_;
        std::string name_;
        std::string type_;
        std::string zone_name_;
        std::string host_name_;
        std::string vault_path_;
        resource_status status_;
        std::string context_string_;
        std::string comments_;
        std::string info_;
        std::string free_space_;
        resource_time_type free_space_time_;
        std::string parent_id_;
        resource_time_type ctime_;
        resource_time_type mtime_;
    }; // class resource_info

    /// \brief A type that holds the necessary information needed to add a new resource to the system.
    struct resource_registration_info
    {
        std::string resource_name;
        std::string resource_type;
        std::string host_name;
        std::string vault_path;
        std::string context_string;
    }; // class resource_registration_info

    struct resource_type_property
    {
        std::string value;
    }; // struct resource_type_property

    struct host_name_property
    {
        std::string value;
    }; // struct host_name_property

    struct vault_path_property
    {
        std::string value;
    }; // struct vault_path_property

    struct resource_status_property
    {
        resource_status value;
    }; // struct resource_status_property

    struct resource_comments_property
    {
        std::string value;
    }; // struct resource_comments_property

    struct resource_info_property
    {
        std::string value;
    }; // struct resource_info_property

    struct free_space_property
    {
        std::string value;
    }; // struct free_space_property

    struct context_string_property
    {
        std::string value;
    }; // struct context_string_property

    /// \brief A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    /// 
    ///     - IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    namespace NAMESPACE_IMPL
    {
        /// \brief Adds a new resource to the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _info The resource information.
        ///
        /// \returns A std::error_code.
        auto add_resource(RxComm& _comm, const resource_registration_info& _info) -> void;

        /// \brief Removes a resource from the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A std::error_code.
        auto remove_resource(RxComm& _comm, const resource_name_type _name) -> void;

        /// \brief Makes a resource a child of another resource.
        ///
        /// \param[in] _comm            The communication object.
        /// \param[in] _parent          The name of the parent resource.
        /// \param[in] _child           The name of the child resource.
        /// \param[in] _context_string  Information used by the parent resource to manage the
        ///                             child resource.
        ///
        /// \returns A std::error_code.
        auto add_child_resource(RxComm& _comm,
                                const resource_name_type _parent,
                                const resource_name_type _child,
                                const std::string_view _context_string = "") -> void;

        /// \brief Removes a child resource from a parent resource.
        ///
        /// \param[in] _comm   The communication object.
        /// \param[in] _parent The name of the parent resource.
        /// \param[in] _child  The name of the child resource.
        ///
        /// \returns A std::error_code.
        auto remove_child_resource(RxComm& _comm,
                                   const resource_name_type _parent,
                                   const resource_name_type _child) -> void;

        /// \brief Checks if a resource exists.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A tuple containing error information and the existence results.
        auto resource_exists(RxComm& _comm, const resource_name_type _name) -> bool;

        /// \brief Retrieves information about a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A tuple containing error information and the resource information.
        auto resource_info(RxComm& _comm, const resource_name_type _name) -> class resource_info;

        template <typename ResourceProperty>
            requires std::same_as<ResourceProperty, resource_type_property> ||
                std::same_as<ResourceProperty, resource_type_property> ||
                std::same_as<ResourceProperty, host_name_property> ||
                std::same_as<ResourceProperty, vault_path_property> ||
                std::same_as<ResourceProperty, resource_status_property> ||
                std::same_as<ResourceProperty, resource_comments_property> ||
                std::same_as<ResourceProperty, resource_info_property> ||
                std::same_as<ResourceProperty, free_space_property> ||
                std::same_as<ResourceProperty, context_string_property>
        auto modify_resource(RxComm& _comm, const resource_name_type _name, const ResourceProperty& _property) -> void
        {
            generalAdminInp_t input{};
            input.arg0 = "modify";
            input.arg1 = "resource";
            input.arg2 = _name.data();

            if constexpr (std::is_same_v<ResourceProperty, resource_type_property>) {
                input.arg3 = "type";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, host_name_property>) {
                input.arg3 = "host";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, vault_path_property>) {
                input.arg3 = "path";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, resource_status_property>) {
                input.arg3 = "status";

                if (_property.value == resource_status::up) {
                    input.arg4 = "up";
                }
                else if (_property.value == resource_status::down) {
                    input.arg4 = "down";
                }
                else {
                    THROW(SYS_INVALID_INPUT_PARAM, "Invalid value for resource status");
                }
            }
            else if constexpr (std::is_same_v<ResourceProperty, resource_comments_property>) {
                input.arg3 = "comment";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, resource_info_property>) {
                input.arg3 = "info";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, free_space_property>) {
                input.arg3 = "free_space";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<ResourceProperty, context_string_property>) {
                input.arg3 = "context";
                input.arg4 = _property.value.data();
            }

            if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                constexpr char* msg = "Could not change [{}] to [{}] for resource [{}]";
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(ec, fmt::format(msg, input.arg3, input.arg4, _name));
            }
        } // modify_resource

        /// \brief Sets the type of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new resource type.
        ///
        /// \returns A std::error_code.
        auto set_resource_type(RxComm& _comm,
                               const resource_name_type _name,
                               const std::string_view _new_value) -> void;

        /// \brief Sets the host name of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new host name.
        ///
        /// \returns A std::error_code.
        auto set_resource_host_name(RxComm& _comm,
                                    const resource_name_type _name,
                                    const std::string_view _new_value) -> void;

        /// \brief Sets the vault path of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new vault path.
        ///
        /// \returns A std::error_code.
        auto set_resource_vault_path(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> void;

        /// \brief Sets the status of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new status. Passing resource_status::unknown results
        ///                       in an error.
        ///
        /// \returns A std::error_code.
        auto set_resource_status(RxComm& _comm,
                                 const resource_name_type _name,
                                 resource_status _new_value) -> void;

        /// \brief Sets comments on a resource.
        ///
        /// This is a destructive update and will result in an overwrite of the existing
        /// comments value if not careful.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new comments.
        ///
        /// \returns A std::error_code.
        auto set_resource_comments(RxComm& _comm,
                                   const resource_name_type _name,
                                   const std::string_view _new_value) -> void;

        /// \brief Sets textual information on a resource.
        ///
        /// This is a destructive update and will result in an overwrite of the existing
        /// information value if not careful.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new information string.
        ///
        /// \returns A std::error_code.
        auto set_resource_info_string(RxComm& _comm,
                                      const resource_name_type _name,
                                      const std::string_view _new_value) -> void;

        /// \brief Sets the free space on a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new free space value.
        ///
        /// \returns A std::error_code.
        auto set_resource_free_space(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> void;

        /// \brief Sets the context string of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new context string.
        ///
        /// \returns A std::error_code.
        auto set_resource_context_string(RxComm& _comm,
                                         const resource_name_type _name,
                                         const std::string_view _new_value) -> void;

        /// \brief Starts a rebalance on a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A std::error_code.
        auto rebalance_resource(RxComm& _comm, const resource_name_type _name) -> void;

        /// \brief Retrieves the name of a resource given a resource ID.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The ID of the resource.
        ///
        /// \returns A tuple containing error information and the resource name.
        auto resource_name(RxComm& _comm, const resource_id_type _id) -> std::optional<std::string>;
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_RESOURCE_ADMINISTRATION_HPP

