/// \file

#include "irods/icatHighLevelRoutines.hpp"
#include "irods/msParam.h"
#include "irods/rcMisc.h"
#include "irods/generalAdmin.h"
#include "irods/miscServerFunct.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/rodsErrorTable.h"
#include "irods/rsGeneralAdmin.hpp"
#include "irods/irods_re_structs.hpp"
#include "irods/irods_logger.hpp"

#define IRODS_FILESYSTEM_ENABLE_SERVER_SIDE_API
#include "irods/filesystem.hpp"

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <cstring>
#include <string>
#include <string_view>

namespace
{
    using log_re = irods::experimental::log::rule_engine;
} // anonymous namespace

/**
 * \fn msiCreateUser (ruleExecInfo_t *rei)
 *
 * \brief   This microservice creates a new user
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \note From the irods wiki: https://wiki.irods.org/index.php/Rules
 *
 * \note The "acCreateUser" rule, for example, calls "msiCreateUser".
 * If that succeeds it invokes the "acCreateDefaultCollections" rule (which calls other rules and msi routines).
 * Then, if they all succeed, the "msiCommit" function is called to save persistent state information.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval i
 * \pre none
 * \post none
 * \sa none
 **/
int msiCreateUser( ruleExecInfo_t *rei ) {
    int i;

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        i =  chlRegUserRE( rei->rsComm, rei->uoio );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        i =  SYS_NO_ICAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        i = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
    return i;
}

/**
 * \fn msiCreateCollByAdmin (msParam_t *xparColl, msParam_t *xchildName, ruleExecInfo_t *rei)
 *
 * \brief   This microservice creates a collection by an administrator
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] xparColl - a msParam of type STR_MS_T
 * \param[in] xchildName - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval (i)
 * \pre none
 * \post none
 * \sa none
 **/
int msiCreateCollByAdmin( msParam_t* xparColl, msParam_t* xchildName, ruleExecInfo_t *rei ) {
    int i;
    collInfo_t collInfo;
    char *parColl;
    char *childName;

    parColl = ( char * ) xparColl->inOutStruct;
    childName = ( char * ) xchildName->inOutStruct;
    memset( &collInfo, 0, sizeof( collInfo_t ) );
    snprintf( collInfo.collName, sizeof( collInfo.collName ),
              "%s/%s", parColl, childName );
    snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ),
              "%s", rei->uoio->userName );
    snprintf( collInfo.collOwnerZone, sizeof( collInfo.collOwnerZone ),
              "%s", rei->uoio->rodsZone );

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        i =  chlRegCollByAdmin( rei->rsComm, &collInfo );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        i =  SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        i = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
    return i;
}

/**
 * \fn msiDeleteCollByAdmin (msParam_t *xparColl, msParam_t *xchildName, ruleExecInfo_t *rei)
 *
 * \brief   This microservice deletes a collection by administrator
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] xparColl - a msParam of type STR_MS_T
 * \param[in] xchildName - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval (i)
 * \pre none
 * \post none
 * \sa none
 **/
int msiDeleteCollByAdmin( msParam_t* xparColl, msParam_t* xchildName, ruleExecInfo_t *rei ) {
    int i;
    collInfo_t collInfo;
    memset(&collInfo, 0, sizeof(collInfo));
    char *parColl;
    char *childName;

    parColl = ( char * ) xparColl->inOutStruct;
    childName = ( char * ) xchildName->inOutStruct;

    snprintf( collInfo.collName, sizeof( collInfo.collName ),
              "%s/%s", parColl, childName );
    snprintf( collInfo.collOwnerName, sizeof( collInfo.collOwnerName ),
              "%s", rei->uoio->userName );
    snprintf( collInfo.collOwnerZone, sizeof( collInfo.collOwnerZone ),
              "%s", rei->uoio->rodsZone );

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        i = chlDelCollByAdmin( rei->rsComm, &collInfo );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        i = SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        i = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    if ( i == CAT_UNKNOWN_COLLECTION ) {
        /* Not sure where this kind of logic belongs, chl, rules,
           or here; but for now it's here.  */
        /* If the error is that it does not exist, return OK. */
        freeRErrorContent( &rei->rsComm->rError ); /* remove suberrors if any */
        return 0;
    }
    return i;
}

/**
 * \fn msiDeleteUser (ruleExecInfo_t *rei)
 *
 * \brief   This microservice deletes a user
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser.authFlag (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval (i)
 * \pre none
 * \post none
 * \sa none
 **/
int
msiDeleteUser( ruleExecInfo_t *rei ) {
    int i;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        i =  chlDelUserRE( rei->rsComm, rei->uoio );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        i = SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        i = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }
    return i;
}

/**
 * \fn msiAddUserToGroup (msParam_t *msParam, ruleExecInfo_t *rei)
 *
 * \brief   This microservice adds a user to a group
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] msParam - a msParam of type STR_MS_T, the name of the group
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser (must be group admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval (i)
 * \pre none
 * \post none
 * \sa none
 **/
int
msiAddUserToGroup( msParam_t *msParam, ruleExecInfo_t *rei ) {
    int i;
    char *groupName;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        if ( strncmp( rei->uoio->userType, "rodsgroup", 9 ) == 0 ) {
            return 0;
        }
        groupName = ( char * ) msParam->inOutStruct;
        i =  chlModGroup( rei->rsComm, groupName, "add", rei->uoio->userName,
                          rei->uoio->rodsZone );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        i = SYS_NO_RCAT_SERVER_ERR;
    } else {
        rodsLog(
            LOG_ERROR,
            "role not supported [%s]",
            svc_role.c_str() );
        i = SYS_SERVICE_ROLE_NOT_SUPPORTED;
    }

    return i;
}

/// Removes a user from a group.
///
/// \param[in] _group     The name of the group to remove the user from.
/// \param[in] _user_name The part of a fully-qualified username preceding the pound sign.
/// \param[in] _user_zone The part of a fully-qualified username following the pound sign. If empty,
///                       it defaults to the local zone.
/// \param[in] _rei       Automatically handled by the rule engine plugin framework. This parameter
///                       is not visible to callers of the microservice and should therefore be
///                       ignored.
///
/// \return An integer.
/// \retval  0 On success.
/// \retval <0 On failure.
///
/// \since 4.3.1
auto msiRemoveUserFromGroup(MsParam* _group, MsParam* _user_name, MsParam* _user_zone, RuleExecInfo* _rei) -> int
{
    if (!_group || !_user_name || !_rei) {
        log_re::error("{}: Received one or more null pointers as input.", __func__);
        return SYS_INVALID_INPUT_PARAM;
    }

    // NOLINTNEXTLINE(bugprone-lambda-function-name)
    const auto is_msp_type_valid = [fn = __func__](const MsParam* _msp, const std::string_view _param_name) {
        if (!_msp->type) {
            log_re::error("{}: Missing type information for {} parameter.", fn, _param_name);
            return false;
        }

        if (std::strcmp(_msp->type, STR_MS_T) != 0) {
            log_re::error("{}: Incorrect type for {} parameter. Expected a string.", fn, _param_name);
            return false;
        }

        return true;
    };

    // clang-format off
    if (!is_msp_type_valid(_group, "group") ||
        !is_msp_type_valid(_user_name, "user_name") ||
        !is_msp_type_valid(_user_zone, "user zone"))
    {
        return SYS_INVALID_INPUT_PARAM;
    }
    // clang-format on

    try {
        namespace adm = irods::experimental::administration;

        const auto to_c_str = [](const MsParam& _msp) { return static_cast<const char*>(_msp.inOutStruct); };

        const adm::group group{to_c_str(*_group)};
        const adm::user user{to_c_str(*_user_name), to_c_str(*_user_zone)};

        adm::server::remove_user_from_group(*_rei->rsComm, group, user);

        return 0;
    }
    catch (const irods::exception& e) {
        log_re::error("{}: Failed to remove user from group. [{}]", __func__, e.client_display_what());
        return e.code(); // NOLINT(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
    }
    catch (...) {
        log_re::error("{}: Failed to remove user from group.", __func__);
        return SYS_UNKNOWN_ERROR;
    }
} // msiRemoveUserFromGroup

/**
 * \fn msiRenameLocalZone (msParam_t *oldName, msParam_t *newName, ruleExecInfo_t *rei)
 *
 * \brief   This microservice renames the local zone by updating various tables
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] oldName - a msParam of type STR_MS_T
 * \param[in] newName - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser (must be admin)
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval status
 * \pre none
 * \post none
 * \sa none
 **/
int
msiRenameLocalZone( msParam_t* oldName, msParam_t* newName, ruleExecInfo_t *rei ) {
    int status;
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        char *oldNameStr;
        char *newNameStr;

        oldNameStr = ( char * ) oldName->inOutStruct;
        newNameStr = ( char * ) newName->inOutStruct;
        status = chlRenameLocalZone( rei->rsComm, oldNameStr, newNameStr );
    } else if( irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role ) {
        status = SYS_NO_RCAT_SERVER_ERR;
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
 * \fn msiRenameCollection (msParam_t *oldName, msParam_t *newName, ruleExecInfo_t *rei)
 *
 * \brief   This function renames a collection; used via a Rule with #msiRenameLocalZone
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * This is called via an 'iadmin' command.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] oldName - a msParam of type STR_MS_T
 * \param[in] newName - a msParam of type STR_MS_T
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->rsComm->clientUser (must have access (admin))
 * \DolVarModified none
 * \iCatAttrDependence checks various tables
 * \iCatAttrModified updates various tables
 * \sideeffect none
 *
 * \return integer
 * \retval status
 * \pre none
 * \post none
 * \sa none
 **/
int msiRenameCollection(msParam_t* oldName, msParam_t* newName, ruleExecInfo_t *rei)
{
    std::string svc_role;

    if (irods::error ret = get_catalog_service_role(svc_role); !ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    namespace log = irods::experimental::log;

    if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
        namespace fs = irods::experimental::filesystem;

        const fs::path src_path = static_cast<const char*>(oldName->inOutStruct);
        const fs::path dst_path = static_cast<const char*>(newName->inOutStruct);

        log::microservice::debug("{} :: src_path={}, dst_path={}", __func__, src_path.c_str(), dst_path.c_str());

        try {
            if (!fs::server::is_collection(*rei->rsComm, src_path)) {
                return NOT_A_COLLECTION;
            }

            if (const auto s = fs::server::status(*rei->rsComm, dst_path);
                fs::server::exists(s) && !fs::server::is_collection(s))
            {
                return NOT_A_COLLECTION;
            }

            fs::server::rename(*rei->rsComm, src_path, dst_path);

            return 0;
        }
        catch (const fs::filesystem_error& e) {
            log::microservice::error("msiRenameCollection failed: [{}]", e.what());
            return e.code().value();
        }
    }

    if (irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
        return SYS_NO_RCAT_SERVER_ERR;
    }

    log::microservice::error("role not supported [{}]", svc_role);

    return SYS_SERVICE_ROLE_NOT_SUPPORTED;
}

/// Renames the local zone collection.
///
/// \param[in] _new_zone_name The new name for the zone collection. The new name must not
///                           exist.
/// \param[in] _rei           The ::RuleExecInfo managed by the system. This parameter
///                           must be ignored.
///
/// \return An integer.
/// \retval 0        On success.
/// \retval Non-zero Otherwise.
///
/// \since 4.2.9
int msiRenameLocalZoneCollection(msParam_t* _new_zone_name, ruleExecInfo_t* _rei)
{
    namespace log = irods::experimental::log;

    std::string svc_role;

    if (irods::error ret = get_catalog_service_role(svc_role); !ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if (irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role) {
        namespace fs = irods::experimental::filesystem;

        const fs::path root = "/";
        const auto src_path = root / _rei->rsComm->myEnv.rodsZone;
        const auto* new_zone_name = static_cast<const char*>(_new_zone_name->inOutStruct);

        log::microservice::debug("{} :: src_path={}, new_zone_name={}", __func__, src_path.c_str(), new_zone_name);

        try {
            if (src_path == root / new_zone_name) {
                return 0;
            }

            if (fs::server::exists(*_rei->rsComm, root / new_zone_name)) {
                return CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME;
            }

            const auto ec = chlRenameColl(_rei->rsComm, src_path.c_str(), new_zone_name);

            if (ec < 0) {
                chlRollback(_rei->rsComm);
            }
            else {
                chlCommit(_rei->rsComm);
            }

            return ec;
        }
        catch (const fs::filesystem_error& e) {
            log::microservice::error("{} :: Failed to rename local zone collection.", __func__);
            return e.code().value();
        }
    }

    if (irods::KW_CFG_SERVICE_ROLE_CONSUMER == svc_role) {
        return SYS_NO_RCAT_SERVER_ERR;
    }

    log::microservice::error("role not supported [{}]", svc_role);

    return SYS_SERVICE_ROLE_NOT_SUPPORTED;
}

/**
 * \fn msiAclPolicy(msParam_t *msParam, ruleExecInfo_t *)
 *
 * \brief   When called (e.g. from acAclPolicy) and with "STRICT" as the
 *    argument, this will set the ACL policy (for GeneralQuery) to be
 *    extended (most strict).
 *
 * \module core
 *
 * \since pre-2.1
 *
 *
 * \note Should not be used outside of the rules defined in core.re.
 * Once set STRICT, strict mode remains in force (users can't call it in
 * another rule to change the mode back to non-strict).
 * See core.re.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] msParam - a msParam of type STR_MS_T - can have value 'STRICT'
 * \param[in,out] - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence
 * \DolVarModified
 * \iCatAttrDependence
 * \iCatAttrModified
 * \sideeffect none
 *
 * \return integer
 * \retval status
 * \pre none
 * \post none
 * \sa none
 **/
int
msiAclPolicy( msParam_t* msParam, ruleExecInfo_t* ) {
    char *inputArg;

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    std::string strict = "off";
    inputArg = ( char * ) msParam->inOutStruct;
    if ( inputArg != NULL ) {
        if ( strncmp( inputArg, "STRICT", 6 ) == 0 ) {
            if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
                chlGenQueryAccessControlSetup( NULL, NULL, NULL, 0, 2 );
                strict = "on";
            }
        }
    }
    else {
        if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
            chlGenQueryAccessControlSetup( NULL, NULL, NULL, 0, 0 );
        }
    }

    // set a strict acl prop
    try {
        irods::set_server_property<std::string>(irods::STRICT_ACL_KW, strict);
    } catch ( irods::exception& e ) {
        irods::log(e);
        return e.code();
    }

    return 0;
}



/**
 * \fn msiSetQuota(msParam_t *type, msParam_t *name, msParam_t *resource, msParam_t *value, ruleExecInfo_t *rei)
 *
 * \brief Sets disk usage quota for a user or group
 *
 * \module core
 *
 * \since 3.0.x
 *
 *
 *
 * \note  This microservice sets a disk usage quota for a given user or group.
 *          If no resource name is provided the quota will apply across all resources.
 *
 * \usage See clients/icommands/test/rules/
 *
 * \param[in] type - a STR_MS_T - Can be either "user" or "group"
 * \param[in] name - a STR_MS_T with the name of the user or group
 * \param[in] resource - Optional - a STR_MS_T with the name of the resource where
 *      the quota will apply, or "total" for the quota to be system-wide.
 * \param[in] value - an INT_MST_T or DOUBLE_MS_T or STR_MS_T with the quota (in bytes)
 * \param[in,out] rei - The RuleExecInfo structure that is automatically
 *    handled by the rule engine. The user does not include rei as a
 *    parameter in the rule invocation.
 *
 * \DolVarDependence rei->uoic->authInfo.authFlag must be >= 5 (local admin)
 * \DolVarModified None
 * \iCatAttrDependence None
 * \iCatAttrModified Updates r_quota_main
 * \sideeffect None
 *
 * \return integer
 * \retval 0 on success
 * \pre None
 * \post None
 * \sa None
 **/
int
msiSetQuota( msParam_t *type, msParam_t *name, msParam_t *resource, msParam_t *value, ruleExecInfo_t *rei ) {
    generalAdminInp_t generalAdminInp;              /* Input for rsGeneralAdmin */
    char quota[21];
    int status;


    /* For testing mode when used with irule --test */
    RE_TEST_MACRO( "    Calling msiSetQuota" )

    /* Sanity checks */
    if ( rei == NULL || rei->rsComm == NULL ) {
        rodsLog( LOG_ERROR, "msiSetQuota: input rei or rsComm is NULL." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }


    /* Must be called from an admin account */
    if ( rei->uoic->authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        status = CAT_INSUFFICIENT_PRIVILEGE_LEVEL;
        rodsLog( LOG_ERROR, "msiSetQuota: User %s is not local admin. Status = %d",
                 rei->uoic->userName, status );
        return status;
    }


    /* Prepare generalAdminInp. It needs to be set up as follows:
     *    generalAdminInp.arg0: "set-quota"
     *    generalAdminInp.arg1: type ("user" or "group")
     *    generalAdminInp.arg2: name of user/group
     *    generalAdminInp.arg3: resource name or "total"
     *    generalAdminInp.arg4: quota value
     *    generalAdminInp.arg5: ""
     *    generalAdminInp.arg6: ""
     *    generalAdminInp.arg7: ""
     *    generalAdminInp.arg8: ""
     *    generalAdminInp.arg9: ""
     */
    memset( &generalAdminInp, 0, sizeof( generalAdminInp_t ) );
    generalAdminInp.arg0 = "set-quota";

    /* Parse type */
    if ( ( generalAdminInp.arg1 = parseMspForStr( type ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiSetQuota: Null user or group type provided." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }
    if ( strcmp( generalAdminInp.arg1, "user" ) && strcmp( generalAdminInp.arg1, "group" ) ) {
        status = USER_BAD_KEYWORD_ERR;
        rodsLog( LOG_ERROR, "msiSetQuota: Invalid user type: %s. Valid types are \"user\" and \"group\"",
                 generalAdminInp.arg1 );
        return status;
    }

    /* Parse user/group name */
    if ( ( generalAdminInp.arg2 = parseMspForStr( name ) ) == NULL ) {
        rodsLog( LOG_ERROR, "msiSetQuota: Null user or group name provided." );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    /* parse resource name */
    if ( ( generalAdminInp.arg3 = parseMspForStr( resource ) ) == NULL ) {
        generalAdminInp.arg3 = "total";
    }

    /* Parse value */
    if ( value->type && !strcmp( value->type, STR_MS_T ) ) {
        generalAdminInp.arg4 = ( char * )value->inOutStruct;
    }
    else if ( value->type && !strcmp( value->type, INT_MS_T ) ) {
        snprintf( quota, 11, "%d", *( int * )value->inOutStruct );
        generalAdminInp.arg4 = quota;
    }
    else if ( value->type && !strcmp( value->type, DOUBLE_MS_T ) ) {
        snprintf( quota, 21, "%lld", *( rodsLong_t * )value->inOutStruct );
        generalAdminInp.arg4 = quota;
    }
    else {
        status = USER_PARAM_TYPE_ERR;
        rodsLog( LOG_ERROR, "msiSetQuota: Invalid type for param value. Status = %d", status );
        return status;
    }

    /* Fill up the rest of generalAdminInp */
    generalAdminInp.arg5 = "";
    generalAdminInp.arg6 = "";
    generalAdminInp.arg7 = "";
    generalAdminInp.arg8 = "";
    generalAdminInp.arg9 = "";


    /* Call rsGeneralAdmin */
    status = rsGeneralAdmin( rei->rsComm, &generalAdminInp );


    /* Done */
    return status;
}
