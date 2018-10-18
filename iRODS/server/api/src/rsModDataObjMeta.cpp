/*** Copyright (c), The Unregents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* unregDataObj.c
 */

#include "reFuncDefs.hpp"
#include "modDataObjMeta.h"
#include "icatHighLevelRoutines.hpp"
#include "objMetaOpr.hpp"
#include "dataObjOpr.hpp"

#include "irods_file_object.hpp"
#include "irods_stacktrace.hpp"

int _call_file_modified_for_modification(
    rsComm_t*         rsComm,
    modDataObjMeta_t* modDataObjMetaInp );
int
rsModDataObjMeta( rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;
    dataObjInfo_t *dataObjInfo;

    dataObjInfo = modDataObjMetaInp->dataObjInfo;

    status = getAndConnRcatHost(
                 rsComm,
                 MASTER_RCAT,
                 ( const char* )dataObjInfo->objPath,
                 &rodsServerHost );
    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }
    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsModDataObjMeta( rsComm, modDataObjMetaInp );
#else
        status = SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        // Add IN_REPL_KW to prevent replication on the redirected server (the provider)
        addKeyVal( modDataObjMetaInp->regParam, IN_REPL_KW, "" );
        status = rcModDataObjMeta( rodsServerHost->conn, modDataObjMetaInp );
        // Remove the keyword as we will want to replicate on this server (the consumer)
        rmKeyVal(modDataObjMetaInp->regParam, IN_REPL_KW);
    }

    if ( status >= 0 ) {
        char* open_type = getValByKey(modDataObjMetaInp->regParam, OPEN_TYPE_KW);
        if (!getValByKey(modDataObjMetaInp->regParam, IN_REPL_KW) && open_type &&
            (OPEN_FOR_WRITE_TYPE == std::atoi(open_type) || CREATE_TYPE == std::atoi(open_type))) {
            status = _call_file_modified_for_modification( rsComm, modDataObjMetaInp );
        }
    }

    return status;
}

int
_rsModDataObjMeta( rsComm_t *rsComm, modDataObjMeta_t *modDataObjMetaInp ) {
#ifdef RODS_CAT
    int status = 0;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;
    int i;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }
    rei2.doi = modDataObjMetaInp->dataObjInfo;
    rei2.condInputData = modDataObjMetaInp->regParam;
    regParam = modDataObjMetaInp->regParam;
    dataObjInfo = modDataObjMetaInp->dataObjInfo;

    if ( regParam->len == 0 ) {
        return 0;
    }

    /* In dataObjInfo, need just dataId. But it will accept objPath too,
     * but less efficient
     */

    /**  June 1 2009 for pre-post processing rule hooks **/
    rei2.doi = dataObjInfo;
    i =  applyRule( "acPreProcForModifyDataObjMeta", NULL, &rei2, NO_SAVE_REI );
    if ( i < 0 ) {
        if ( rei2.status < 0 ) {
            i = rei2.status;
        }
        rodsLog( LOG_ERROR, "_rsModDataObjMeta:acPreProcForModifyDataObjMeta error stat=%d", i );

        return i;
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    if ( getValByKey( regParam, ALL_KW ) != NULL ) {
        /* all copies */
        dataObjInfo_t *dataObjInfoHead = NULL;
        dataObjInfo_t *tmpDataObjInfo;
        dataObjInp_t dataObjInp;

        bzero( &dataObjInp, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
        status = getDataObjInfoIncSpecColl( rsComm, &dataObjInp, &dataObjInfoHead );

        if ( status < 0 )  {
            rodsLog( LOG_NOTICE, "%s - Failed to get data objects.", __FUNCTION__ );
            return status;
        }
        tmpDataObjInfo = dataObjInfoHead;
        while ( tmpDataObjInfo != NULL ) {
            if ( tmpDataObjInfo->specColl != NULL ) {
                break;
            }
            status = chlModDataObjMeta( rsComm, tmpDataObjInfo, regParam );
            if ( status < 0 ) {
                rodsLog( LOG_ERROR,
                         "_rsModDataObjMeta:chlModDataObjMeta %s error stat=%d",
                         tmpDataObjInfo->objPath, status );
            }
            tmpDataObjInfo = tmpDataObjInfo->next;
        }
        freeAllDataObjInfo( dataObjInfoHead );
    }
    else {
        status = chlModDataObjMeta( rsComm, dataObjInfo, regParam );
        if ( status < 0 ) {
            char* sys_error;
            char* rods_error = rodsErrorName( status, &sys_error );
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to modify the database for object \"";
            msg << dataObjInfo->objPath;
            msg << "\" - " << rods_error << " " << sys_error;
            irods::error ret = ERROR( status, msg.str() );
            irods::log( ret );
        }
    }

    /**  June 1 2009 for pre-post processing rule hooks **/
    if ( status >= 0 ) {
        i =  applyRule( "acPostProcForModifyDataObjMeta", NULL, &rei2, NO_SAVE_REI );
        if ( i < 0 ) {
            if ( rei2.status < 0 ) {
                i = rei2.status;
            }
            rodsLog( LOG_ERROR,
                     "_rsModDataObjMeta:acPostProcForModifyDataObjMeta error stat=%d", i );
            return i;
        }
    }
    else {
        rodsLog( LOG_NOTICE, "%s - Failed updating the database with object info.", __FUNCTION__ );
        return status;
    }
    /**  June 1 2009 for pre-post processing rule hooks **/

    return status;
#else
    return SYS_NO_RCAT_SERVER_ERR;
#endif

}

int _call_file_modified_for_modification(
    rsComm_t*         rsComm,
    modDataObjMeta_t* modDataObjMetaInp ) {
    int status = 0;
    dataObjInfo_t *dataObjInfo;
    keyValPair_t *regParam;
    ruleExecInfo_t rei2;

    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    rei2.uoic = &rsComm->clientUser;
    rei2.uoip = &rsComm->proxyUser;
    rei2.doi = modDataObjMetaInp->dataObjInfo;
    rei2.condInputData = modDataObjMetaInp->regParam;
    regParam = modDataObjMetaInp->regParam;
    dataObjInfo = modDataObjMetaInp->dataObjInfo;

    if ( regParam->len == 0 ) {
        return 0;
    }

    if ( getValByKey( regParam, ALL_KW ) != NULL ) {
        /* all copies */
        dataObjInfo_t *dataObjInfoHead = NULL;
        dataObjInfo_t *tmpDataObjInfo;
        dataObjInp_t dataObjInp;

        bzero( &dataObjInp, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );
        status = getDataObjInfoIncSpecColl( rsComm, &dataObjInp, &dataObjInfoHead );

        if ( status < 0 )  {
            rodsLog( LOG_NOTICE, "%s - Failed to get data objects.", __FUNCTION__ );
            return status;
        }
        tmpDataObjInfo = dataObjInfoHead;
        while ( tmpDataObjInfo != NULL ) {
            if ( tmpDataObjInfo->specColl != NULL ) {
                break;
            }

            irods::file_object_ptr file_obj(
                new irods::file_object(
                    rsComm,
                    tmpDataObjInfo ) );

            char* admin_kw = getValByKey( regParam, ADMIN_KW );
            if ( admin_kw != NULL ) {
                addKeyVal( (keyValPair_t*)&file_obj->cond_input(), ADMIN_KW, "" );
            }

            char* pdmo_kw = getValByKey( regParam, IN_PDMO_KW );
            if ( pdmo_kw != NULL ) {
                file_obj->in_pdmo( pdmo_kw );
            }

            char* open_type = getValByKey(regParam, OPEN_TYPE_KW);
            if ( open_type != NULL ) {
                addKeyVal( (keyValPair_t*)&file_obj->cond_input(), OPEN_TYPE_KW, open_type );
            }

            irods::error ret = fileModified( rsComm, file_obj );
            if ( !ret.ok() ) {
                std::stringstream msg;
                msg << __FUNCTION__;
                msg << " - Failed to signal resource that the data object \"";
                msg << tmpDataObjInfo->objPath;
                msg << " was modified.";
                ret = PASSMSG( msg.str(), ret );
                irods::log( ret );
                status = ret.code();
            }

            tmpDataObjInfo = tmpDataObjInfo->next;
        }
        freeAllDataObjInfo( dataObjInfoHead );
    }
    else {
        dataObjInp_t dataObjInp;
        bzero( &dataObjInp, sizeof( dataObjInp ) );
        rstrcpy( dataObjInp.objPath, dataObjInfo->objPath, MAX_NAME_LEN );

        // Construct file_obj twice because ctor gives some info that factory does not
        irods::file_object_ptr file_obj(new irods::file_object(rsComm, dataObjInfo));
        irods::error fac_err = file_object_factory( rsComm, &dataObjInp, file_obj);
        // Factory overwrites rescHier with the resource which holds replica 0 - put it back
        file_obj->resc_hier(dataObjInfo->rescHier);

        char* admin_kw = getValByKey( regParam, ADMIN_KW );
        if ( admin_kw != NULL ) {
            addKeyVal( (keyValPair_t*)&file_obj->cond_input(), ADMIN_KW, "" );
        }

        char* pdmo_kw = getValByKey( regParam, IN_PDMO_KW );
        if ( pdmo_kw != NULL ) {
            file_obj->in_pdmo( pdmo_kw );
        }

        char* open_type = getValByKey(regParam, OPEN_TYPE_KW);
        if ( open_type != NULL ) {
            addKeyVal( (keyValPair_t*)&file_obj->cond_input(), OPEN_TYPE_KW, open_type );
        }
        irods::error ret = fileModified( rsComm, file_obj );
        if ( !ret.ok() ) {
            std::stringstream msg;
            msg << __FUNCTION__;
            msg << " - Failed to signal the resource that the data object \"";
            msg << dataObjInfo->objPath;
            msg << "\" was modified.";
            ret = PASSMSG( msg.str(), ret );
            irods::log( ret );
            status = ret.code();
        }

    }

    return status;

}

