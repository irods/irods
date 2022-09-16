#include "irods/resource_administration.hpp"

#undef rxGeneralAdmin

#ifdef IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
    #include "irods/rsGeneralAdmin.hpp"
    #define rxGeneralAdmin  rsGeneralAdmin
    #define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#else
    #include "irods/generalAdmin.h"
    #define rxGeneralAdmin  rcGeneralAdmin
#endif // IRODS_RESOURCE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/rodsErrorTable.h"
#include "irods/query_builder.hpp"

#include <fmt/format.h>

namespace irods::experimental::administration::NAMESPACE_IMPL
{
    auto add_resource(RxComm& _comm, const resource_registration_info& _info) -> void
    {
        std::string location;

        if (!_info.host_name.empty() && !_info.vault_path.empty()) {
            location = fmt::format("{}:{}", _info.host_name, _info.vault_path);
        }

        generalAdminInp_t input{};
        input.arg0 = "add";
        input.arg1 = "resource";
        input.arg2 = _info.resource_name.data();
        input.arg3 = _info.resource_type.data();
        input.arg4 = location.data();
        input.arg5 = _info.context_string.data();
        input.arg6 = ""; // Zone name (cannot be null).

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            constexpr char* msg = "Could not create resource [name=[{}], type=[{}], location=[{}], context=[{}]]";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format(msg, _info.resource_name, _info.resource_type, location, _info.context_string));
        }
    }

    auto remove_resource(RxComm& _comm, const resource_name_type _name) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "rm";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = ""; // Dryrun flag (cannot be null).

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not remove resource [{}]", _name));
        }
    }

    auto add_child_resource(RxComm& _comm,
                            const resource_name_type _parent,
                            const resource_name_type _child,
                            const std::string_view _context_string) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "add";
        input.arg1 = "childtoresc";
        input.arg2 = _parent.data();
        input.arg3 = _child.data();
        input.arg4 = _context_string.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not add child resource [{}] to resource [{}]", _child, _parent));
        }
    }

    auto remove_child_resource(RxComm& _comm,
                               const resource_name_type _parent,
                               const resource_name_type _child) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "rm";
        input.arg1 = "childfromresc";
        input.arg2 = _parent.data();
        input.arg3 = _child.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not remove child resource [{}] from resource [{}]", _child, _parent));
        }
    }

    auto resource_exists(RxComm& _comm, const resource_name_type _name) -> bool
    {
        try {
            return query_builder{}.build(_comm, fmt::format("select RESC_ID where RESC_NAME = '{}'", _name)).size() > 0;
        }
        catch (const irods::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(e.code(), e.what());
        }
        catch (const std::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_LIBRARY_ERROR, e.what());
        }
        catch (...) {
            constexpr char* msg = "An unknown error occurred while verifying existence of resource [{}]";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_UNKNOWN_ERROR, fmt::format(msg, _name));
        }
    }

    auto resource_info(RxComm& _comm, const resource_name_type _name) -> class resource_info
    {
        const auto to_resource_status = [](const std::string_view _status)
        {
            // clang-format off
            if (_status == "up")   { return resource_status::up; }
            if (_status == "down") { return resource_status::down; }
            // clang-format on

            return resource_status::unknown;
        };

        class resource_info info;
        info.name_ = std::string{_name};

        try {
            const auto gql = fmt::format("select RESC_ID, RESC_TYPE_NAME, RESC_ZONE_NAME, "
                                         "RESC_LOC, RESC_VAULT_PATH, RESC_STATUS, "
                                         "RESC_CONTEXT, RESC_COMMENT, RESC_INFO, "
                                         "RESC_FREE_SPACE, RESC_FREE_SPACE_TIME, "
                                         "RESC_PARENT, RESC_CREATE_TIME, RESC_MODIFY_TIME "
                                         "where RESC_NAME = '{}'", _name);

            for (auto&& row : query_builder{}.build(_comm, gql)) {
                // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
                info.id_ = row[0];
                info.type_ = row[1];
                info.zone_name_ = row[2];
                info.host_name_ = row[3];
                info.vault_path_ = row[4];
                info.status_ = to_resource_status(row[5]);
                info.context_string_ = row[6];
                info.comments_ = row[7];
                info.info_ = row[8];
                info.free_space_ = row[9];
                info.parent_id_ = row[11];
                info.ctime_ = resource_time_type{std::chrono::seconds{std::stoull(row[12])}};
                info.mtime_ = resource_time_type{std::chrono::seconds{std::stoull(row[13])}};

                if (!row[10].empty()) {
                    info.free_space_time_ = resource_time_type{std::chrono::seconds{std::stoull(row[10])}};
                }
                // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
            }

            return info;
        }
        catch (const irods::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(e.code(), e.what());
        }
        catch (const std::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_LIBRARY_ERROR, e.what());
        }
        catch (...) {
            constexpr char* msg = "An unknown error occurred while retrieving information about resource [{}]";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_UNKNOWN_ERROR, fmt::format(msg, _name));
        }
    }

    auto set_resource_type(RxComm& _comm,
                           const resource_name_type _name,
                           const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "type";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change resource type to [{}] for resource [{}] ", _new_value, _name));
        }
    }

    auto set_resource_host_name(RxComm& _comm,
                                const resource_name_type _name,
                                const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "host";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change hostname to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto set_resource_vault_path(RxComm& _comm,
                                 const resource_name_type _name,
                                 const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "path";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change vault path to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto set_resource_status(RxComm& _comm,
                             const resource_name_type _name,
                             resource_status _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "status";

        // new value
        switch (_new_value) {
            case resource_status::up:
                input.arg4 = "up";
                break;

            case resource_status::down:
                input.arg4 = "down";
                break;

            case resource_status::unknown:
            default:
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format("Invalid status for resource [{}]", _name));
        }

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change status to [{}] for resource [{}]", input.arg4, _name));
        }
    }

    auto set_resource_comments(RxComm& _comm,
                               const resource_name_type _name,
                               const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "comment";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change comment to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto set_resource_info_string(RxComm& _comm,
                                  const resource_name_type _name,
                                  const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "info";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change info-string to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto set_resource_free_space(RxComm& _comm,
                                 const resource_name_type _name,
                                 const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "free_space";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change free-space to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto set_resource_context_string(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "context";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Could not change context to [{}] for resource [{}]", _new_value, _name));
        }
    }

    auto rebalance_resource(RxComm& _comm, const resource_name_type _name) -> void
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "rebalance";
        input.arg4 = ""; // Required to avoid segfaults in server.

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Encountered rebalance error for resource [{}]", _name));
        }
    }

    auto resource_name(RxComm& _comm, const resource_id_type _id) -> std::optional<std::string>
    {
        try {
            const auto gql = fmt::format("select RESC_NAME where RESC_ID = '{}'", _id);

            for (auto&& row : query_builder{}.build(_comm, gql)) {
                return row[0];
            }

            return std::nullopt;
        }
        catch (const irods::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(e.code(), e.what());
        }
        catch (const std::exception& e) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_LIBRARY_ERROR, e.what());
        }
        catch (...) {
            constexpr char* msg = "An unknown error occurred while retrieving resource name with id [{}]";
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_UNKNOWN_ERROR, fmt::format(msg, _id));
        }
    }
} // namespace irods::experimental::administration::NAMESPACE_IMPL

