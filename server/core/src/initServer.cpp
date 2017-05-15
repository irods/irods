/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.cpp - Server initialization routines
 */

#include "rcMisc.h"
#include "initServer.hpp"
#include "rodsConnect.h"
#include "rodsConnect.h"
#include "procLog.h"
#include "resource.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "genQuery.h"
#include "rsIcatOpr.hpp"
#include "miscServerFunct.hpp"
#include "getRemoteZoneResc.h"
#include "getRescQuota.h"
#include "physPath.hpp"
#include "rsLog.hpp"
#include "sockComm.h"
#include "irods_stacktrace.hpp"
#include "rsGenQuery.hpp"
#include "rsExecCmd.hpp"

#include "irods_get_full_path_for_config_file.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_log.hpp"
#include "irods_exception.hpp"
#include "irods_threads.hpp"
#include "irods_server_properties.hpp"
#include "irods_random.hpp"

#include <vector>
#include <set>
#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>

static time_t LastBrokenPipeTime = 0;
static int BrokenPipeCnt = 0;


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

        /* que the local zone */
        status = queZone(
                zone_name.c_str(),
                zone_port, NULL, NULL );
        if ( status < 0 ) {
            rodsLog(
                    LOG_DEBUG,
                    "initServerInfo - queZone failed %d",
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

    int status = matchHostConfig( LocalServerHost );

    queHostName( ServerHostHead, "localhost", 0 );
    char myHostName[MAX_NAME_LEN];
    status = gethostname( myHostName, MAX_NAME_LEN );
    if ( status < 0 ) {
        status = SYS_GET_HOSTNAME_ERR - errno;
        rodsLog( LOG_NOTICE,
                 "initLocalServerHost: gethostname error, status = %d",
                 status );
        return status;
    }
    status = queHostName( ServerHostHead, myHostName, 0 );
    if ( status < 0 ) {
        return status;
    }

    status = queAddr( ServerHostHead, myHostName );
    if ( status < 0 ) {
        /* some configuration may not be able to resolve myHostName. So don't
         * exit. Just print out warning */
        rodsLog( LOG_NOTICE,
                 "initLocalServerHost: queAddr error, status = %d",
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

    // re host
    // xmsg host
    try {
        rodsHostAddr_t    addr;
        memset( &addr, 0, sizeof( addr ) );
        rodsServerHost_t* tmp_host = 0;
        snprintf( addr.hostAddr, sizeof( addr.hostAddr ), "%s", irods::get_server_property<const std::string>(irods::CFG_IRODS_XMSG_HOST_KW).c_str() );
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
        tmp_host->xmsgHostFlag = 1;
    } catch ( const irods::exception& e ) {}

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
        queZone( tmpZoneName, addr.portNum, tmpRodsServerHost, NULL );
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

    if( irods::CFG_SERVICE_ROLE_PROVIDER == svc_role ) {
        disconnectRcat();
    }

    if ( InitialState == INITIAL_DONE ) {
        /* close all opened descriptors */
        closeAllL1desc( ThisComm );
        /* close any opened server to server connection */
        disconnectAllSvrToSvrConn();
    }
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
int initHostConfigByFile() {

    // =-=-=-=-=-=-=-
    // request fully qualified path to the config file
    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file(
                           HOST_CONFIG_FILE,
                           cfg_file );
    if ( !ret.ok() ) {
        rodsLog(
            LOG_NOTICE,
            "config file [%s] not found",
            HOST_CONFIG_FILE );
        return 0;
    }

    irods::configuration_parser cfg;
    ret = cfg.load( cfg_file );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    try {
        for ( const auto& el : cfg.get< const std::vector< boost::any > > ("host_entries") ) {
            try {
                const auto& host_entry = boost::any_cast<const std::unordered_map<std::string, boost::any>&>(el);
                const auto& address_type = boost::any_cast<const std::string&>(host_entry.at("address_type"));
                const auto& addresses = boost::any_cast<const std::vector<boost::any>&>(host_entry.at("addresses"));

                rodsServerHost_t* svr_host = ( rodsServerHost_t* )malloc( sizeof( rodsServerHost_t ) );
                memset( svr_host, 0, sizeof( rodsServerHost_t ) );

                if ( "remote" == address_type ) {
                    svr_host->localFlag = REMOTE_HOST;
                }
                else if ( "local" == address_type ) {
                    svr_host->localFlag = LOCAL_HOST;

                }
                else {
                    free( svr_host );
                    rodsLog(
                            LOG_ERROR,
                            "unsupported address type [%s]",
                            address_type.c_str() );
                    continue;
                }

                // local zone
                svr_host->zoneInfo = ZoneInfoHead;
                if ( queRodsServerHost(
                            &HostConfigHead,
                            svr_host ) < 0 ) {
                    rodsLog(
                            LOG_ERROR,
                            "queRodsServerHost failed" );
                }

                for ( const auto& el : addresses ) {
                    try {
                        if ( queHostName(
                                    svr_host,
                                    boost::any_cast<const std::string&>(
                                        boost::any_cast<const std::unordered_map<std::string, boost::any>&>(el
                                            ).at("address")
                                        ).c_str(),
                                    0 ) < 0 ) {
                            rodsLog( LOG_ERROR, "queHostName failed" );
                        }

                    } catch ( const boost::bad_any_cast& e ) {
                        irods::log( ERROR( INVALID_ANY_CAST, e.what() ) );
                        continue;
                    } catch ( const std::out_of_range& e ) {
                        irods::log( ERROR( KEY_NOT_FOUND, e.what() ) );
                    }

                } // for addr_idx
            } catch ( const boost::bad_any_cast& e ) {
                irods::log( ERROR( INVALID_ANY_CAST, e.what() ) );
                continue;
            } catch ( const std::out_of_range& e ) {
                irods::log( ERROR( KEY_NOT_FOUND, e.what() ) );
            }

        } // for i
    } catch ( const irods::exception& e ) {
        irods::log( irods::error(e) );
        return e.code();
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
queAgentProc( agentProc_t *agentProc, agentProc_t **agentProcHead,
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
