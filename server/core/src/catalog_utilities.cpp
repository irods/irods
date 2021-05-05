#include "catalog_utilities.hpp"

#include "rcConnect.h"
#include "rodsConnect.h"
#include "miscServerFunct.hpp"
#include "irods_logger.hpp"
#include "irods_rs_comm_query.hpp"

namespace
{
    using log = irods::experimental::log;

    auto connected_to_catalog_provider(RsComm& _comm, const rodsServerHost& _host) -> bool
    {
        return LOCAL_HOST == _host.localFlag;
    } // connected_to_catalog_provider
} // anonymous namespace

namespace irods::experimental::catalog
{
    auto bind_string_to_statement(bind_parameters& _bp) -> void
    {
        const std::string& v = _bp.json_input.at(_bp.column_name.data()).get<std::string>();
        _bp.bind_values.push_back(v);

        const std::string& value = std::get<std::string>(_bp.bind_values.back());
        log::database::trace("[{}:{}] - binding [{}] to [{}] at [{}]", __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index);

        _bp.statement.bind(_bp.index, value.c_str());
    } // bind_string_to_statement

    auto bind_bigint_to_statement(bind_parameters& _bp) -> void
    {
        const std::uint64_t v = std::stoul(_bp.json_input.at(_bp.column_name.data()).get<std::string>());
        _bp.bind_values.push_back(v);

        const std::uint64_t& value = std::get<std::uint64_t>(_bp.bind_values.back());
        log::database::trace("[{}:{}] - binding [{}] to [{}] at [{}]", __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index);

        _bp.statement.bind(_bp.index, &value);
    } // bind_bigint_to_statement

    auto bind_integer_to_statement(bind_parameters& _bp) -> void
    {
        const int v = std::stoi(_bp.json_input.at(_bp.column_name.data()).get<std::string>());
        _bp.bind_values.push_back(v);

        const int& value = std::get<int>(_bp.bind_values.back());
        log::database::trace("[{}:{}] - binding [{}] to [{}] at [{}]", __FUNCTION__, __LINE__, _bp.column_name, value, _bp.index);

        _bp.statement.bind(_bp.index, &value);
    } // bind_integer_to_statement

    auto user_has_permission_to_modify_entity(RsComm& _comm,
                                              nanodbc::connection& _db_conn,
                                              int _object_id,
                                              const entity_type _entity_type) -> bool
    {
        using log = irods::experimental::log;

        switch (_entity_type) {
            case entity_type::data_object:
                [[fallthrough]];
            case entity_type::collection:
            {
                const auto query = fmt::format("select t.token_id from R_TOKN_MAIN t"
                                               " inner join R_OBJT_ACCESS a on t.token_id = a.access_type_id "
                                               "where"
                                               " a.user_id = (select user_id from R_USER_MAIN where user_name = '{}') and"
                                               " a.object_id = '{}'", _comm.clientUser.userName, _object_id);

                if (auto row = execute(_db_conn, query); row.next()) {
                    return static_cast<access_type>(row.get<int>(0)) >= access_type::modify_object;
                }
                break;
            }

            case entity_type::user:
                [[fallthrough]];
            case entity_type::resource:
                return irods::is_privileged_client(_comm);

            default:
                log::database::error("Invalid entity type [entity_type => {}]", _entity_type);
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

    auto redirect_to_catalog_provider(RsComm& _comm) -> rodsServerHost
    {
        rodsServerHost host = get_catalog_provider_host();

        if (::connected_to_catalog_provider(_comm, host)) {
            return host;
        }

        if (int ec = svrToSvrConnect(&_comm, &host); ec < 0) {
            if (REMOTE_ICAT == host.rcatEnabled) {
                ec = convZoneSockError(ec);
            }

            THROW(ec, fmt::format("svrToSvrConnect to {} failed", host.hostName->name));
        }

        return host;
    } // redirect_to_catalog_provider
} // namespace irods::experimental::catalog
