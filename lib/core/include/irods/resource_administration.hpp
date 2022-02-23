#ifndef IRODS_RESOURCE_ADMINISTRATION_HPP
#define IRODS_RESOURCE_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef RxComm

#ifdef IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    #define NAMESPACE_IMPL      server
    #define RxComm              RsComm

    struct RsComm;
#else
    #define NAMESPACE_IMPL      client
    #define RxComm              RcComm

    struct RcComm;
#endif // IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <optional>
#include <chrono>

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
        auto resource_info(RxComm&, const resource_name_type)
            -> std::tuple<std::error_code, std::optional<class resource_info>>;
    } // namespace NAMESPACE_IMPL

    /// \brief A namespace defining the default set of resource types.
    namespace resource_type
    {
        // clang-format off
        const std::string_view compound       = "compound";
        const std::string_view deferred       = "deferred";
        const std::string_view load_balanced  = "load_balanced";
        const std::string_view mockarchive    = "mockarchive";
        const std::string_view nonblocking    = "nonblocking";
        const std::string_view passthrough    = "passthru";
        const std::string_view random         = "random";
        const std::string_view replication    = "replication";
        const std::string_view struct_file    = "structfile";
        const std::string_view universal_mss  = "univmss";
        const std::string_view unixfilesystem = "unixfilesystem";
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
        auto id() const noexcept -> const std::string&                       { return id_; }
        auto name() const noexcept -> const std::string&                     { return name_; }
        auto type() const noexcept -> const std::string&                     { return type_; }
        auto zone_name() const noexcept -> const std::string&                { return zone_name_; }
        auto host_name() const noexcept -> const std::string&                { return host_name_; }
        auto vault_path() const noexcept -> const std::string&               { return vault_path_; }
        auto status() const noexcept -> resource_status                      { return status_; }
        auto context_string() const noexcept -> const std::string&           { return context_string_; }
        auto comments() const noexcept -> const std::string&                 { return comments_; }
        auto information() const noexcept -> const std::string&              { return info_; }
        auto free_space() const noexcept -> const std::string&               { return free_space_; }
        auto free_space_last_modified() const noexcept -> resource_time_type { return free_space_time_; }
        auto parent_id() const noexcept -> const std::string&                { return parent_id_; }
        auto created() const noexcept -> resource_time_type                  { return ctime_; }
        auto last_modified() const noexcept -> resource_time_type            { return mtime_; }
        // clang-format on

        friend auto NAMESPACE_IMPL::resource_info(RxComm&, const resource_name_type)
            -> std::tuple<std::error_code, std::optional<class resource_info>>;

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

    /// \brief A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    /// 
    ///     - IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    namespace NAMESPACE_IMPL
    {
        // clang-format off

        /// \brief Adds a new resource to the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _info The resource information.
        ///
        /// \returns A std::error_code.
        auto add_resource(RxComm& _comm, const resource_registration_info& _info) -> std::error_code;

        /// \brief Removes a resource from the system.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A std::error_code.
        auto remove_resource(RxComm& _comm, const resource_name_type _name) -> std::error_code;

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
                                const std::string_view _context_string = "") -> std::error_code;

        /// \brief Removes a child resource from a parent resource.
        ///
        /// \param[in] _comm   The communication object.
        /// \param[in] _parent The name of the parent resource.
        /// \param[in] _child  The name of the child resource.
        ///
        /// \returns A std::error_code.
        auto remove_child_resource(RxComm& _comm,
                                   const resource_name_type _parent,
                                   const resource_name_type _child) -> std::error_code;

        /// \brief Checks if a resource exists.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A tuple containing error information and the existence results.
        auto resource_exists(RxComm& _comm, const resource_name_type _name)
            -> std::tuple<std::error_code, bool>;

        /// \brief Retrieves information about a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A tuple containing error information and the resource information.
        auto resource_info(RxComm& _comm, const resource_name_type _name)
            -> std::tuple<std::error_code, std::optional<class resource_info>>;

        /// \brief Sets the type of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new resource type.
        ///
        /// \returns A std::error_code.
        auto set_resource_type(RxComm& _comm,
                               const resource_name_type _name,
                               const std::string_view _new_value) -> std::error_code;

        /// \brief Sets the host name of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new host name.
        ///
        /// \returns A std::error_code.
        auto set_resource_host_name(RxComm& _comm,
                                    const resource_name_type _name,
                                    const std::string_view _new_value) -> std::error_code;

        /// \brief Sets the vault path of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new vault path.
        ///
        /// \returns A std::error_code.
        auto set_resource_vault_path(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> std::error_code;

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
                                 resource_status _new_value) -> std::error_code;

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
                                   const std::string_view _new_value) -> std::error_code;

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
                                      const std::string_view _new_value) -> std::error_code;

        /// \brief Sets the free space on a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new free space value.
        ///
        /// \returns A std::error_code.
        auto set_resource_free_space(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> std::error_code;

        /// \brief Sets the context string of a resource.
        ///
        /// \param[in] _comm      The communication object.
        /// \param[in] _name      The name of the resource.
        /// \param[in] _new_value The new context string.
        ///
        /// \returns A std::error_code.
        auto set_resource_context_string(RxComm& _comm,
                                         const resource_name_type _name,
                                         const std::string_view _new_value) -> std::error_code;

        /// \brief Starts a rebalance on a resource.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The name of the resource.
        ///
        /// \returns A std::error_code.
        auto rebalance_resource(RxComm& _comm, const resource_name_type _name) -> std::error_code;

        /// \brief Retrieves the name of a resource given a resource ID.
        ///
        /// \param[in] _comm The communication object.
        /// \param[in] _name The ID of the resource.
        ///
        /// \returns A tuple containing error information and the resource name.
        auto resource_name(RxComm& _comm, const resource_id_type _id)
            -> std::tuple<std::error_code, std::optional<std::string>>;

        // clang-format on
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_RESOURCE_ADMINISTRATION_HPP

