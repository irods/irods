#include "irods/zone_administration.hpp"

#ifdef IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
// The server maintains a list of the zones known to the system.
// The library takes advantage of this information where possible to avoid
// unnecessary network and database operations.
extern zoneInfo* ZoneInfoHead; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
#endif // IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

namespace irods::experimental::administration::NAMESPACE_IMPL
{
    auto add_zone(RxComm& _comm, const std::string_view _zone_name, const zone_options& _opts) -> void
    {
        if (_zone_name.empty()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid zone name: cannot be empty");
        }

        generalAdminInp_t input{};
        input.arg0 = "add";
        input.arg1 = "zone";
        input.arg2 = _zone_name.data();
        input.arg3 = "remote";
        input.arg4 = _opts.connection_info.data();
        input.arg5 = _opts.comment.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to add zone [{}].", _zone_name));
        }
    } // add_zone

    auto remove_zone(RxComm& _comm, const std::string_view _zone_name) -> void
    {
        if (_zone_name.empty()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid zone name: cannot be empty");
        }

        generalAdminInp_t input{};
        input.arg0 = "rm";
        input.arg1 = "zone";
        input.arg2 = _zone_name.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec < 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to remove zone [{}].", _zone_name));
        }
    } // remove_zone

    auto zone_exists([[maybe_unused]] RxComm& _comm, const std::string_view _zone_name) -> bool
    {
        if (_zone_name.empty()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid zone name: cannot be empty");
        }

#ifdef IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API
        for (const auto* p = ZoneInfoHead; p; p = p->next) { // NOLINT(readability-implicit-bool-conversion)
            if (_zone_name == p->zoneName) { // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                return true;
            }
        }
#else
        const auto gql = fmt::format("select ZONE_NAME where ZONE_NAME = '{}'", _zone_name);

        for ([[maybe_unused]] auto&& row : irods::query{&_comm, gql}) {
            return true;
        }
#endif // IRODS_ZONE_ADMINISTRATION_ENABLE_SERVER_SIDE_API

        return false;
    } // zone_exists

    auto zone_info(RxComm& _comm, const std::string_view _zone_name) -> std::optional<struct zone_info>
    {
        if (_zone_name.empty()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(SYS_INVALID_INPUT_PARAM, "Invalid zone name: cannot be empty");
        }

        const auto gql =
            fmt::format("select ZONE_ID, ZONE_TYPE, ZONE_CONNECTION, ZONE_COMMENT where ZONE_NAME = '{}'", _zone_name);

        for (auto&& row : irods::query{&_comm, gql}) {
            // clang-format off
            return administration::zone_info{
                .name{_zone_name},
                .connection_info = std::move(row[2]),
                .comment = std::move(row[3]),
                .id = std::stoi(row[0]),
                .type = (row[1] == "local") ? zone_type::local : zone_type::remote
            };
            // clang-format on
        }

        return std::nullopt;
    } // zone_info
} // namespace irods::experimental::administration::NAMESPACE_IMPL

