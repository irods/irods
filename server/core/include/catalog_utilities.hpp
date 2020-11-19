#ifndef IRODS_CATALOG_UTILITIES_HPP
#define IRODS_CATALOG_UTILITIES_HPP

#include "irods_exception.hpp"

#include "json.hpp"
#include "nanodbc/nanodbc.h"
#include "rodsConnect.h"

#include <functional>
#include <map>
#include <string>
#include <variant>

struct RsComm;
struct rodsServerHost;

namespace irods::experimental::catalog
{
    // alias for the currently supported types for storing values in the catalog
    using bind_type = std::variant<std::string, std::uint64_t, int>;

    /// \brief Struct containing information for binding values to an SQL statement
    ///
    /// \since 4.2.9
    struct bind_parameters
    {
        /// \var The statement to which parameters will be bound.
        nanodbc::statement& statement;

        /// \var Index of the bind variable in the statement.
        const std::size_t index;

        /// \var JSON object from which the value to bind is derived.
        const nlohmann::json& json_input;

        /// \var The name of the column whose value will be bound to the statement.
        std::string_view column_name;

        /// \var An external variable to store the bind values
        ///
        /// This exists in order to have a place for the values to survive until the statement
        /// can be executed. Otherwise, the values could go out of scope too soon.
        std::vector<bind_type>& bind_values;
    };

    // alias for the mapping operator functions
    using mapping_operator_type = std::function<void(bind_parameters&)>;
    // alias for the map between column names and mapping operator
    using column_mapping_operator_type = std::map<std::string, mapping_operator_type>;

    /// \brief Binds a string value to the given index of the ODBC statement
    ///
    /// \param[in] _bp - Parameters for binding values to a statement
    ///
    /// \since 4.2.9
    auto bind_string_to_statement(bind_parameters& _bp) -> void;

    /// \brief Binds a string value to the given index of the ODBC statement
    ///
    /// \param[in] _bp - Parameters for binding values to a statement
    ///
    /// \since 4.2.9
    auto bind_bigint_to_statement(bind_parameters& _bp) -> void;

    /// \brief Binds a string value to the given index of the ODBC statement
    ///
    /// \param[in] _bp - Parameters for binding values to a statement
    ///
    /// \since 4.2.9
    auto bind_integer_to_statement(bind_parameters& _bp) -> void;

    namespace data_objects
    {
        const column_mapping_operator_type column_mapping_operators{
            {"data_id",         bind_bigint_to_statement},
            {"coll_id",         bind_bigint_to_statement},
            {"data_name",       bind_string_to_statement},
            {"data_repl_num",   bind_integer_to_statement},
            {"data_version",    bind_string_to_statement},
            {"data_type_name",  bind_string_to_statement},
            {"data_size",       bind_bigint_to_statement},
            {"data_path",       bind_string_to_statement},
            {"data_owner_name", bind_string_to_statement},
            {"data_owner_zone", bind_string_to_statement},
            {"data_is_dirty",   bind_integer_to_statement},
            {"data_status",     bind_string_to_statement},
            {"data_checksum",   bind_string_to_statement},
            {"data_expiry_ts",  bind_string_to_statement},
            {"data_map_id",     bind_bigint_to_statement},
            {"data_mode",       bind_string_to_statement},
            {"r_comment",       bind_string_to_statement},
            {"create_ts",       bind_string_to_statement},
            {"modify_ts",       bind_string_to_statement},
            {"resc_id",         bind_bigint_to_statement}
        };
    } // namespace data_objects

    /// \brief Describes the different entity types within iRODS as represented in the catalog.
    /// \since 4.2.9
    enum class entity_type {
        data_object,
        collection,
        user,
        resource,
        zone
    };

    /// \brief Maps the enum values to strings.
    /// \since 4.2.9
    const std::map<std::string, entity_type> entity_type_map{
        {"data_object", entity_type::data_object},
        {"collection", entity_type::collection},
        {"user", entity_type::user},
        {"resource", entity_type::resource},
        {"zone", entity_type::zone}
    };

    /// \brief Determines whether the connected user can modify metadata on a given entity.
    ///
    /// \param[in] _comm iRODS connection structure
    /// \param[in] _db_conn ODBC connection to the database
    /// \param[in] _object_id ID of the object in question in the catalog
    /// \param[in] _entity The type of entity that the object ID refers to
    ///
    /// \retval true if user has permission to modify metadata
    /// \retval false if user does not have permission to modify metadata
    ///
    /// \since 4.2.9
    auto user_has_permission_to_modify_metadata(RsComm& _comm,
                                                nanodbc::connection& _db_conn,
                                                int _object_id,
                                                const entity_type _entity) -> bool;

    /// \throws irods::exception
    /// \since 4.2.9
    auto throw_if_catalog_provider_service_role_is_invalid() -> void;

    /// \brief Thin wrapper around getRcatHost
    ///
    /// \returns Information about the catalog provider host
    ///
    /// \throws irods::exception If an error occurs while retrieving the host information
    ///
    /// \since 4.2.9
    auto get_catalog_provider_host() -> rodsServerHost;

    /// \brief Determine if connected to the catalog provider host
    ///
    /// \param[in] _comm iRODS connection structure
    ///
    /// \throws irods::exception
    ///
    /// \retval true if catalog provider is on the local host
    /// \retval false if the catalog provider is on a remote host
    ///
    /// \since 4.2.9
    auto connected_to_catalog_provider(RsComm& _comm) -> bool;

    /// \brief Establishes a connection with the catalog provider host, if remote.
    ///
    /// If already connected to the catalog provider, simply returns the host information.
    ///
    /// \param[in] _comm iRODS connection structure
    ///
    /// \throws irods::exception If fails to find or connect to the catalog provider host.
    ///
    /// \retval true if catalog provider is remote
    /// \retval false if catalog provider is local
    ///
    /// \since 4.2.9
    auto redirect_to_catalog_provider(RsComm& _comm) -> rodsServerHost;
} // namespace irods::experimental::catalog

#endif // #ifndef IRODS_CATALOG_UTILITIES_HPP
