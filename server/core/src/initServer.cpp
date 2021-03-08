#include "genQuery.h"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "initServer.hpp"
#include "irods_stacktrace.hpp"
#include "miscServerFunct.hpp"
#include "physPath.hpp"
#include "procLog.h"
#include "rcGlobalExtern.h"
#include "rcMisc.h"
#include "resource.hpp"
#include "rodsConnect.h"
#include "rodsErrorTable.h"
#include "rsDataObjClose.hpp"
#include "rsExecCmd.hpp"
#include "rsGenQuery.hpp"
#include "rsGlobalExtern.hpp"
#include "rsIcatOpr.hpp"
#include "rsLog.hpp"
#include "rsModDataObjMeta.hpp"
#include "rs_replica_close.hpp"
#include "sockComm.h"
#include "objMetaOpr.hpp"

#include "finalize_utilities.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_exception.hpp"
#include "irods_get_full_path_for_config_file.hpp"
#include "irods_log.hpp"
#include "irods_random.hpp"
#include "irods_resource_backport.hpp"
#include "irods_server_properties.hpp"
#include "irods_threads.hpp"
#include "key_value_proxy.hpp"
#include "replica_state_table.hpp"

#define IRODS_REPLICA_ENABLE_SERVER_SIDE_API
#include "replica_proxy.hpp"

#include <vector>
#include <set>
#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

#include <json.hpp>

static time_t LastBrokenPipeTime = 0;
static int BrokenPipeCnt = 0;

namespace
{
    namespace rst = irods::replica_state_table;

    auto calculate_checksum(RsComm& _comm, l1desc& _l1desc, DataObjInfo& _info) -> std::string
    {
        if (!std::string_view{_info.chksum}.empty()) {
            _l1desc.chksumFlag = REG_CHKSUM;
        }

        char* checksum_string = nullptr;
        irods::at_scope_exit free_checksum_string{[&checksum_string] { free(checksum_string); }};

        auto destination_replica = irods::experimental::replica::make_replica_proxy(_info);

        if (!_l1desc.chksumFlag) {
            if (destination_replica.checksum().empty()) {
                return {};
            }
            _l1desc.chksumFlag = VERIFY_CHKSUM;
        }

        if (VERIFY_CHKSUM == _l1desc.chksumFlag) {
            if (!std::string_view{_l1desc.chksum}.empty()) {
                return irods::verify_checksum(_comm, _info, _l1desc.chksum);
            }

            return {};
        }

        return irods::register_new_checksum(_comm, _info, _l1desc.chksum);
    } // calculate_checksum

    auto finalize_replica_opened_for_create_or_write(RsComm& _comm, DataObjInfo& _info, l1desc& _l1desc) -> void
    {
        namespace ir = irods::experimental::replica;

        auto replica = ir::make_replica_proxy(_info);

        if (!rst::contains(replica.data_id())) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - no entry found in replica state table for [{}]",
                __FUNCTION__, __LINE__, replica.logical_path()));

            return;
        }

        const auto cond_input = irods::experimental::make_key_value_proxy(_l1desc.dataObjInp->condInput);

        //  - Check size in vault
        try {
            constexpr bool verify_size = false;
            replica.size(irods::get_size_in_vault(_comm, *replica.get(), verify_size, _l1desc.dataSize));

            //  - Compute checksum if flag is present
            if (!cond_input.contains(CHKSUM_KW) && !replica.checksum().empty()) {
                if (const auto checksum = calculate_checksum(_comm, _l1desc, *replica.get()); !checksum.empty()) {
                    replica.checksum(checksum);
                }
            }
        }
        catch (const irods::exception& e) {
            irods::log(e);
        }

        //  TODO?
        //  - Update modify time to now

        //  - Update replica status to stale
        replica.replica_status(STALE_REPLICA);

        if (cond_input.contains(CHKSUM_KW)) {
            replica.checksum(cond_input.at(CHKSUM_KW).value());
        }

        rst::update(replica.data_id(), replica);

        if (const int ec = rst::publish_to_catalog(_comm, replica.data_id(), replica.replica_number(), nlohmann::json{}); ec < 0) {
            irods::log(LOG_ERROR, fmt::format(
                "[{}:{}] - failed to publish to catalog:[{}]",
                __FUNCTION__, __LINE__, ec));
        }
    } // finalize_replica_opened_for_create_or_write

    void close_all_l1_descriptors(RsComm& _comm)
    {
        for (int fd = 3; fd < NUM_L1_DESC; ++fd) {
            auto& l1desc = L1desc[fd];
            if (FD_INUSE != l1desc.inuseFlag || l1desc.l3descInx < 3) {
                continue;
            }

            if (!l1desc.dataObjInfo) {
                irods::log(LOG_ERROR, fmt::format(
                    "[{}:{}] - l1 descriptor [{}] has no data object info",
                    __FUNCTION__, __LINE__, fd));

                continue;
            }

            try {
                auto [replica, replica_lm] = irods::experimental::replica::duplicate_replica(*l1desc.dataObjInfo);
                struct l1desc fd_cache = irods::duplicate_l1_descriptor(l1desc);
                const irods::at_scope_exit free_fd{[&fd_cache] { freeL1desc_struct(fd_cache); }};

                // close the replica, free L1 descriptor
                if (const int ec = irods::close_replica_without_catalog_update(_comm, fd); ec < 0) {
                    irods::log(LOG_ERROR, fmt::format(
                        "[{}:{}] - error closing replica; ec:[{}]",
                        __FUNCTION__, __LINE__, ec));

                    continue;
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
} // anonymous namespace

InformationRequiredToSafelyRenameProcess::InformationRequiredToSafelyRenameProcess(char**argv) {
    argv0 = argv[0];
    argv0_size = strlen(argv[0]);
}

int
//initServerInfo( rsComm_t *rsComm ) {
initServerInfo( int processType, rsComm_t * rsComm) {
    int status = 0;

    try {
        const auto& zone_name = irods::get_server_property<const std::string>(irods::CFG_ZONE_NAME);
        const auto zone_port = irods::get_server_property<const int>( irods::CFG_ZONE_PORT);

        /* queue the local zone */
        status = queueZone(
                zone_name.c_str(),
                zone_port, NULL, NULL );
        if ( status < 0 ) {
            rodsLog(
                    LOG_DEBUG,
                    "initServerInfo - queueZone failed %d",
                    status );
            // do not error out
        }
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
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

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
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
        ret = resc_mgr.init_from_catalog( rsComm );
        if ( !ret.ok() ) {
            irods::error log_err = PASSMSG( "init_from_catalog failed", ret );
            irods::log( log_err );
            return ret.code();
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

int
initRcatServerHostByFile() {
    std::string prop_str;
    try {
        snprintf( KerberosName, sizeof( KerberosName ), "%s", irods::get_server_property<const std::string>(irods::CFG_KERBEROS_NAME_KW).c_str());
    } catch ( const irods::exception& e ) {}

    try {
        rodsHostAddr_t    addr;
        memset( &addr, 0, sizeof( addr ) );
        rodsServerHost_t* tmp_host = 0;
        snprintf( addr.hostAddr, sizeof( addr.hostAddr ), "%s", boost::any_cast<const std::string&>(irods::get_server_property<const std::vector<boost::any>>(irods::CFG_CATALOG_PROVIDER_HOSTS_KW)[0]).c_str() );
        int rem_flg = resolveHost(
                          &addr,
                          &tmp_host );
        if ( rem_flg < 0 ) {
            rodsLog( LOG_SYS_FATAL,
                     "initRcatServerHostByFile: resolveHost error for %s, status = %d",
                     addr.hostAddr,
                     rem_flg );
            return rem_flg;
        }
        tmp_host->rcatEnabled = LOCAL_ICAT;

    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    // slave icat host

    try {
        snprintf( localSID, sizeof( localSID ), "%s", irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW).c_str() );
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
        for ( const auto& el : irods::get_server_property< const std::vector<boost::any> > ( irods::CFG_FEDERATION_KW ) ) {
            try {
                const auto& federation = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(el);
                try {
                    const auto& fed_zone_key = boost::any_cast< std::string >(federation.at(irods::CFG_ZONE_KEY_KW));
                    const auto& fed_zone_name = boost::any_cast< std::string >(federation.at(irods::CFG_ZONE_NAME_KW));
                    const auto& fed_zone_negotiation_key = boost::any_cast< std::string >(federation.at(irods::CFG_NEGOTIATION_KEY_KW));
                    // store in remote_SID_key_map
                    remote_SID_key_map[fed_zone_name] = std::make_pair( fed_zone_key, fed_zone_negotiation_key );
                }
                catch ( boost::bad_any_cast& _e ) {
                    rodsLog(
                            LOG_ERROR,
                            "initRcatServerHostByFile - failed to cast federation entry to string" );
                    continue;
                } catch ( const std::out_of_range& ) {
                    rodsLog(
                            LOG_ERROR,
                            "%s - federation object did not contain required keys",
                            __PRETTY_FUNCTION__);
                    continue;
                }
            } catch ( const boost::bad_any_cast& ) {
                rodsLog(
                        LOG_ERROR,
                        "%s - failed to cast array member to federation object",
                        __PRETTY_FUNCTION__);
                continue;
            }

        }
    } catch ( const irods::exception& ) {
        // try the old remote sid config
        try {
            for ( const auto& rem_sid : irods::get_server_property< std::vector< std::string > > ( REMOTE_ZONE_SID_KW ) ) {
                // legacy format should be zone_name-SID
                size_t pos = rem_sid.find( "-" );
                if ( pos == std::string::npos ) {
                    rodsLog( LOG_ERROR, "initRcatServerHostByFile - Unable to parse remote SID %s", rem_sid.c_str() );
                }
                else {
                    // store in remote_SID_key_map
                    std::string fed_zone_name = rem_sid.substr( 0, pos );
                    std::string fed_zone_key = rem_sid.substr( pos + 1 );
                    // use our negotiation key for the old configuration
                    try {
                        const auto& neg_key = irods::get_server_property<const std::string>(irods::CFG_NEGOTIATION_KEY_KW);
                        remote_SID_key_map[fed_zone_name] = std::make_pair( fed_zone_key, neg_key );
                    } catch ( const irods::exception& e ) {
                        irods::log( irods::error(e) );
                        return e.code();
                    }
                }
            }
        } catch ( const irods::exception& ) {}

    } // else

    return 0;
}

int
initZone( rsComm_t *rsComm ) {
    if (ZoneInfoHead && (strlen(ZoneInfoHead->zoneName) > 0)
                     && ZoneInfoHead->masterServerHost) {
        return 0;
    }

    rodsServerHost_t *tmpRodsServerHost;
    rodsServerHost_t *masterServerHost = NULL;
    rodsServerHost_t *slaveServerHost = NULL;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status, i;
    sqlResult_t *zoneName, *zoneType, *zoneConn, *zoneComment;
    char *tmpZoneName, *tmpZoneType, *tmpZoneConn;//, *tmpZoneComment;

    boost::optional<const std::string&> zone_name;
    try {
        zone_name.reset(irods::get_server_property<const std::string>(irods::CFG_ZONE_NAME));
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
    }

    /* configure the local zone first or rsGenQuery would not work */

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ) {
            tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            masterServerHost = tmpRodsServerHost;
        }
        else if ( tmpRodsServerHost->rcatEnabled == LOCAL_SLAVE_ICAT ) {
            tmpRodsServerHost->zoneInfo = ZoneInfoHead;
            slaveServerHost = tmpRodsServerHost;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    ZoneInfoHead->masterServerHost = masterServerHost;
    ZoneInfoHead->slaveServerHost = slaveServerHost;

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

void
cleanup() {
    std::string svc_role;
    irods::error ret = get_catalog_service_role(svc_role);
    if(!ret.ok()) {
        irods::log(PASS(ret));
    }

    if (INITIAL_DONE == InitialState) {
        close_all_l1_descriptors(*ThisComm);

        irods::replica_state_table::deinit();

        disconnectAllSvrToSvrConn();
    }

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        disconnectRcat();
    }

    irods::re_plugin_globals->global_re_mgr.call_stop_operations();
}

void
cleanupAndExit( int status ) {
#if 0
    // RTS - rodsLog calls in signal handlers are unsafe - #3326
    rodsLog( LOG_NOTICE,
             "Agent exiting with status = %d", status );
#endif

    cleanup();

    if ( status >= 0 ) {
        exit( 0 );
    }
    else {
        exit( 1 );
    }
}

void
signalExit( int ) {
#if 0
    // RTS - rodsLog calls in signal handlers are unsafe - #3326
    rodsLog( LOG_NOTICE,
             "caught a signal and exiting\n" );
#endif
    cleanupAndExit( SYS_CAUGHT_SIGNAL );
}

void
rsPipeSignalHandler( int ) {
    time_t curTime;

    if ( ThisComm == NULL || ThisComm->reconnSock <= 0 ) {
        rodsLog( LOG_NOTICE,
                 "caught a broken pipe signal and exiting" );
        cleanupAndExit( SYS_CAUGHT_SIGNAL );
    }
    else {
        curTime = time( 0 );
        if ( curTime - LastBrokenPipeTime < BROKEN_PIPE_INT ) {
            BrokenPipeCnt ++;
            if ( BrokenPipeCnt > MAX_BROKEN_PIPE_CNT ) {
                rodsLog( LOG_NOTICE,
                         "caught a broken pipe signal and exiting" );
                cleanupAndExit( SYS_CAUGHT_SIGNAL );
            }
        }
        else {
            BrokenPipeCnt = 1;
        }
        LastBrokenPipeTime = curTime;
        rodsLog( LOG_NOTICE,
                 "caught a broken pipe signal. Attempt to reconnect" );
        signal( SIGPIPE, ( void ( * )( int ) ) rsPipeSignalHandler );

    }
}

/// @brief parse the irodsHost file, creating address
int initHostConfigByFile()
{
    try {
        const auto& hosts_config = irods::get_server_property<nlohmann::json&>(irods::HOSTS_CONFIG_JSON_OBJECT_KW);

        for (auto&& entry : hosts_config.at("host_entries")) {
            try {
                auto* svr_host = static_cast<rodsServerHost_t*>(std::malloc(sizeof(rodsServerHost_t)));
                std::memset(svr_host, 0, sizeof(rodsServerHost_t));

                if (const auto type = entry.at("address_type").get<std::string>(); "remote" == type) {
                    svr_host->localFlag = REMOTE_HOST;
                }
                else if ("local" == type) {
                    svr_host->localFlag = LOCAL_HOST;
                }
                else {
                    std::free(svr_host);
                    rodsLog(LOG_ERROR, "unsupported address type [%s]", type.data());
                    continue;
                }

                svr_host->zoneInfo = ZoneInfoHead;

                if (queueRodsServerHost(&HostConfigHead, svr_host) < 0) {
                    rodsLog(LOG_ERROR, "queueRodsServerHost failed");
                }

                for (auto&& address : entry.at("addresses")) {
                    try {
                        if (queueHostName(svr_host, address.at("address").get<std::string>().data(), 0) < 0) {
                            rodsLog(LOG_ERROR, "queHostName failed");
                        }
                    }
                    catch (const nlohmann::json::exception& e) {
                        rodsLog(LOG_ERROR, "Could not process hosts_config.json address [address=%s, error_message=%s]",
                                address.dump().data(), e.what());
                    }
                }
            }
            catch (const nlohmann::json::exception& e) {
                rodsLog(LOG_ERROR, "Could not process hosts_config.json entry [entry=%s, error_message=%s].",
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
        const auto& zone_name = irods::get_server_property<const std::string>(irods::CFG_ZONE_NAME);
        const auto& zone_user = irods::get_server_property<const std::string>(irods::CFG_ZONE_USER);
        const auto& zone_auth_scheme = irods::get_server_property<const std::string>(irods::CFG_ZONE_AUTH_SCHEME);

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

void
daemonize( int runMode, int logFd ) {
    if ( runMode == SINGLE_PASS ) {
        return;
    }

    if ( runMode == STANDALONE_SERVER ) {
        if ( fork() ) {
            exit( 0 );
        }

        if ( setsid() < 0 ) {
            fprintf( stderr, "daemonize" );
            perror( "cannot create a new session." );
            exit( 1 );
        }
    }

    close( 0 );
    close( 1 );
    close( 2 );

    ( void ) dup2( logFd, 0 );
    ( void ) dup2( logFd, 1 );
    ( void ) dup2( logFd, 2 );
    close( logFd );
}

/* logFileOpen - Open the logFile for the reServer.
 *
 * Input - None
 * OutPut - the log file descriptor
 */

int
logFileOpen( int runMode, const char *logDir, const char *logFileName ) {
    char *logFile = NULL;
#ifdef SYSLOG
    int logFd = 0;
#else
    int logFd;
#endif

    if ( runMode == SINGLE_PASS && logDir == NULL ) {
        return 1;
    }

    if ( logFileName == NULL ) {
        fprintf( stderr, "logFileOpen: NULL input logFileName\n" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    getLogfileName( &logFile, logDir, logFileName );
    if ( NULL == logFile ) { // JMC cppcheck - nullptr
        fprintf( stderr, "logFileOpen: unable to open log file" );
        return -1;
    }

#ifndef SYSLOG
    logFd = open( logFile, O_CREAT | O_WRONLY | O_APPEND, 0666 );
#endif
    if ( logFd < 0 ) {
        fprintf( stderr, "logFileOpen: Unable to open %s. errno = %d\n",
                logFile, errno );
        free( logFile );
        return -1 * errno;
    }

    free( logFile );
    return logFd;
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
    if ( rsComm->sock != 0 ) {
        /* remove error messages from xmsLog */
        setLocalAddr( rsComm->sock, &rsComm->localAddr );
        setRemoteAddr( rsComm->sock, &rsComm->remoteAddr );
    }

    tmpStr = inet_ntoa( rsComm->remoteAddr.sin_addr );
    if ( tmpStr == NULL || *tmpStr == '\0' ) {
        tmpStr = "UNKNOWN";
    }
    rstrcpy( rsComm->clientAddr, tmpStr, NAME_LEN );

    return 0;
}

std::set<std::string>
construct_controlled_user_set(const std::unordered_map<std::string, boost::any>& controlled_user_connection_list) {
    std::set<std::string> user_set;
    try {
        for ( const auto& user : boost::any_cast<const std::vector<boost::any>&>(controlled_user_connection_list.at("users")) ) {
            user_set.insert(boost::any_cast<const std::string&>(user));
        }
    } catch ( const boost::bad_any_cast& e ) {
        THROW( INVALID_ANY_CAST, "controlled_user_connection_list must contain  a list of string values at key \"users\"" );
    } catch ( const std::out_of_range& e ) {
        THROW( KEY_NOT_FOUND, "controlled_user_connection_list must contain  a list of string values at key \"users\"" );
    }
    return user_set;
}

int
chkAllowedUser( const char *userName, const char *rodsZone ) {

    if ( userName == NULL || rodsZone == NULL ) {
        return SYS_USER_NOT_ALLOWED_TO_CONN;
    }

    if ( strlen( userName ) == 0 ) {
        /* XXXXXXXXXX userName not yet defined. allow it for now */
        return 0;
    }
    boost::optional<const std::unordered_map<std::string, boost::any>&> controlled_user_connection_list;
    try {
        controlled_user_connection_list.reset(irods::get_server_property<const std::unordered_map<std::string, boost::any> >("controlled_user_connection_list"));
    } catch ( const irods::exception& e ) {
        if ( e.code() == KEY_NOT_FOUND ) {
            return 0;
        }
        return e.code();
    }
    const auto user_and_zone = (boost::format("%s#%s") % userName % rodsZone).str();
    try {
        const auto& control_type = boost::any_cast<const std::string&>(controlled_user_connection_list->at("control_type"));
        static const auto controlled_user_set = construct_controlled_user_set(*controlled_user_connection_list);
        if ( control_type == "whitelist" ) {
            if ( controlled_user_set.count( user_and_zone ) == 0 ) {
                return SYS_USER_NOT_ALLOWED_TO_CONN;
            }
        }
        else if ( control_type == "blacklist" ) {
            if ( controlled_user_set.count( user_and_zone ) != 0 ) {
                return SYS_USER_NOT_ALLOWED_TO_CONN;
            }
        }
        else {
            rodsLog(LOG_ERROR, "controlled_user_connection_list must specify \"whitelist\" or \"blacklist\"");
            return -1;
        }
    } catch ( const irods::exception& e ) {
        irods::log(e);
        return e.code();
    } catch ( const std::out_of_range& e ) {
        rodsLog( LOG_ERROR, "%s", e.what());
        return KEY_NOT_FOUND;
    } catch ( const boost::bad_any_cast& e ) {
        rodsLog( LOG_ERROR, "%s", e.what());
        return INVALID_ANY_CAST;
    }
    return 0;

}

int
setRsCommFromRodsEnv( rsComm_t *rsComm ) {
    try {
        const auto& zone_name = irods::get_server_property<const std::string>(irods::CFG_ZONE_NAME);
        const auto& zone_user = irods::get_server_property<const std::string>(irods::CFG_ZONE_USER);

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

int
queueAgentProc( agentProc_t *agentProc, agentProc_t **agentProcHead,
        irodsPosition_t position ) {
    if ( agentProc == NULL || agentProcHead == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( *agentProcHead == NULL ) {
        *agentProcHead = agentProc;
        agentProc->next = NULL;
        return 0;
    }

    if ( position == TOP_POS ) {
        agentProc->next = *agentProcHead;
        *agentProcHead = agentProc;
    }
    else {
        agentProc_t *tmpAgentProc = *agentProcHead;
        while ( tmpAgentProc->next != NULL ) {
            tmpAgentProc = tmpAgentProc->next;
        }
        tmpAgentProc->next = agentProc;
        agentProc->next = NULL;
    }
    return 0;
}

// =-=-=-=-=-=-=-
// JMC - backport 4612
int
purgeLockFileDir( int chkLockFlag ) {
    char lockFilePath[MAX_NAME_LEN * 2];
    struct dirent *myDirent;
    struct stat statbuf;
    int status;
    int savedStatus = 0;
    struct flock myflock;
    uint purgeTime;

    std::string lock_dir;
    irods::error ret = irods::get_full_path_for_unmoved_configs(
            LOCK_FILE_DIR,
            lock_dir );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    DIR *dirPtr = opendir( lock_dir.c_str() );
    if ( dirPtr == NULL ) {
        rodsLog( LOG_ERROR,
                "purgeLockFileDir: opendir error for %s, errno = %d",
                lock_dir.c_str(), errno );
        return UNIX_FILE_OPENDIR_ERR - errno;
    }
    bzero( &myflock, sizeof( myflock ) );
    myflock.l_whence = SEEK_SET;
    purgeTime = time( 0 ) - LOCK_FILE_PURGE_TIME;
    while ( ( myDirent = readdir( dirPtr ) ) != NULL ) {
        if ( strcmp( myDirent->d_name, "." ) == 0 ||
                strcmp( myDirent->d_name, ".." ) == 0 ) {
            continue;
        }
        snprintf( lockFilePath, MAX_NAME_LEN, "%-s/%-s",
                lock_dir.c_str(), myDirent->d_name );
        if ( chkLockFlag ) {
            int myFd;
            myFd = open( lockFilePath, O_RDWR | O_CREAT, 0644 );
            if ( myFd < 0 ) {
                savedStatus = FILE_OPEN_ERR - errno;
                rodsLogError( LOG_ERROR, savedStatus,
                        "purgeLockFileDir: open error for %s", lockFilePath );
                continue;
            }
            myflock.l_type = F_WRLCK;
            int error_code = fcntl( myFd, F_GETLK, &myflock );
            if ( error_code != 0 ) {
                rodsLog( LOG_ERROR, "fcntl failed in purgeLockFileDir with error code %d", error_code );
            }
            close( myFd );
            /* some process is locking it */
            if ( myflock.l_type != F_UNLCK ) {
                continue;
            }
        }
        status = stat( lockFilePath, &statbuf );

        if ( status != 0 ) {
            rodsLog( LOG_ERROR,
                    "purgeLockFileDir: stat error for %s, errno = %d",
                    lockFilePath, errno );
            savedStatus = UNIX_FILE_STAT_ERR - errno;
            boost::system::error_code err;
            boost::filesystem::remove( boost::filesystem::path( lockFilePath ), err );
            continue;
        }
        if ( chkLockFlag && ( int )purgeTime < statbuf.st_mtime ) {
            continue;
        }
        if ( ( statbuf.st_mode & S_IFREG ) == 0 ) {
            continue;
        }
        boost::system::error_code err;
        boost::filesystem::remove( boost::filesystem::path( lockFilePath ), err );
    }
    closedir( dirPtr );

    return savedStatus;
}

// =-=-=-=-=-=-=-
