#include "json_serialization.hpp"
#include "irods_at_scope_exit.hpp"
#include "irods_resource_manager.hpp"
#include "replica_state_table.hpp"
#include "rs_data_object_finalize.hpp"

//#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
//#include "data_object_proxy.hpp"

#include "fmt/format.h"

#include <map>
#include <mutex>

extern irods::resource_manager resc_mgr;

namespace irods::replica_state_table
{
    namespace
    {
        // clang-format off
        namespace id = irods::experimental::data_object;
        namespace ir = irods::experimental::replica;
        using json   = nlohmann::json;
        // clang-format on

        // Global Constants
        static const std::string BEFORE_KW = "before";
        static const std::string AFTER_KW = "after";
        static const std::string REPLICAS_KW = "replicas";

        // Global Variables
        std::map<key_type, json> replica_state_json_map;

        std::mutex rst_mutex;

        // Local functions
        // extracts the key information from the replica_proxy and returns
        // the string-ified key for use with the JSON map.
        auto get_key(const ir::replica_proxy_t& _r) -> key_type
        {
            return _r.data_id();
        } // get_key

        // extracts the key information from the data_object_proxy and returns
        // the string-ified key for use with the JSON map.
        auto get_key(const id::data_object_proxy_t& _o) -> key_type
        {
            return _o.data_id();
        } // get_key

        // convenience function for accessing the ref_count
        // NOTE: no lock acquisition
        auto get_ref_count(const key_type& _key) -> int
        {
            return replica_state_json_map.at(_key).at("ref_count");
        } // get_ref_count

        auto index_of(const key_type& _key, const int _replica_number) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (std::end(replica_state_json_map) != replica_state_json_map.find(_key)) {
                int index = -1;

                for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
                    ++index;
                    if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - replica number [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _key));
        } // index_of

        auto index_of(const key_type& _key, const rodsLong_t _leaf_resource_id) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (std::end(replica_state_json_map) != replica_state_json_map.find(_key)) {
                int index = -1;

                for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
                    ++index;
                    if (_leaf_resource_id == std::stoi(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - resource id [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _leaf_resource_id, _key));
        } // index_of

        auto index_of(const key_type& _key, const std::string_view _leaf_resource_name) -> int
        {
            std::scoped_lock rst_lock{rst_mutex};

            if (std::end(replica_state_json_map) != replica_state_json_map.find(_key)) {
                int index = -1;

                const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

                for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
                    ++index;
                    if (resc_id == std::stoi(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                        return index;
                    }
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - resource name [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _leaf_resource_name, _key));
        } // index_of

        auto update_impl(
            const key_type& _key,
            const int _replica_index,
            const json& _updates) -> void
        {
            try {
                std::scoped_lock rst_lock{rst_mutex};

                auto& target_replica = replica_state_json_map.at(_key).at(REPLICAS_KW).at(_replica_index);

                irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, target_replica.dump()));

                target_replica.at(AFTER_KW).update(_updates);
            }
            catch (const json::exception& e) {
                THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
            }
        } // update_impl

        auto publish_to_catalog_impl(
            RsComm& _comm,
            const key_type& _key,
            const int _replica_index,
            const json& _file_modified_parameters) -> int
        {
            const bool trigger_file_modified = !_file_modified_parameters.empty();

            const auto input = [&]() -> json
            {
                std::scoped_lock rst_lock{rst_mutex};

                auto& target_entry = replica_state_json_map.at(_key);

                if (trigger_file_modified) {
                    target_entry.at(REPLICAS_KW).at(_replica_index)[FILE_MODIFIED_KW] = _file_modified_parameters;
                }

                return json{
                    {REPLICAS_KW, target_entry.at(REPLICAS_KW)},
                    {"trigger_file_modified", trigger_file_modified}
                };
            }();

            // Completely erase the replica state table entry -- file_modified could open other replicas
            if (trigger_file_modified) {
                std::scoped_lock rst_lock{rst_mutex};

                replica_state_json_map.erase(_key);
            }

            char* error_string{};
            const irods::at_scope_exit free_error_string{[&error_string] { free(error_string); }};

            const int ec = rs_data_object_finalize(&_comm, input.dump().data(), &error_string);

            if (ec < 0) {
                irods::log(LOG_ERROR, fmt::format("failed to publish replica states for [{}]", _key));
            }

            return ec;
        } // publish_to_catalog_impl
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
        const auto& key = get_key(_obj);

        try {
            std::scoped_lock rst_lock{rst_mutex};

            if (std::end(replica_state_json_map) != replica_state_json_map.find(key)) {
                irods::log(LOG_DEBUG, fmt::format("[{}:{}] - entry exists;path:[{}]", __FUNCTION__, __LINE__, _obj.logical_path()));

                replica_state_json_map.at(key)["ref_count"] = get_ref_count(key) + 1;

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

            irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - entry:[{}]", __FUNCTION__, __LINE__, entry.dump()));

            replica_state_json_map[key] = entry;
        }
        catch (const json::exception& e) {
            THROW(SYS_LIBRARY_ERROR, fmt::format("[{}:{}] - JSON error:[{}]", __FUNCTION__, __LINE__, e.what()));
        }
    } // insert

    auto insert(const ir::replica_proxy_t& _replica) -> void
    {
        try {
            if (!contains(_replica.data_id())) {
                const auto obj = id::make_data_object_proxy(*_replica.get());
                return insert(obj);
            }

            const auto& key = get_key(_replica);

            std::scoped_lock rst_lock{rst_mutex};

            const auto json_replica = ir::to_json(_replica);

            auto& entry = replica_state_json_map.at(key);
            entry["ref_count"] = get_ref_count(key) + 1;
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

    auto erase(const key_type& _key) -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        if (std::end(replica_state_json_map) == replica_state_json_map.find(_key)) {
            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no key found for [{}]",
                __FUNCTION__, __LINE__, _key));
        }

        if (const auto ref_count = get_ref_count(_key); ref_count > 1) {
            replica_state_json_map.at(_key)["ref_count"] = ref_count - 1;
        }
        else {
            replica_state_json_map.erase(_key);
        }
    } // erase

    auto contains(const key_type& _key) -> bool
    {
        std::scoped_lock rst_lock{rst_mutex};

        return std::end(replica_state_json_map) != replica_state_json_map.find(_key);
    } // contains

    auto contains(const key_type& _key, const std::string_view _leaf_resource_name) -> bool
    {
        if (!contains(_key)) {
            return false;
        }

        const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

        std::scoped_lock rst_lock{rst_mutex};

        for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
            if (resc_id == std::stoll(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                return true;
            }
        }

        return false;
    } // contains

    auto contains(const key_type& _key, const int _replica_number) -> bool
    {
        if (!contains(_key)) {
            return false;
        }

        std::scoped_lock rst_lock{rst_mutex};

        for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
            if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                return true;
            }
        }

        return false;
    } // contains

    auto at(const key_type& _key) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        if (std::end(replica_state_json_map) == replica_state_json_map.find(_key)) {
            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no key found for [{}]",
                __FUNCTION__, __LINE__, _key));
        }

        return replica_state_json_map.at(_key).at(REPLICAS_KW);
    } // at

    auto at(
        const key_type& _key,
        const std::string_view _leaf_resource_name,
        const state_type _state) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

        const auto& replica_json = [&_key, &resc_id]
        {
            for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
                if (resc_id == std::stoll(e.at(BEFORE_KW).at("resc_id").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no resc id [{}] found for [{}]",
                __FUNCTION__, __LINE__, resc_id, _key));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at(BEFORE_KW);   break;
            case state_type::after:     return replica_json.at(AFTER_KW);    break;
            case state_type::both:      return replica_json;                 break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type",
                    __FUNCTION__, __LINE__));
        }
    } // at

    auto at(
        const key_type& _key,
        const int _replica_number,
        const state_type _state) -> json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto& replica_json = [&_key, &_replica_number]
        {
            for (const auto& e : replica_state_json_map.at(_key).at(REPLICAS_KW)) {
                if (_replica_number == std::stoi(e.at(BEFORE_KW).at("data_repl_num").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no replica number [{}] found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _key));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at(BEFORE_KW);   break;
            case state_type::after:     return replica_json.at(AFTER_KW);    break;
            case state_type::both:      return replica_json;                 break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type",
                    __FUNCTION__, __LINE__));
        }
    } // at

    auto update(
        const key_type& _key,
        const std::string_view _leaf_resource_name,
        const json& _updates) -> void
    {
        const auto replica_index = index_of(_key, _leaf_resource_name);

        return update_impl(_key, replica_index, _updates);
    } // update

    auto update(
        const key_type& _key,
        const int _replica_number,
        const json& _updates) -> void
    {
        const auto replica_index = index_of(_key, _replica_number);

        return update_impl(_key, replica_index, _updates);
    } // update

    auto update(
        const key_type& _key,
        const ir::replica_proxy_t& _replica) -> void
    {

        const auto replica_index = index_of(_key, _replica.resource_id());

        return update_impl(_key, replica_index, ir::to_json(_replica));
    } // update

    auto get_property(
        const key_type& _key,
        const int _replica_number,
        const std::string_view _property_name,
        const state_type _state) -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        const auto replica_json = at(_key, _replica_number, _state);

        return replica_json.at(_property_name.data());
    } // get_property

    auto get_property(
        const key_type& _key,
        const std::string_view _leaf_resource_name,
        const std::string_view _property_name,
        const state_type _state) -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        const auto replica_json = at(_key, _leaf_resource_name, _state);

        return replica_json.at(_property_name.data());
    } // get_property

    auto publish_to_catalog(
        RsComm& _comm,
        const key_type& _key,
        const int _replica_number_for_file_modified,
        const json& _file_modified_parameters) -> int
    {
        try {
            const auto replica_index = index_of(_key, _replica_number_for_file_modified);

            return publish_to_catalog_impl(_comm, _key, replica_index, _file_modified_parameters);
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

    auto publish_to_catalog(
        RsComm& _comm,
        const key_type& _key,
        const std::string_view _leaf_resource_name_for_file_modified,
        const json& _file_modified_parameters) -> int
    {
        try {
            const auto replica_index = index_of(_key, _leaf_resource_name_for_file_modified);

            return publish_to_catalog_impl(_comm, _key, replica_index, _file_modified_parameters);
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

    auto publish_to_catalog(RsComm& _comm, const ir::replica_proxy_t& _replica) -> int
    {
        try {
            const auto& key = get_key(_replica);

            const auto replica_index = index_of(key, _replica.resource_id());

            return publish_to_catalog_impl(_comm, key, replica_index, irods::to_json(_replica.cond_input().get()));
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

