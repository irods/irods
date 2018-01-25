/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* See genQuery.h for a description of this API call.*/

#include "rcMisc.h"
#include "getRescQuota.h"
#include "miscUtil.h"

#include "objMetaOpr.hpp"
#include "resource.hpp"
#include "rsGetRescQuota.hpp"
#include "genQuery.h"
#include "rsGenQuery.hpp"

// =-=-=-=-=-=-=-
#include "irods_resource_backport.hpp"


int
rsGetRescQuota( rsComm_t *rsComm, getRescQuotaInp_t *getRescQuotaInp,
                rescQuota_t **rescQuota ) {
    rodsServerHost_t *rodsServerHost;
    int status = 0;

    status = getAndConnRcatHost(
                 rsComm,
                 SLAVE_RCAT,
                 ( const char* )getRescQuotaInp->zoneHint,
                 &rodsServerHost );

    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        status = _rsGetRescQuota( rsComm, getRescQuotaInp, rescQuota );
    }
    else {
        status = rcGetRescQuota( rodsServerHost->conn, getRescQuotaInp,
                                 rescQuota );
    }

    return status;
}

int _rsGetRescQuota(
    rsComm_t*          rsComm,
    getRescQuotaInp_t* getRescQuotaInp,
    rescQuota_t**      rescQuota ) {
    int status = 0;

    genQueryOut_t* genQueryOut = NULL;

    if ( rescQuota == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    *rescQuota = NULL;

    status = getQuotaByResc(
                 rsComm,
                 getRescQuotaInp->userName,
                 getRescQuotaInp->rescName,
                 &genQueryOut );
    if ( status >= 0 ) {
        queRescQuota( rescQuota, genQueryOut );
    }

    freeGenQueryOut( &genQueryOut );

    return 0;

}

/* getQuotaByResc - get the quoto for an individual resource. The code is
 * mostly from doTest7 of iTestGenQuery.c
 */

int getQuotaByResc(
    rsComm_t*       rsComm,
    char*           userName,
    char*           rescName,
    genQueryOut_t** genQueryOut ) {
    int status;
    genQueryInp_t genQueryInp;
    char condition1[MAX_NAME_LEN];
    char condition2[MAX_NAME_LEN];

    if ( genQueryOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    *genQueryOut = NULL;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );

    genQueryInp.options = QUOTA_QUERY;

    snprintf(
        condition1,
        MAX_NAME_LEN, "%s",
        userName );
    addInxVal(
        &genQueryInp.sqlCondInp,
        COL_USER_NAME,
        condition1 );
    if ( rescName != NULL && strlen( rescName ) > 0 ) {
        snprintf(
            condition2,
            MAX_NAME_LEN, "%s",
            rescName );
        addInxVal(
            &genQueryInp.sqlCondInp,
            COL_R_RESC_NAME,
            condition2 );
    }

    genQueryInp.maxRows = MAX_SQL_ROWS;
    status = rsGenQuery(
                 rsComm,
                 &genQueryInp,
                 genQueryOut );
    clearGenQueryInp( &genQueryInp );

    return status;
}

int queRescQuota(
    rescQuota_t**  rescQuotaHead,
    genQueryOut_t* genQueryOut ) {

    sqlResult_t *quotaLimit, *quotaOver, *rescName, *quotaRescId, *quotaUserId;
    char *tmpQuotaLimit, *tmpQuotaOver, *tmpRescName, *tmpQuotaRescId,
         *tmpQuotaUserId;
    int i;
    rescQuota_t *tmpRescQuota;

    if ( ( quotaLimit = getSqlResultByInx( genQueryOut, COL_QUOTA_LIMIT ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "queRescQuota: getSqlResultByInx for COL_QUOTA_LIMIT failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( quotaOver = getSqlResultByInx( genQueryOut, COL_QUOTA_OVER ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queRescQuota: getSqlResultByInx for COL_QUOTA_OVER failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( rescName = getSqlResultByInx( genQueryOut, COL_R_RESC_NAME ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "queRescQuota: getSqlResultByInx for COL_R_RESC_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( quotaRescId = getSqlResultByInx( genQueryOut, COL_QUOTA_RESC_ID ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "queRescQuota: getSqlResultByInx for COL_QUOTA_RESC_ID failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( quotaUserId = getSqlResultByInx( genQueryOut, COL_QUOTA_USER_ID ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "queRescQuota: getSqlResultByInx for COL_QUOTA_USER_ID failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        tmpQuotaLimit =  &quotaLimit->value[quotaLimit->len * i];
        tmpQuotaOver =  &quotaOver->value[quotaOver->len * i];
        tmpRescName =  &rescName->value[rescName->len * i];
        tmpQuotaRescId =  &quotaRescId->value[quotaRescId->len * i];
        tmpQuotaUserId =  &quotaUserId->value[quotaUserId->len * i];
        tmpRescQuota = ( rescQuota_t* )malloc( sizeof( rescQuota_t ) );
        fillRescQuotaStruct( tmpRescQuota, tmpQuotaLimit, tmpQuotaOver,
                             tmpRescName, tmpQuotaRescId, tmpQuotaUserId );
        tmpRescQuota->next = *rescQuotaHead;
        *rescQuotaHead = tmpRescQuota;
    }

    return 0;
}

int fillRescQuotaStruct(
    rescQuota_t* rescQuota,
    char*        tmpQuotaLimit,
    char*        tmpQuotaOver,
    char*        tmpRescName,
    char*        tmpQuotaRescId,
    char*        tmpQuotaUserId ) {

    bzero( rescQuota, sizeof( rescQuota_t ) );

    rescQuota->quotaLimit = strtoll( tmpQuotaLimit, 0, 0 );
    rescQuota->quotaOverrun = strtoll( tmpQuotaOver, 0, 0 );

    if ( strtoll( tmpQuotaRescId, 0, 0 ) > 0 ) {
        /* quota by resource */
        rstrcpy( rescQuota->rescName, tmpRescName, NAME_LEN );
    }
    else {
        rescQuota->flags = GLOBAL_QUOTA;
    }

    rstrcpy( rescQuota->userId, tmpQuotaUserId, NAME_LEN );

    return 0;
}

int setRescQuota(
    rsComm_t*   _comm,
    const char* _obj_path,
    const char* _resc_name,
    rodsLong_t  _data_size ) {
    int status = chkRescQuotaPolicy( _comm );
    if ( status != RESC_QUOTA_ON ) {
        return 0;
    }

    rodsLong_t resc_id = 0;
    irods::error ret = resc_mgr.hier_to_leaf_id(_resc_name,resc_id);
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    rodsLong_t resc_overrun = 0;
    ret = irods::get_resource_property<rodsLong_t>(
                           resc_id,
                           irods::RESOURCE_QUOTA_OVERRUN,
                           resc_overrun );

    // fetch a global quota for the user if it exists
    getRescQuotaInp_t get_resc_quota_inp;
    bzero(&get_resc_quota_inp,
          sizeof( get_resc_quota_inp ) );
    rstrcpy(
        get_resc_quota_inp.zoneHint,
        _obj_path,
        MAX_NAME_LEN );
    snprintf(
        get_resc_quota_inp.userName,
        NAME_LEN, "%s#%s",
        _comm->clientUser.userName,
        _comm->clientUser.rodsZone );

    rescQuota_t *total_quota = NULL;
    status = rsGetRescQuota(
                 _comm,
                 &get_resc_quota_inp,
                 &total_quota );
    if ( status >= 0 && total_quota ) {
        rodsLong_t compare_overrun = total_quota->quotaOverrun;
        freeAllRescQuota(total_quota);
        if ( compare_overrun != 0 && // disabled if 0
             compare_overrun + resc_overrun + _data_size > 0 ) {
            return SYS_RESC_QUOTA_EXCEEDED;
        }
    } else {
        rodsLog(
            LOG_DEBUG,
            "%s rsGetRescQuota for total quota - status %d",
            __FUNCTION__,
            status );
    }

    // if no global enforcement was successful try per resource
    rstrcpy(
        get_resc_quota_inp.rescName,
        _resc_name,
        NAME_LEN );

    rescQuota_t *resc_quota = NULL;
    status = rsGetRescQuota(
                 _comm,
                 &get_resc_quota_inp,
                 &resc_quota );
    if( status < 0 || !resc_quota ) {
        return status;
    }

    rodsLong_t compare_overrun = resc_quota->quotaOverrun;
    freeAllRescQuota(resc_quota);
    if ( compare_overrun != 0 && // disabled if 0
         compare_overrun + resc_overrun + _data_size > 0 ) {
        return SYS_RESC_QUOTA_EXCEEDED;
    }

    return 0;

} // setRescQuota

int updatequotaOverrun(
    const char* _resc_hier,
    rodsLong_t  _data_size,
    int         _flags ) {
    if( ( _flags & GLB_QUOTA ) > 0 ) {
        GlobalQuotaOverrun += _data_size;
    }

    if( ( _flags & RESC_QUOTA ) > 0 ) {
        irods::hierarchy_parser parser;
        parser.set_string( _resc_hier );

        std::string root;
        parser.first_resc( root );

        rodsLong_t resc_id = 0;
        irods::error ret = resc_mgr.hier_to_leaf_id(root, resc_id);
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            return ret.code();
        }

        rodsLong_t over_run = 0;
        ret = irods::get_resource_property<rodsLong_t>(
                               resc_id,
                               irods::RESOURCE_QUOTA_OVERRUN,
                               over_run );
        // property may not be set yet so skip error
        over_run += _data_size;
        ret = irods::set_resource_property<rodsLong_t>(
                  root,
                  irods::RESOURCE_QUOTA_OVERRUN,
                  over_run );
        if( !ret.ok() ) {
            irods::log(PASS(ret));
            return ret.code();
        }
    }
    return 0;
}

int chkRescQuotaPolicy( rsComm_t *rsComm ) {
    if ( RescQuotaPolicy == RESC_QUOTA_UNINIT ) {
        ruleExecInfo_t rei;
        initReiWithDataObjInp( &rei, rsComm, NULL );
        int status = applyRule(
                         "acRescQuotaPolicy",
                         NULL, &rei,
                         NO_SAVE_REI );
        clearKeyVal(rei.condInputData);
        free(rei.condInputData);
        if ( status < 0 ) {
            rodsLog(
                LOG_ERROR,
                "queRescQuota: acRescQuotaPolicy error status = %d",
                status );
            RescQuotaPolicy = RESC_QUOTA_OFF;
        }
    }
    return RescQuotaPolicy;
}
