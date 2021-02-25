#include "irods_at_scope_exit.hpp"
#include "irods_resource_manager.hpp"
#include "replica_state_table.hpp"
#include "rs_data_object_finalize.hpp"

//#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
//#include "data_object_proxy.hpp"

#include "fmt/format.h"

#include <mutex>

extern irods::resource_manager resc_mgr;

namespace irods::replica_state_table
{
    namespace
    {
        // clang-format off
        namespace id     = irods::experimental::data_object;
        namespace ir     = irods::experimental::replica;
        using json       = nlohmann::json;
        // clang-format on

        // Global Variables
        json replica_state_json_map;

        std::mutex rst_mutex;

        auto index_of(const std::string_view _logical_path, const int _replica_number) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (replica_state_json_map.contains(_logical_path.data())) {
                int index = -1;

                for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
                    ++index;
                    if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - replica number [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _logical_path));
        } // index_of

        auto index_of(
            const std::string_view _logical_path,
            const rodsLong_t _leaf_resource_id) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (replica_state_json_map.contains(_logical_path.data())) {
                int index = -1;

                for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
                    ++index;
                    if (_leaf_resource_id == std::stoi(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - resource id [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _leaf_resource_id, _logical_path));
        } // index_of

        auto index_of(
            const std::string_view _logical_path,
            const std::string_view _leaf_resource_name) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (replica_state_json_map.contains(_logical_path.data())) {
                int index = -1;

                const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

                for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
                    ++index;
                    if (resc_id == std::stoi(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - resource name [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _leaf_resource_name, _logical_path));
        } // index_of

        // convenience function for accessing the ref_count
        // NOTE: no lock acquisition
        auto get_ref_count(const std::string_view _logical_path) -> int
        {
            return replica_state_json_map.at(_logical_path.data()).at("ref_count");
        } // get_ref_count

        auto update_impl(
            const std::string_view _logical_path,
            const int _replica_index,
            const json& _updates) -> void
        {
            try {
                std::scoped_lock rst_lock{rst_mutex};

                auto& target_replica = replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW).at(_replica_index);

                irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, target_replica.dump()));

                if (_updates.contains(REPLICAS_KW)) {
                    target_replica.at(AFTER_KW).update(_updates.at(REPLICAS_KW));
                }

                if (_updates.contains(FILE_MODIFIED_KW)) {
                    irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - file_modified:[{}]", __FUNCTION__, __LINE__, _updates.at(FILE_MODIFIED_KW).dump()));
                    //target_replica[FILE_MODIFIED_KW].update(_updates.at(FILE_MODIFIED_KW));
                    target_replica.erase(FILE_MODIFIED_KW);
                    target_replica[FILE_MODIFIED_KW] = _updates.at(FILE_MODIFIED_KW);
                }
            }
            catch (const json::exception& e) {
                THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
            }
        } // update_impl
    } // anonymouse namespace

    auto init() -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - initializing state table", __FUNCTION__, __LINE__));

        replica_state_json_map.clear();
    } // init

    auto deinit() -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - de-initializing state table", __FUNCTION__, __LINE__));

        replica_state_json_map.clear();
    } // deinit

    auto insert(const id::data_object_proxy_t& _obj) -> void
    {
        try {
            std::scoped_lock rst_lock{rst_mutex};

            if (replica_state_json_map.contains(_obj.logical_path().data())) {
                irods::log(LOG_DEBUG, fmt::format("[{}:{}] - entry exists;path:[{}]", __FUNCTION__, __LINE__, _obj.logical_path()));

                replica_state_json_map.at(_obj.logical_path().data())["ref_count"] = get_ref_count(_obj.logical_path()) + 1;

                return;
            }

            json replica_list;

            for (const auto& r : _obj.replicas()) {
                const auto json_replica = ir::to_json(r);

                replica_list.push_back({
                    {BEFORE_KW, json_replica},
                    {AFTER_KW, json_replica}
                });
            }

            const json entry{{REPLICAS_KW, replica_list}, {"ref_count", 1}};

            irods::log(LOG_DEBUG9, fmt::format("entry:[{}]", entry.dump()));

            replica_state_json_map[_obj.logical_path().data()] = entry;
        }
        catch (const json::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
        }
    } // insert

    auto insert(
        const std::string_view _logical_path,
        const ir::replica_proxy_t& _replica) -> void
    {
        try {
            std::scoped_lock rst_lock{rst_mutex};

            if (!replica_state_json_map.contains(_logical_path.data())) {
                const auto obj = id::make_data_object_proxy(*_replica.get());
                return insert(obj);
            }

            const auto json_replica = ir::to_json(_replica);

            auto& entry = replica_state_json_map.at(_logical_path.data());
            entry["ref_count"] = get_ref_count(_logical_path) + 1;
            entry.at(REPLICAS_KW).push_back(
                {
                    {BEFORE_KW, json_replica},
                    {AFTER_KW, json_replica}
                }
            );
        }
        catch (const json::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
        }
    } // insert

    auto erase(const std::string_view _logical_path) -> void
    {
        if (!contains(_logical_path)) {
            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no key found for [{}]",
                __FUNCTION__, __LINE__, _logical_path));
        }

        std::scoped_lock rst_lock{rst_mutex};

        if (const auto ref_count = get_ref_count(_logical_path); ref_count > 1) {
            replica_state_json_map.at(_logical_path.data())["ref_count"] = ref_count - 1;
        }
        else {
            replica_state_json_map.erase(_logical_path.data());
        }
    } // erase

    auto contains(const std::string_view _logical_path) -> bool
    {
        std::scoped_lock rst_lock{rst_mutex};

        return replica_state_json_map.contains(_logical_path.data());
    } // contains

    auto contains(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name) -> bool
    {
        if (!contains(_logical_path)) {
            return false;
        }

        const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

        std::scoped_lock rst_lock{rst_mutex};

        for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
            if (resc_id == std::stoll(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                return true;
            }
        }

        return false;
    } // contains

    auto contains(
        const std::string_view _logical_path,
        const int _replica_number) -> bool
    {
        if (!contains(_logical_path)) {
            return false;
        }

        std::scoped_lock rst_lock{rst_mutex};

        for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
            if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                return true;
            }
        }

        return false;
    } // contains

    auto at(const std::string_view _logical_path) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        return replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW);
    } // at

    auto at(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name,
        const state_type _state) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

        const auto& replica_json = [&_logical_path, &resc_id]
        {
            for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
                if (resc_id == std::stoll(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no resc id [{}] found for [{}]",
                __FUNCTION__, __LINE__, resc_id, _logical_path));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at(BEFORE_KW);   break;
            case state_type::after:     return replica_json.at(AFTER_KW);    break;
            case state_type::both:      return replica_json;                break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type",
                    __FUNCTION__, __LINE__));
        }
    } // at

    auto at(
        const std::string_view _logical_path,
        const int _replica_number,
        const state_type _state) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto& replica_json = [&_logical_path, &_replica_number]
        {
            for (const auto& e : replica_state_json_map.at(_logical_path.data()).at(REPLICAS_KW)) {
                if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no replica number [{}] found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _logical_path));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at(BEFORE_KW);   break;
            case state_type::after:     return replica_json.at(AFTER_KW);    break;
            case state_type::both:      return replica_json;                break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type",
                    __FUNCTION__, __LINE__));
        }
    } // at

    auto update(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name,
        const json& _updates) -> void
    {
        const auto replica_index = index_of(_logical_path, _leaf_resource_name);

        return update_impl(_logical_path, replica_index, _updates);
    } // update

    auto update(
        const std::string_view _logical_path,
        const int _replica_number,
        const json& _updates) -> void
    {
        const auto replica_index = index_of(_logical_path, _replica_number);

        return update_impl(_logical_path, replica_index, _updates);
    } // update

    auto update(
        const std::string_view _logical_path,
        const ir::replica_proxy_t& _replica) -> void
    {
        const json replica_json{ir::to_json(_replica)};
        json updates{{REPLICAS_KW, replica_json}};

        if (_replica.cond_input().contains(FILE_MODIFIED_KW)) {
            updates[FILE_MODIFIED_KW] = json::parse(_replica.cond_input().at(FILE_MODIFIED_KW).value().data());
        }

        const auto replica_index = index_of(_logical_path, _replica.resource_id());

        return update_impl(_logical_path, replica_index, updates);
    } // update

    auto get_property(
        const std::string_view _logical_path,
        const int _replica_number,
        const std::string_view _property_name,
        const state_type _state) -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        const auto replica_json = at(_logical_path, _replica_number, _state);

        return replica_json.at(_property_name.data());
    } // get_property

    auto get_property(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name,
        const std::string_view _property_name,
        const state_type _state) -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        const auto replica_json = at(_logical_path, _leaf_resource_name, _state);

        return replica_json.at(_property_name.data());
    } // get_property

    auto publish_to_catalog(
        RsComm& _comm,
        const std::string_view _logical_path,
        const trigger_file_modified _trigger_file_modified) -> int
    {
        try {
            const auto input = [&]() -> json
            {
                std::scoped_lock rst_lock{rst_mutex};

                auto& target_entry = replica_state_json_map.at(_logical_path.data());

                irods::log(LOG_DEBUG9, fmt::format(
                    "[{}:{}] - target:[{}]",
                    __FUNCTION__, __LINE__, target_entry.dump()));

                return json{
                    {"data_id", target_entry.at(REPLICAS_KW).at(0).at(AFTER_KW).at("data_id")},
                    {REPLICAS_KW, target_entry.at(REPLICAS_KW)},
                    {FILE_MODIFIED_KW, trigger_file_modified::yes == _trigger_file_modified}
                };
            }();

            // Completely erase the replica state table entry -- file_modified could open other replicas
            if (trigger_file_modified::yes == _trigger_file_modified) {
                std::scoped_lock rst_lock{rst_mutex};
                replica_state_json_map.erase(_logical_path.data());
            }

            char* error_string{};
            const irods::at_scope_exit free_error_string{[&error_string] { free(error_string); }};

            const int ec = rs_data_object_finalize(&_comm, input.dump().data(), &error_string);
            if (ec < 0) {
                irods::log(LOG_ERROR, fmt::format("failed to publish replica states for [{}]", _logical_path));
            }

            return ec;
        }
        catch (const json::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
        }
        catch (const std::exception& e) {
            THROW(SYS_INTERNAL_ERR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.what()));
        }
        catch (...) {
            THROW(SYS_UNKNOWN_ERROR, fmt::format("[{}:{}] - unknown error occurred", __FUNCTION__, __LINE__));
        }
    } // publish_to_catalog
} // namespace irods

