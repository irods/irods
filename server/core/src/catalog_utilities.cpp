#include "catalog_utilities.hpp"
#include "irods_logger.hpp"
#include "irods_rs_comm_query.hpp"

namespace irods::experimental::catalog {

    auto user_has_permission_to_modify_metadata(rsComm_t& _comm,
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
                    constexpr int access_modify_object = 1120;
                    return row.get<int>(0) >= access_modify_object;
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
    } // user_has_permission_to_modify_metadata

} // namespace irods::experimental::catalog
