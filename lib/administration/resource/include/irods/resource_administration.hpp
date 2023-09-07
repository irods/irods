#ifndef IRODS_RESOURCE_ADMINISTRATION_HPP
#define IRODS_RESOURCE_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxGeneralAdmin

#ifdef IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#  define NAMESPACE_IMPL server
#  define RxComm         RsComm
#  define rxGeneralAdmin rsGeneralAdmin

#  include "irods/rsGeneralAdmin.hpp"

struct RsComm;
#else
#  define NAMESPACE_IMPL client
#  define RxComm         RcComm
#  define rxGeneralAdmin rcGeneralAdmin

#  include "irods/generalAdmin.h"

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

/// A namespace that encapsulates administrative types and operations.
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
        auto resource_info(RxComm&, const resource_name_type) -> std::optional<class resource_info>;
    } // namespace NAMESPACE_IMPL

    /// A namespace defining the default set of resource types.
    namespace resource_type
    {
        // clang-format off
        inline constexpr const char* compound       = "compound";
        inline constexpr const char* deferred       = "deferred";
        inline constexpr const char* load_balanced  = "load_balanced";
        inline constexpr const char* mockarchive    = "mockarchive";
        inline constexpr const char* nonblocking    = "nonblocking";
        inline constexpr const char* passthrough    = "passthru";
        inline constexpr const char* random         = "random";
        inline constexpr const char* replication    = "replication";
        inline constexpr const char* struct_file    = "structfile";
        inline constexpr const char* universal_mss  = "univmss";
        inline constexpr const char* unixfilesystem = "unixfilesystem";
        // clang-format on
    } // namespace resource_type

    /// An enumeration defining all valid resource status values.
    enum class resource_status
    {
        up,     ///< The resource is available.
        down,   ///< The resource is not available.
        unknown ///< The library does not understand the value.
    }; // enum class resource_status

    /// A type that holds information about an existing resource.
    class resource_info
    {
    public:
        // clang-format off
        [[nodiscard]] auto id() const noexcept -> const std::string&                          { return id_; }
        [[nodiscard]] auto name() const noexcept -> const std::string&                        { return name_; }
        [[nodiscard]] auto type() const noexcept -> const std::string&                        { return type_; }
        [[nodiscard]] auto zone_name() const noexcept -> const std::string&                   { return zone_name_; }
        [[nodiscard]] auto host_name() const noexcept -> const std::string&                   { return host_name_; }
        [[nodiscard]] auto vault_path() const noexcept -> const std::string&                  { return vault_path_; }
        [[nodiscard]] auto status() const noexcept -> resource_status                         { return status_; }
        [[nodiscard]] auto context_string() const noexcept -> const std::string&              { return context_string_; }
        [[nodiscard]] auto comments() const noexcept -> const std::string&                    { return comments_; }
        [[nodiscard]] auto information() const noexcept -> const std::string&                 { return info_; }
        [[nodiscard]] auto free_space() const noexcept -> const std::string&                  { return free_space_; }
        [[nodiscard]] auto free_space_last_modified() const noexcept -> resource_time_type    { return free_space_time_; }
        [[nodiscard]] auto parent_id() const noexcept -> const std::string&                   { return parent_id_; }
        [[nodiscard]] auto created() const noexcept -> resource_time_type                     { return ctime_; }
        [[nodiscard]] auto last_modified() const noexcept -> resource_time_type               { return mtime_; }
        [[nodiscard]] auto last_modified_millis() const noexcept -> std::chrono::milliseconds { return mtime_millis_; }

        friend auto NAMESPACE_IMPL::resource_info(RxComm&, const resource_name_type)
            -> std::optional<class resource_info>;
        // clang-format on

    private:
        resource_info() = default;

        std::string id_;
        std::string name_;
        std::string type_;
        std::string zone_name_;
        std::string host_name_;
        std::string vault_path_;
        resource_status status_ = resource_status::unknown;
        std::string context_string_;
        std::string comments_;
        std::string info_;
        std::string free_space_;
        resource_time_type free_space_time_;
        std::string parent_id_;
        resource_time_type ctime_;
        resource_time_type mtime_;
        std::chrono::milliseconds mtime_millis_;
    }; // class resource_info

    /// A type that holds the necessary information needed to add a new resource to the system.
    struct resource_registration_info
    {
        std::string resource_name;
        std::string resource_type;
        std::string host_name;
        std::string vault_path;
        std::string context_string;
    }; // class resource_registration_info

    /// An input type used to update the type property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct resource_type_property
    {
        std::string value;
    }; // struct resource_type_property

    /// An input type used to update the host name property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct host_name_property
    {
        std::string value;
    }; // struct host_name_property

    /// An input type used to update the vault path property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct vault_path_property
    {
        std::string value;
    }; // struct vault_path_property

    /// An input type used to update the status property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct resource_status_property
    {
        resource_status value;
    }; // struct resource_status_property

    /// An input type used to update the comments property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct resource_comments_property
    {
        std::string value;
    }; // struct resource_comments_property

    /// An input type used to update the info property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct resource_info_property
    {
        std::string value;
    }; // struct resource_info_property

    /// An input type used to update the free space property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct free_space_property
    {
        std::string value;
    }; // struct free_space_property

    /// An input type used to update the context string property of a resource.
    ///
    /// Primarily used to modify a resource.
    ///
    /// \since 4.3.1
    struct context_string_property
    {
        std::string value;
    }; // struct context_string_property

    // clang-format off
    /// The \p ResourceProperty concept is satisfied if and only if \p T matches one of the following types:
    /// - \p resource_type_property
    /// - \p host_name_property
    /// - \p vault_path_property
    /// - \p resource_status_property
    /// - \p resource_comments_property
    /// - \p resource_info_property
    /// - \p free_space_property
    /// - \p context_string_property
    ///
    /// \since 4.3.1
    template <typename T>
    concept ResourceProperty = std::same_as<T, resource_type_property> ||
        std::same_as<T, resource_type_property> ||
        std::same_as<T, host_name_property> ||
        std::same_as<T, vault_path_property> ||
        std::same_as<T, resource_status_property> ||
        std::same_as<T, resource_comments_property> ||
        std::same_as<T, resource_info_property> ||
        std::same_as<T, free_space_property> ||
        std::same_as<T, context_string_property>;
    // clang-format on

    /// Converts a resource property type to its string representation.
    ///
    /// \tparam Property The type of the resource property.
    ///
    /// \since 4.3.1
    template <ResourceProperty Property>
    constexpr auto to_string() -> const char*
    {
        if constexpr (std::is_same_v<Property, resource_type_property>) {
            return "resource type";
        }
        else if constexpr (std::is_same_v<Property, host_name_property>) {
            return "resource host name";
        }
        else if constexpr (std::is_same_v<Property, vault_path_property>) {
            return "resource vault path";
        }
        else if constexpr (std::is_same_v<Property, resource_status_property>) {
            return "resource status";
        }
        else if constexpr (std::is_same_v<Property, resource_comments_property>) {
            return "resource comments";
        }
        else if constexpr (std::is_same_v<Property, resource_info_property>) {
            return "resource information";
        }
        else if constexpr (std::is_same_v<Property, free_space_property>) {
            return "resource free space";
        }
        else if constexpr (std::is_same_v<Property, context_string_property>) {
            return "resource context";
        }
    } // to_string

    /// Converts the resource status string to its corresponding enumeration if a mapping exists.
    ///
    /// \param[in] _status The string representation of a resource enumeration.
    ///
    /// \return A resource_status enumeration.
    ///
    /// \since 4.3.1
    constexpr auto to_resource_status(const std::string_view _status) -> resource_status
    {
        // clang-format off
        if (_status == "up")   { return resource_status::up; }
        if (_status == "down") { return resource_status::down; }
        // clang-format on

        return resource_status::unknown;
    } // to_resource_status

    /// A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    ///
    ///     - IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    namespace NAMESPACE_IMPL
    {
        /// Adds a new resource to the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _info The resource information.
        ///
        /// \throws irods::exception If an error occurs.
        auto add_resource(RxComm& _comm, const resource_registration_info& _info) -> void;

        /// Removes a resource from the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \throws irods::exception If an error occurs.
        auto remove_resource(RxComm& _comm, const resource_name_type _name) -> void;

        /// Makes a resource a child of another resource.
        ///
        /// \param[in] _comm           The communication object.
        /// \param[in] _parent         The name of the parent resource.
        /// \param[in] _child          The name of the child resource.
        /// \param[in] _context_string Information used by the parent resource to manage the
        ///                            child resource.
        ///
        /// \throws irods::exception If an error occurs.
        auto add_child_resource(
            RxComm& _comm,
            const resource_name_type _parent,
            const resource_name_type _child,
            const std::string_view _context_string = "") -> void;

        /// Removes a child resource from a parent resource.
        ///
        /// \param[in] _comm   The communication object.
        /// \param[in] _parent The name of the parent resource.
        /// \param[in] _child  The name of the child resource.
        ///
        /// \throws irods::exception If an error occurs.
        auto remove_child_resource(RxComm& _comm, const resource_name_type _parent, const resource_name_type _child)
            -> void;

        /// Checks if a resource exists.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \returns A tuple containing error information and the existence results.
        auto resource_exists(RxComm& _comm, const resource_name_type _name) -> bool;

        /// Retrieves information about a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \returns A tuple containing error information and the resource information.
        auto resource_info(RxComm& _comm, const resource_name_type _name) -> std::optional<class resource_info>;

        /// Modifies a property of a resource.
        ///
        /// \tparam Property \parblock The type of the property to modify.
        ///
        /// Property must satisfy the ResourceProperty concept. Failing to do so will result in a compile-time error.
        /// \endparblock
        ///
        /// \param[in] _comm     The communication object.
        /// \param[in] _name     The name of the resource.
        /// \param[in] _property An object containing the required information for applying the change.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.3.1
        template <ResourceProperty Property>
        auto modify_resource(RxComm& _comm, const resource_name_type _name, const Property& _property) -> void
        {
            generalAdminInp_t input{};
            input.arg0 = "modify";
            input.arg1 = "resource";
            input.arg2 = _name.data();

            if constexpr (std::is_same_v<Property, resource_type_property>) {
                input.arg3 = "type";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, host_name_property>) {
                input.arg3 = "host";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, vault_path_property>) {
                input.arg3 = "path";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, resource_status_property>) {
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
            else if constexpr (std::is_same_v<Property, resource_comments_property>) {
                input.arg3 = "comment";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, resource_info_property>) {
                input.arg3 = "info";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, free_space_property>) {
                input.arg3 = "free_space";
                input.arg4 = _property.value.data();
            }
            else if constexpr (std::is_same_v<Property, context_string_property>) {
                input.arg3 = "context";
                input.arg4 = _property.value.data();
            }

            if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                constexpr char* msg = "Could not change {} from [{}] to [{}] for resource [{}]";
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(ec, fmt::format(msg, to_string<Property>(), input.arg3, input.arg4, _name));
            }
        } // modify_resource

        /// Starts a rebalance on a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \returns A std::error_code.
        auto rebalance_resource(RxComm& _comm, const resource_name_type _name) -> void;

        /// Retrieves the name of a resource given a resource ID.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The ID of the resource.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \returns A tuple containing error information and the resource name.
        auto resource_name(RxComm& _comm, const resource_id_type _id) -> std::optional<std::string>;
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_RESOURCE_ADMINISTRATION_HPP
