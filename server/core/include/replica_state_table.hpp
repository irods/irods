#ifndef IRODS_REPLICA_STATE_TABLE_HPP
#define IRODS_REPLICA_STATE_TABLE_HPP

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "data_object_proxy.hpp"

#include "json.hpp"

#include <string_view>

struct RsComm;

/// \brief Library that maintains a globally accessible JSON structure describing the changes to opened data objects.
///
/// \parblock
/// The main use case for the replica state table (RST) is keeping track
/// of the state of a data object from open to close. A list of replicas
/// for each opened data object is maintained here with the attendant
/// system metadata (representative of the catalog state).
///
/// The "before" state represents the state of the replica when the data
/// object was opened. The "after" state represents changes to the
/// replica since being opened.
///
/// The JSON structure has the following form:
/// \code{.js}
/// {
///     "replicas": [
///         {
///             "before": {
///                 "data_id": <string>,
///                 "coll_id": <string>,
///                 "data_repl_num": <string>,
///                 "data_version": <string>,
///                 "data_type_name": <string>,
///                 "data_size": <string>,
///                 "data_path": <string>,
///                 "data_owner_name": <string>,
///                 "data_owner_zone": <string>,
///                 "data_is_dirty": <string>,
///                 "data_status": <string>,
///                 "data_checksum": <string>,
///                 "data_expiry_ts": <string>,
///                 "data_map_id": <string>,
///                 "data_mode": <string>,
///                 "r_comment": <string>,
///                 "create_ts": <string>,
///                 "modify_ts": <string>,
///                 "resc_id": <string>
///             },
///             "after": {
///                 "data_id": <string>,
///                 "coll_id": <string>,
///                 "data_repl_num": <string>,
///                 "data_version": <string>,
///                 "data_type_name": <string>,
///                 "data_size": <string>,
///                 "data_path": <string>,
///                 "data_owner_name": <string>,
///                 "data_owner_zone": <string>,
///                 "data_is_dirty": <string>,
///                 "data_status": <string>,
///                 "data_checksum": <string>,
///                 "data_expiry_ts": <string>,
///                 "data_map_id": <string>,
///                 "data_mode": <string>,
///                 "r_comment": <string>,
///                 "create_ts": <string>,
///                 "modify_ts": <string>,
///                 "resc_id": <string>
///             },
///             "file_modified": {
///                 <string>: <string>,
///                 ...
///             }
///         },
///         ...
///     ]
/// }
/// \endcode
///
/// - "replicas" is an array of JSON structures describing replicas for a particular data object:
///     - "before" refers to the replica as it appeared in the catalog on open
///     - "after" refers to the current state of the replica (may or may not be representative of the catalog state)
///     - "file_modified" (OPTIONAL) is a set of key-value pairs which are used in the fileModified plugin operation
///
/// Each entry uses the data_id (std::uint64_t) for the represented data object as the key into the map.
///
/// \endparblock
///
/// \since 4.2.9
namespace irods::replica_state_table
{
    // data_id serves as the key to entries
    using key_type = std::uint64_t;

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
    auto init() -> void;

    /// \brief De-initialize the replica_state_table by making sure it is completely cleared out
    ///
    /// \since 4.2.9
    auto deinit() -> void;

    /// \brief Create a new entry in the replica_state_table
    ///
    /// If the specified object already exists in the replica state table, nothing happens.
    ///
    /// \param[in] _obj Initial data object state to serve as both "before" and "after" at the start
    ///
    /// \retval 0 on successful insertion
    /// \retval SYS_LIBRARY_ERROR If an nlohmann::json exception occurs
    ///
    /// \since 4.2.9
    auto insert(const irods::experimental::data_object::data_object_proxy_t& _obj) -> int;

    /// \brief Create a new replica in an existing entry in the replica_state_table
    ///
    /// If the specified object already exists in the replica state table, nothing happens.
    ///
    /// \param[in] _replica Initial replica state to serve as both "before" and "after" at the start
    ///
    /// \retval 0 on successful insertion
    /// \retval SYS_LIBRARY_ERROR If an nlohmann::json exception occurs
    ///
    /// \since 4.2.9
    auto insert(const irods::experimental::replica::replica_proxy_t& _replica) -> int;

    /// \brief Erase replica_state_table entry indicated by _key
    ///
    /// \param[in] _key
    ///
    /// \throws irods::exception If no key matches _key
    ///
    /// \since 4.2.9
    auto erase(const key_type& _key) -> void;

    /// \brief Erase replica from the replica_state_table entry indicated by _key and _leaf_resource_name
    ///
    /// \param[in] _key
    /// \param[in] _leaf_resource_name
    ///
    /// \throws irods::exception If no key matches _key
    ///
    /// \since 4.2.9
    auto erase(const key_type& _key, const std::string_view _leaf_resource_name) -> void;

    /// \brief Erase replica from the replica_state_table entry indicated by _key and _replica_number
    ///
    /// \param[in] _key
    /// \param[in] _replica_number
    ///
    /// \throws irods::exception If no key matches _key
    ///
    /// \since 4.2.9
    auto erase(const key_type& _key, const int _replica_number) -> void;

    /// \brief Returns whether or not an entry in the replica state table keys on the provided logical path
    ///
    /// \param[in] _key
    ///
    /// \retval true if replica state table contains key _key
    /// \retval false if replica state table does not contain key _key
    ///
    /// \since 4.2.9
    auto contains(const key_type& _key) -> bool;

    /// \brief Returns whether or not a replica exists for an existing entry replica state table
    ///
    /// \param[in] _key
    /// \param[in] _leaf_resource_name
    ///
    /// \retval true if replica state table contains key _key and a replica on the specified leaf resource
    /// \retval false if replica state table does not contain key _key or a replica on the specified leaf resource
    ///
    /// \since 4.2.9
    auto contains(const key_type& _key,
                  const std::string_view _leaf_resource_name) -> bool;

    /// \brief Returns whether or not a replica exists for an existing entry replica state table
    ///
    /// \param[in] _key
    /// \param[in] _replica_number
    ///
    /// \retval true if replica state table contains key _key and a replica with the specified replica number
    /// \retval false if replica state table does not contain key _key or a replica with the specified replica number
    ///
    /// \since 4.2.9
    auto contains(const key_type& _key,
                  const int _replica_number) -> bool;

    /// \brief return all information for all states of all replicas for a particular data object
    ///
    /// \param[in] _key
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
    ///     },
    ///     ...
    /// ]
    /// \endcode
    /// \endparblock
    ///
    /// \since 4.2.9
    auto at(const key_type& _key) -> nlohmann::json;

    /// \brief return a specific replica by replica number with optional before/after specification (defaults to both)
    ///
    /// \param[in] _key
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
    auto at(const key_type& _key,
            const std::string_view _leaf_resource_name,
            const state_type _state = state_type::both) -> nlohmann::json;

    /// \brief return a specific replica by replica number with optional before/after specification (defaults to both)
    ///
    /// \param[in] _key
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
    auto at(const key_type& _key,
            const int _replica_number,
            const state_type _state = state_type::both) -> nlohmann::json;

    /// \brief Updates the specified replica with the specified changes (after state only)
    ///
    /// \param[in] _key
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
    auto update(
        const key_type& _key,
        const std::string_view _leaf_resource_name,
        const nlohmann::json& _updates) -> void;

    /// \brief Updates the specified replica with the specified changes (after state only)
    ///
    /// \param[in] _key
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
    auto update(const key_type& _key,
                const int _replica_number,
                const nlohmann::json& _updates) -> void;

    /// \brief Updates all columns of the specified replica
    ///
    /// \param[in] _key
    /// \param[in] _replica replica_proxy which is converted to JSON format: \parblock
    /// \code{.js}
    ///     {
    ///         <r_data_main_column>: <string>,
    ///         ...
    ///     }
    /// \endcode
    /// \endparblock
    ///
    /// \since 4.2.9
    auto update(const key_type& _key,
                const irods::experimental::replica::replica_proxy_t& _replica) -> void;

    /// \brief Returns the value of a given property of the given replica in the state table
    ///
    /// \param[in] _key
    /// \param[in] _replica_number Replica number in the "before" entry
    /// \param[in] _property_name
    /// \param[in] _state Must be state_type::before or state_type::after
    ///
    /// \throws irods::exception If specified replica does not exist
    ///
    /// \since 4.2.9
    auto get_property(
        const key_type& _key,
        const int _replica_number,
        const std::string_view _property_name,
        const state_type _state = state_type::before) -> std::string;

    /// \brief Prepares the specified data object as input to data_object_finalize and updates the catalog
    ///
    /// \param[in/out] _comm
    /// \param[in] _key
    /// \param[in] _replica_number_for_file_modified
    /// \param[in] _file_modified_parameters
    ///
    /// \returns error code returned by rs_data_object_finalize
    /// \retval 0 success
    ///
    /// \throws irods::exception
    ///
    /// \since 4.2.9
    auto publish_to_catalog(
        RsComm& _comm,
        const key_type& _key,
        const int _replica_number_for_file_modified,
        const nlohmann::json& _file_modified_parameters) -> int;

    /// \brief Prepares the specified data object as input to data_object_finalize and updates the catalog
    ///
    /// \param[in/out] _comm
    /// \param[in] _key
    /// \param[in] _leaf_resource_name_for_file_modified
    /// \param[in] _file_modified_parameters
    ///
    /// \returns error code returned by rs_data_object_finalize
    /// \retval 0 success
    ///
    /// \throws irods::exception
    ///
    /// \since 4.2.9
    auto publish_to_catalog(
        RsComm& _comm,
        const key_type& _key,
        const std::string_view _leaf_resource_name_for_file_modified,
        const nlohmann::json& _file_modified_parameters) -> int;

    /// \brief Prepares the data object indicated by _replica as input to data_object_finalize and updates the catalog
    ///
    /// \param[in/out] _comm
    /// \param[in] _replica replica_proxy containing the key, replica number, and fileModified input
    ///
    /// \returns error code returned by rs_data_object_finalize
    /// \retval 0 success
    ///
    /// \throws irods::exception
    ///
    /// \since 4.2.9
    auto publish_to_catalog(
        RsComm& _comm,
        const irods::experimental::replica::replica_proxy_t& _replica) -> int;


    /// \brief Get the logical path for the entry with key _key
    ///
    /// Because replica_state_table entries are keyed on data_id, this convenience function is a way
    /// to easily retrieve the logical path for a given entry in the table.
    ///
    /// \since 4.2.9
    auto get_logical_path(const key_type& _key) -> std::string;
} // namespace irods::replica_state_table

#endif // IRODS_REPLICA_STATE_TABLE_HPP
