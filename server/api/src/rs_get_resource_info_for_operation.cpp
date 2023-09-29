#include "irods/rs_get_resource_info_for_operation.hpp"

#include "irods/getRemoteZoneResc.h"
#include "irods/get_resource_info_for_operation.h"
#include "irods/irods_error.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_resource_redirect.hpp"
#include "irods/rcMisc.h"
#include "irods/rodsConnect.h"
#include "irods/rodsError.h"
#include "irods/rodsErrorTable.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstring> // for strdup
#include <string>

int rs_get_resource_info_for_operation(RsComm* _comm, DataObjInp* _dataObjInp, char** _out_info)
{
    using log_api = irods::experimental::log::api;

    if (nullptr == _comm || nullptr == _dataObjInp || nullptr == _out_info) {
        log_api::error("{}:{} Invalid input. Received one or more null pointers.", __func__, __LINE__);
        return SYS_NULL_INPUT;
    }

    *_out_info = nullptr;

    rodsServerHost_t* rodsServerHost = nullptr;
    const int remoteFlag = getAndConnRemoteZone(_comm, _dataObjInp, &rodsServerHost, REMOTE_OPEN);
    if (remoteFlag < 0) {
        log_api::error("{}: getAndConnRemoteZone returned error: [{}]", __func__, remoteFlag);
        return remoteFlag;
    }

    if (REMOTE_HOST == remoteFlag) {
        return rc_get_resource_info_for_operation(rodsServerHost->conn, _dataObjInp, _out_info);
    }

    const char* op_type = getValByKey(&_dataObjInp->condInput, GET_RESOURCE_INFO_OP_TYPE_KW);
    if (nullptr == op_type) {
        constexpr auto ec = SYS_INVALID_INPUT_PARAM;
        const auto message{fmt::format(
            "{}: GET_RESOURCE_INFO_OP_TYPE_KW must be set to a valid operation. Received a null pointer.", __func__)};
        log_api::error(message);
        addRErrorMsg(&_comm->rError, ec, message.c_str());
        return ec;
    }
    const std::array allowed_ops{
        irods::CREATE_OPERATION, irods::OPEN_OPERATION, irods::WRITE_OPERATION, irods::UNLINK_OPERATION};
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto found_operation = std::find(allowed_ops.begin(), allowed_ops.end(), op_type);
    if (allowed_ops.end() == found_operation) {
        constexpr auto ec = INVALID_OPERATION;
        const auto message{fmt::format(
            "{}: GET_RESOURCE_INFO_OP_TYPE_KW must be set to a valid operation. Received [{}].", __func__, op_type)};
        log_api::error(message);
        addRErrorMsg(&_comm->rError, ec, message.c_str());
        return ec;
    }
    std::string hier;
    if (const char* hier_cstr = getValByKey(&_dataObjInp->condInput, RESC_HIER_STR_KW); nullptr == hier_cstr) {
        try {
            auto result = irods::resolve_resource_hierarchy(*found_operation, _comm, *_dataObjInp);
            hier = std::get<std::string>(result);
        }
        catch (const irods::exception& e) {
            log_api::error(e.client_display_what());
            return static_cast<int>(e.code());
        }
    }
    else {
        hier = hier_cstr;
    }
    std::string location;
    // extract the host location from the resource hierarchy
    irods::error ret = irods::get_loc_for_hier_string(hier, location);
    if (!ret.ok()) {
        auto error = PASSMSG(fmt::format("{} - failed in get_loc_for_hier_string.", __func__), ret);
        log_api::error(error.result());
        return static_cast<int>(ret.code());
    }

    nlohmann::json resc_info;
    resc_info["host"] = location;
    resc_info["resource_hierarchy"] = hier;
    *_out_info = strdup(resc_info.dump().c_str());
    return 0;
} // rs_get_resource_info_for_operation
