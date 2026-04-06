#include "irods/regDataObj.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/fileDriver.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rsRegDataObj.hpp"
#include "irods/rs_get_logical_quota.hpp"

#include "irods/irods_file_object.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_log.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/replica_proxy.hpp"

#include "irods/filesystem.hpp"


namespace
{
    // clang-format off
    namespace fs            = irods::experimental::filesystem;
    using log_api           = irods::experimental::log::api;
    // clang-format on

    int checkQuotaViolationForReg(rsComm_t *rsComm, dataObjInfo_t *dataObjInfo) {
        fs::path path{dataObjInfo->objPath};
        int status = check_logical_quota_violation(rsComm, path.parent_path().c_str());
        if(status < 0) {
            log_api::error("check_logical_quota_violation failed with error [{}]", status);
            return status;
        }
        // Always fail if over object limit (registration makes new objects)
        // Fail when trying to register a nonempty object if over byte limit
        if((status & 2) || ((status & 1) && dataObjInfo->dataSize > 0)) {
            return LOGICAL_QUOTA_EXCEEDED;
        }
        return 0;
    }
} // anonymous namespace

/* rsRegDataObj - This call is strictly an API handler and should not be
 * called directly in the server. For server calls, use svrRegDataObj
 */
int
rsRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
              dataObjInfo_t **outDataObjInfo ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    *outDataObjInfo = NULL;

    // Check quota enforcement before register
    if(int ec = checkQuotaViolationForReg(rsComm, dataObjInfo); ec < 0) {
        log_api::warn("[{}] failure due to logical quota violation or error; ec=[{}]", __func__, ec);
        return ec;
    }

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
            status = _rsRegDataObj( rsComm, dataObjInfo );
            if ( status >= 0 ) {
                *outDataObjInfo = ( dataObjInfo_t * ) malloc( sizeof( dataObjInfo_t ) );
                /* fake pointers will be deleted by the packing */
                //**outDataObjInfo = *dataObjInfo;
                memcpy( *outDataObjInfo, dataObjInfo, sizeof( dataObjInfo_t ) );
            }
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
        status = rcRegDataObj( rodsServerHost->conn, dataObjInfo,
                               outDataObjInfo );
    }
    return status;
}

int
_rsRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;
        irods::error ret;
        status = chlRegDataObj( rsComm, dataObjInfo );
        if ( status < 0 ) {
            char* sys_error = NULL;
            const char* rods_error = rodsErrorName( status, &sys_error );
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to register data object \"" << dataObjInfo->objPath << "\"";
            msg << " - " << rods_error << " " << sys_error;
            ret = ERROR( status, msg.str() );
            irods::log( ret );
            free( sys_error );
        }
        else {
            irods::file_object_ptr file_obj(
                new irods::file_object(
                    rsComm,
                    dataObjInfo ) );
            ret = fileRegistered( rsComm, file_obj );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to signal resource that the data object \"";
                msg << dataObjInfo->objPath;
                msg << "\" was registered";
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

int
svrRegDataObj( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    if ( dataObjInfo->specColl != NULL ) {
        rodsLog( LOG_NOTICE,
                 "svrRegDataObj: Reg path %s is in spec coll",
                 dataObjInfo->objPath );
        return SYS_REG_OBJ_IN_SPEC_COLL;
    }

    // Check quota enforcement before register
    if(int ec = checkQuotaViolationForReg(rsComm, dataObjInfo); ec < 0) {
        log_api::warn("[{}] failure due to logical quota violation or error; ec=[{}]", __func__, ec);
        return ec;
    }

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
            status = _rsRegDataObj( rsComm, dataObjInfo );
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
        dataObjInfo_t *outDataObjInfo = NULL;
        status = rcRegDataObj( rodsServerHost->conn, dataObjInfo,
                               &outDataObjInfo );
        if ( status >= 0 && NULL != outDataObjInfo ) { // JMC cppcheck - nullptr
            auto registered_replica = irods::experimental::replica::make_replica_proxy(*dataObjInfo);
            registered_replica.owner_user_name(outDataObjInfo->dataOwnerName);
            registered_replica.owner_zone_name(outDataObjInfo->dataOwnerZone);
            registered_replica.ctime(outDataObjInfo->dataCreate);
            registered_replica.mtime(outDataObjInfo->dataModify);
            registered_replica.data_expiry(outDataObjInfo->dataExpiry);
            registered_replica.data_id(outDataObjInfo->dataId);
            clearKeyVal( &outDataObjInfo->condInput );
            free( outDataObjInfo );
        }
    }

    return status;
}
