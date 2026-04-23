#include "irods/rs_get_logical_quota.hpp"

#include "irods/irods_at_scope_exit.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rs_get_grid_configuration_value.hpp"

#include <cstdlib>
#include <string>
#include <tuple>
#include <vector>

namespace {
    using log_api = irods::experimental::log::api;

    int _rs_get_logical_quota(
        RsComm& _comm,
        const getLogicalQuotaInp_t& _get_logical_quota_inp,
        logicalQuotaList_t*& _logical_quota_list) {

        std::vector<std::tuple<std::string, std::int64_t, std::int64_t, std::int64_t, std::int64_t>> quota_values;

        int status = chl_check_logical_quota(&_comm, _get_logical_quota_inp.coll_name, &quota_values);
        if(status < 0) {
            log_api::error("{}: chl_check_logical_quota failed with ec=[{}] ", __func__, status);
            return status;
        }
        _logical_quota_list = static_cast<logicalQuotaList_t*>(std::malloc(sizeof(logicalQuotaList_t)));
        _logical_quota_list->len = static_cast<int>(quota_values.size());
        _logical_quota_list->list = static_cast<logicalQuota_t*>(std::malloc(sizeof(logicalQuota_t)*(_logical_quota_list->len)));

        for(int i = 0; i < _logical_quota_list->len; i++) {
           _logical_quota_list->list[i].coll_name = strdup(std::get<0>(quota_values[i]).c_str());
           _logical_quota_list->list[i].max_bytes = std::get<1>(quota_values[i]);
           _logical_quota_list->list[i].max_objects = std::get<2>(quota_values[i]);
           _logical_quota_list->list[i].over_bytes = std::get<3>(quota_values[i]);
           _logical_quota_list->list[i].over_objects = std::get<4>(quota_values[i]);
        }

        return status;
    } // _rs_get_logical_quota
}

int
rs_get_logical_quota(RsComm *_comm, getLogicalQuotaInp_t *_get_logical_quota_inp,
                logicalQuotaList_t **_logical_quota_list ) {
    rodsServerHost_t *rodsServerHost{};
    auto status = getAndConnRcatHost(_comm, PRIMARY_RCAT, _get_logical_quota_inp->coll_name, &rodsServerHost);

    if ( status < 0 ) {
        return status;
    }

    if (LOCAL_HOST == rodsServerHost->localFlag) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            log_api::error("{}: get_catalog_service_role failed with ec=[{}] ", __func__, ret.code());
            return ret.code();
        }

        if(irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
            status = _rs_get_logical_quota(*_comm, *_get_logical_quota_inp, *_logical_quota_list);
        } else if(irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            log_api::error("{}: role not supported [{}]", __func__, svc_role.c_str());
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }
    }
    else {
        status = rc_get_logical_quota( rodsServerHost->conn, _get_logical_quota_inp,
                                 _logical_quota_list );
    }

    return status;
} // rs_get_logical_quota
