#include "irods/catalog_utilities.hpp"
#include "irods/client_connection.hpp"
#include "irods/filesystem/path.hpp"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_rs_comm_query.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/modColl.h"
#include "irods/rcMisc.h"
#include "irods/rsModColl.hpp"

namespace
{
    using log_api = irods::experimental::log::api;

    auto _rsModColl(rsComm_t* rsComm, collInp_t* modCollInp) -> int
    {
        ruleExecInfo_t rei2{};
        if (rsComm) { // NOLINT(readability-implicit-bool-conversion)
            rei2.rsComm = rsComm;
            rei2.uoic = &rsComm->clientUser;
            rei2.uoip = &rsComm->proxyUser;
        }

        collInfo_t collInfo{};
        rstrcpy(static_cast<char*>(collInfo.collName), static_cast<char*>(modCollInp->collName), MAX_NAME_LEN);

        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (const char* tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_TYPE_KW); tmpStr) {
            rstrcpy(static_cast<char*>(collInfo.collType), tmpStr, NAME_LEN);
        }

        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (const char* tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_INFO1_KW); tmpStr) {
            rstrcpy(static_cast<char*>(collInfo.collInfo1), tmpStr, MAX_NAME_LEN);
        }

        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (const char* tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_INFO2_KW); tmpStr) {
            rstrcpy(static_cast<char*>(collInfo.collInfo2), tmpStr, MAX_NAME_LEN);
        }

        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if (const char* tmpStr = getValByKey(&modCollInp->condInput, COLLECTION_MTIME_KW); tmpStr) {
            rstrcpy(static_cast<char*>(collInfo.collModify), tmpStr, TIME_LEN);
        }

        rei2.coi = &collInfo;
        if (int ec = applyRule("acPreProcForModifyCollMeta", nullptr, &rei2, NO_SAVE_REI); ec < 0) {
            if (rei2.status < 0) {
                ec = rei2.status;
            }
            log_api::error("acPreProcForModifyCollMeta error for {},stat={}", modCollInp->collName, ec);
            return ec;
        }

        if (const int ec = chlModColl(rsComm, &collInfo); ec < 0) {
            chlRollback(rsComm);
            return ec;
        }

        if (int ec = applyRule("acPostProcForModifyCollMeta", nullptr, &rei2, NO_SAVE_REI); ec < 0) {
            if (rei2.status < 0) {
                ec = rei2.status;
            }
            log_api::error("acPostProcForModifyCollMeta error for {},stat={}", modCollInp->collName, ec);
            return ec;
        }

        return chlCommit(rsComm);
    } // _rsModColl
} // anonymous namespace

auto rsModColl(rsComm_t* rsComm, collInp_t* modCollInp) -> int
{
    namespace ic = irods::experimental::catalog;
    namespace fs = irods::experimental::filesystem;

    if (!rsComm || !modCollInp) {
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    try {
        // If the zone name cannot be derived from the collection name in the input, the path cannot be valid.
        const auto zone_name = fs::zone_name(fs::path{static_cast<char*>(modCollInp->collName)});
        if (!zone_name) {
            return SYS_INVALID_FILE_PATH;
        }

        if (!isLocalZone(zone_name->c_str())) {
            auto* host = ic::redirect_to_catalog_provider(*rsComm, static_cast<char*>(modCollInp->collName));
            return rcModColl(host->conn, modCollInp);
        }

        const auto catalog_provider_host = ic::get_catalog_provider_host();

        if (LOCAL_HOST == catalog_provider_host.localFlag) {
            ic::throw_if_catalog_provider_service_role_is_invalid();
            return _rsModColl(rsComm, modCollInp);
        }

        // Some privileged clients may be affected by temporary elevated privileges. The redirect to the catalog service
        // provider here will cause the temporary privileged status granted to this connection to be lost in the
        // server-to-server connection. Many operations in iRODS use this API to update the collection mtime after a
        // data object modification or creation and requires privileged status to modify the collection in the event
        // that the user does not have permissions on the parent collection. A client connection is made here using the
        // service account rodsadmin credentials to modify the collection and then is immediately disconnected.
        if (irods::is_privileged_client(*rsComm)) {
            auto conn = irods::experimental::client_connection{catalog_provider_host.hostName->name,
                                                               rsComm->myEnv.rodsPort,
                                                               static_cast<char*>(rsComm->myEnv.rodsUserName),
                                                               static_cast<char*>(rsComm->myEnv.rodsZone)};

            return rcModColl(static_cast<RcComm*>(conn), modCollInp);
        }

        auto* host = ic::redirect_to_catalog_provider(*rsComm);
        return rcModColl(host->conn, modCollInp);
    }
    catch (const irods::exception& e) {
        log_api::error("[{}:{}] - caught iRODS exception [{}]", __func__, __LINE__, e.client_display_what());
        // The irods::exceptions thrown here are constructed from error codes found in rodsErrorTable and explicit ints
        // found in structs. There will never be a narrowing conversion which affects the value here.
        // NOLINTNEXTLINE(bugprone-narrowing-conversions,cppcoreguidelines-narrowing-conversions)
        return e.code();
    }
    catch (const std::exception& e) {
        log_api::error("[{}:{}] - caught std::exception [{}]", __func__, __LINE__, e.what());
        return SYS_INTERNAL_ERR;
    }
    catch (...) {
        log_api::error("[{}:{}] - caught unknown error", __func__, __LINE__);
        return SYS_UNKNOWN_ERROR;
    }
} // rsModColl
