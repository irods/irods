/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* modColl.c
 */

#include "rsModColl.hpp"
#include "modColl.h"
#include "rcMisc.h"
#include "icatHighLevelRoutines.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"

int
rsModColl( rsComm_t *rsComm, collInp_t *modCollInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )modCollInp->collName,
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
            status = _rsModColl( rsComm, modCollInp );
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
        status = rcModColl( rodsServerHost->conn, modCollInp );
    }

    return status;
}

int
_rsModColl( rsComm_t *rsComm, collInp_t *modCollInp ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        int status;
        collInfo_t collInfo;
        char *tmpStr;

        int i;
        ruleExecInfo_t rei2;

        memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
        rei2.rsComm = rsComm;
        if ( rsComm != NULL ) {
            rei2.uoic = &rsComm->clientUser;
            rei2.uoip = &rsComm->proxyUser;
        }

        memset( &collInfo, 0, sizeof( collInfo ) );

        rstrcpy( collInfo.collName, modCollInp->collName, MAX_NAME_LEN );

        if ((tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_TYPE_KW))) {
            rstrcpy(collInfo.collType, tmpStr, NAME_LEN);
        }

        if ((tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_INFO1_KW))) {
            rstrcpy(collInfo.collInfo1, tmpStr, MAX_NAME_LEN);
        }

        if ((tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_INFO2_KW))) {
            rstrcpy(collInfo.collInfo2, tmpStr, MAX_NAME_LEN);
        }

        if ((tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_MTIME_KW))) {
            rstrcpy(collInfo.collModify, tmpStr, TIME_LEN);
        }

        /**  June 1 2009 for pre-post processing rule hooks **/
        rei2.coi = &collInfo;
        i =  applyRule( "acPreProcForModifyCollMeta", NULL, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            if ( rei2.status < 0 ) {
                i = rei2.status;
            }
            rodsLog( LOG_ERROR,
                     "rsGeneralAdmin:acPreProcForModifyCollMeta error for %s,stat=%d",
                     modCollInp->collName, i );
            return i;
        }
        /**  June 1 2009 for pre-post processing rule hooks **/

        status = chlModColl( rsComm, &collInfo );

        /**  June 1 2009 for pre-post processing rule hooks **/
        if ( status >= 0 ) {
            i =  applyRule( "acPostProcForModifyCollMeta", NULL, &rei2, NO_SAVE_REI );
            if ( i < 0 ) {
                if ( rei2.status < 0 ) {
                    i = rei2.status;
                }
                rodsLog( LOG_ERROR,
                         "rsGeneralAdmin:acPostProcForModifyCollMeta error for %s,stat=%d",
                         modCollInp->collName, i );
                return i;
            }
        }
        /**  June 1 2009 for pre-post processing rule hooks **/

        /* XXXX need to commit */
        if ( status >= 0 ) {
            status = chlCommit( rsComm );
        }
        else {
            chlRollback( rsComm );
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
