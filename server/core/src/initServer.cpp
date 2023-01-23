#include "irods/initServer.hpp"

#include "irods/genQuery.h"
#include "irods/getRemoteZoneResc.h"
#include "irods/getRescQuota.h"
#include "irods/irods_configuration_keywords.hpp"
#include "irods/irods_stacktrace.hpp"
#include "irods/miscServerFunct.hpp"
#include "irods/objDesc.hpp"
#include "irods/physPath.hpp"
#include "irods/procLog.h"
#include "irods/rcGlobalExtern.h"
#include "irods/rcMisc.h"
#include "irods/resource.hpp"
#include "irods/rodsConnect.h"
#include "irods/rodsErrorTable.h"
#include "irods/rsDataObjClose.hpp"
#include "irods/rsExecCmd.hpp"
#include "irods/rsGenQuery.hpp"
#include "irods/rsGlobalExtern.hpp"
#include "irods/rsIcatOpr.hpp"
#include "irods/rsModDataObjMeta.hpp"
#include "irods/rs_replica_close.hpp"
#include "irods/sockComm.h"
#include "irods/objMetaOpr.hpp"
#include "irods/finalize_utilities.hpp"
#include "irods/irods_configuration_parser.hpp"
#include "irods/irods_exception.hpp"
#include "irods/irods_get_full_path_for_config_file.hpp"
#include "irods/irods_log.hpp"
#include "irods/irods_random.hpp"
#include "irods/irods_resource_backport.hpp"
#include "irods/irods_server_properties.hpp"
#include "irods/irods_threads.hpp"
#include "irods/key_value_proxy.hpp"
#include "irods/replica_state_table.hpp"
#include "irods/irods_logger.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "irods/replica_proxy.hpp"

#include "irods/logical_locking.hpp"

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <cstring>
#include <vector>
#include <set>
#include <string>
#include <fstream>

namespace
{
    namespace ill = irods::logical_locking;
    namespace rst = irods::replica_state_table;

    using log_agent = irods::experimental::log::agent;

    auto finalize_replica_opened_for_create_or_write(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> void
    {
        rodsLog(LOG_DEBUG, "[%s:%d] Closing L1 descriptor [path=%s, replica_number=%d] ...",
                __func__, __LINE__, _info.objPath, _info.replNum);

        namespace ir = irods::experimental::replica;

        auto r = ir::make_replica_proxy(_info);

        if (!rst::contains(r.data_id())) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - no entry found in replica state table for [{}]",
                __FUNCTION__, __LINE__, r.logical_path()));

            return;
        }

        // Check size in vault
        try {
            constexpr bool verify_size = false;
            const auto size = irods::get_size_in_vault(_comm, *r.get(), verify_size, _l1desc.dataSize);
            if (size < 0) {
                THROW(size, fmt::format(
                    "[{}:{}] - failed to get size from vault "
                    "[ec=[{}], path=[{}], repl num=[{}]]",
                    __FUNCTION__, __LINE__, size, r.logical_path(), r.replica_number()));
            }

            r.size(size);
        }
        catch (const irods::exception& e) {
            irods::log(LOG_ERROR, fmt::format("[{}:{}] - [{}]", __FUNCTION__, __LINE__, e.client_display_what()));
        }

        // If the replica was opened for write or create and failed to close, we set the
        // mtime because the status is going to change to stale due to failure to finalize.
        r.mtime(SET_TIME_TO_NOW_KW);

        r.checksum("");
        r.replica_status(STALE_REPLICA);

        rst::update(r.data_id(), r);

        // Update replica status to stale
        const auto admin_op = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput).contains(ADMIN_KW);

        if (const int ec = ill::unlock_and_publish(_comm, {r, admin_op}, STALE_REPLICA, ill::restore_status); ec < 0) {
            irods::log(LOG_NOTICE, fmt::format(
                "[{}:{}] - Failed to unlock data object "
                "[error_code=[{}], path=[{}], hierarchy=[{}]]",
                __FUNCTION__, __LINE__, ec, r.logical_path(), r.hierarchy()));
        }
    } // finalize_replica_opened_for_create_or_write
} // anonymous namespace

int initServerInfo(int processType, rsComm_t* rsComm)
{
    int status = 0;

    try {
        const auto& zone_name = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_NAME);
        const auto zone_port = irods::get_server_property<const int>( irods::KW_CFG_ZONE_PORT);

        /* queue the local zone */
        status = queueZone(zone_name.c_str(), zone_port, nullptr, nullptr);
        if (status < 0) {
            rodsLog(LOG_DEBUG, "initServerInfo - queueZone failed %d", status);
            // do not error out
        }
    }
    catch (const irods::exception& e) {
        irods::log(irods::error(e));
        return e.code();
    }

    status = initHostConfigByFile();
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "initServerInfo: initHostConfigByFile error, status = %d",
                 status );
        return status;
    }

    status = initLocalServerHost();
    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "initServerInfo: initLocalServerHost error, status = %d",
                 status );
        return status;
    }

    status = initRcatServerHostByFile();
    if ( status < 0 ) {
        rodsLog( LOG_SYS_FATAL,
                 "initServerInfo: initRcatServerHostByFile error, status = %d",
                 status );
        return status;
    }

    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
        return ret.code();
    }

    if( irods::KW_CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        status = connectRcat();
        if ( status < 0 ) {
            rodsLog(
                LOG_SYS_FATAL,
                "initServerInfo: connectRcat failed, status = %d",
                status );

            return status;
        }
    }

    status = initZone( rsComm );
    if ( status < 0 ) {
        rodsLog( LOG_SYS_FATAL,
                 "initServerInfo: initZone error, status = %d",
                 status );
        return status;
    }

    if (processType) {
        try {
            if (const auto ret = resc_mgr.init_from_catalog(*rsComm); !ret.ok()) {
                irods::log(PASSMSG("init_from_catalog failed", ret));
                return ret.code();
            }
        }
        catch (const irods::exception& e) {
            irods::log(e);
            return e.code();
        }
    }

    return status;
}

int
initLocalServerHost() {

    LocalServerHost = ServerHostHead = ( rodsServerHost_t* )malloc( sizeof( rodsServerHost_t ) );
    memset( ServerHostHead, 0, sizeof( rodsServerHost_t ) );

    LocalServerHost->localFlag = LOCAL_HOST;
    LocalServerHost->zoneInfo = ZoneInfoHead;

    auto status{matchHostConfig(LocalServerHost)};
    if (status < 0) {
        const auto err{ERROR(status, "matchHostConfig failed")};
        irods::log(err);
        return status;
    }

    queueHostName( ServerHostHead, "localhost", 0 );
    char myHostName[MAX_NAME_LEN];
    status = gethostname( myHostName, MAX_NAME_LEN );
    if ( status < 0 ) {
        status = SYS_GET_HOSTNAME_ERR - errno;
        rodsLog( LOG_NOTICE,
                 "initLocalServerHost: gethostname error, status = %d",
                 status );
        return status;
    }
    status = queueHostName( ServerHostHead, myHostName, 0 );
    if ( status < 0 ) {
        return status;
    }

    status = queueAddr( ServerHostHead, myHostName );
    if ( status < 0 ) {
        /* some configuration may not be able to resolve myHostName. So don't
         * exit. Just print out warning */
        rodsLog( LOG_NOTICE,
                 "initLocalServerHost: queueAddr error, status = %d",
                 status );
        status = 0;
    }

    if ( ProcessType == SERVER_PT ) {
        printServerHost( LocalServerHost );
    }

    return status;
}

int initRcatServerHostByFile()
{
    std::string prop_str;
    try {
        snprintf( KerberosName, sizeof( KerberosName ), "%s", irods::get_server_property<const std::string>(irods::KW_CFG_KERBEROS_NAME).c_str());
    }
    catch (const irods::exception& e) {}

    try {
        rodsHostAddr_t addr{};
        const auto& catalog_provider_hosts = irods::get_server_property<const nlohmann::json&>(irods::KW_CFG_CATALOG_PROVIDER_HOSTS);
        snprintf(addr.hostAddr, sizeof(addr.hostAddr), "%s", catalog_provider_hosts[0].get_ref<const std::string&>().c_str());

        rodsServerHost_t* tmp_host{};

        if (const int rem_flg = resolveHost(&addr, &tmp_host); rem_flg < 0) {
            rodsLog(LOG_SYS_FATAL,
                    "initRcatServerHostByFile: resolveHost error for %s, status = %d",
                    addr.hostAddr,
                    rem_flg);
            return rem_flg;
        }

        tmp_host->rcatEnabled = LOCAL_ICAT;
    }
    catch (const irods::exception& e) {
        irods::log(irods::error(e));
        return e.code();
    }

    // secondary icat host

    try {
        snprintf( localSID, sizeof( localSID ), "%s", irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_KEY).c_str() );
    } catch ( const irods::exception& e ) {
        try {
            snprintf( localSID, sizeof( localSID ), "%s", irods::get_server_property<const std::string>(LOCAL_ZONE_SID_KW).c_str() );
        } catch ( const irods::exception& e ) {
            irods::log( irods::error(e) );
            return e.code();
        }
    }

    // try for new federation config
    try {
        for (const auto& federation : irods::get_server_property<const nlohmann::json&>(irods::KW_CFG_FEDERATION)) {
            try {
                try {
                    const auto& fed_zone_key             = federation.at(irods::KW_CFG_ZONE_KEY).get_ref<const std::string&>();
                    const auto& fed_zone_name            = federation.at(irods::KW_CFG_ZONE_NAME).get_ref<const std::string&>();
                    const auto& fed_zone_negotiation_key = federation.at(irods::KW_CFG_NEGOTIATION_KEY).get_ref<const std::string&>();

                    // store in remote_SID_key_map
                    remote_SID_key_map[fed_zone_name] = std::make_pair(fed_zone_key, fed_zone_negotiation_key);
                }
                catch (boost::bad_any_cast&) {
                    rodsLog(LOG_ERROR, "initRcatServerHostByFile - failed to cast federation entry to string");
                    continue;
                }
                catch (const std::out_of_range&) {
                    rodsLog(LOG_ERROR, "%s - federation object did not contain required keys", __PRETTY_FUNCTION__);
                    continue;
                }
            }
            catch (const boost::bad_any_cast&) {
                rodsLog(LOG_ERROR, "%s - failed to cast array member to federation object", __PRETTY_FUNCTION__);
                continue;
            }
        }
    }
    catch (const irods::exception&) {
        // try the old remote sid config
        try {
            for (const auto& rem_sid : irods::get_server_property<std::vector<std::string>>(REMOTE_ZONE_SID_KW)) {
                // legacy format should be zone_name-SID
                const size_t pos = rem_sid.find("-");
                if (pos == std::string::npos) {
                    rodsLog(LOG_ERROR, "initRcatServerHostByFile - Unable to parse remote SID %s", rem_sid.c_str());
                }
                else {
                    // store in remote_SID_key_map
                    std::string fed_zone_name = rem_sid.substr(0, pos);
                    std::string fed_zone_key = rem_sid.substr(pos + 1);
                    // use our negotiation key for the old configuration
                    try {
                        const auto& neg_key = irods::get_server_property<const std::string>(irods::KW_CFG_NEGOTIATION_KEY);
                        remote_SID_key_map[fed_zone_name] = std::make_pair(fed_zone_key, neg_key);
                    }
                    catch (const irods::exception& e) {
                        irods::log(irods::error(e));
                        return e.code();
                    }
                }
            }
        } catch (const irods::exception&) {}
    } // else

    return 0;
}

int
initZone( rsComm_t *rsComm ) {
    if (ZoneInfoHead && (strlen(ZoneInfoHead->zoneName) > 0) && ZoneInfoHead->primaryServerHost) {
        return 0;
    }

    rodsServerHost_t *tmpRodsServerHost;
    rodsServerHost_t* primaryServerHost = NULL;
    rodsServerHost_t* secondaryServerHost = NULL;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status, i;
    sqlResult_t *zoneName, *zoneType, *zoneConn, *zoneComment;
    char *tmpZoneName, *tmpZoneType, *tmpZoneConn;//, *tmpZoneComment;

    boost::optional<std::string> zone_name;
    try {
        zone_name.reset(irods::get_server_property<std::string>(irods::KW_CFG_ZONE_NAME));
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    /* configure the local zone first or rsGenQuery would not work */

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ) {
            tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            primaryServerHost = tmpRodsServerHost;
        }
        else if (tmpRodsServerHost->rcatEnabled == LOCAL_SECONDARY_ICAT) {
            tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            secondaryServerHost = tmpRodsServerHost;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    ZoneInfoHead->primaryServerHost = primaryServerHost;
    ZoneInfoHead->secondaryServerHost = secondaryServerHost;

    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    addInxIval( &genQueryInp.selectInp, COL_ZONE_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_ZONE_TYPE, 1 );
    addInxIval( &genQueryInp.selectInp, COL_ZONE_CONNECTION, 1 );
    addInxIval( &genQueryInp.selectInp, COL_ZONE_COMMENT, 1 );
    genQueryInp.maxRows = MAX_SQL_ROWS;

    status =  rsGenQuery( rsComm, &genQueryInp, &genQueryOut );

    clearGenQueryInp( &genQueryInp );

    if ( status < 0 ) {
        freeGenQueryOut( &genQueryOut );
        rodsLog( LOG_NOTICE,
                 "initZone: rsGenQuery error, status = %d", status );
        return status;
    }

    if ( genQueryOut == NULL ) {
        rodsLog( LOG_NOTICE,
                 "initZone: NULL genQueryOut" );
        return CAT_NO_ROWS_FOUND;
    }

    if ( ( zoneName = getSqlResultByInx( genQueryOut, COL_ZONE_NAME ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "initZone: getSqlResultByInx for COL_ZONE_NAME failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( zoneType = getSqlResultByInx( genQueryOut, COL_ZONE_TYPE ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "initZone: getSqlResultByInx for COL_ZONE_TYPE failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( zoneConn = getSqlResultByInx( genQueryOut, COL_ZONE_CONNECTION ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "initZone: getSqlResultByInx for COL_ZONE_CONNECTION failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    if ( ( zoneComment = getSqlResultByInx( genQueryOut, COL_ZONE_COMMENT ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "initZone: getSqlResultByInx for COL_ZONE_COMMENT failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
        rodsHostAddr_t addr;

        tmpZoneName = &zoneName->value[zoneName->len * i];
        tmpZoneType = &zoneType->value[zoneType->len * i];
        tmpZoneConn = &zoneConn->value[zoneConn->len * i];
        //tmpZoneComment = &zoneComment->value[zoneComment->len * i];
        if ( strcmp( tmpZoneType, "local" ) == 0 ) {
            if ( strcmp( zone_name->c_str(), tmpZoneName ) != 0 ) {
                rodsLog( LOG_ERROR,
                         "initZone: zoneName in env %s does not match %s in icat ",
                         zone_name->c_str(), tmpZoneName );
            }
            /* fillin rodsZone if it is not defined */
            if ( strlen( rsComm->proxyUser.rodsZone ) == 0 ) {
                rstrcpy( rsComm->proxyUser.rodsZone, tmpZoneName, NAME_LEN );
            }
            if ( strlen( rsComm->clientUser.rodsZone ) == 0 ) {
                rstrcpy( rsComm->clientUser.rodsZone, tmpZoneName, NAME_LEN );
            }
            continue;
        }
        else if ( strlen( tmpZoneConn ) <= 0 ) {
            rodsLog( LOG_ERROR,
                     "initZone: connection info for zone %s not configured",
                     tmpZoneName );
            continue;
        }

        memset( &addr, 0, sizeof( addr ) );
        /* assume address:port */
        parseHostAddrStr( tmpZoneConn, &addr );
        if ( addr.portNum == 0 ) {
            addr.portNum = ZoneInfoHead->portNum;
        }
        rstrcpy( addr.zoneName, tmpZoneName, NAME_LEN ); // JMC - bacport 4562
        status = resolveHost( &addr, &tmpRodsServerHost );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "initZone: resolveHost error for %s for zone %s. status = %d",
                     addr.hostAddr, tmpZoneName, status );
            continue;
        }
        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ) {
            rodsLog( LOG_ERROR,
                     "initZone: address %s for remote zone %s already in use",
                     tmpZoneConn, tmpZoneName );
            continue;
        }

        tmpRodsServerHost->rcatEnabled = REMOTE_ICAT;
        /* REMOTE_ICAT is always on a remote host even if it is one the same
         * host, but will be on different port */
        tmpRodsServerHost->localFlag = REMOTE_HOST; // JMC - bacport 4562
        queueZone( tmpZoneName, addr.portNum, tmpRodsServerHost, NULL );
    }

    freeGenQueryOut( &genQueryOut );

    return 0;
}

int
initAgent( int processType, rsComm_t *rsComm ) {

    initProcLog();

    int status = initServerInfo( 1, rsComm );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: initServerInfo error, status = %d",
                 status );
        return status;
    }

    irods::replica_state_table::init();
    initL1desc();
    initSpecCollDesc();
    status = initFileDesc();
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: initFileDesc error, status = %d",
                 status );
        return status;
    }

    ruleExecInfo_t rei;
    memset( &rei, 0, sizeof( rei ) );
    rei.rsComm = rsComm;

    if ( ProcessType == AGENT_PT ) {
        status = applyRule( "acChkHostAccessControl", NULL, &rei,
                            NO_SAVE_REI );

        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "initAgent: acChkHostAccessControl error, status = %d",
                     status );
            return status;
        }
    }

    /* Initialize the global quota */

    GlobalQuotaLimit = RESC_QUOTA_UNINIT;
    GlobalQuotaOverrun = 0;
    RescQuotaPolicy = RESC_QUOTA_UNINIT;

    if ( rsComm->reconnFlag == RECONN_TIMEOUT ) {
        rsComm->reconnSock = svrSockOpenForInConn( rsComm, &rsComm->reconnPort,
                             &rsComm->reconnAddr, SOCK_STREAM );
        if ( rsComm->reconnSock < 0 ) {
            rsComm->reconnPort = 0;
            rsComm->reconnAddr = NULL;
        }
        else {
            rsComm->cookie = ( int )( irods::getRandom<unsigned int>() >> 1 );
        }
        try {
            rsComm->thread_ctx->lock      = new boost::mutex;
            rsComm->thread_ctx->cond      = new boost::condition_variable;
            rsComm->thread_ctx->reconnThr = new boost::thread( reconnManager, rsComm );
        }
        catch ( boost::thread_resource_error& ) {
            rodsLog( LOG_ERROR, "boost encountered thread_resource_error." );
            return SYS_THREAD_RESOURCE_ERR;
        }
    }
    initExecCmdMutex();

    InitialState = INITIAL_DONE;
    ThisComm = rsComm;

    /* use a tmp myComm is needed to get the permission right for the call */
    rsComm_t myComm = *rsComm;
    myComm.clientUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    rei.rsComm = &myComm;

    status = applyRule( "acSetPublicUserPolicy", NULL, &rei, NO_SAVE_REI );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: acSetPublicUserPolicy error, status = %d",
                 status );
        return status;
    }

    return status;
}



/// @brief parse the irodsHost file, creating address
int initHostConfigByFile()
{
    try {
        const auto& hosts_config = irods::get_server_property<const nlohmann::json&>(irods::KW_CFG_HOST_RESOLUTION);
        const auto& host_entries = hosts_config.at(irods::KW_CFG_HOST_ENTRIES); // An array.

        for (auto&& entry : host_entries) {
            try {
                auto* svr_host = static_cast<rodsServerHost_t*>(std::malloc(sizeof(rodsServerHost_t)));
                std::memset(svr_host, 0, sizeof(rodsServerHost_t));

                const auto& address_type = entry.at(irods::KW_CFG_ADDRESS_TYPE).get_ref<const std::string&>();

                if (address_type == "remote") {
                    svr_host->localFlag = REMOTE_HOST;
                }
                else if (address_type == "local") {
                    svr_host->localFlag = LOCAL_HOST;
                }
                else {
                    std::free(svr_host);
                    rodsLog(LOG_ERROR, "unsupported address type [%s]", address_type.data());
                    continue;
                }

                svr_host->zoneInfo = ZoneInfoHead;

                if (queueRodsServerHost(&HostConfigHead, svr_host) < 0) {
                    rodsLog(LOG_ERROR, "queueRodsServerHost failed");
                }

                for (auto&& address : entry.at(irods::KW_CFG_ADDRESSES)) {
                    try {
                        if (queueHostName(svr_host, address.get_ref<const std::string&>().data(), 0) < 0) {
                            rodsLog(LOG_ERROR, "queueHostName failed");
                        }
                    }
                    catch (const nlohmann::json::exception& e) {
                        rodsLog(LOG_ERROR, "Could not process host_resolution address [address_type=%s, address=%s, error_message=%s]",
                                address_type.data(), address.dump().data(), e.what());
                    }
                }
            }
            catch (const nlohmann::json::exception& e) {
                rodsLog(LOG_ERROR, "Could not process host_resolution entry [entry=%d, error_message=%s].",
                        entry.dump().data(), e.what());
            }
        }
    }
    catch (const irods::exception& e) {
        irods::log(irods::error(e));
        return e.code();
    }
    catch (const std::exception& e) {
        rodsLog(LOG_ERROR, "%s error.", __func__);
        return SYS_INTERNAL_ERR;
    }

    return 0;
} // initHostConfigByFile

int
initRsComm( rsComm_t *rsComm ) {
    memset( rsComm, 0, sizeof( rsComm_t ) );

    if ( int status = getRodsEnv( &rsComm->myEnv ) ) {
        irods::log( ERROR( status, "getRodsEnv failed in initRsComm" ) );
        return status;
    }

    try {
        const auto& zone_name = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_NAME);
        const auto& zone_user = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_USER);
        const auto& zone_auth_scheme = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_AUTH_SCHEME);

        /* fill in the proxyUser info from server_config. clientUser
         * has to come from the rei */
        rstrcpy( rsComm->proxyUser.userName, zone_user.c_str(), NAME_LEN );
        rstrcpy( rsComm->proxyUser.rodsZone, zone_name.c_str(), NAME_LEN );
        rstrcpy( rsComm->proxyUser.authInfo.authScheme,
                zone_auth_scheme.c_str(), NAME_LEN );
        rstrcpy( rsComm->clientUser.userName, zone_user.c_str(), NAME_LEN );
        rstrcpy( rsComm->clientUser.rodsZone, zone_name.c_str(), NAME_LEN );
        rstrcpy( rsComm->clientUser.authInfo.authScheme,
                zone_auth_scheme.c_str(), NAME_LEN );
        /* assume LOCAL_PRIV_USER_AUTH */
        rsComm->clientUser.authInfo.authFlag =
            rsComm->proxyUser.authInfo.authFlag = LOCAL_PRIV_USER_AUTH;
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    return 0;
}

int
initRsCommWithStartupPack( rsComm_t *rsComm, startupPack_t *startupPack ) {
    char *tmpStr;
    static char tmpStr2[LONG_NAME_LEN];
    /* always use NATIVE_PROT as a client. e.g., server to server comm */
    snprintf( tmpStr2, LONG_NAME_LEN, "%s=%d", IRODS_PROT, NATIVE_PROT );
    putenv( tmpStr2 );

    if ( startupPack != NULL ) {
        rsComm->connectCnt = startupPack->connectCnt;
        rsComm->irodsProt = startupPack->irodsProt;
        rsComm->reconnFlag = startupPack->reconnFlag;
        rstrcpy( rsComm->proxyUser.userName, startupPack->proxyUser,
                NAME_LEN );
        if ( strcmp( startupPack->proxyUser, PUBLIC_USER_NAME ) == 0 ) {
            rsComm->proxyUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }
        rstrcpy( rsComm->proxyUser.rodsZone, startupPack->proxyRodsZone,
                NAME_LEN );
        rstrcpy( rsComm->clientUser.userName, startupPack->clientUser,
                NAME_LEN );
        if ( strcmp( startupPack->clientUser, PUBLIC_USER_NAME ) == 0 ) {
            rsComm->clientUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }
        rstrcpy( rsComm->clientUser.rodsZone, startupPack->clientRodsZone,
                NAME_LEN );
        rstrcpy( rsComm->cliVersion.relVersion, startupPack->relVersion,
                NAME_LEN );
        rstrcpy( rsComm->cliVersion.apiVersion, startupPack->apiVersion,
                NAME_LEN );
        rstrcpy( rsComm->option, startupPack->option, LONG_NAME_LEN );
    }
    else {      /* have to depend on env variable */
        tmpStr = getenv( SP_NEW_SOCK );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_NEW_SOCK );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rsComm->sock = atoi( tmpStr );

        tmpStr = getenv( SP_CONNECT_CNT );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_CONNECT_CNT );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rsComm->connectCnt = atoi( tmpStr ) + 1;

        tmpStr = getenv( SP_PROTOCOL );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_PROTOCOL );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rsComm->irodsProt = ( irodsProt_t )atoi( tmpStr );

        if (rsComm->irodsProt != NATIVE_PROT && rsComm->irodsProt != XML_PROT) {
            rodsLog( LOG_NOTICE, "initRsCommWithStartupPack: Invalid protocol value.");
            return SYS_GETSTARTUP_PACK_ERR;
        }

        tmpStr = getenv( SP_RECONN_FLAG );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_RECONN_FLAG );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rsComm->reconnFlag = atoi( tmpStr );

        tmpStr = getenv( SP_PROXY_USER );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_PROXY_USER );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->proxyUser.userName, tmpStr, NAME_LEN );
        if ( strcmp( tmpStr, PUBLIC_USER_NAME ) == 0 ) {
            rsComm->proxyUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }

        tmpStr = getenv( SP_PROXY_RODS_ZONE );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_PROXY_RODS_ZONE );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->proxyUser.rodsZone, tmpStr, NAME_LEN );

        tmpStr = getenv( SP_CLIENT_USER );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_CLIENT_USER );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->clientUser.userName, tmpStr, NAME_LEN );
        if ( strcmp( tmpStr, PUBLIC_USER_NAME ) == 0 ) {
            rsComm->clientUser.authInfo.authFlag = PUBLIC_USER_AUTH;
        }

        tmpStr = getenv( SP_CLIENT_RODS_ZONE );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_CLIENT_RODS_ZONE );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->clientUser.rodsZone, tmpStr, NAME_LEN );

        tmpStr = getenv( SP_REL_VERSION );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "getstartupPackFromEnv: env %s does not exist",
                    SP_REL_VERSION );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->cliVersion.relVersion, tmpStr, NAME_LEN );

        tmpStr = getenv( SP_API_VERSION );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_API_VERSION );
            return SYS_GETSTARTUP_PACK_ERR;
        }
        rstrcpy( rsComm->cliVersion.apiVersion, tmpStr, NAME_LEN );

        tmpStr = getenv( SP_OPTION );
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                    "initRsCommWithStartupPack: env %s does not exist",
                    SP_OPTION );
        }
        else {
            rstrcpy( rsComm->option, tmpStr, LONG_NAME_LEN );
        }
    }

    if (const auto ec = setLocalAddr(rsComm->sock, &rsComm->localAddr); ec == USER_RODS_HOSTNAME_ERR) {
        return ec;
    }

    if (const auto ec = setRemoteAddr(rsComm->sock, &rsComm->remoteAddr); ec != 0) {
        return ec;
    }

    tmpStr = inet_ntoa( rsComm->remoteAddr.sin_addr );

    if ( tmpStr == NULL || *tmpStr == '\0' ) {
        tmpStr = "UNKNOWN";
    }
    rstrcpy( rsComm->clientAddr, tmpStr, NAME_LEN );

    return 0;
}

std::set<std::string>
construct_controlled_user_set(const nlohmann::json& controlled_user_connection_list) {
    std::set<std::string> user_set;

    try {
        for (auto&& user : controlled_user_connection_list.at("users")) {
            user_set.insert(user.get_ref<const std::string&>());
        }
    }
    catch (const nlohmann::json::exception& e) {
        THROW(SYS_LIBRARY_ERROR, fmt::format("Encountered exception while processing [controlled_user_connection_list]. {}", e.what()));
    }

    return user_set;
}

int
chkAllowedUser( const char *userName, const char *rodsZone ) {

    if (!userName || !rodsZone) {
        return SYS_USER_NOT_ALLOWED_TO_CONN;
    }

    if (strlen(userName) == 0) {
        /* XXXXXXXXXX userName not yet defined. allow it for now */
        return 0;
    }

    const nlohmann::json* controlled_user_connection_list;

    try {
        controlled_user_connection_list = &irods::get_server_property<const nlohmann::json&>("controlled_user_connection_list");
    }
    catch (const irods::exception& e) {
        if (e.code() == KEY_NOT_FOUND) {
            return 0;
        }

        return e.code();
    }

    const auto user_and_zone = fmt::format("{}#{}", userName, rodsZone);

    try {
        static const auto controlled_user_set = construct_controlled_user_set(*controlled_user_connection_list);
        const auto& control_type = controlled_user_connection_list->at("control_type").get_ref<const std::string&>();

        if (control_type == "allowlist") {
            if (controlled_user_set.count(user_and_zone) == 0) {
                return SYS_USER_NOT_ALLOWED_TO_CONN;
            }
        }
        else if (control_type == "denylist") {
            if (controlled_user_set.count(user_and_zone) != 0) {
                return SYS_USER_NOT_ALLOWED_TO_CONN;
            }
        }
        else {
            rodsLog(LOG_ERROR, "controlled_user_connection_list must specify \"allowlist\" or \"denylist\"");
            return -1;
        }
    }
    catch (const irods::exception& e) {
        irods::log(e);
        return e.code();
    }
    catch (const std::out_of_range& e) {
        rodsLog(LOG_ERROR, "%s", e.what());
        return KEY_NOT_FOUND;
    }

    return 0;
}

int
setRsCommFromRodsEnv( rsComm_t *rsComm ) {
    try {
        const auto& zone_name = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_NAME);
        const auto& zone_user = irods::get_server_property<const std::string>(irods::KW_CFG_ZONE_USER);

        rstrcpy( rsComm->proxyUser.userName,  zone_user.c_str(), NAME_LEN );
        rstrcpy( rsComm->clientUser.userName, zone_user.c_str(), NAME_LEN );

        rstrcpy( rsComm->proxyUser.rodsZone,  zone_name.c_str(), NAME_LEN );
        rstrcpy( rsComm->clientUser.rodsZone, zone_name.c_str(), NAME_LEN );
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    return 0;
}

void close_all_l1_descriptors(RsComm& _comm)
{
    log_agent::debug("[{}:{}] Closing all L1 descriptors ...", __func__, __LINE__);

    for (int fd = 3; fd < NUM_L1_DESC; ++fd) {
        auto& l1desc = L1desc[fd];
        if (FD_INUSE != l1desc.inuseFlag || l1desc.l3descInx < 3) {
            continue;
        }

        if (!l1desc.dataObjInfo) {
            irods::log(
                LOG_ERROR,
                fmt::format("[{}:{}] - l1 descriptor [{}] has no data object info", __FUNCTION__, __LINE__, fd));

            continue;
        }

        try {
            auto [replica, replica_lm] = irods::experimental::replica::duplicate_replica(*l1desc.dataObjInfo);
            struct l1desc fd_cache = irods::duplicate_l1_descriptor(l1desc);
            const irods::at_scope_exit free_fd{[&fd_cache] { freeL1desc_struct(fd_cache); }};
            constexpr auto preserve_rst = true;

            // close the replica, free L1 descriptor
            if (const int ec = irods::close_replica_without_catalog_update(_comm, fd, preserve_rst); ec < 0) {
                irods::log(
                    LOG_ERROR, fmt::format("[{}:{}] - error closing replica; ec:[{}]", __FUNCTION__, __LINE__, ec));

                // Even though closing failed, continue to the end of the loop so that the data object is finalized.
            }

            // Don't do anything for special collections - they do not enter intermediate state
            if (replica.special_collection_info()) {
                continue;
            }

            // TODO: need to unlock read-locked replicas
            if (OPEN_FOR_READ_TYPE != fd_cache.openType) {
                finalize_replica_opened_for_create_or_write(_comm, *replica.get(), fd_cache);
            }
        }
        catch (const irods::exception& e) {
            irods::log(e);
        }
    }
} // close_all_l1_descriptors
