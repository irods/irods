#include "catalog_utilities.hpp"

#include "rcConnect.h"
#include "rodsConnect.h"
#include "miscServerFunct.hpp"
#include "irods_log.hpp"
#include "irods_rs_comm_query.hpp"

#include "fmt/format.h"

namespace
{
    auto connected_to_catalog_provider(RsComm& _comm, const rodsServerHost& _host) -> bool
    {
        return LOCAL_HOST == _host.localFlag;
    } // connected_to_catalog_provider
} // anonymous namespace

namespace irods::experimental::catalog
{
    auto bind_string_to_statement(bind_parameters& _bp) -> void
    {
        std::string v = _bp.json_input.at(_bp.column_name.data()).get<std::string>();
        _bp.bind_values.push_back(std::move(v));

        const std::string& value = std::get<std::string>(_bp.bind_values.back());
        irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - binding [{}] to [{}] at [{}]",
            __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index));

        _bp.statement.bind(_bp.index, value.c_str());
    } // bind_string_to_statement

    auto bind_bigint_to_statement(bind_parameters& _bp) -> void
    {
        // The Oracle ODBC driver will fail on execution of the prepared statement if
        // a 64-bit integer is bound. To get around this limitation, Oracle allows the
        // integer to be bound as a string. See the following thread for a little more
        // information:
        //
        //   https://stackoverflow.com/questions/338609/binding-int64-sql-bigint-as-query-parameter-causes-error-during-execution-in-o
        //
        if ("oracle" == _bp.db_instance_name) {
            std::string v = _bp.json_input.at(_bp.column_name.data()).get<std::string>();
            _bp.bind_values.push_back(std::move(v));

            const auto& value = std::get<std::string>(_bp.bind_values.back());
            irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - binding [{}] to [{}] at [{}]",
                __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index));

            _bp.statement.bind(_bp.index, value.c_str());
        }
        else {
            const std::uint64_t v = std::stoull(_bp.json_input.at(_bp.column_name.data()).get<std::string>());
            _bp.bind_values.push_back(v);

            const std::uint64_t& value = std::get<std::uint64_t>(_bp.bind_values.back());
            irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - binding [{}] to [{}] at [{}]",
                __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index));

            _bp.statement.bind(_bp.index, &value);
        }
    } // bind_bigint_to_statement

    auto bind_integer_to_statement(bind_parameters& _bp) -> void
    {
        const int v = std::stoi(_bp.json_input.at(_bp.column_name.data()).get<std::string>());
        _bp.bind_values.push_back(v);

        const int& value = std::get<int>(_bp.bind_values.back());
        irods::log(LOG_DEBUG8, fmt::format("[{}:{}] - binding [{}] to [{}] at [{}]",
            __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index));

        _bp.statement.bind(_bp.index, &value);
    } // bind_integer_to_statement

    auto user_has_permission_to_modify_entity(RsComm& _comm,
                                              nanodbc::connection& _db_conn,
                                              const std::string_view _db_instance_name,
                                              std::int64_t _object_id,
                                              const entity_type _entity_type) -> bool
    {
        switch (_entity_type) {
            case entity_type::data_object:
                [[fallthrough]];
            case entity_type::collection: {
                nanodbc::statement stmt{_db_conn};

                prepare(stmt, "select t.token_id from R_TOKN_MAIN t"
                              " inner join R_OBJT_ACCESS a on t.token_id = a.access_type_id "
                              "where"
                              " a.user_id = (select user_id from R_USER_MAIN where user_name = ?) and"
                              " a.object_id = ?");
                
                if ("oracle" == _db_instance_name) {
                    const auto object_id_string = std::to_string(_object_id);

                    stmt.bind(0, _comm.clientUser.userName);
                    stmt.bind(1, object_id_string.data());

                    if (auto row = execute(stmt); row.next()) {
                        return static_cast<access_type>(row.get<int>(0)) >= access_type::modify_object;
                    }
                }
                else {
                    stmt.bind(0, _comm.clientUser.userName);
                    stmt.bind(1, &_object_id);

                    if (auto row = execute(stmt); row.next()) {
                        return static_cast<access_type>(row.get<int>(0)) >= access_type::modify_object;
                    }
                }

                break;
            }

            case entity_type::user: [[fallthrough]];
            case entity_type::resource:
                return irods::is_privileged_client(_comm);

            default:
                irods::log(LOG_ERROR, fmt::format("Invalid entity type [entity_type => {}]", _entity_type));
                break;
        }
        return false;
    } // user_has_permission_to_modify_entity

    auto throw_if_catalog_provider_service_role_is_invalid() -> void
    {
        std::string role;

        if (const auto err = get_catalog_service_role(role); !err.ok()) {
            THROW(err.code(), "Failed to retrieve service role");
        }

        if (irods::CFG_SERVICE_ROLE_CONSUMER == role) {
            THROW(SYS_NO_ICAT_SERVER_ERR, "Remote catalog provider not found");
        }

        if (irods::CFG_SERVICE_ROLE_PROVIDER != role) {
            THROW(SYS_SERVICE_ROLE_NOT_SUPPORTED, fmt::format("Role not supported [role => {}]", role));
        }
    } // throw_if_service_role_is_invalid

    auto get_catalog_provider_host() -> rodsServerHost
    {
        rodsServerHost* host{};

        if (const int status = getRcatHost(MASTER_RCAT, nullptr, &host); status < 0 || !host) {
            THROW(status, "failed getting catalog provider host");
        }

        return *host;
    } // get_catalog_provider_host

    auto connected_to_catalog_provider(RsComm& _comm) -> bool
    {
        return ::connected_to_catalog_provider(_comm, get_catalog_provider_host());
    } // connected_to_catalog_provider

    auto redirect_to_catalog_provider(RsComm& _comm) -> rodsServerHost*
    {
        rodsServerHost* host = nullptr;

        if (const int ec = getAndConnRcatHost(&_comm, MASTER_RCAT, nullptr, &host); ec < 0) {
            THROW(ec, "failed to connect to catalog provider host");
        }

        if (!host) {
            THROW(SYS_INVALID_SERVER_HOST, "failed to get catalog provider host");
        }

        return host;
    } // redirect_to_catalog_provider
} // namespace irods::experimental::catalog
