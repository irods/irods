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
    auto add_resource(RxComm& _comm, const resource_registration_info& _info) -> std::error_code
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
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto remove_resource(RxComm& _comm, const resource_name_type _name) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "rm";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = ""; // Dryrun flag (cannot be null).

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto add_child_resource(RxComm& _comm,
                            const resource_name_type _parent,
                            const resource_name_type _child,
                            const std::string_view _context_string) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "add";
        input.arg1 = "childtoresc";
        input.arg2 = _parent.data();
        input.arg3 = _child.data();
        input.arg4 = _context_string.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto remove_child_resource(RxComm& _comm,
                               const resource_name_type _parent,
                               const resource_name_type _child) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "rm";
        input.arg1 = "childfromresc";
        input.arg2 = _parent.data();
        input.arg3 = _child.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto resource_exists(RxComm& _comm, const resource_name_type _name)
        -> std::tuple<std::error_code, bool>
    {
        try {
            const auto gql = fmt::format("select RESC_ID where RESC_NAME = '{}'", _name);
            return {std::error_code{}, query_builder{}.build(_comm, gql).size() > 0};
        }
        catch (...) {
            return {std::error_code{SYS_UNKNOWN_ERROR, std::generic_category()}, false};
        }
    }

    auto resource_info(RxComm& _comm, const resource_name_type _name)
        -> std::tuple<std::error_code, std::optional<class resource_info>>
    {
        const auto to_resource_status = [](const std::string_view _status)
        {
            // clang-format off
            if (_status == "up")   return resource_status::up;
            if (_status == "down") return resource_status::down;
            // clang-format on

            return resource_status::unknown;
        };

        class resource_info info;
        info.name_ = _name.data();

        try {
            // clang-format off
            const auto gql = fmt::format("select RESC_ID, RESC_TYPE_NAME, RESC_ZONE_NAME,"
                                         "       RESC_LOC, RESC_VAULT_PATH, RESC_STATUS,"
                                         "       RESC_CONTEXT, RESC_COMMENT, RESC_INFO,"
                                         "       RESC_FREE_SPACE, RESC_FREE_SPACE_TIME,"
                                         "       RESC_PARENT, RESC_CREATE_TIME, RESC_MODIFY_TIME "
                                         "where RESC_NAME = '{}'", _name);
            // clang-format on

            for (auto&& row : query_builder{}.build(_comm, gql)) {
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
            }

            return {std::error_code{}, std::move(info)};
        }
        catch (...) {
            return {std::error_code{SYS_UNKNOWN_ERROR, std::generic_category()}, std::nullopt};
        }
    }

    auto set_resource_type(RxComm& _comm,
                           const resource_name_type _name,
                           const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "type";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_host_name(RxComm& _comm,
                                const resource_name_type _name,
                                const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "host";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_vault_path(RxComm& _comm,
                                 const resource_name_type _name,
                                 const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "path";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_status(RxComm& _comm,
                             const resource_name_type _name,
                             resource_status _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "status";

        // new value
        switch (_new_value) {
            // clang-format off
            case resource_status::up:      input.arg4 = "up"; break;
            case resource_status::down:    input.arg4 = "down"; break;
            case resource_status::unknown: return std::error_code{SYS_INVALID_INPUT_PARAM, std::generic_category()};
            // clang-format on
        }

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_comments(RxComm& _comm,
                               const resource_name_type _name,
                               const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "comment";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_info_string(RxComm& _comm,
                                  const resource_name_type _name,
                                  const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "info";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_free_space(RxComm& _comm,
                                 const resource_name_type _name,
                                 const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "free_space";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto set_resource_context_string(RxComm& _comm,
                                     const resource_name_type _name,
                                     const std::string_view _new_value) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "context";
        input.arg4 = _new_value.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto rebalance_resource(RxComm& _comm, const resource_name_type _name) -> std::error_code
    {
        generalAdminInp_t input{};
        input.arg0 = "modify";
        input.arg1 = "resource";
        input.arg2 = _name.data();
        input.arg3 = "rebalance";
        input.arg4 = ""; // Required to avoid segfaults in server.

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            return std::error_code{ec, std::generic_category()};
        }

        return {};
    }

    auto resource_name(RxComm& _comm, const resource_id_type _id)
        -> std::tuple<std::error_code, std::optional<std::string>>
    {
        try {
            const auto gql = fmt::format("select RESC_NAME where RESC_ID = '{}'", _id);

            for (auto&& row : query_builder{}.build(_comm, gql)) {
                return {std::error_code{}, row[0]};
            }

            return {std::error_code{}, std::nullopt};
        }
        catch (...) {
            return {std::error_code{SYS_UNKNOWN_ERROR, std::generic_category()}, std::nullopt};
        }
    }
} // namespace irods::experimental::administration::NAMESPACE_IMPL

