#ifndef IRODS_ZONE_ADMINISTRATION_HPP
#define IRODS_ZONE_ADMINISTRATION_HPP

/// \file

#undef NAMESPACE_IMPL
#undef RxComm
#undef rxGeneralAdmin

#ifdef IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#  define NAMESPACE_IMPL server
#  define RxComm         RsComm
#  define rxGeneralAdmin rsGeneralAdmin
#  define IRODS_QUERY_ENABLE_SERVER_SIDE_API
// NOLINTEND(cppcoreguidelines-macro-usage)

#  include "irods/rodsConnect.h"
#  include "irods/rsGeneralAdmin.hpp"

struct RsComm;
#else
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#  define NAMESPACE_IMPL client
#  define RxComm         RcComm
#  define rxGeneralAdmin rcGeneralAdmin
// NOLINTEND(cppcoreguidelines-macro-usage)

#  include "irods/generalAdmin.h"

struct RcComm;
#endif // IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/irods_exception.hpp"
#include "irods/irods_query.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/zone_type.hpp"

#include <fmt/format.h>

#include <concepts>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace irods::experimental::administration
{
    /// Represents the set of valid ACL values for a zone collection.
    ///
    /// \since 4.3.1
    enum class zone_collection_acl
    {
        null, ///< Indicates the user or group's permission to read the zone collection should be revoked.
        read  ///< Indicates the user or group should be allowed to read the zone collection.
    }; // enum class zone_collection_acl

    /// Holds the name of a zone. Primarily used to modify a zone.
    ///
    /// \since 4.3.1
    struct zone_name_property
    {
        std::string value;
    }; // struct zone_name_property

    /// Holds the connection information of a zone. Primarily used to modify a zone.
    ///
    /// \since 4.3.1
    struct connection_info_property
    {
        std::string value;
    }; // struct connection_info_property

    /// Holds the comment attached to a zone. Primarily used to modify a zone.
    ///
    /// \since 4.3.1
    struct comment_property
    {
        std::string value;
    }; // struct comment_property

    /// Holds the ACL and entity to attach to a zone. Primarily used to modify a zone.
    ///
    /// \since 4.3.1
    struct zone_collection_acl_property
    {
        // clang-format off
        zone_collection_acl acl; ///< The permission for a zone collection.
        std::string name;        ///< The name of a user or group.
        // clang-format on
    }; // struct zone_collection_acl_property

    // clang-format off
    /// The \p ZoneProperty concept is satisfied if and only if \p T matches one of the following types:
    /// - \p zone_name_property
    /// - \p connection_info_property
    /// - \p comment_property
    /// - \p zone_collection_acl_property
    ///
    /// \since 4.3.1
    template <typename T>
    concept ZoneProperty = std::same_as<T, zone_name_property> ||
        std::same_as<T, connection_info_property> ||
        std::same_as<T, comment_property> ||
        std::same_as<T, zone_collection_acl_property>;
    // clang-format on

    /// Holds various pieces of information that uniquely identify a zone.
    ///
    /// \since 4.3.1
    struct zone_info
    {
        std::string name;
        std::string connection_info;
        std::string comment;
        int id;
        zone_type type;
    }; // struct zone_info

    /// Holds information that is used when adding remote zones to the system.
    ///
    /// \since 4.3.1
    struct zone_options
    {
        // clang-format off
        std::string connection_info; ///< Holds the connection information that will be associated with a zone.
        std::string comment;         ///< Holds the comment that will be associated with a zone.
        // clang-format on
    }; // struct zone_options

    /// \brief A namespace defining the set of client-side or server-side API functions.
    ///
    /// This namespace's name changes based on the presence of a special macro. If the following macro
    /// is defined, then the NAMESPACE_IMPL will be \p server, else it will be \p client.
    ///
    ///     - IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    ///
    /// \since 4.3.1
    namespace NAMESPACE_IMPL
    {
        /// Adds a remote zone to the system.
        ///
        /// \param[in] _comm      The connection object.
        /// \param[in] _zone_name The name of the zone.
        /// \param[in] _opts      Optional options used when creating the zone.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.3.1
        auto add_zone(RxComm& _comm, const std::string_view _zone_name, const zone_options& _opts = {}) -> void;

        /// Removes a zone from the system.
        ///
        /// \param[in] _comm      The connection object.
        /// \param[in] _zone_name The name of the zone.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.3.1
        auto remove_zone(RxComm& _comm, const std::string_view _zone_name) -> void;

        /// Modifies a property of a zone.
        ///
        /// \tparam Property \parblock The type of the property to modify.
        ///
        /// Property must satisfy the ZoneProperty concept. Failing to do so will result in a compile-time error.
        /// \endparblock
        ///
        /// \param[in] _comm      The connection object.
        /// \param[in] _zone_name The name of the zone.
        /// \param[in] _property  An object containing the required information for applying the change.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \since 4.3.1
        template <ZoneProperty Property>
        auto modify_zone(RxComm& _comm, const std::string_view _zone_name, const Property& _property) -> void
        {
            if (_zone_name.empty()) {
                THROW(SYS_INVALID_INPUT_PARAM, "Invalid zone name: cannot be empty");
            }

            if constexpr (std::is_same_v<Property, zone_name_property>) {
#ifdef IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
                const std::string_view local_zone_name = getLocalZoneName();
#else
                std::string local_zone_name;

                for (auto&& row : irods::query{&_comm, "select ZONE_NAME where ZONE_TYPE = 'local'"}) {
                    local_zone_name = std::move(row[0]);
                }
#endif // IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

                generalAdminInp_t input{};
                input.arg0 = "modify";

                if (_zone_name == local_zone_name) {
                    input.arg1 = "localzonename";
                    input.arg2 = _zone_name.data();
                    input.arg3 = _property.value.data();
                }
                else {
                    input.arg1 = "zone";
                    input.arg2 = _zone_name.data();
                    input.arg3 = "name";
                    input.arg4 = _property.value.data();
                }

                if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                    THROW(ec, fmt::format("Failed to rename zone from [{}] to [{}].", _zone_name, _property.value));
                }
            }
            else if constexpr (std::is_same_v<Property, connection_info_property>) {
                generalAdminInp_t input{};
                input.arg0 = "modify";
                input.arg1 = "zone";
                input.arg2 = _zone_name.data();
                input.arg3 = "conn";
                input.arg4 = _property.value.data();

                if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                    THROW(ec, fmt::format("Failed to update connection information for zone [{}].", _zone_name));
                }
            }
            else if constexpr (std::is_same_v<Property, comment_property>) {
                generalAdminInp_t input{};
                input.arg0 = "modify";
                input.arg1 = "zone";
                input.arg2 = _zone_name.data();
                input.arg3 = "comment";
                input.arg4 = _property.value.data();

                if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                    THROW(ec, fmt::format("Failed to update comment for zone [{}].", _zone_name));
                }
            }
            else if constexpr (std::is_same_v<Property, zone_collection_acl_property>) {
                const auto* const perm = (_property.acl == zone_collection_acl::null) ? "null" : "read";
                const auto zone_collection = fmt::format("/{}", _zone_name);

                generalAdminInp_t input{};
                input.arg0 = "modify";
                input.arg1 = "zonecollacl";
                input.arg2 = perm;
                input.arg3 = _property.name.data();
                input.arg4 = zone_collection.data();

                if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
                    constexpr auto* msg = "Failed to set zone collection permission to [{}] for zone [{}].";
                    THROW(ec, fmt::format(msg, perm, _zone_name));
                }
            }
        } // modify_zone

        /// Checks if a zone exists.
        ///
        /// \param[in] _comm      The connection object.
        /// \param[in] _zone_name The name of the zone.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \return A boolean.
        /// \retval true  If the zone exists.
        /// \retval false Otherwise.
        ///
        /// \since 4.3.1
        [[nodiscard]] auto zone_exists(RxComm& _comm, const std::string_view _zone_name) -> bool;

        /// Returns information about a zone.
        ///
        /// \param[in] _comm      The connection object.
        /// \param[in] _zone_name The name of the zone.
        ///
        /// \throws irods::exception If an error occurs.
        ///
        /// \return A std::optional<zone_info>.
        ///
        /// \since 4.3.1
        [[nodiscard]] auto zone_info(RxComm& _comm, const std::string_view _zone_name)
            -> std::optional<struct zone_info>;
    } // namespace NAMESPACE_IMPL
} // namespace irods::experimental::administration

#endif // IRODS_ZONE_ADMINISTRATION_HPP

