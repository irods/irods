/**
 * @file  icatGeneralMS.cpp
 *
 */

#include "rcMisc.h"
#include "icatHighLevelRoutines.hpp"
#include "objMetaOpr.hpp"
#include "miscServerFunct.hpp"
#include "modAccessControl.h"
#include "irods_configuration_keywords.hpp"
#include "rsModAccessControl.hpp"
#include "irods_re_structs.hpp"

/**
 * \fn msiGetIcatTime (msParam_t *timeOutParam, msParam_t *typeInParam, ruleExecInfo_t *)
 *
 * \brief   This function returns the system time for the iCAT server
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[out] timeOutParam - a msParam of type STR_MS_T
 * \param[in] typeInParam - a msParam of type STR_MS_T
 *    \li "icat" or "unix" will return seconds since epoch
 *    \li otherwise, human friendly
 * \param[in,out] - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiGetIcatTime( msParam_t* timeOutParam, msParam_t* typeInParam, ruleExecInfo_t* ) {
    char *type;
    char tStr0[TIME_LEN], tStr[TIME_LEN];

    type = ( char* )typeInParam->inOutStruct;

    if ( !strcmp( type, "icat" ) || !strcmp( type, "unix" ) ) {
        getNowStr( tStr );
    }
    else { /* !strcmp(type,"human") */
        getNowStr( tStr0 );
        getLocalTimeFromRodsTime( tStr0, tStr );
    }
    fillStrInMsParam( timeOutParam, tStr );
    return 0;
}

/**
 * \fn msiQuota (ruleExecInfo_t *rei)
 *
 * \brief  Calculates storage usage and sets quota values (over/under/how-much).
 *
 * \module core
 *
 * \since pre-2.3
 *
 *
 * \note Causes the iCAT quota tables to be updated.
 *
 * \note This is run via an admin rule
 *
 * \usage See clients/icommands/test/rules/ and https://wiki.irods.org/index.php/Quotas
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence Utilizes ICAT data-object information
 * \iCatAttrModified Updates the quota tables
 * \sideeffect none
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiQuota( ruleExecInfo_t *rei ) {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if (irods::CFG_SERVICE_ROLE_PROVIDER != svc_role) {
        return SYS_NO_RCAT_SERVER_ERR;
    }
    rodsLog( LOG_NOTICE, "msiQuota/chlCalcUsageAndQuota called\n" );
    return chlCalcUsageAndQuota( rei->rsComm );
}

/**
 *\fn msiSetResource (msParam_t *xrescName, ruleExecInfo_t *rei)
 *
 * \brief   This microservice sets the resource as part of a workflow execution.
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note  This microservice sets the resource as part of a workflow execution.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] xrescName - is a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int  msiSetResource( msParam_t* xrescName, ruleExecInfo_t *rei ) {
    char *rescName;

    rescName = ( char * ) xrescName->inOutStruct;

    snprintf( rei->doi->rescName, sizeof( rei->doi->rescName ), "%s", rescName );
    return 0;
}


/**
 * \fn msiCheckOwner (ruleExecInfo_t *rei)
 *
 * \brief   This microservice checks whether the user is the owner
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int msiCheckOwner( ruleExecInfo_t *rei ) {
    if ( !strcmp( rei->doi->dataOwnerName, rei->uoic->userName ) &&
            !strcmp( rei->doi->dataOwnerZone, rei->uoic->rodsZone ) ) {
        return 0;
    }
    else {
        return ACTION_FAILED_ERR;
    }

}

/**
 * \fn msiCheckPermission (msParam_t *xperm, ruleExecInfo_t *rei)
 *
 * \brief   This microservice checks the authorization permission (whether or not permission is granted)
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] xperm - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int msiCheckPermission( msParam_t* xperm, ruleExecInfo_t *rei ) {
    char *perm;

    perm = ( char * ) xperm->inOutStruct;
    if ( strstr( rei->doi->dataAccess, perm ) != NULL ) {
        return 0;
    }
    else {
        return ACTION_FAILED_ERR;
    }

}


/**
 * \fn msiCheckAccess(msParam_t *inObjName, msParam_t * inOperation, msParam_t * outResult, ruleExecInfo_t *rei)
 *
 * \brief   This microservice checks the access permission to perform a given operation
 *
 * \module core
 *
 * \since 3.0
 *
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] inObjName - name of Object. A param of type STR_MS_T
 * \param[in] inOperation - type of Operation that will be performed. A param of type STR_MS_T.
 * \param[out] outResult - result of the operation. 0 for failure and 1 for success. a param of type INT_MS_T.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int msiCheckAccess( msParam_t *inObjName, msParam_t * inOperation,
                    msParam_t * outResult, ruleExecInfo_t *rei ) {
    char *objName, *oper;
    char objType[MAX_NAME_LEN];
    int i = 0;
    char *user;
    char *zone;

    RE_TEST_MACRO( "  Calling msiCheckAccess" );

    if ( inObjName == NULL || inObjName->inOutStruct == NULL ||
            inObjName->type == NULL || strcmp( inObjName->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( inOperation == NULL || inOperation->inOutStruct == NULL ||
            inOperation->type == NULL || strcmp( inOperation->type, STR_MS_T ) != 0 ) {
        return USER_PARAM_TYPE_ERR;
    }

    if ( rei == NULL || rei->rsComm == NULL ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    if ( strlen( rei->rsComm->clientUser.userName ) == 0 ||
            strlen( rei->rsComm->clientUser.rodsZone ) == 0 ) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    oper = ( char * ) inOperation->inOutStruct;
    objName = ( char * ) inObjName->inOutStruct;
    user = rei->rsComm->clientUser.userName;
    zone = rei->rsComm->clientUser.rodsZone;

    i = getObjType( rei->rsComm, objName, objType );
    if ( i < 0 ) {
        return i;
    }

    i = checkPermissionByObjType( rei->rsComm, objName, objType, user, zone, oper );

    if ( i < 0 ) {
        return i;
    }
    fillIntInMsParam( outResult, i );

    return 0;

}


/**
 * \fn msiCommit (ruleExecInfo_t *rei)
 *
 * \brief This microservice commits pending database transactions,
 * registering the new state information into the iCAT.
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \note This is used to commit changes (in any) into the iCAT
 * database as part of a rule and microservice chain.  See core.re
 * for examples.  In other cases, iCAT updates and inserts are
 * automatically committed into the iCAT Database as part of the
 * normal operations (in the 'C' code).
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified  none
 * \iCatAttrDependence commits pending updates (if any)
 * \iCatAttrModified pending updates (if any) are committed into the iCAT db
 * \sideeffect none
 *
 * \return integer
 * \retval (status)
 * \pre none
 * \post none
 * \sa none
**/
int
msiCommit( ruleExecInfo_t *rei ) {
    int status;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        status = chlCommit( rei->rsComm );
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        status =  SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
    return status;
}

/**
 * \fn msiRollback (ruleExecInfo_t *rei)
 *
 * \brief   This function deletes user and collection information from the iCAT by rolling back the database transaction
 *
 * \module core
 *
 * \since pre-2.1
 *
 * \note This is used to not-commit changes into the iCAT database as
 * part of a rule and microservice chain.  See core.re for examples.
 * In other cases, iCAT updates and inserts are automatically
 * rolled-back as part of the normal operations.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence none
 * \DolVarModified none
 * \iCatAttrDependence pending updates (if any) are canceled
 * \iCatAttrModified  pending updates (if any) are canceled
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre none
 * \post none
 * \sa none
**/
int
msiRollback( ruleExecInfo_t *rei ) {
    int status;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        status = chlRollback( rei->rsComm );
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        status =  SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
    return status;
}

/**
 * \fn msiSetACL (msParam_t *recursiveFlag, msParam_t *accessLevel, msParam_t *userName, msParam_t *pathName, ruleExecInfo_t *rei)
 *
 * \brief   This microservice changes the ACL for a given pathname,
 *            either a collection or a data object.
 *
 * \module core
 *
 * \since 2.3
 *
 *
 * \note This microservice modifies the access rights on a given iRODS object or
 *    collection. For the collections, the modification can be recursive and the
 *    inheritence bit can be changed as well.
 *    For admin mode, add MOD_ADMIN_MODE_PREFIX to the access level string,
 *    e.g: msiSetACL("default", "admin:read", "rods", *path)
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] recursiveFlag - a STR_MS_T, either "default" or "recursive".  "recursive"
 *    is only relevant if set with accessLevel set to "inherit".
 * \param[in] accessLevel - a STR_MS_T containing one of the following:
 *    \li read
 *    \li write
 *    \li own
 *    \li inherit
 *    \li null
 * \param[in] userName - a STR_MS_T, the user name or group name who will have ACL changed.
 * \param[in] pathName - a STR_MS_T, the collection or data object that will have its ACL changed.
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence $userName and/or $objPath and/or $collName
 * \DolVarModified none
 * \iCatAttrDependence none
 * \iCatAttrModified none
 * \sideeffect none
 *
 * \return integer
 * \retval 0 on success
 * \pre N/A
 * \post N/A
 * \sa N/A
**/
int msiSetACL( msParam_t *recursiveFlag, msParam_t *accessLevel, msParam_t *userName,
               msParam_t *pathName, ruleExecInfo_t *rei ) {
    char *acl, *path, *recursiveFlg, *user, uname[NAME_LEN], *zone;
    int recFlg, rc;
    modAccessControlInp_t modAccessControlInp;
    rsComm_t *rsComm = 0; // JMC cppcheck - uninit var

    RE_TEST_MACRO( "    Calling msiSetACL" )
    /* the above line is needed for loop back testing using irule -i option */

    if ( recursiveFlag == NULL || accessLevel == NULL || userName == NULL ||
            pathName == NULL ) {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSetACL: one of the input parameter is NULL" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    recFlg = 0; /* non recursive mode */
    if ( strcmp( recursiveFlag->type, STR_MS_T ) == 0 ) {
        recursiveFlg = ( char * ) recursiveFlag->inOutStruct;
        if ( strcmp( recursiveFlg, "recursive" ) == 0 ) {
            /* recursive mode */
            recFlg = 1;
        }
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSetACL: Unsupported input recursiveFlag type %i",
                            recursiveFlag->type );
        return USER_PARAM_TYPE_ERR;
    }

    if ( strcmp( accessLevel->type, STR_MS_T ) == 0 ) {
        acl = ( char * ) accessLevel->inOutStruct;
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSetACL: Unsupported input accessLevel type %s",
                            accessLevel->type );
        return USER_PARAM_TYPE_ERR;
    }

    if ( strcmp( userName->type, STR_MS_T ) == 0 ) {
        user = ( char * ) userName->inOutStruct;
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSetACL: Unsupported input userName type %s",
                            userName->type );
        return USER_PARAM_TYPE_ERR;
    }

    if ( strcmp( pathName->type, STR_MS_T ) == 0 ) {
        path = ( char * ) pathName->inOutStruct;
    }
    else {
        rodsLogAndErrorMsg( LOG_ERROR, &rsComm->rError, rei->status,
                            "msiSetACL: Unsupported input pathName type %s",
                            pathName->type );
        return USER_PARAM_TYPE_ERR;
    }

    rsComm = rei->rsComm;
    modAccessControlInp.recursiveFlag = recFlg;
    modAccessControlInp.accessLevel = acl;
    if ( strchr( user, '#' ) == NULL ) {
        modAccessControlInp.userName = user;
        modAccessControlInp.zone = rei->uoic->rodsZone;
    }
    else {
        zone = strchr( user, '#' ) + 1;
        memset( uname, '\0', NAME_LEN );
        strncpy( uname, user, strlen( user ) - strlen( zone ) - 1 );
        modAccessControlInp.userName = uname;
        modAccessControlInp.zone = zone;
    }
    modAccessControlInp.path = path;
    rc = rsModAccessControl( rsComm, &modAccessControlInp );
    if ( rc < 0 ) {
        rodsLog( LOG_NOTICE, "msiSetACL: ACL modifications has failed for user %s on object %s, error = %i\n", user, path, rc );
    }

    return rc;
}

/**
 * \fn msiDeleteUnusedAVUs (ruleExecInfo_t *rei)
 *
 * \brief   This microservice deletes unused AVUs from the iCAT.  See 'iadmin rum'.
 *
 * \module  core
 *
 * \since   post-2.3
 *
 *
 * \note See 'iadmin help rum'.  Do not call this directly.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence check AVU table
 * \iCatAttrModified update AVU table
 * \sideeffect none
 *
 * \return integer
 * \retval (status)
 * \pre none
 * \post none
 * \sa none
**/
int
msiDeleteUnusedAVUs( ruleExecInfo_t *rei ) {
    int status;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        rodsLog( LOG_NOTICE, "msiDeleteUnusedAVUs/chlDelUnusedAVUs called\n" );
        status = chlDelUnusedAVUs( rei->rsComm );
    } else if( irods::CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        status =  SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        status = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    return status;
}
