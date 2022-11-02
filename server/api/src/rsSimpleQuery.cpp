#include "irods/rsSimpleQuery.hpp"

#include "irods/apiNumber.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/rodsConnect.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace
{
    auto _rsSimpleQuery(rsComm_t* rsComm, simpleQueryInp_t* simpleQueryInp, simpleQueryOut_t** simpleQueryOut) -> int
    {
        int control = simpleQueryInp->control;
        int maxBufSize = simpleQueryInp->maxBufSize;
        char* outBuf = static_cast<char*>(malloc(maxBufSize));

        int status = chlSimpleQuery(
            rsComm,
            simpleQueryInp->sql,
            simpleQueryInp->arg1,
            simpleQueryInp->arg2,
            simpleQueryInp->arg3,
            simpleQueryInp->arg4,
            simpleQueryInp->form,
            &control,
            outBuf,
            maxBufSize);

        if (status < 0) {
            if (status != CAT_NO_ROWS_FOUND) {
                rodsLog(LOG_NOTICE, "_rsSimpleQuery: simpleQuery for %s, status = %d", simpleQueryInp->sql, status);
            }
            return status;
        }

        simpleQueryOut_t* myQueryOut = static_cast<simpleQueryOut_t*>(malloc(sizeof(simpleQueryOut_t)));
        myQueryOut->control = control;
        myQueryOut->outBuf = outBuf;

        *simpleQueryOut = myQueryOut;

        return status;
    } // _rsSimpleQuery
} // anonymous namespace

int
rsSimpleQuery( rsComm_t *rsComm, simpleQueryInp_t *simpleQueryInp,
               simpleQueryOut_t **simpleQueryOut ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    addRErrorMsg(
        &rsComm->rError, DEPRECATED_API, "SimpleQuery has been deprecated. Use GenQuery or SpecificQuery instead.");

    status = getAndConnRcatHost(rsComm, PRIMARY_RCAT, (const char*) NULL, &rodsServerHost);
    if ( status < 0 ) {
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
            status = _rsSimpleQuery( rsComm, simpleQueryInp, simpleQueryOut );
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
        status = rcSimpleQuery(rodsServerHost->conn, simpleQueryInp, simpleQueryOut);
    }

    if ( status < 0 && status != CAT_NO_ROWS_FOUND ) {
        rodsLog( LOG_NOTICE,
                 "rsSimpleQuery: rcSimpleQuery failed, status = %d", status );
    }
    return status;
}

#pragma clang diagnostic pop // ignored "-Wdeprecated-declarations"
