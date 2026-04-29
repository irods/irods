#include "irods/logical_quota_utilities.hpp"

#include "irods/client_connection.hpp"
#include "irods/fully_qualified_username.hpp"
#include "irods/get_grid_configuration_value.h"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rodsConnect.h"
#include "irods/rs_get_grid_configuration_value.hpp"
#include "irods/rs_get_logical_quota.hpp"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <cstdlib>
#include <string>

namespace irods::logical_quotas {
    int check_logical_quota_violation(RsComm *_comm, const char* _coll_name) {
        using log_server = irods::experimental::log::server;

        namespace ua = irods::experimental::administration;

        gridConfigurationInp_t gcinp { {"logical_quotas"}, {"enabled"}, { 0 } };
        gridConfigurationOut_t *gcout{};

        const auto free_gcout = irods::at_scope_exit{[&gcout] { std::free(gcout); }};

        rodsServerHost_t *rodsServerHost{};

        auto status = getAndConnRcatHost(_comm, PRIMARY_RCAT, _coll_name, &rodsServerHost);

        if ( status < 0 ) {
            return status;
        }

        if (LOCAL_HOST == rodsServerHost->localFlag) {
            std::string svc_role;
            irods::error ret = get_catalog_service_role(svc_role);
            if(!ret.ok()) {
                log_server::error("{}: get_catalog_service_role failed with ec=[{}] ", __func__, ret.code());
                return ret.code();
            }

            if(irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
                status = rs_get_grid_configuration_value(_comm, &gcinp, &gcout);
            } else if(irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
                status = SYS_NO_RCAT_SERVER_ERR;
            } else {
                log_server::error("{}: role not supported [{}]", __func__, svc_role.c_str());
                status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
            }
        }
        else {
            try {
                const auto client_user_type = *ua::server::type(*_comm, ua::user{_comm->clientUser.userName, _comm->clientUser.rodsZone});
                if (ua::user_type::rodsadmin != client_user_type) {
                    // Allow a privilege escalation to fetch the grid config value.
                    auto local_admin = irods::experimental::fully_qualified_username{_comm->myEnv.rodsUserName, _comm->myEnv.rodsZone};
                    auto conn = irods::experimental::client_connection{rodsServerHost->hostName->name, _comm->myEnv.rodsPort, local_admin};
                    status = rc_get_grid_configuration_value(static_cast<RcComm*>(conn), &gcinp, &gcout);
                }
                else {
                    status = rc_get_grid_configuration_value(rodsServerHost->conn, &gcinp, &gcout);
                }
            }
            catch (const irods::exception& e) {
                log_server::error("{}: Caught iRODS exception with ec=[{}] while connecting to catalog provider. Logical quotas will not be enforced. Exception: [{}]", __func__, e.code(), e.client_display_what());
                return static_cast<int>(violation::none);
            }
            catch (const std::exception& e) {
                log_server::error("{}: Caught std::exception while connecting to catalog provider. Logical quotas will not be enforced. Exception: [{}]", __func__, e.what());
                return static_cast<int>(violation::none);
            }
            catch (...) {
                log_server::error("{}: Caught unknown error while connecting to catalog provider. Logical quotas will not be enforced.", __func__);
                return static_cast<int>(violation::none);
            }
        }

        if(status < 0) {
            log_server::warn("{}: Failed to get logical quota enforcement status. (ec=[{}]) Logical quotas will not be enforced.", __func__, status);
            return static_cast<int>(violation::none);
        }

        if(std::strncmp(gcout->option_value, "0", 2) == 0) {
            // No logical quota enforcement
            return static_cast<int>(violation::none);
        }

        if(std::strncmp(gcout->option_value, "1", 2) != 0) {
            // Strange value set, so log a message
            log_server::warn("{}: Received unknown value [{}] when fetching logical quota enforcement status. (Set to \"1\" to enable enforcement.) Logical quotas will not be enforced.", __func__, gcout->option_value);
            return static_cast<int>(violation::none);
        }

        getLogicalQuotaInp_t inp{};
        logicalQuotaList_t* out{};
        auto tmp = std::string(_coll_name);
        inp.coll_name = const_cast<char*>(tmp.c_str());
        status = rs_get_logical_quota(_comm, &inp, &out);
        if(status < 0) {
            log_server::error("{}: rs_get_logical_quota failed with ec=[{}] ", __func__, status);
            return status;
        }

        violation violation_flags = violation::none;
        for(int i = 0; i < out->len && ((violation_flags ^ violation::bytes ^ violation::objects) != violation::none); i++) {
           if(out->list[i].over_bytes > 0) {
                violation_flags |= violation::bytes;
           }
           if(out->list[i].over_objects > 0) {
                violation_flags |= violation::objects;
           }
        }
        clear_logical_quota_list(out);
        std::free(out);
        return static_cast<int>(violation_flags);
    } // check_logical_quota_violation
}
