#include "irods/logical_quota_utilities.hpp"

#include "irods/rs_get_grid_configuration_value.hpp"
#include "irods/rs_get_logical_quota.hpp"
#include "irods/irods_at_scope_exit.hpp"
#include "irods/rcMisc.h"
#include "irods/irods_logger.hpp"

#include <cstdlib>
#include <string>

namespace irods::logical_quotas {
    int check_logical_quota_violation(RsComm *_comm, const char* _coll_name) {
        using log_server = irods::experimental::log::server;

        gridConfigurationInp_t gcinp { {"logical_quotas"}, {"enabled"}, { 0 } };
        gridConfigurationOut_t *gcout{};

        const auto free_gcout = irods::at_scope_exit{[&gcout] { std::free(gcout); }};

        int status = rs_get_grid_configuration_value(_comm, &gcinp, &gcout);
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
        return static_cast<int>(violation_flags);
    } // check_logical_quota_violation
}
