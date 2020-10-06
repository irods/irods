#ifndef IRODS_REPLICA_STATE_TABLE_HPP
#define IRODS_REPLICA_STATE_TABLE_HPP

#include "json.hpp"

#include <string_view>

struct DataObjInfo;

namespace irods
{
    /// \brief Singleton which maintains a JSON structure describing the state of change of opened data objects.
    ///
    /// \since 4.2.9
    class replica_state_table
    {
    public:
        // Prevent copying replica_state_table
        replica_state_table(const replica_state_table&) = delete;
        auto operator=(const replica_state_table&) -> replica_state_table& = delete;

        /// \brief Specify which version of the data object information to fetch from the map entry
        ///
        /// \since 4.2.9
        enum class state_type
        {
            before,
            after,
            both
        }; // enum class state_type

        /// \brief Initialize the replica_state_table by making sure it is completely cleared out
        ///
        /// \since 4.2.9
        static auto init() -> void;

        /// \brief De-initialize the replica_state_table by making sure it is completely cleared out
        ///
        /// \since 4.2.9
        static auto deinit() -> void;

        /// \returns static Singleton instance
        ///
        /// \since 4.2.9
        static auto instance() noexcept -> replica_state_table&;

        /// \brief Create a new entry in the replica_state_table
        ///
        /// If the specified object already exists in the replica state table, nothing happens.
        ///
        /// \param[in] _obj Initial data object state to serve as both "before" and "after" at the start
        ///
        /// \since 4.2.9
        auto insert(const DataObjInfo& _obj) -> void;

        /// \brief Erase replica_state_table entry indicated by _logical_path
        ///
        /// \param[in] _logical_path
        ///
        /// \throws irods::exception If no key matches _logical_path
        ///
        /// \since 4.2.9
        auto erase(const std::string_view _logical_path) -> void;

        /// \brief Returns whether or not an entry in the replica state table keys on the provided logical path
        ///
        /// \param[in] _logical_path
        ///
        /// \retval true if replica state table contains key _logical_path
        /// \retval false if replica state table does not contain key _logical_path
        ///
        /// \since 4.2.9
        auto contains(const std::string_view _logical_path) const -> bool;

        /// \brief return all information for all states of all replicas for a particular data object
        ///
        /// \param[in] _logical_path
        ///
        /// \returns JSON object of the following form: \parblock
        /// \code{.js}
        /// [
        ///     {
        ///         "before": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         },
        ///         "after": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         }
        ///   },
        ///   ...
        /// ]
        /// \endcode
        /// \endparblock
        ///
        /// \since 4.2.9
        auto at(const std::string_view _logical_path) const -> nlohmann::json;

        /// \brief return a specific replica by replica number with optional before/after specification (defaults to both)
        ///
        /// \param[in] _logical_path
        /// \param[in] _replica_number Replica number in the "before" entry
        /// \param[in] _state
        ///
        /// \returns JSON object of the following form: \parblock
        /// - For state_type::both:
        /// \code{.js}
        ///     {
        ///         "before": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         },
        ///         "after": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         }
        ///     }
        /// \endcode
        ///
        /// - For state_type::before/state_type::after:
        /// \code{.js}
        ///     {
        ///         "data_id": <string>,
        ///         "coll_id": <string>,
        ///         "data_repl_num": <string>,
        ///         "data_version": <string>,
        ///         "data_type_name": <string>,
        ///         "data_size": <string>,
        ///         "data_path": <string>,
        ///         "data_owner_name": <string>,
        ///         "data_owner_zone": <string>,
        ///         "data_is_dirty": <string>,
        ///         "data_status": <string>,
        ///         "data_checksum": <string>,
        ///         "data_expiry_ts": <string>,
        ///         "data_map_id": <string>,
        ///         "data_mode": <string>,
        ///         "r_comment": <string>,
        ///         "create_ts": <string>,
        ///         "modify_ts": <string>,
        ///         "resc_id": <string>
        ///     }
        /// \endcode
        /// \endparblock
        ///
        /// \since 4.2.9
        auto at(const std::string_view _logical_path,
                const std::string_view _leaf_resource_name,
                const state_type _state = state_type::both) const -> nlohmann::json;

        /// \brief return a specific replica by replica number with optional before/after specification (defaults to both)
        ///
        /// \param[in] _logical_path
        /// \param[in] _replica_number Replica number in the "before" entry
        /// \param[in] _state
        ///
        /// \returns JSON object of the following form: \parblock
        /// - For state_type::both...
        /// \code{.js}
        ///     {
        ///         "before": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         },
        ///         "after": {
        ///             "data_id": <string>,
        ///             "coll_id": <string>,
        ///             "data_repl_num": <string>,
        ///             "data_version": <string>,
        ///             "data_type_name": <string>,
        ///             "data_size": <string>,
        ///             "data_path": <string>,
        ///             "data_owner_name": <string>,
        ///             "data_owner_zone": <string>,
        ///             "data_is_dirty": <string>,
        ///             "data_status": <string>,
        ///             "data_checksum": <string>,
        ///             "data_expiry_ts": <string>,
        ///             "data_map_id": <string>,
        ///             "data_mode": <string>,
        ///             "r_comment": <string>,
        ///             "create_ts": <string>,
        ///             "modify_ts": <string>,
        ///             "resc_id": <string>
        ///         }
        ///     }
        /// \endcode
        ///
        /// - For state_type::before/state_type::after...
        /// \code{.js}
        ///     {
        ///         "data_id": <string>,
        ///         "coll_id": <string>,
        ///         "data_repl_num": <string>,
        ///         "data_version": <string>,
        ///         "data_type_name": <string>,
        ///         "data_size": <string>,
        ///         "data_path": <string>,
        ///         "data_owner_name": <string>,
        ///         "data_owner_zone": <string>,
        ///         "data_is_dirty": <string>,
        ///         "data_status": <string>,
        ///         "data_checksum": <string>,
        ///         "data_expiry_ts": <string>,
        ///         "data_map_id": <string>,
        ///         "data_mode": <string>,
        ///         "r_comment": <string>,
        ///         "create_ts": <string>,
        ///         "modify_ts": <string>,
        ///         "resc_id": <string>
        ///     }
        /// \endcode
        /// \endparblock
        ///
        /// \since 4.2.9
        auto at(const std::string_view _logical_path,
                const int _replica_number,
                const state_type _state = state_type::both) const -> nlohmann::json;

        /// \brief Updates the specified replica with the specified changes (after state only)
        ///
        /// \param[in] _logical_path
        /// \param[in] _replica_number Replica number in the "before" entry
        /// \param[in] _updates JSON input representing changes to be made. \parblock
        /// Must be of the following form (only fields which need updating need to be included):
        /// \code{.js}
        ///     {
        ///         <r_data_main_column>: <string>,
        ///         ...
        ///     }
        /// \endcode
        /// \endparblock
        ///
        /// \since 4.2.9
        auto update(const std::string_view _logical_path,
                    const int _replica_number,
                    const nlohmann::json& _updates) -> void;

        /// \brief Updates the specified replica with the specified changes (after state only)
        ///
        /// \param[in] _logical_path
        /// \param[in] _leaf_resource_name Leaf resource name in the "before" entry
        /// \param[in] _updates JSON input representing changes to be made. \parblock
        /// Must be of the following form (only fields which need updating need to be included):
        /// \code{.js}
        ///     {
        ///         <r_data_main_column>: <string>,
        ///         ...
        ///     }
        /// \endcode
        /// \endparblock
        ///
        /// \since 4.2.9
        auto update(const std::string_view _logical_path,
                    const std::string_view _leaf_resource_name,
                    const nlohmann::json& _updates) -> void;

        /// \brief Returns the value of a given property of the given replica in the state table
        ///
        /// \param[in] _logical_path
        /// \param[in] _replica_number Replica number in the "before" entry
        /// \param[in] _property_name
        /// \param[in] _state Must be state_type::before or state_type::after
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        auto get_property(
            const std::string_view _logical_path,
            const int _replica_number,
            const std::string_view _property_name,
            const state_type _state = state_type::before) const -> std::string;

        /// \brief Returns the value of a given property of the given replica in the state table
        ///
        /// \param[in] _logical_path
        /// \param[in] _leaf_resource_name Leaf resource name in the "before" entry
        /// \param[in] _property_name
        /// \param[in] _state Must be state_type::before or state_type::after
        ///
        /// \throws irods::exception
        ///
        /// \since 4.2.9
        auto get_property(
            const std::string_view _logical_path,
            const std::string_view _leaf_resource_name,
            const std::string_view _property_name,
            const state_type _state = state_type::before) const -> std::string;

    private:
        // Disallow construction of the replica_state_table outside of member functions
        replica_state_table() = default;
    }; // class replica_state_table
} // namespace irods

#endif // IRODS_REPLICA_STATE_TABLE_HPP
