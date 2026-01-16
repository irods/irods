#include "irods/rsUnregDataObj.hpp"

#include "irods/dataObjOpr.hpp"
#include "irods/fileDriver.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_file_object.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/logical_locking.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/unregDataObj.h"

namespace
{
    using log_api = irods::experimental::log::api;
    namespace ill = irods::logical_locking;
} // anonymous namespace

int
rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;

    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = getAndConnRcatHost(rsComm, PRIMARY_RCAT, (const char*) dataObjInfo->objPath, &rodsServerHost);
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        std::string svc_role;
        irods::error ret = get_catalog_service_role(svc_role);
        if(!ret.ok()) {
            irods::log(PASS(ret));
            return ret.code();
        }

        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsUnregDataObj( rsComm, unregDataObjInp );
        } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
            status = SYS_NO_RCAT_SERVER_ERR;
        } else {
            rodsLog(
                LOG_ERROR,
                "role not supported [%s]",
                svc_role.c_str() );
            status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
        }

    }
    else {
        // This logical locking bypass is only allowed for server-to-server connections. If this is not a
        // server-to-server connection, remove the bypass keyword so that logical locking can be properly enforced.
        bool restore_ill_bypass_keyword = false;
        if (nullptr != getValByKey(unregDataObjInp->condInput, ill::keywords::bypass)) {
            if (!irods::server_property_exists(irods::AGENT_CONN_KW)) {
                restore_ill_bypass_keyword = true;
                static_cast<void>(rmKeyVal(unregDataObjInp->condInput, ill::keywords::bypass));
            }
        }

        status = rcUnregDataObj( rodsServerHost->conn, unregDataObjInp );

        if (restore_ill_bypass_keyword) {
            static_cast<void>(addKeyVal(unregDataObjInp->condInput, ill::keywords::bypass, ""));
        }
    }

    return status;
}

int
_rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        dataObjInfo_t *dataObjInfo;
        keyValPair_t *condInput;
        int status;
        irods::error ret;

        condInput = unregDataObjInp->condInput;
        dataObjInfo = unregDataObjInp->dataObjInfo;

        // Check to see if the object is locked. If so, an error is returned.
        if (!irods::server_property_exists(irods::AGENT_CONN_KW) ||
            nullptr == getValByKey(condInput, ill::keywords::bypass)) {
            DataObjInfo* info{};
            const auto free_DataObjInfo = irods::at_scope_exit{[&info] { freeAllDataObjInfo(info); }};

            // Get data object information. Returns error if object does not exist.
            DataObjInp inp{};
            const auto clear_condInput = irods::at_scope_exit{[&inp] { clearKeyVal(&inp.condInput); }};
            if (nullptr != getValByKey(unregDataObjInp->condInput, ADMIN_KW)) {
                addKeyVal(&inp.condInput, ADMIN_KW, "");
            }
            std::strncpy(static_cast<char*>(inp.objPath), dataObjInfo->objPath, sizeof(inp.objPath) - 1);
            if (const auto ret = getDataObjInfo(rsComm, &inp, &info, ACCESS_DELETE_OBJECT, 0); ret < 0) {
                log_api::error("{}: Data object [{}] does not exist. [error code={}]", __func__, inp.objPath, ret);
                return ret;
            }

            if (const auto ret = ill::try_lock(*info, ill::lock_type::write); ret < 0) {
                const auto msg =
                    fmt::format("Unregister not allowed because data object is locked. error code=[{}], path=[{}]",
                                ret,
                                dataObjInfo->objPath);
                log_api::info("{}: {}", __func__, msg);
                return ret;
            }
        }

        status = chlUnregDataObj( rsComm, dataObjInfo, condInput );
        if ( status < 0 ) {
            char* sys_error = NULL;
            const char* rods_error = rodsErrorName( status, &sys_error );
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to unregister the data object \"";
            msg << dataObjInfo->objPath;
            msg << "\" - ";
            msg << rods_error << " " << sys_error;
            ret = ERROR( status, msg.str() );
            irods::log( ret );
            free( sys_error );
        }
        else {
            irods::file_object_ptr file_obj(
                new irods::file_object(
                    rsComm,
                    dataObjInfo ) );
            ret = fileUnregistered( rsComm, file_obj );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to signal resource that the data object \"";
                msg << dataObjInfo->objPath;
                msg << "\" was unregistered";
                ret = PASSMSG( msg.str(), ret );
                irods::log( ret );
                status = ret.code();
            }
        }
        return status;
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}
