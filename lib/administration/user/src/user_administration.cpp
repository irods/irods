#include "irods/user_administration.hpp"

#ifdef IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#  define IRODS_QUERY_ENABLE_SERVER_SIDE_API
#endif // IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API

#include "irods/irods_query.hpp"
#include "irods/query_builder.hpp"

namespace irods::experimental::administration::NAMESPACE_IMPL
{
    namespace
    {
        auto get_local_zone([[maybe_unused]] RxComm& _comm) -> std::string
        {
#ifdef IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
            return getLocalZoneName();
#else
            for (auto&& row : irods::query{&_comm, "select ZONE_NAME where ZONE_TYPE = 'local'"}) {
                return row[0];
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(CAT_NO_ROWS_FOUND, "Cannot get local zone name.");
#endif // IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
        } // get_local_zone
    } // anonymous namespace

    auto local_unique_name(RxComm& _comm, const user& _user) -> std::string
    {
        // Implies that the user belongs to the local zone and
        // is not a remote user (i.e. federation).
        if (_user.zone.empty()) {
            return _user.name;
        }

        auto name = _user.name;

        if (_user.zone != get_local_zone(_comm)) {
            name += '#';
            name += _user.zone;
        }

        return name;
    } // local_unique_name

    auto add_user(RxComm& _comm, const user& _user, user_type _user_type, zone_type _zone_type) -> void
    {
        std::string name = local_unique_name(_comm, _user);
        std::string zone;

        if (zone_type::local == _zone_type) {
            zone = get_local_zone(_comm);
        }

        GeneralAdminInput input{};
        input.arg0 = "add";
        input.arg1 = "user";
        input.arg2 = name.data();
        input.arg3 = to_c_str(_user_type);
        input.arg4 = zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to add user [{}].", name));
        }
    } // add_user

    auto remove_user(RxComm& _comm, const user& _user) -> void
    {
        const auto name = local_unique_name(_comm, _user);

        GeneralAdminInput input{};
        input.arg0 = "rm";
        input.arg1 = "user";
        input.arg2 = name.data();
        input.arg3 = _user.zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to remove user [{}].", name));
        }
    } // remove_user

    auto add_group(RxComm& _comm, const group& _group) -> void
    {
        const auto zone = get_local_zone(_comm);

        GeneralAdminInput input{};
        input.arg0 = "add";
        input.arg1 = "user";
        input.arg2 = _group.name.data();
        input.arg3 = "rodsgroup";
        input.arg4 = zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to add group [{}].", _group.name));
        }
    } // add_group

    auto remove_group(RxComm& _comm, const group& _group) -> void
    {
        const auto zone = get_local_zone(_comm);

        GeneralAdminInput input{};
        input.arg0 = "rm";
        input.arg1 = "user";
        input.arg2 = _group.name.data();
        input.arg3 = zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to remove group [{}].", _group.name));
        }
    } // remove_group

    auto add_user_to_group(RxComm& _comm, const group& _group, const user& _user) -> void
    {
        GeneralAdminInput input{};
        input.arg0 = "modify";
        input.arg1 = "group";
        input.arg2 = _group.name.data();
        input.arg3 = "add";
        input.arg4 = _user.name.data();
        input.arg5 = _user.zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            const auto name = local_unique_name(_comm, _user);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to add user [{}] to group [{}].", name, _group.name));
        }
    } // add_user_to_group

    auto remove_user_from_group(RxComm& _comm, const group& _group, const user& _user) -> void
    {
        GeneralAdminInput input{};
        input.arg0 = "modify";
        input.arg1 = "group";
        input.arg2 = _group.name.data();
        input.arg3 = "remove";
        input.arg4 = _user.name.data();
        input.arg5 = _user.zone.data();

        if (const auto ec = rxGeneralAdmin(&_comm, &input); ec != 0) {
            const auto name = local_unique_name(_comm, _user);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            THROW(ec, fmt::format("Failed to remove user [{}] from group [{}].", name, _group.name));
        }
    } // remove_user_from_group

    auto users(RxComm& _comm) -> std::vector<user>
    {
        std::vector<user> users;

        for (auto&& row : irods::query{&_comm, "select USER_NAME, USER_ZONE where USER_TYPE != 'rodsgroup'"}) {
            users.emplace_back(row[0], row[1]);
        }

        return users;
    } // users

    auto users(RxComm& _comm, const group& _group) -> std::vector<user>
    {
        std::vector<user> users;

        std::string gql = "select USER_NAME, USER_ZONE where USER_TYPE != 'rodsgroup' and USER_GROUP_NAME = '";
        gql += _group.name;
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) {
            users.emplace_back(row[0], row[1]);
        }

        return users;
    } // users

    auto groups(RxComm& _comm) -> std::vector<group>
    {
        std::vector<group> groups;

        for (auto&& row : irods::query{&_comm, "select USER_GROUP_NAME where USER_TYPE = 'rodsgroup'"}) {
            groups.emplace_back(row[0]);
        }

        return groups;
    } // groups

    auto groups(RxComm& _comm, const user& _user) -> std::vector<group>
    {
        std::vector<std::string> args{local_unique_name(_comm, _user)};

        query_builder qb;

        // clang-format off
        qb.type(query_type::specific)
          .zone_hint(_user.zone.empty() ? get_local_zone(_comm) : _user.zone)
          .bind_arguments(args);
        // clang-format on

        std::vector<group> groups;

        try {
            for (auto&& row : qb.build(_comm, "listGroupsForUser")) {
                groups.emplace_back(row[1]);
            }
        }
        catch (...) {
            // GenQuery fallback.
            groups.clear();

            for (auto&& g : NAMESPACE_IMPL::groups(_comm)) {
                if (user_is_member_of_group(_comm, g, _user)) {
                    groups.push_back(std::move(g));
                }
            }
        }

        return groups;
    } // groups

    auto exists(RxComm& _comm, const user& _user) -> bool
    {
        std::string gql = "select USER_ID where USER_TYPE != 'rodsgroup' and USER_NAME = '";
        gql += _user.name;
        gql += "' and USER_ZONE = '";
        gql += (_user.zone.empty() ? get_local_zone(_comm) : _user.zone);
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) { // NOLINTNEXTLINE(readability-use-anyofallof)
            static_cast<void>(row);
            return true;
        }

        return false;
    } // exists

    auto exists(RxComm& _comm, const group& _group) -> bool
    {
        std::string gql = "select USER_GROUP_ID where USER_TYPE = 'rodsgroup' and USER_GROUP_NAME = '";
        gql += _group.name;
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) { // NOLINTNEXTLINE(readability-use-anyofallof)
            static_cast<void>(row);
            return true;
        }

        return false;
    } // exists

    auto id(RxComm& _comm, const user& _user) -> std::optional<std::string>
    {
        std::string gql = "select USER_ID where USER_TYPE != 'rodsgroup' and USER_NAME = '";
        gql += _user.name;
        gql += "' and USER_ZONE = '";
        gql += (_user.zone.empty() ? get_local_zone(_comm) : _user.zone);
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) {
            return row[0];
        }

        return std::nullopt;
    } // id

    auto id(RxComm& _comm, const group& _group) -> std::optional<std::string>
    {
        std::string gql = "select USER_GROUP_ID where USER_TYPE = 'rodsgroup' and USER_GROUP_NAME = '";
        gql += _group.name;
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) {
            return row[0];
        }

        return std::nullopt;
    } // id

    auto type(RxComm& _comm, const user& _user) -> std::optional<user_type>
    {
        std::string gql = "select USER_TYPE where USER_TYPE != 'rodsgroup' and USER_NAME = '";
        gql += _user.name;
        gql += "' and USER_ZONE = '";
        gql += (_user.zone.empty() ? get_local_zone(_comm) : _user.zone);
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) {
            return to_user_type(row[0]);
        }

        return std::nullopt;
    } // type

    auto auth_names(RxComm& _comm, const user& _user) -> std::vector<std::string>
    {
        std::vector<std::string> auth_names;

        std::string gql = "select USER_DN where USER_TYPE != 'rodsgroup' and USER_NAME = '";
        gql += _user.name;
        gql += "' and USER_ZONE = '";
        gql += (_user.zone.empty() ? get_local_zone(_comm) : _user.zone);
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) {
            auth_names.push_back(row[0]);
        }

        return auth_names;
    } // auth_names

    auto user_is_member_of_group(RxComm& _comm, const group& _group, const user& _user) -> bool
    {
        std::string gql = "select USER_ID where USER_TYPE != 'rodsgroup' and USER_NAME = '";
        gql += _user.name;
        gql += "' and USER_ZONE = '";
        gql += (_user.zone.empty() ? get_local_zone(_comm) : _user.zone);
        gql += "' and USER_GROUP_NAME = '";
        gql += _group.name;
        gql += "'";

        for (auto&& row : irods::query{&_comm, gql}) { // NOLINTNEXTLINE(readability-use-anyofallof)
            static_cast<void>(row);
            return true;
        }

        return false;
    } // user_is_member_of_group
} // namespace irods::experimental::administration::NAMESPACE_IMPL
