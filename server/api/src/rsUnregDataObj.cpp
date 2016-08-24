/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "unregDataObj.h"
#include "icatHighLevelRoutines.hpp"
#include "fileDriver.hpp"
#include "miscServerFunct.hpp"
#include "rsUnregDataObj.hpp"

#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"
#include "irods_configuration_keywords.hpp"

int
rsUnregDataObj( rsComm_t *rsComm, unregDataObj_t *unregDataObjInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;

    dataObjInfo = unregDataObjInp->dataObjInfo;

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )dataObjInfo->objPath,
                                 &rodsServerHost );
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

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsUnregDataObj( rsComm, unregDataObjInp );
        } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
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
        status = rcUnregDataObj( rodsServerHost->conn, unregDataObjInp );
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

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        dataObjInfo_t *dataObjInfo;
        keyValPair_t *condInput;
        int status;
        irods::error ret;

        condInput = unregDataObjInp->condInput;
        dataObjInfo = unregDataObjInp->dataObjInfo;

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
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        return SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        return SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
}
