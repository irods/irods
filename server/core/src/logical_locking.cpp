#include "logical_locking.hpp"

#include "irods_log.hpp"
#include "replica_access_table.hpp"
#include "replica_state_table.hpp"
#include "scoped_privileged_client.hpp"

namespace
{
    namespace id  = irods::experimental::data_object;
    namespace ir  = irods::experimental::replica;
    namespace ill = irods::logical_locking;
    namespace rat = irods::experimental::replica_access_table;
    namespace rst = irods::replica_state_table;
    using json = nlohmann::json;

    static const std::map<ill::lock_type, repl_status_t> lock_type_to_repl_status_map
    {
        {ill::lock_type::read,  READ_LOCKED},
        {ill::lock_type::write, WRITE_LOCKED}
    };

    auto get_original_replica_status_impl(const json& _json_replica) -> int
    {
        const auto data_status_str = _json_replica.at("after").at("data_status").get<std::string>();

        if (data_status_str.empty()) {
            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - data_status empty",
                __FUNCTION__, __LINE__));

            return -1;
        }

        irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - data_status:[{}]",
            __FUNCTION__, __LINE__, data_status_str));

        try {
            const auto data_status_json = json::parse(data_status_str);

            if (!data_status_json.empty() && data_status_json.contains("original_status")) {
                return std::stoi(data_status_json.at("original_status").get<std::string>());
            }

            return -1;
        }
        catch (const json::exception& e) {
            irods::log(LOG_WARNING, fmt::format(
                "[{}:{}] - failed to parse data_status value [{}]",
                __FUNCTION__, __LINE__, data_status_str));

            throw;
        }
    } // get_original_replica_status_impl

    auto insert_data_status(const std::uint64_t _data_id) -> void
    {
        auto entry = rst::at(_data_id);

        const auto logical_path = rst::get_logical_path(_data_id);

        for (auto& json_replica : entry) {
            const auto replica_number = std::stoi(json_replica.at("before").at("data_repl_num").get<std::string>());

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - replica:[{}]",
                __FUNCTION__, __LINE__, json_replica.dump()));

            if (const auto original_status = get_original_replica_status_impl(json_replica); -1 != original_status) {
                irods::log(LOG_DEBUG, fmt::format(
                    "[{}:{}] - existing data_status entry; "
                    "[data_id:[{}], repl_num:[{}], status:[{}]]",
                    __FUNCTION__, __LINE__, _data_id, replica_number, original_status));

                continue;
            }

            const auto old_status = rst::get_property(_data_id, replica_number, "data_is_dirty");

            const auto original_status = json{{"original_status", old_status}};
            const auto update = json{{"data_status", original_status.dump()}};

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - update:[{}]",
                __FUNCTION__, __LINE__, update.dump()));

            rst::update(_data_id, replica_number, update);
        }
    } // insert_data_status

    auto remove_data_status(const std::uint64_t _data_id) -> void
    {
        for (const auto& json_replica : rst::at(_data_id)) {
            const auto replica_number = std::stoi(json_replica.at("before").at("data_repl_num").get<std::string>());
            rst::update(_data_id, replica_number, json{{"data_status", ""}});
        }
    } // remove_data_status

    auto restore_replica_statuses(
        const std::uint64_t _data_id,
        const int           _replica_number) -> void
    {
        auto entry = rst::at(_data_id);

        const auto logical_path = rst::get_logical_path(_data_id);

        for (auto& json_replica : entry) {
            const auto replica_number = std::stoi(json_replica.at("before").at("data_repl_num").get<std::string>());

            if (_replica_number == replica_number) {
                continue;
            }

            const auto original_status = get_original_replica_status_impl(json_replica);

            if (-1 == original_status) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - failed to restore status for replica; "
                    "original status not found in data_status column "
                    "[data_id=[{}], repl_num=[{}]]",
                    __FUNCTION__, __LINE__, _data_id, replica_number));

                continue;
            }

            rst::update(_data_id, replica_number, json{{"data_is_dirty", std::to_string(original_status)}});
        }
    } // restore_replica_statuses

    auto set_replica_statuses(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const repl_status_t _replica_status) -> void
    {
        auto entry = rst::at(_data_id);

        for (auto& json_replica : entry) {
            const auto replica_number = std::stoi(json_replica.at("before").at("data_repl_num").get<std::string>());

            irods::log(LOG_DEBUG, fmt::format(
                "[{}:{}] - data_id:[{}], repl_num:[{}], status:[{}], target repl_num:[{}]",
                __FUNCTION__, __LINE__,
                _data_id, replica_number,
                rst::get_property(_data_id, replica_number, "data_status"),
                _replica_number));

            if (_replica_number != replica_number) {
                rst::update(_data_id, replica_number,
                    json{{"data_is_dirty", std::to_string(_replica_status)}});
            }
        }
    } // set_replica_statuses

    auto lock_impl(
        const std::uint64_t  _data_id,
        const int            _replica_number,
        const ill::lock_type _lock_type) -> int
    {
        try {
            insert_data_status(_data_id);

            const auto lock_status = lock_type_to_repl_status_map.at(_lock_type);

            // The target replica should have its state set as well, but in a special manner
            if (rst::unknown_replica_id != _replica_number) {
                if (ill::lock_type::write == _lock_type) {
                    // A replica opened for write should be set to intermediate
                    rst::update(_data_id, _replica_number,
                        json{{"data_is_dirty", std::to_string(INTERMEDIATE_REPLICA)}});
                }
                else {
                    // Even the target replica enters read lock state, so set that here
                    rst::update(_data_id, _replica_number,
                        json{{"data_is_dirty", std::to_string(lock_status)}});
                }
            }

            set_replica_statuses(_data_id, _replica_number, lock_status);

            return 0;
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
            return SYS_UNKNOWN_ERROR;
        }
    } // lock_impl

    auto unlock_impl(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const int           _replica_status,
        const int           _other_replica_statuses) -> int
    {
        try {
            if (rst::unknown_replica_id != _replica_number) {
                auto replica_status = _replica_status;

                if (ill::restore_status == _replica_status) {
                    if (const auto original_status = ill::get_original_replica_status(_data_id, _replica_number); -1 == original_status) {
                        irods::log(LOG_WARNING, fmt::format(
                            "[{}:{}] - failed to restore status for replica "
                            "because no original status found in data_status column. "
                            "Setting replica status to stale. "
                            "[data_id=[{}], repl_num=[{}]]",
                            __FUNCTION__, __LINE__, _data_id, _replica_number));

                        replica_status = STALE_REPLICA;
                    }
                    else {
                        replica_status = original_status;
                    }
                }

                rst::update(_data_id, _replica_number,
                    json{{"data_is_dirty", std::to_string(replica_status)}});
            }

            if (ill::restore_status == _other_replica_statuses) {
                restore_replica_statuses(_data_id, _replica_number);
            }
            else {
                set_replica_statuses(_data_id, _replica_number, _other_replica_statuses);
            }

            remove_data_status(_data_id);

            return 0;
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
            return e.code();
        }
        catch (const std::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
            return SYS_INTERNAL_ERR;
        }
        catch (...) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
            return SYS_UNKNOWN_ERROR;
        }
    } // unlock_impl

    auto try_lock_for_intermediate_replica_access(
        const ir::replica_proxy<const DataObjInfo>& _r,
        const std::string_view _target_replica_token) -> int
    {
        // If the catalog information indicates that the selected replica is intermediate, check
        // to see if the provided replica token will be accepted by the replica access table.
        // If not, the open request is disallowed because multiple opens of the same replica are
        // not allowed without a valid replica token.
        const auto replica_access_granted = [&_r, &_target_replica_token]() -> bool
        {
            if (_r.at_rest()) {
                return true;
            }

            if (_target_replica_token.empty()) {
                return false;
            }

            return rat::contains(_target_replica_token.data(), _r.data_id(), _r.replica_number());
        }();

        if (!replica_access_granted) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - selected replica is in intermediate state. "
                "[path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, _r.logical_path(), _r.hierarchy()));

            return INTERMEDIATE_REPLICA_ACCESS;
        }

        return 0;
    } // try_lock_for_intermediate_replica_access

    auto try_lock_for_locked_data_object(
        const id::data_object_proxy<const DataObjInfo>& _obj,
        const ill::lock_type _lock_type,
        const bool _check_intermediate_replicas = true) -> int
    {
        for (const auto& r : _obj.replicas()) {
            // If any replica is locked, opening is not allowed.
            switch (r.replica_status()) {
                // TODO: Read locks
                case INTERMEDIATE_REPLICA:
                    if (!_check_intermediate_replicas) {
                        continue;
                    }
                    [[fallthrough]];

                case WRITE_LOCKED:
                    irods::log(LOG_NOTICE, fmt::format(
                        "[{}:{}] - data object is locked. "
                        "[path=[{}], hierarchy=[{}]]",
                        __FUNCTION__, __LINE__, r.logical_path(), r.hierarchy()));

                    return LOCKED_DATA_OBJECT_ACCESS;

                default:
                    break;
            }
        }

        return 0;
    } // try_lock_for_locked_data_object

    auto try_lock_for_specific_replica(const ir::replica_proxy<const DataObjInfo>& _r,
                                       const std::string_view _target_replica_token) -> int
    {
        // If the replica is found, check to see if the target replica is in the intermediate status and,
        // if so, whether the provided replica token is valid for the given replica.
        if (const auto ec = try_lock_for_intermediate_replica_access(_r, _target_replica_token); ec < 0) {
            return ec;
        }

        // This replica is either at rest, or it is intermediate and a valid replica token is present.
        // Now make sure that the replica is not locked, which is a subset of "at rest" and indicates
        // that it is a sibling to an intermediate replica. It should not be opened for any reason.
        if (_r.locked()) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - data object is locked. "
                "[path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, _r.logical_path(), _r.hierarchy()));

            return LOCKED_DATA_OBJECT_ACCESS;
        }

        return 0;
    } // try_lock_for_specific_replica
} // anonymous namespace

namespace irods::logical_locking
{
    auto get_original_replica_status(
        const std::uint64_t _data_id,
        const int           _replica_number) -> int
    {
        return get_original_replica_status_impl(rst::at(_data_id, _replica_number));
    } // get_original_replica_status

    auto lock(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const lock_type     _lock_type) -> int
    {
        return lock_impl(_data_id, _replica_number, _lock_type);
    } // lock

    auto unlock(
        const std::uint64_t _data_id,
        const int           _replica_number,
        const int           _replica_status,
        const int           _other_replica_statuses) -> int
    {
        return unlock_impl(_data_id, _replica_number, _replica_status, _other_replica_statuses);
    } // unlock

    auto lock_and_publish(
        RsComm&                _comm,
        const rst::publish::context& _ctx,
        const lock_type        _lock_type) -> int
    {
        // TODO: std::visit with overloaded impls
        if (const int ec = lock_impl(_ctx.key, std::get<const int>(_ctx.replica_id), _lock_type); ec < 0) {
            return ec;
        }

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, _ctx); ec < 0) {
            return ec;
        }

        return 0;
    } // lock_and_publish

    auto unlock_and_publish(
        RsComm&             _comm,
        const rst::publish::context& _ctx,
        const int           _replica_status,
        const int           _other_replica_statuses) -> int
    {
        // TODO: std::visit with overloaded impls
        if (const int ec = unlock_impl(_ctx.key, std::get<const int>(_ctx.replica_id), _replica_status, _other_replica_statuses); ec < 0) {
            return ec;
        }

        if (const auto [ret, ec] = rst::publish::to_catalog(_comm, _ctx); ec < 0) {
            return ec;
        }

        return 0;
    } // unlock_and_publish

    auto try_lock(const DataObjInfo& _obj, const lock_type _lock_type) -> int
    {
        const auto obj = id::make_data_object_proxy(_obj);

        return try_lock_for_locked_data_object(obj, _lock_type);
    } // try_lock

    auto try_lock(
        const DataObjInfo&     _obj,
        const lock_type        _lock_type,
        const std::string_view _target_replica_hierarchy,
        const std::string_view _target_replica_token) -> int
    {
        const auto obj = id::make_data_object_proxy(_obj);

        const auto maybe_replica = id::find_replica(obj, _target_replica_hierarchy);

        // If the replica was not found, check to see whether any of the existing replicas are locked or
        // in the intermediate status.
        if (!maybe_replica) {
            return try_lock_for_locked_data_object(obj, _lock_type);
        }

        return try_lock_for_specific_replica(*maybe_replica, _target_replica_token);
    } // try_lock

    auto try_lock(
        const DataObjInfo&     _obj,
        const lock_type        _lock_type,
        const int              _target_replica_number,
        const std::string_view _target_replica_token) -> int
    {
        const auto obj = id::make_data_object_proxy(_obj);

        irods::log(LOG_DEBUG, fmt::format(
            "[{}:{}] - path=[{}], replica_number=[{}], replica count=[{}]",
            __FUNCTION__, __LINE__, obj.logical_path(), _target_replica_number, obj.replica_count()));

        const auto maybe_replica = id::find_replica(obj, _target_replica_number);

        // If the replica was not found, check to see whether any of the existing replicas are locked or
        // in the intermediate status.
        if (!maybe_replica) {
            return try_lock_for_locked_data_object(obj, _lock_type);
        }

        return try_lock_for_specific_replica(*maybe_replica, _target_replica_token);
    } // try_lock
} // namespace irods::logical_locking
