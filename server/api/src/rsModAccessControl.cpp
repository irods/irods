#include "irods/rsModAccessControl.hpp"

#include "irods/catalog_utilities.hpp"
#include "irods/client_connection.hpp"
#include "irods/filesystem/path.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_logger.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/modAccessControl.h"
#include "irods/rsModAccessControl.hpp"
#include "irods/specColl.hpp"

using log_api = irods::experimental::log::api;

#define IRODS_USER_ADMINISTRATION_ENABLE_SERVER_SIDE_API
#include "irods/user_administration.hpp"

#include <fmt/format.h>

int rsModAccessControl(rsComm_t* rsComm, modAccessControlInp_t* modAccessControlInp)
{
    namespace fs = irods::experimental::filesystem;
    namespace ic = irods::experimental::catalog;
    namespace ua = irods::experimental::administration;

    if (!rsComm || !modAccessControlInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    specCollCache_t* specCollCache = nullptr;
    modAccessControlInp_t newModAccessControlInp = *modAccessControlInp;

    char newPath[MAX_NAME_LEN]{};
    rstrcpy(newPath, newModAccessControlInp.path, MAX_NAME_LEN);
    resolveLinkedPath(rsComm, newPath, &specCollCache, nullptr);
    if (strcmp(newPath, newModAccessControlInp.path)) {
        newModAccessControlInp.path = newPath;
    }

    try {
        // If the zone name cannot be derived from the collection name in the input, the path cannot be valid.
        const auto zone_name = fs::zone_name(fs::path{static_cast<char*>(newModAccessControlInp.path)});
        if (!zone_name) {
            return SYS_INVALID_FILE_PATH;
        }

        if (!isLocalZone(zone_name->data())) {
            auto* host = ic::redirect_to_catalog_provider(*rsComm, static_cast<char*>(newModAccessControlInp.path));
            return rcModAccessControl(host->conn, &newModAccessControlInp);
        }

        const auto catalog_provider_host = ic::get_catalog_provider_host();

        if (LOCAL_HOST == catalog_provider_host.localFlag) {
            ic::throw_if_catalog_provider_service_role_is_invalid();
            return _rsModAccessControl(rsComm, &newModAccessControlInp);
        }

        // Some privileged clients may be affected by temporary elevated privileges. The redirect to the catalog service
        // provider here will cause the temporary privileged status granted to this connection to be lost in the
        // server-to-server connection. The compound resource temporarily elevates permissions for users with read-only
        // access to data objects which need to be staged to cache. A client connection is made here using the service
        // account rodsadmin credentials to modify the permissions and then is immediately disconnected. This is only
        // done for connections with elevated privileges that are not rodsadmins.
        if (irods::is_privileged_client(*rsComm)) {
            const auto client_user_type =
                *ua::server::type(*rsComm, ua::user{rsComm->clientUser.userName, rsComm->clientUser.rodsZone});
            if (ua::user_type::rodsadmin != client_user_type) {
                auto conn = irods::experimental::client_connection{catalog_provider_host.hostName->name,
                                                                   rsComm->myEnv.rodsPort,
                                                                   static_cast<char*>(rsComm->myEnv.rodsUserName),
                                                                   static_cast<char*>(rsComm->myEnv.rodsZone)};

                return rcModAccessControl(static_cast<RcComm*>(conn), &newModAccessControlInp);
            }
        }

        auto* host = ic::redirect_to_catalog_provider(*rsComm);
        return rcModAccessControl(host->conn, &newModAccessControlInp);
    }
    catch (const irods::exception& e) {
        irods::log(LOG_ERROR,
                   fmt::format("[{}:{}] - caught iRODS exception [{}]", __func__, __LINE__, e.client_display_what()));
        // The irods::exceptions thrown here are constructed from error codes found in rodsErrorTable and explicit ints
        // found in structs. There will never be a narrowing conversion which affects the value here.
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        return e.code();
    }
    catch (const std::exception& e) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - caught std::exception [{}]", __func__, __LINE__, e.what()));
        return SYS_INTERNAL_ERR;
    }
    catch (...) {
        irods::log(LOG_ERROR, fmt::format("[{}:{}] - caught unknown error", __func__, __LINE__));
        return SYS_UNKNOWN_ERROR;
    }
}

int
_rsModAccessControl( rsComm_t *rsComm,
                     modAccessControlInp_t *modAccessControlInp ) {
    int status, status2;

    const char *args[MAX_NUM_OF_ARGS_IN_ACTION];
    ruleExecInfo_t rei2;
    memset( ( char* )&rei2, 0, sizeof( ruleExecInfo_t ) );
    rei2.rsComm = rsComm;
    if ( rsComm != NULL ) {
        rei2.uoic = &rsComm->clientUser;
        rei2.uoip = &rsComm->proxyUser;
    }

    char rFlag[15];
    snprintf( rFlag, sizeof( rFlag ), "%d", modAccessControlInp->recursiveFlag );
    args[0] = rFlag;
    args[1] = modAccessControlInp->accessLevel;
    args[2] = modAccessControlInp->userName;
    args[3] = modAccessControlInp->zone;
    args[4] = modAccessControlInp->path;
    int argc = 5;
    status2 = applyRuleArg( "acPreProcForModifyAccessControl", args, argc, &rei2, NO_SAVE_REI );
    if ( status2 < 0 ) {
        if ( rei2.status < 0 ) {
            status2 = rei2.status;
        }
        rodsLog( LOG_ERROR,
                 "rsModAVUMetadata:acPreProcForModifyAccessControl error for %s.%s of level %s for %s,stat=%d",
                 modAccessControlInp->zone,
                 modAccessControlInp->userName,
                 modAccessControlInp->accessLevel,
                 modAccessControlInp->path, status2 );
        return status2;
    }

    status = chlModAccessControl( rsComm,
                                  modAccessControlInp->recursiveFlag,
                                  modAccessControlInp->accessLevel,
                                  modAccessControlInp->userName,
                                  modAccessControlInp->zone,
                                  modAccessControlInp->path );

    if ( status == 0 ) {
        status2 = applyRuleArg( "acPostProcForModifyAccessControl",
                                args, argc, &rei2, NO_SAVE_REI );
        if ( status2 < 0 ) {
            if ( rei2.status < 0 ) {
                status2 = rei2.status;
            }
            rodsLog( LOG_ERROR,
                     "rsModAVUMetadata:acPostProcForModifyAccessControl error for %s.%s of level %s for %s,stat=%d",
                     modAccessControlInp->zone,
                     modAccessControlInp->userName,
                     modAccessControlInp->accessLevel,
                     modAccessControlInp->path,
                     status2 );
            return status;
        }
    }

    return status;
}
