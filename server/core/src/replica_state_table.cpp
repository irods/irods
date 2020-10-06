#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"
#include "irods_resource_manager.hpp"
#include "replica_state_table.hpp"

#include "fmt/format.h"

#include <mutex>

extern irods::resource_manager resc_mgr;

namespace irods
{
    namespace
    {
        using state_type = replica_state_table::state_type;

        // Global Variables
        nlohmann::json replica_state_map;

        std::mutex rst_mutex;

        auto index_of(const std::string_view _logical_path, const int _replica_number) -> int
        {
            // NOTE: This function is not inherently thread-safe -- make sure to acquire the lock!

            int index = -1;

            for (const auto& e : replica_state_map.at(_logical_path.data())) {
                ++index;
                if (_replica_number == std::stoi(e.at("before").at("data_repl_num").get<std::string>())) {
                    return index;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - replica number [{}] not found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _logical_path));
        } // index_of
    } // anonymouse namespace

    auto replica_state_table::init() -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - initializing state table", __FUNCTION__, __LINE__));

        replica_state_map.clear();
    } // init

    auto replica_state_table::deinit() -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - de-initializing state table", __FUNCTION__, __LINE__));

        replica_state_map.clear();
    } // deinit

    auto replica_state_table::instance() noexcept -> replica_state_table&
    {
        static replica_state_table instance;

        irods::log(LOG_DEBUG9, fmt::format("[{}:{}] - retrieving instance of replica state table", __FUNCTION__, __LINE__));

        return instance;
    } // instance

    auto replica_state_table::insert(const DataObjInfo& _obj) -> void
    {
        const std::string_view logical_path = _obj.objPath;

        std::scoped_lock rst_lock{rst_mutex};

        if (replica_state_map.contains(logical_path.data())) {
            irods::log(LOG_DEBUG, fmt::format("[{}:{}] - entry exists;path:[{}]", __FUNCTION__, __LINE__, logical_path));
            return;
        }

        const auto obj = irods::experimental::data_object::make_data_object_proxy(_obj);

        nlohmann::json entry;

        for (const auto& r : obj.replicas()) {
            const auto json_replica = irods::experimental::replica::to_json(r);

            entry.push_back({
                {"before", json_replica},
                {"after", json_replica}
            });
        }

        replica_state_map[obj.logical_path().data()] = entry;
    } // insert

    auto replica_state_table::erase(const std::string_view _logical_path) -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        if (replica_state_map.contains(_logical_path.data())) {
            replica_state_map.erase(_logical_path.data());
        }
    } // erase

    auto replica_state_table::contains(const std::string_view _logical_path) const -> bool
    {
        std::scoped_lock rst_lock{rst_mutex};

        return replica_state_map.contains(_logical_path.data());
    } // contains

    auto replica_state_table::at(const std::string_view _logical_path) const -> nlohmann::json
    {
        std::scoped_lock rst_lock{rst_mutex};

        return replica_state_map.at(_logical_path.data());
    } // at

    auto replica_state_table::at(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name,
        const state_type _state) const -> nlohmann::json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto resc_id = resc_mgr.hier_to_leaf_id(resc_mgr.get_hier_to_root_for_resc(_leaf_resource_name));

        const auto& replica_json = [&_logical_path, &resc_id]
        {
            for (const auto& e : replica_state_map.at(_logical_path.data())) {
                if (resc_id == std::stoll(e.at("before").at("resc_id").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no resc id [{}] found for [{}]",
                __FUNCTION__, __LINE__, resc_id, _logical_path));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at("before");   break;
            case state_type::after:     return replica_json.at("after");    break;
            case state_type::both:      return replica_json;                break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type:[{}]",
                    __FUNCTION__, __LINE__, _state));
        }
    } // at

    auto replica_state_table::at(
        const std::string_view _logical_path,
        const int _replica_number,
        const state_type _state) const -> nlohmann::json
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto& replica_json = [&_logical_path, &_replica_number]
        {
            for (const auto& e : replica_state_map.at(_logical_path.data())) {
                if (_replica_number == std::stoi(e.at("before").at("data_repl_num").get<std::string>())) {
                    return e;
                }
            }

            THROW(KEY_NOT_FOUND, fmt::format(
                "[{}:{}] - no replica number [{}] found for [{}]",
                __FUNCTION__, __LINE__, _replica_number, _logical_path));
        }();

        switch (_state) {
            // clang-format off
            case state_type::before:    return replica_json.at("before");   break;
            case state_type::after:     return replica_json.at("after");    break;
            case state_type::both:      return replica_json;                break;
            // clang-format on

            default:
                THROW(SYS_INVALID_INPUT_PARAM, fmt::format(
                    "[{}:{}] - invalid state_type:[{}]",
                    __FUNCTION__, __LINE__, _state));
        }
    } // at

    auto replica_state_table::update(
        const std::string_view _logical_path,
        const int _replica_number,
        const nlohmann::json& _updates) -> void
    {
        std::scoped_lock rst_lock{rst_mutex};

        const auto replica_index = index_of(_logical_path, _replica_number);

        auto& target_replica = replica_state_map.at(_logical_path.data()).at(replica_index).at("after");

        target_replica.update(_updates);
    } // update

    auto replica_state_table::get_property(
        const std::string_view _logical_path,
        const int _replica_number,
        const std::string_view _property_name,
        const state_type _state) const -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        auto& rst = irods::replica_state_table::instance();

        const auto replica_json = rst.at(_logical_path, _replica_number, _state);

        return replica_json.at(_property_name.data());
    } // get_property

    auto replica_state_table::get_property(
        const std::string_view _logical_path,
        const std::string_view _leaf_resource_name,
        const std::string_view _property_name,
        const state_type _state) const -> std::string
    {
        if (state_type::both == _state) {
            THROW(SYS_INVALID_INPUT_PARAM, fmt::format("state type must be before or after"));
        }

        auto& rst = irods::replica_state_table::instance();

        const auto replica_json = rst.at(_logical_path, _leaf_resource_name, _state);

        return replica_json.at(_property_name.data());
    } // get_property
} // namespace irods

