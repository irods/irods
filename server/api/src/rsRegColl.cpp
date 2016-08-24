/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* regColl.c
 */

#include "rcMisc.h"
#include "regColl.h"
#include "icatHighLevelRoutines.hpp"
#include "collection.hpp"
#include "miscServerFunct.hpp"
#include "irods_configuration_keywords.hpp"
#include "rsRegColl.hpp"
#include "rsObjStat.hpp"

int
rsRegColl( rsComm_t *rsComm, collInp_t *regCollInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInp_t dataObjInp;
    rodsObjStat_t *rodsObjStatOut = NULL;


    memset( &dataObjInp, 0, sizeof( dataObjInp ) );

    rstrcpy( dataObjInp.objPath, regCollInp->collName, MAX_NAME_LEN );
    status = rsObjStat( rsComm, &dataObjInp, &rodsObjStatOut );
    if ( status >= 0 ) {
        if ( rodsObjStatOut != NULL ) {
            if ( rodsObjStatOut->specColl != NULL ) { // JMC cppcheck null ptr ref
                rodsLog( LOG_ERROR,
                         "rsRegColl: Reg path %s is in spec coll",
                         dataObjInp.objPath );
                //if (rodsObjStatOut != NULL)  // JMC cppcheck null ptr ref
                freeRodsObjStat( rodsObjStatOut );
                return SYS_REG_OBJ_IN_SPEC_COLL;
            }
        }
    }
    freeRodsObjStat( rodsObjStatOut );

    status = getAndConnRcatHost( rsComm, MASTER_RCAT, ( const char* )regCollInp->collName,
                                 &rodsServerHost );
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

        if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            status = _rsRegColl( rsComm, regCollInp );
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
        status = rcRegColl( rodsServerHost->conn, regCollInp );
    }

    return status;
}

int
_rsRegColl( rsComm_t *rsComm, collInp_t *collCreateInp ) {
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

        memset( &collInfo, 0, sizeof( collInfo ) );

        rstrcpy( collInfo.collName, collCreateInp->collName, MAX_NAME_LEN );

        if ( ( tmpStr = getValByKey( &collCreateInp->condInput, COLLECTION_TYPE_KW ) ) != NULL ) {
            rstrcpy( collInfo.collType, tmpStr, NAME_LEN );
            if ( ( tmpStr = getValByKey( &collCreateInp->condInput, COLLECTION_INFO1_KW ) ) != NULL ) {
                rstrcpy( collInfo.collInfo1, tmpStr, NAME_LEN );
            }
            if ( ( tmpStr = getValByKey( &collCreateInp->condInput, COLLECTION_INFO2_KW ) ) != NULL ) {
                rstrcpy( collInfo.collInfo2, tmpStr, NAME_LEN );
            }
        }

        status = chlRegColl( rsComm, &collInfo );
        clearKeyVal( &collInfo.condInput );
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
