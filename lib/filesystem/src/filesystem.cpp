#include "irods/filesystem/filesystem.hpp"

#include "irods/filesystem/path.hpp"
#include "irods/filesystem/collection_iterator.hpp"

// clang-format off
#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
    #define IRODS_QUERY_ENABLE_SERVER_SIDE_API

    #include "irods/rods.h"
    #include "irods/apiHeaderAll.h"
    #include "irods/rsSpecificQuery.hpp"
    #include "irods/rsObjStat.hpp"
    #include "irods/rsDataObjCopy.hpp"
    #include "irods/rsDataObjRename.hpp"
    #include "irods/rsDataObjUnlink.hpp"
    #include "irods/rsDataObjChksum.hpp"
    #include "irods/rsModAccessControl.hpp"
    #include "irods/rsCollCreate.hpp"
    #include "irods/rsModColl.hpp"
    #include "irods/rsRmColl.hpp"
    #include "irods/rsModAVUMetadata.hpp"
    #include "irods/rsModDataObjMeta.hpp"
#else
    #include "irods/rodsClient.h"
    #include "irods/specificQuery.h"
    #include "irods/objStat.h"
    #include "irods/dataObjCopy.h"
    #include "irods/dataObjRename.h"
    #include "irods/dataObjUnlink.h"
    #include "irods/dataObjChksum.h"
    #include "irods/modAccessControl.h"
    #include "irods/collCreate.h"
    #include "irods/modColl.h"
    #include "irods/rmColl.h"
    #include "irods/modAVUMetadata.h"
    #include "irods/data_object_modify_info.h"
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
// clang-format on

#include "irods/rcMisc.h"
#include "irods/rodsErrorTable.h"
#include "irods/irods_log.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_query.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/query_builder.hpp"

#include <fmt/format.h>

#include <cctype>
#include <cstring>
#include <string>
#include <iterator>
#include <exception>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace irods::experimental::filesystem::NAMESPACE_IMPL
{
    namespace
    {
        using filesystem::detail::make_error_code;

#ifdef IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
        int rsDataObjCopy(rsComm_t* _comm, dataObjCopyInp_t* _dataObjCopyInp)
        {
            transferStat_t* ts_ptr{};

            const auto ec = rsDataObjCopy(_comm, _dataObjCopyInp, &ts_ptr);

            if (ts_ptr) {
                std::free(ts_ptr);
            }

            return ec;
        }

        const auto rsRmColl = [](rsComm_t* _comm, collInp_t* _rmCollInp, bool _track_progress) -> int
        {
            collOprStat_t* stat{};
            return ::rsRmColl(_comm, _rmCollInp, _track_progress ? &stat : nullptr);
        };
#endif // IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API

        struct stat
        {
            int error;
            long long size;
            int type;
            int mode;
            long long id;
            std::string owner_name;
            std::string owner_zone;
            long long ctime;
            long long mtime;
            std::vector<entity_permission> prms;
            bool inheritance;
        };

        auto to_permission_enum(const std::string& _perm) -> perms
        {
            // clang-format off
            if (_perm == "read_metadata")   { return perms::read_metadata; }
            if (_perm == "read_object")     { return perms::read; }
            if (_perm == "read")            { return perms::read; }
            if (_perm == "create_metadata") { return perms::create_metadata; }
            if (_perm == "modify_metadata") { return perms::modify_metadata; }
            if (_perm == "delete_metadata") { return perms::delete_metadata; }
            if (_perm == "create_object")   { return perms::create_object; }
            if (_perm == "modify_object")   { return perms::write; }
            if (_perm == "write")           { return perms::write; }
            if (_perm == "delete_object")   { return perms::delete_object; }
            if (_perm == "own")             { return perms::own; }
            // clang-format on

            return perms::null;
        }

        auto set_permissions(rxComm& _comm, const path& _p, stat& _s) -> void
        {
            if (DATA_OBJ_T == _s.type) {
                std::string sql = "select DATA_USER_NAME, DATA_ZONE_NAME, DATA_ACCESS_NAME, USER_TYPE "
                                   "where COLL_NAME = '";
                sql += _p.parent_path();
                sql += "' and DATA_NAME = '";
                sql += _p.object_name();
                sql += "' and DATA_TOKEN_NAMESPACE = 'access_type'";

                irods::experimental::query_builder qb;

                if (const auto zone = zone_name(_p); zone) {
                    qb.zone_hint(*zone);
                }

                for (const auto& row : qb.build(_comm, sql)) {
                    _s.prms.push_back({row[0], row[1], to_permission_enum(row[2]), row[3]});
                }
            }
            else if (COLL_OBJ_T == _s.type) {
                irods::experimental::query_builder qb;

                if (const auto zone = zone_name(_p); zone) {
                    qb.zone_hint(*zone);
                }

                try {
                    std::vector<std::string> args{_p.string()};

                    qb.type(irods::experimental::query_type::specific)
                      .bind_arguments(args);

                    for (const auto& row : qb.build(_comm, "ShowCollAcls")) {
                        _s.prms.push_back({row[0], row[1], to_permission_enum(row[2]), row[3]});
                    }
                }
                catch (...) {
                    // Fallback to GenQuery if the specific query fails (does not exist).
                    //
                    // In the case where the specific query, "ShowCollAcls", does not exist,
                    // this implementation is required to fallback to GenQuery. The following
                    // code does not require any information from the exception object, therefore
                    // it is ignored.

                    qb.type(irods::experimental::query_type::general);

                    std::string sql = "select COLL_USER_NAME, COLL_ZONE_NAME, COLL_ACCESS_NAME, USER_TYPE "
                                      "where COLL_TOKEN_NAMESPACE = 'access_type' and COLL_NAME = '";
                    sql += _p.c_str();
                    sql += "'";

                    for (const auto& row : qb.build(_comm, sql)) {
                        _s.prms.push_back({row[0], row[1], to_permission_enum(row[2]), row[3]});
                    }
                }
            }
        }

        auto get_inheritance(rxComm& _comm, const path& _p, int _object_type) -> bool
        {
            if (COLL_OBJ_T != _object_type) {
                return false;
            }

            irods::experimental::query_builder qb;

            if (const auto zone = zone_name(_p); zone) {
                qb.zone_hint(*zone);
            }

            qb.type(irods::experimental::query_type::general);

            const auto gql = fmt::format("select COLL_INHERITANCE where COLL_NAME = '{}'", _p.c_str());

            for (const auto& row : qb.build(_comm, gql)) {
                return std::atoi(row[0].data());
            }

            return false;
        }

        auto stat(rxComm& _comm, const path& _p) -> stat
        {
            dataObjInp_t input{};
            std::strncpy(input.objPath, _p.c_str(), std::strlen(_p.c_str()));

            rodsObjStat_t* output{};
            struct stat s{};

            if (s.error = rxObjStat(&_comm, &input, &output); s.error >= 0) {
                irods::at_scope_exit at_scope_exit{[output] {
                    freeRodsObjStat(output);
                }};

                try {
                    s.id = std::stoll(output->dataId);
                    s.ctime = std::stoll(output->createTime);
                    s.mtime = std::stoll(output->modifyTime);
                }
                catch (...) {
                    throw filesystem_error{"stat error: cannot convert string to integer", _p, make_error_code(SYS_INTERNAL_ERR)};
                }

                s.size = output->objSize;
                s.type = output->objType;
                s.mode = static_cast<int>(output->dataMode);
                s.owner_name = output->ownerName;
                s.owner_zone = output->ownerZone;
                s.inheritance = get_inheritance(_comm, _p, s.type);

                set_permissions(_comm, _p, s);
            }
            else if (USER_FILE_DOES_NOT_EXIST == s.error) {
                s.error = 0;
                s.type = UNKNOWN_OBJ_T;
            }

            return s;
        }

        auto is_collection_empty(rxComm& _comm, const path& _p) -> bool
        {
            return collection_iterator{} == collection_iterator{_comm, _p};
        }

        auto remove_impl(rxComm& _comm, const path& _p, extended_remove_options _opts) -> bool
        {
            filesystem::detail::throw_if_path_length_exceeds_limit(_p);

            const auto s = status(_comm, _p);

            if (!exists(s)) {
                return false;
            }

            if (is_data_object(s)) {
                dataObjInp_t input{};
                at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};

                input.oprType = _opts.unregister ? UNREG_OPR : 0;

                std::strncpy(input.objPath, _p.c_str(), std::strlen(_p.c_str()));

                if (_opts.no_trash) {
                    addKeyVal(&input.condInput, FORCE_FLAG_KW, "");
                }

                return rxDataObjUnlink(&_comm, &input) == 0;
            }

            if (is_collection(s)) {
                if (!_opts.recursive && !is_collection_empty(_comm, _p)) {
                    throw filesystem_error{"cannot remove non-empty collection", _p, make_error_code(SYS_COLLECTION_NOT_EMPTY)};
                }

                collInp_t input{};
                at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};

                std::strncpy(input.collName, _p.c_str(), std::strlen(_p.c_str()));

                if (_opts.no_trash) {
                    addKeyVal(&input.condInput, FORCE_FLAG_KW, "");
                }

                if (_opts.recursive) {
                    addKeyVal(&input.condInput, RECURSIVE_OPR__KW, "");
                }

                return rxRmColl(&_comm, &input, _opts.progress) >= 0;
            }

            throw filesystem_error{"cannot remove: unknown object type", _p, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        auto set_permissions(bool _add_admin_flag,
                             rxComm& _comm,
                             const path& _p,
                             const std::string& _user_or_group,
                             perms _prms) -> void
        {
            filesystem::detail::throw_if_path_length_exceeds_limit(_p);

            char username[NAME_LEN]{};
            char zone[NAME_LEN]{};

            auto ec = parseUserName(_user_or_group.c_str(), username, zone);

            if (ec != 0) {
                throw filesystem_error{"cannot parse user/group name", _p, make_error_code(ec)};
            }

            modAccessControlInp_t input{};

            input.userName = username;
            input.zone = zone;

            char path[MAX_NAME_LEN]{};
            std::strncpy(path, _p.c_str(), std::strlen(_p.c_str()));
            input.path = path;

            char access[16]{};

            if (_add_admin_flag) {
                std::strcpy(access, "admin:");
            }

            switch (_prms) {
                case perms::null:
                    std::strncat(access, "null", 4);
                    break;

                case perms::read_metadata:
                    std::strncat(access, "read_metadata", 13);
                    break;

                case perms::read_object:
                case perms::read:
                    std::strncat(access, "read", 4);
                    break;

                case perms::create_metadata:
                    std::strncat(access, "create_metadata", 15);
                    break;

                case perms::modify_metadata:
                    std::strncat(access, "modify_metadata", 15);
                    break;

                case perms::delete_metadata:
                    std::strncat(access, "delete_metadata", 15);
                    break;

                case perms::create_object:
                    std::strncat(access, "create_object", 13);
                    break;

                case perms::modify_object:
                case perms::write:
                    std::strncat(access, "write", 5);
                    break;

                case perms::delete_object:
                    std::strncat(access, "delete_object", 13);
                    break;

                case perms::own:
                    std::strncat(access, "own", 3);
                    break;
            }

            input.accessLevel = access;

            ec = rxModAccessControl(&_comm, &input);

            if (ec != 0) {
                throw filesystem_error{"cannot set permissions", _p, make_error_code(ec)};
            }
        }

        auto set_inheritance(bool _add_admin_flag, rxComm& _comm, const path& _p, bool _value) -> void
        {
            filesystem::detail::throw_if_path_is_empty(_p);
            filesystem::detail::throw_if_path_length_exceeds_limit(_p);

            if (!is_collection(_comm, _p)) {
                throw filesystem_error{"existing path is not a collection", _p, make_error_code(NOT_A_COLLECTION)};
            }

            modAccessControlInp_t input{};

            input.userName = "";
            input.zone = "";

            char path[MAX_NAME_LEN]{};
            std::strncpy(path, _p.c_str(), std::strlen(_p.c_str()));
            input.path = path;

            char access[16]{};

            if (_add_admin_flag) {
                std::strcpy(access, "admin:");
            }

            std::strcat(access, _value ? "inherit" : "noinherit");

            input.accessLevel = access;

            const auto ec = rxModAccessControl(&_comm, &input);

            if (ec != 0) {
                throw filesystem_error{"cannot set inheritance", _p, make_error_code(ec)};
            }
        }

        auto has_prefix(const path& _p, const path& _prefix) -> bool
        {
            if (_p == _prefix) {
                return false;
            }

            auto p_iter = std::begin(_prefix);
            auto p_last = std::end(_prefix);
            auto c_iter = std::begin(_p);
            auto c_last = std::end(_p);

            for (; p_iter != p_last && c_iter != c_last && *p_iter == *c_iter; ++p_iter, ++c_iter);

            return (p_iter == p_last);
        }

        auto do_metadata_op(bool _add_admin_flag,
                            rxComm& _comm,
                            const path& _p,
                            const metadata& _metadata,
                            std::string_view op) -> void
        {
            filesystem::detail::throw_if_path_is_empty(_p);
            filesystem::detail::throw_if_path_length_exceeds_limit(_p);

            modAVUMetadataInp_t input{};

            char* command = const_cast<char*>(op.data());
            input.arg0 = command;

            char type[3]{};

            if (const auto s = status(_comm, _p); is_data_object(s)) {
                std::strncpy(type, "-d", std::strlen("-d"));
            }
            else if (is_collection(s)) {
                std::strncpy(type, "-C", std::strlen("-C"));
            }
            else {
                std::string_view op_full_name = (op == "rm") ? "remove" : op;
                throw filesystem_error{fmt::format("cannot {} metadata: unknown object type", op_full_name), _p,
                                       make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
            }

            input.arg1 = type;

            char path_buf[MAX_NAME_LEN]{};
            std::strncpy(path_buf, _p.c_str(), std::strlen(_p.c_str()));
            input.arg2 = path_buf;

            char attr_buf[MAX_NAME_LEN]{};
            std::strncpy(attr_buf, _metadata.attribute.c_str(), _metadata.attribute.size());
            input.arg3 = attr_buf;

            char value_buf[MAX_NAME_LEN]{};
            std::strncpy(value_buf, _metadata.value.c_str(), _metadata.value.size());
            input.arg4 = value_buf;

            char units_buf[MAX_NAME_LEN]{};
            std::strncpy(units_buf, _metadata.units.c_str(), _metadata.units.size());
            input.arg5 = units_buf;

            at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};

            if (_add_admin_flag) {
                addKeyVal(&input.condInput, ADMIN_KW, "");
            }

            if (const auto ec = rxModAVUMetadata(&_comm, &input); ec != 0) {
                std::string_view op_full_name = (op == "rm") ? "remove" : op;
                throw filesystem_error{fmt::format("cannot {} metadata", op_full_name), _p, make_error_code(ec)};
            }
        }
    } // anonymous namespace

    // Operational functions

    auto copy(rxComm& _comm, const path& _from, const path& _to, copy_options _options) -> void
    {
        const auto from_status = status(_comm, _from);

        if (!exists(from_status)) {
            throw filesystem_error{"path does not exist", _from, make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
        }

        const auto to_status = status(_comm, _to);

        if (exists(to_status) && equivalent(_comm, _from, _to)) {
            throw filesystem_error{"paths cannot point to the same object", _from, _to, make_error_code(SAME_SRC_DEST_PATHS_ERR)};
        }

        if (is_other(from_status)) {
            throw filesystem_error{"cannot copy: unknown object type", _from, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        if (is_other(to_status)) {
            throw filesystem_error{"cannot copy: unknown object type", _to, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        if (is_collection(from_status) && is_data_object(to_status)) {
            throw filesystem_error{"cannot copy a collection into a data object", _from, _to, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }

        if (is_data_object(from_status)) {
            if (copy_options::collections_only == (copy_options::collections_only & _options)) {
                return;
            }

            if (is_collection(to_status)) {
                copy_data_object(_comm, _from, _to / _from.object_name(), _options);
                return;
            }

            copy_data_object(_comm, _from, _to, _options);
        }
        else if (is_collection(from_status)) {
            if (copy_options::recursive == (copy_options::recursive & _options) ||
                copy_options::none == _options)
            {
                if (!exists(to_status)) {
                    if (!create_collection(_comm, _to, _from)) {
                        throw filesystem_error{"cannot create collection", _to, make_error_code(FILE_CREATE_ERROR)};
                    }
                }

                for (const auto& e : collection_iterator{_comm, _from}) {
                    copy(_comm, e.path(), _to / e.path().object_name(), _options | copy_options::in_recursive_copy);
                }
            }
        }
    }

    auto copy_data_object(rxComm& _comm, const path& _from, const path& _to, copy_options _options) -> bool
    {
        filesystem::detail::throw_if_path_length_exceeds_limit(_from);
        filesystem::detail::throw_if_path_length_exceeds_limit(_to);

        if (!is_data_object(_comm, _from)) {
            throw filesystem_error{"path does not point to a data object", _from, make_error_code(INVALID_OBJECT_TYPE)};
        }

        dataObjCopyInp_t input{};

        at_scope_exit free_memory{[&input] {
            clearKeyVal(&input.srcDataObjInp.condInput);
            clearKeyVal(&input.destDataObjInp.condInput);
        }};

        if (const auto s = status(_comm, _to); exists(s)) {
            if (equivalent(_comm, _from, _to)) {
                throw filesystem_error{"paths cannot point to the same object", _from, _to, make_error_code(SAME_SRC_DEST_PATHS_ERR)};
            }

            if (!is_data_object(s)) {
                throw filesystem_error{"path does not point to a data object", _to, make_error_code(INVALID_OBJECT_TYPE)};
            }

            if (copy_options::none == _options) {
                throw filesystem_error{"copy options not set", make_error_code(SYS_INVALID_INPUT_PARAM)};
            }

            if (copy_options::skip_existing == _options) {
                return false;
            }

            if (copy_options::overwrite_existing == _options) {
                addKeyVal(&input.destDataObjInp.condInput, FORCE_FLAG_KW, "");
            }

            if (copy_options::update_existing == _options) {
                if (last_write_time(_comm, _from) <= last_write_time(_comm, _to)) {
                    return false;
                }
            }
        }

        std::strncpy(input.srcDataObjInp.objPath, _from.c_str(), std::strlen(_from.c_str()));
        std::strncpy(input.destDataObjInp.objPath, _to.c_str(), std::strlen(_to.c_str()));
        addKeyVal(&input.destDataObjInp.condInput, DEST_RESC_NAME_KW, "");

        if (const auto ec = rxDataObjCopy(&_comm, &input); ec < 0) {
            throw filesystem_error{"cannot copy data object", _from, _to, make_error_code(ec)};
        }

        return true;
    }

    auto create_collection(rxComm& _comm, const path& _p) -> bool // Implies perms::all
    {
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        if (!exists(_comm, _p.parent_path())) {
            throw filesystem_error{"path does not exist", _p.parent_path(), make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
        }

        if (exists(_comm, _p)) {
            return false;
        }

        collInp_t input{};
        std::strncpy(input.collName, _p.c_str(), std::strlen(_p.c_str()));

        if (const auto ec = rxCollCreate(&_comm, &input); ec != 0) {
            throw filesystem_error{"cannot create collection", _p, make_error_code(ec)};
        }

        return true;
    }

    auto create_collection(rxComm& _comm, const path& _p, const path& _existing_p) -> bool
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_is_empty(_existing_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_existing_p);

        const auto s = status(_comm, _existing_p);

        if (!is_collection(s)) {
            throw filesystem_error{"existing path is not a collection", _existing_p, make_error_code(INVALID_OBJECT_TYPE)};
        }

        create_collection(_comm, _p);

        for (auto&& p : s.permissions()) {
            permissions(_comm, _p, p.name, p.prms);
        }

        return true;
    }

    auto create_collections(rxComm& _comm, const path& _p) -> bool
    {
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        if (exists(_comm, _p)) {
            return false;
        }

        collInp_t input{};
        at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};
        std::strncpy(input.collName, _p.c_str(), std::strlen(_p.c_str()));
        addKeyVal(&input.condInput, RECURSIVE_OPR__KW, "");

        return rxCollCreate(&_comm, &input) == 0;
    }

    auto exists(const object_status& _s) noexcept -> bool
    {
        return status_known(_s) && _s.type() != object_type::not_found;
    }

    auto exists(rxComm& _comm, const path& _p) -> bool
    {
        return exists(status(_comm, _p));
    }

    auto is_collection_registered(rxComm& _comm, const path& _p) -> bool
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        const auto gql = fmt::format("select COLL_ID where COLL_NAME = '{}'", _p.c_str());

        return qb.build(_comm, gql).size() > 0;
    }

    auto is_data_object_registered(rxComm& _comm, const path& _p) -> bool
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        const auto gql = fmt::format("select DATA_ID where COLL_NAME = '{}' and DATA_NAME = '{}'",
                                     _p.parent_path().c_str(),
                                     _p.object_name().c_str());

        return qb.build(_comm, gql).size() > 0;
    }

    auto equivalent(rxComm& _comm, const path& _p1, const path& _p2) -> bool
    {
        filesystem::detail::throw_if_path_is_empty(_p1);
        filesystem::detail::throw_if_path_is_empty(_p2);

        const auto p1_info = stat(_comm, _p1);

        if (p1_info.error < 0) {
            throw filesystem_error{"cannot stat path", _p1, make_error_code(p1_info.error)};
        }

        if (p1_info.type == UNKNOWN_OBJ_T) {
            throw filesystem_error{"path does not exist", _p1, make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
        }

        const auto p2_info = stat(_comm, _p2);

        if (p2_info.error < 0) {
            throw filesystem_error{"cannot stat path", _p2, make_error_code(p2_info.error)};
        }

        if (p2_info.type == UNKNOWN_OBJ_T) {
            throw filesystem_error{"path does not exist", _p2, make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
        }

        return p1_info.id == p2_info.id;
    }

    auto data_object_size(rxComm& _comm, const path& _p) -> std::uintmax_t
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        if (!is_data_object(_comm, _p)) {
            throw filesystem_error{"path does not point to a data object", _p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }

        // Fetch information for good replicas only (i.e. DATA_REPL_STATUS = '1').
        const auto gql = fmt::format("select DATA_SIZE, DATA_MODIFY_TIME "
                                     "where"
                                     " COLL_NAME = '{}' and"
                                     " DATA_NAME = '{}' and"
                                     " DATA_REPL_STATUS = '1'",
                                     _p.parent_path().c_str(),
                                     _p.object_name().c_str());

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        auto query = qb.build(_comm, gql);

        if (query.size() == 0) {
            throw filesystem_error{"no good replica found", _p, make_error_code(SYS_NO_GOOD_REPLICA)};
        }

        // This implementation assumes that any good replica will always satisfy
        // the requirement within the loop, therefore the first iteration always causes
        // the size to be captured. The size object should be empty if and only if there
        // are no good replicas.
        std::uint64_t latest_mtime = 0;
        std::uintmax_t size = 0;

        for (auto&& row : query) {
            // As we iterate over the replicas, compare the mtimes and capture the size
            // of the latest good replica.
            if (const auto current_mtime = std::stoull(row[1]); current_mtime > latest_mtime) {
                latest_mtime = current_mtime;
                size = std::stoull(row[0]);
            }
        }

        return size;
    }

    auto is_collection(const object_status& _s) noexcept -> bool
    {
        return _s.type() == object_type::collection;
    }

    auto is_collection(rxComm& _comm, const path& _p) -> bool
    {
        return is_collection(status(_comm, _p));
    }

    auto is_special_collection(rxComm& _comm, const path& _p) -> bool
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        const auto gql = fmt::format("select COLL_TYPE, COLL_INFO_1, COLL_INFO_2 "
                                     "where COLL_NAME = '{}'", _p.c_str());

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        for (auto&& row : qb.build(_comm, gql)) {
            return !row[0].empty() && (!row[1].empty() || !row[2].empty());
        }

        return false;
    }

    auto is_empty(rxComm& _comm, const path& _p) -> bool
    {
        const auto s = status(_comm, _p);

        if (is_collection(s)) {
            return is_collection_empty(_comm, _p);
        }

        throw filesystem_error{"cannot check emptiness: unknown object type", _p, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
    }

    auto is_other(const object_status& _s) noexcept -> bool
    {
        return _s.type() == object_type::unknown;
    }

    auto is_other(rxComm& _comm, const path& _p) -> bool
    {
        return is_other(status(_comm, _p));
    }

    auto is_data_object(const object_status& _s) noexcept -> bool
    {
        return _s.type() == object_type::data_object;
    }

    auto is_data_object(rxComm& _comm, const path& _p) -> bool
    {
        return is_data_object(status(_comm, _p));
    }

    auto last_write_time(rxComm& _comm, const path& _p) -> object_time_type
    {
        std::string gql;

        if (const auto s = status(_comm, _p); is_data_object(s)) {
            // Fetch information for good replicas only (i.e. DATA_REPL_STATUS = '1').
            gql = fmt::format("select max(DATA_MODIFY_TIME) "
                              "where"
                              " COLL_NAME = '{}' and"
                              " DATA_NAME = '{}' and"
                              " DATA_REPL_STATUS = '1'",
                              _p.parent_path().c_str(),
                              _p.object_name().c_str());
        }
        else if (is_collection(s)) {
            gql = fmt::format("select COLL_MODIFY_TIME where COLL_NAME = '{}'", _p.c_str());
        }
        else {
            throw filesystem_error{"cannot get mtime", _p, make_error_code(INVALID_OBJECT_TYPE)};
        }

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        for (auto&& row : qb.build(_comm, gql)) {
            return object_time_type{std::chrono::seconds{std::stoull(row[0])}};
        }

        throw filesystem_error{"cannot get mtime", _p, make_error_code(CAT_NO_ROWS_FOUND)};
    }

    auto last_write_time(rxComm& _comm, const path& _p, object_time_type _new_time) -> void
    {
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        if (!is_collection(_comm, _p)) {
            throw filesystem_error{"path does not point to a collection", _p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }

        const auto timestamp = fmt::format("{:011}", _new_time.time_since_epoch().count());

        collInp_t input{};
        at_scope_exit free_memory{[&input] { clearKeyVal(&input.condInput); }};
        std::strncpy(input.collName, _p.c_str(), std::strlen(_p.c_str()));
        addKeyVal(&input.condInput, COLLECTION_MTIME_KW, timestamp.c_str());

        if (const auto ec = rxModColl(&_comm, &input); ec != 0) {
            throw filesystem_error{"cannot set mtime", _p, make_error_code(ec)};
        }
    }

    auto remove(rxComm& _comm, const path& _p, remove_options _opts) -> bool
    {
        extended_remove_options opts{};
        opts.no_trash = (remove_options::no_trash == _opts);
        opts.recursive = false;
        return remove_impl(_comm, _p, opts);
    }

    auto remove(rxComm& _comm, const path& _p, extended_remove_options _opts) -> bool
    {
        return remove_impl(_comm, _p, _opts);
    }

    auto remove_all(rxComm& _comm, const path& _p, remove_options _opts) -> std::uintmax_t
    {
        std::string data_obj_sql = "select count(DATA_ID) where COLL_NAME like '";
        data_obj_sql += _p;
        data_obj_sql += "%'";

        std::string colls_sql = "select count(COLL_ID) where COLL_NAME like '";
        colls_sql += _p;
        colls_sql += "%'";

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        std::uintmax_t count = 0;

        for (const auto& sql : {data_obj_sql, colls_sql}) {
            for (const auto& row : qb.build(_comm, sql)) {
                count += std::stoull(row[0]);
            }
        }

        if (0 == count) {
            return 0;
        }

        extended_remove_options opts{};
        opts.no_trash = (remove_options::no_trash == _opts);
        opts.recursive = true;

        if (remove_impl(_comm, _p, opts)) {
            return count;
        }

        return 0;
    }

    auto remove_all(rxComm& _comm, const path& _p, extended_remove_options _opts) -> std::uintmax_t
    {
        std::string data_obj_sql = "select count(DATA_ID) where COLL_NAME like '";
        data_obj_sql += _p;
        data_obj_sql += "%'";

        std::string colls_sql = "select count(COLL_ID) where COLL_NAME like '";
        colls_sql += _p;
        colls_sql += "%'";

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        std::uintmax_t count = 0;

        for (const auto& sql : {data_obj_sql, colls_sql}) {
            for (const auto& row : qb.build(_comm, sql)) {
                count += std::stoull(row[0]);
            }
        }

        if (0 == count) {
            return 0;
        }

        _opts.recursive = true;

        if (remove_impl(_comm, _p, _opts)) {
            return count;
        }

        return 0;
    }

    auto permissions(rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void
    {
        constexpr auto add_admin_flag = false;
        set_permissions(add_admin_flag, _comm, _p, _user_or_group, _prms);
    }

    auto permissions(admin_tag, rxComm& _comm, const path& _p, const std::string& _user_or_group, perms _prms) -> void
    {
        constexpr auto add_admin_flag = true;
        set_permissions(add_admin_flag, _comm, _p, _user_or_group, _prms);
    }

    auto enable_inheritance(rxComm& _comm, const path& _p, bool _value) -> void
    {
        constexpr auto add_admin_flag = false;
        set_inheritance(add_admin_flag, _comm, _p, _value);
    }

    auto enable_inheritance(admin_tag, rxComm& _comm, const path& _p, bool _value) -> void
    {
        constexpr auto add_admin_flag = true;
        set_inheritance(add_admin_flag, _comm, _p, _value);
    }

    auto rename(rxComm& _comm, const path& _old_p, const path& _new_p) -> void
    {
        filesystem::detail::throw_if_path_is_empty(_old_p);
        filesystem::detail::throw_if_path_is_empty(_new_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_old_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_new_p);

        const auto old_p_stat = status(_comm, _old_p.lexically_normal());
        const auto new_p_stat = status(_comm, _new_p.lexically_normal());

        // Case 1: "_new_p" is the same object as "_old_p".
        if (exists(old_p_stat) && exists(new_p_stat) && equivalent(_comm, _old_p, _new_p)) {
            return;
        }

        if (has_prefix(_new_p, _old_p)) {
            throw filesystem_error{"old path cannot be an ancestor of the new path", _old_p, _new_p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }

        {
            const char* dot = ".";
            const char* dot_dot = "..";

            if (_new_p.object_name() == dot || _new_p.object_name() == dot_dot) {
                throw filesystem_error{R"_(path cannot end with "." or "..")_", _new_p, make_error_code(SYS_INVALID_INPUT_PARAM)};
            }
        }

        if (is_data_object(old_p_stat)) {
            // Case 2: "_new_p" is an existing non-collection object.
            if (exists(new_p_stat)) {
                if (!is_data_object(new_p_stat)) {
                    throw filesystem_error{"path is not a data object", _new_p, make_error_code(INVALID_OBJECT_TYPE)};
                }
            }
            // Case 3: "_new_p" is a non-existing data object in an existing collection.
            else if (!exists(_comm, _new_p.parent_path())) {
                throw filesystem_error{"path does not exist", _new_p.parent_path(), make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
            }
        }
        else if (is_collection(old_p_stat)) {
            // Case 2: "_new_p" is an existing collection.
            if (exists(new_p_stat)) {
                if (!is_collection(new_p_stat)) {
                    throw filesystem_error{"path is not a collection", _new_p, make_error_code(INVALID_OBJECT_TYPE)};
                }
            }
            // Case 3: "_new_p" is a non-existing collection w/ the following requirements:
            //  1. Does not end with a collection separator.
            //  2. The parent collection must exist.
            else if (filesystem::detail::is_separator(_new_p.string().back())) {
                throw filesystem_error{"path cannot end with a separator", _new_p, make_error_code(SYS_INVALID_INPUT_PARAM)};
            }
            else if (!is_collection(_comm, _new_p.parent_path())) {
                throw filesystem_error{"path does not exist", _new_p.parent_path(), make_error_code(OBJ_PATH_DOES_NOT_EXIST)};
            }
        }
        else {
            throw filesystem_error{"cannot rename: unknown object type", _new_p, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        dataObjCopyInp_t input{};
        std::strncpy(input.srcDataObjInp.objPath, _old_p.c_str(), std::strlen(_old_p.c_str()));
        std::strncpy(input.destDataObjInp.objPath, _new_p.c_str(), std::strlen(_new_p.c_str()));

        if (const auto ec = rxDataObjRename(&_comm, &input); ec < 0) {
            throw filesystem_error{"cannot rename object", _old_p, _new_p, make_error_code(ec)};
        }
    }

    auto move(rxComm& _comm, const path& _old_p, const path& _new_p) -> void
    {
        rename(_comm, _old_p, _new_p);
    }

    auto data_object_checksum(rxComm& _comm, const path& _p) -> std::string
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        if (!is_data_object(_comm, _p)) {
            throw filesystem_error{"path does not point to a data object", _p, make_error_code(SYS_INVALID_INPUT_PARAM)};
        }

        // Fetch information for good replicas only (i.e. DATA_REPL_STATUS = '1').
        const auto gql = fmt::format("select DATA_CHECKSUM, DATA_MODIFY_TIME "
                                     "where"
                                     " COLL_NAME = '{}' and"
                                     " DATA_NAME = '{}' and"
                                     " DATA_REPL_STATUS = '1'",
                                     _p.parent_path().c_str(),
                                     _p.object_name().c_str());

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        std::uintmax_t latest_mtime = 0;
        std::string checksum;

        // This implementation assumes that any good replica will always satisfy
        // the requirement within the loop, therefore the first iteration always causes
        // the checksum to be captured. The checksum object should be empty if and only if there
        // are no good replicas.
        for (const auto& row : qb.build(_comm, gql)) {
            // As we iterate over the replicas, compare the mtimes and capture the checksum
            // of the latest good replica.
            if (const auto current_mtime = std::stoull(row[1]); current_mtime > latest_mtime) {
                latest_mtime = current_mtime;
                checksum = row[0];
            }
        }

        return checksum;
    }

    auto status(rxComm& _comm, const path& _p) -> object_status
    {
        const auto s = stat(_comm, _p);

        if (s.error < 0) {
            throw filesystem_error{"cannot get status", _p, make_error_code(s.error)};
        }

        object_status status;

        status.permissions(s.prms);
        status.inheritance(s.inheritance);

        // XXX This does not handle the case of object_type::unknown.
        // This type means a file exists, but the type is unknown.
        // Maybe this case is not possible in iRODS.
        switch (s.type) {
            case DATA_OBJ_T:
                status.type(object_type::data_object);
                break;

            case COLL_OBJ_T:
                status.type(object_type::collection);
                break;

            // This case indicates that iRODS does not contain a data object or
            // collection at path "_p".
            case UNKNOWN_OBJ_T:
                status.type(object_type::not_found);
                break;

            /*
            case ?:
                status.type(object_type::unknown);
                break;
            */

            default:
                status.type(object_type::none);
                break;
        }

        return status;
    }

    auto status_known(const object_status& _s) noexcept -> bool
    {
        return _s.type() != object_type::none;
    }

    auto get_metadata(rxComm& _comm, const path& _p) -> std::vector<metadata>
    {
        filesystem::detail::throw_if_path_is_empty(_p);
        filesystem::detail::throw_if_path_length_exceeds_limit(_p);

        std::string sql;

        if (const auto s = status(_comm, _p); is_data_object(s)) {
            sql = "select META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS where DATA_NAME = '";
            sql += _p.object_name();
            sql += "' and COLL_NAME = '";
            sql += _p.parent_path();
            sql += "'";
        }
        else if (is_collection(s)) {
            sql = "select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS where COLL_NAME = '";
            sql += _p;
            sql += "'";
        }
        else {
            throw filesystem_error{"cannot get metadata: unknown object type", _p, make_error_code(CAT_NOT_A_DATAOBJ_AND_NOT_A_COLLECTION)};
        }

        irods::experimental::query_builder qb;

        if (const auto zone = zone_name(_p); zone) {
            qb.zone_hint(*zone);
        }

        std::vector<metadata> results;

        for (const auto& row : qb.build(_comm, sql)) {
            results.push_back({row[0], row[1], row[2]});
        }

        return results;
    }

    auto set_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = false;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "set");
    }

    auto set_metadata(admin_tag, rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = true;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "set");
    }

    auto add_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = false;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "add");
    }

    auto add_metadata(admin_tag, rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = true;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "add");
    }

    auto remove_metadata(rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = false;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "rm");
    }

    auto remove_metadata(admin_tag, rxComm& _comm, const path& _p, const metadata& _metadata) -> void
    {
        constexpr auto add_admin_flag = true;
        do_metadata_op(add_admin_flag, _comm, _p, _metadata, "rm");
    }
} // namespace irods::experimental::filesystem::NAMESPACE_IMPL

