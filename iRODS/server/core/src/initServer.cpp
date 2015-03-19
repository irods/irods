/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.cpp - Server initialization routines
 */

#include "initServer.hpp"
#include "rodsConnect.h"
#include "rodsConnect.h"
#include "procLog.h"
#include "resource.hpp"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.hpp"
#include "genQuery.hpp"
#include "rsIcatOpr.hpp"
#include "miscServerFunct.hpp"
#include "reGlobalsExtern.hpp"
#include "reDefines.hpp"
#include "getRemoteZoneResc.hpp"
#include "getRescQuota.hpp"
#include "physPath.hpp"
#include "reFuncDefs.hpp"
#include "irods_stacktrace.hpp"

#include "irods_get_full_path_for_config_file.hpp"
#include "irods_configuration_parser.hpp"
#include "irods_resource_backport.hpp"
#include "irods_log.hpp"
#include "irods_threads.hpp"
#include "irods_server_properties.hpp"

#include <vector>
#include <string>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

static time_t LastBrokenPipeTime = 0;
static int BrokenPipeCnt = 0;

int
initServerInfo( rsComm_t *rsComm ) {
    int status = 0;

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    int zone_port = 0;
    ret = props.get_property <
          int > (
              irods::CFG_ZONE_PORT,
              zone_port );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

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

#ifdef RODS_CAT
    status = connectRcat();
    if ( status < 0 ) {
        rodsLog(
            LOG_SYS_FATAL,
            "initServerInfo: connectRcat failed, status = %d",
            status );

        return status;
    }
#endif

    status = initZone( rsComm );
    if ( status < 0 ) {
        rodsLog( LOG_SYS_FATAL,
                 "initServerInfo: initZone error, status = %d",
                 status );
        return status;
    }

    ret = resc_mgr.init_from_catalog( rsComm );
    if ( !ret.ok() ) {
        irods::error log_err = PASSMSG( "init_from_catalog failed", ret );
        irods::log( log_err );
    }

    return status;
}

int
initLocalServerHost() {
    int status;
    char myHostName[MAX_NAME_LEN];

    LocalServerHost = ServerHostHead = ( rodsServerHost_t* )malloc( sizeof( rodsServerHost_t ) );
    memset( ServerHostHead, 0, sizeof( rodsServerHost_t ) );

    LocalServerHost->localFlag = LOCAL_HOST;
    LocalServerHost->zoneInfo = ZoneInfoHead;

    status = matchHostConfig( LocalServerHost );

    queHostName( ServerHostHead, "localhost", 0 );
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
    typedef irods::configuration_parser::object_t object_t;
    typedef irods::configuration_parser::array_t  array_t;

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    object_t env_obj;
    ret = props.get_property <
          object_t > (
              irods::CFG_ENVIRONMENT_VARIABLES_KW,
              env_obj );

    if ( ret.ok() ) {
        object_t::iterator itr;
        for ( itr = env_obj.begin();
                itr != env_obj.end();
                ++itr ) {
            std::string val = boost::any_cast <
                              std::string > (
                                  itr->second );
            setenv(
                itr->first.c_str(), // variable
                val.c_str(),        // value
                1 );                // overwrite
            rodsLog(
                LOG_DEBUG,
                "environment setting [%s]=[%s]",
                itr->first.c_str(),
                getenv( itr->first.c_str() ) );

        }

    }
    else {
        rodsLog(
            LOG_NOTICE,
            "initRcatServerHostByFile - did not get [%s] property",
            irods::CFG_ENVIRONMENT_VARIABLES_KW.c_str() );
    }

    array_t prop_arr;
    ret = props.get_property <
          array_t > (
              irods::CFG_RE_RULEBASE_SET_KW,
              prop_arr );

    std::string prop_str;
    if ( ret.ok() ) {
        std::string rule_arr;
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            rule_arr += boost::any_cast< std::string >(
                            prop_arr[i][ irods::CFG_FILENAME_KW ] );
            rule_arr += prop_str + ",";
        }

        rule_arr = rule_arr.substr( 0, rule_arr.size() - 1 );

        snprintf(
            reRuleStr,
            sizeof( reRuleStr ),
            "%s",
            rule_arr.c_str() );

    }
    else {
        std::string prop_str;
        ret = props.get_property< std::string >(
                  RE_RULESET_KW,
                  prop_str );
        if ( ret.ok() ) {
            snprintf(
                reRuleStr,
                sizeof( reRuleStr ),
                "%s",
                prop_str.c_str() );

        }
        else {
            irods::log( PASS( ret ) );
            return ret.code();
        }

    }

    ret = props.get_property <
          array_t > (
              irods::CFG_RE_FUNCTION_NAME_MAPPING_SET_KW,
              prop_arr );
    if ( ret.ok() ) {
        std::string rule_arr;
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            rule_arr += boost::any_cast< std::string >(
                            prop_arr[i][ irods::CFG_FILENAME_KW ] );
            rule_arr += prop_str + ",";
        }

        rule_arr = rule_arr.substr( 0, rule_arr.size() - 1 );

        snprintf(
            reFuncMapStr,
            sizeof( reFuncMapStr ),
            "%s",
            rule_arr.c_str() );

    }
    else {
        ret = props.get_property< std::string >(
                  RE_FUNCMAPSET_KW,
                  prop_str );
        if ( ret.ok() ) {
            snprintf(
                reFuncMapStr,
                sizeof( reFuncMapStr ),
                "%s",
                prop_str.c_str() );

        }
        else {
            irods::log( PASS( ret ) );
            return ret.code();
        }
    }

    ret = props.get_property <
          array_t > (
              irods::CFG_RE_DATA_VARIABLE_MAPPING_SET_KW,
              prop_arr );
    if ( ret.ok() ) {
        std::string rule_arr;
        for ( size_t i = 0;
                i < prop_arr.size();
                ++i ) {
            rule_arr += boost::any_cast< std::string >(
                            prop_arr[i][ irods::CFG_FILENAME_KW ] );
            rule_arr += prop_str + ",";
        }

        rule_arr = rule_arr.substr( 0, rule_arr.size() - 1 );

        snprintf(
            reVariableMapStr,
            sizeof( reVariableMapStr ),
            "%s",
            rule_arr.c_str() );

    }
    else {
        ret = props.get_property< std::string >(
                  RE_VARIABLEMAPSET_KW,
                  prop_str );
        if ( ret.ok() ) {
            snprintf(
                reVariableMapStr,
                sizeof( reVariableMapStr ),
                "%s",
                prop_str.c_str() );

        }
        else {
            irods::log( PASS( ret ) );
            return ret.code();
        }
    }

    ret = props.get_property< std::string >(
              KERBEROS_NAME_KW,
              prop_str );
    if ( ret.ok() ) {
        snprintf(
            KerberosName,
            sizeof( KerberosName ),
            "%s",
            prop_str.c_str() );

    }

    ret = props.get_property< std::string >(
              ICAT_HOST_KW,
              prop_str );
    if ( ret.ok() ) {
        rodsHostAddr_t    addr;
        memset( &addr, 0, sizeof( addr ) );
        rodsServerHost_t* tmp_host = 0;
        snprintf(
            addr.hostAddr,
            sizeof( addr.hostAddr ),
            "%s",
            prop_str.c_str() );
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

    }
    else {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // re host
    // xmsg host
    ret = props.get_property< std::string >(
              irods::CFG_IRODS_XMSG_HOST_KW,
              prop_str );
    if ( ret.ok() ) {
        rodsHostAddr_t    addr;
        memset( &addr, 0, sizeof( addr ) );
        rodsServerHost_t* tmp_host = 0;
        snprintf(
            addr.hostAddr,
            sizeof( addr.hostAddr ),
            "%s",
            prop_str.c_str() );
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
    }

    // slave icat host

    ret = props.get_property< std::string >(
              LOCAL_ZONE_SID_KW,
              prop_str );
    if ( ret.ok() ) {
        snprintf( localSID, sizeof( localSID ), "%s", prop_str.c_str() );
    }
    else {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    // try for new federation config
    array_t fed_arr;
    ret = props.get_property <
          array_t > (
              irods::CFG_FEDERATION_KW,
              fed_arr );
    if ( ret.ok() ) {
        for ( size_t i = 0; i < fed_arr.size(); ++i ) {
            std::string fed_zone_id   = boost::any_cast< std::string >(
                                            fed_arr[ i ][ irods::CFG_ZONE_ID_KW ] );
            std::string fed_zone_name = boost::any_cast< std::string >(
                                            fed_arr[ i ][ irods::CFG_ZONE_NAME_KW ] );
            std::string fed_zone_key = boost::any_cast< std::string >(
                                           fed_arr[ i ][ irods::CFG_NEGOTIATION_KEY_KW ] );
            // store in remote_SID_key_map
            remote_SID_key_map[fed_zone_name] = std::make_pair( fed_zone_id, fed_zone_key );
        }
    }
    else {
        // try the old remote sid config
        std::vector< std::string > rem_sids;
        ret = props.get_property <
              std::vector< std::string > > (
                  REMOTE_ZONE_SID_KW,
                  rem_sids );
        if ( ret.ok() ) {
            for ( size_t i = 0; i < rem_sids.size(); ++i ) {
                // legacy format should be zone_name-SID
                size_t pos = rem_sids[i].find( "-" );
                if ( pos == std::string::npos ) {
                    rodsLog( LOG_ERROR, "initRcatServerHostByFile - Unable to parse remote SID %s", rem_sids[i].c_str() );
                }
                else {
                    // store in remote_SID_key_map
                    std::string fed_zone_name = rem_sids[i].substr( 0, pos );
                    std::string fed_zone_id = rem_sids[i].substr( pos + 1 );
                    remote_SID_key_map[fed_zone_name] = std::make_pair( fed_zone_id, "" );
                }
            }
        }
    } // else

    return 0;
}

int
initZone( rsComm_t *rsComm ) {
    rodsServerHost_t *tmpRodsServerHost;
    rodsServerHost_t *masterServerHost = NULL;
    rodsServerHost_t *slaveServerHost = NULL;
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut = NULL;
    int status, i;
    sqlResult_t *zoneName, *zoneType, *zoneConn, *zoneComment;
    char *tmpZoneName, *tmpZoneType, *tmpZoneConn;//, *tmpZoneComment;

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

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
            if ( strcmp( zone_name.c_str(), tmpZoneName ) != 0 ) {
                rodsLog( LOG_ERROR,
                         "initZone: zoneName in env %s does not match %s in icat ",
                         zone_name.c_str(), tmpZoneName );
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
    int status;
    rsComm_t myComm;
    ruleExecInfo_t rei;

    initProcLog();

    status = initServerInfo( rsComm );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: initServerInfo error, status = %d",
                 status );
        return status;
    }

    initL1desc();
    initSpecCollDesc();
    initCollHandle();
    status = initFileDesc();
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: initFileDesc error, status = %d",
                 status );
        return status;
    }
#ifdef TAR_STRUCT_FILE
//    initStructFileDesc ();
//    initTarSubFileDesc ();
#endif
    status = initRuleEngine( processType, rsComm, reRuleStr, reVariableMapStr, reFuncMapStr );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: initRuleEngine error, status = %d", status );
        return status;
    }

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

    status = seedRandom();
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "initAgent: seedRandom error, status = %d",
                 status );
        return status;
    }

#ifndef windows_platform
    if ( rsComm->reconnFlag == RECONN_TIMEOUT ) {
        rsComm->reconnSock = svrSockOpenForInConn( rsComm, &rsComm->reconnPort,
                             &rsComm->reconnAddr, SOCK_STREAM );
        if ( rsComm->reconnSock < 0 ) {
            rsComm->reconnPort = 0;
            rsComm->reconnAddr = NULL;
        }
        else {
            rsComm->cookie = random();
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
#endif

    InitialState = INITIAL_DONE;
    ThisComm = rsComm;

    /* use a tmp myComm is needed to get the permission right for the call */
    myComm = *rsComm;
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
#ifdef RODS_CAT
    disconnectRcat();
#endif

    finalizeRuleEngine();

    if ( InitialState == INITIAL_DONE ) {
        /* close all opened descriptors */
        closeAllL1desc( ThisComm );
        /* close any opened server to server connection */
        disconnectAllSvrToSvrConn();
    }
}

void
cleanupAndExit( int status ) {
    rodsLog( LOG_NOTICE,
             "Agent exiting with status = %d", status );

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
    rodsLog( LOG_NOTICE,
             "caught a signal and exiting\n" );
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
#ifndef _WIN32
        signal( SIGPIPE, ( void ( * )( int ) ) rsPipeSignalHandler );
#endif

    }
}

/// @brief parse the irodsHost file, creating address
int initHostConfigByFile() {
    typedef irods::configuration_parser::object_t object_t;
    typedef irods::configuration_parser::array_t  array_t;

    // =-=-=-=-=-=-=-
    // request fully qualified path to the config file
    std::string cfg_file;
    irods::error ret = irods::get_full_path_for_config_file(
                           HOST_CONFIG_FILE,
                           cfg_file );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    irods::configuration_parser cfg;
    ret = cfg.load( cfg_file );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    array_t host_entries;
    ret = cfg.get< array_t > (
              "host_entries",
              host_entries );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    for ( size_t he_idx = 0;
            he_idx < host_entries.size();
            ++he_idx ) {
        object_t obj = host_entries[ he_idx ];

        std::string address_type;
        ret = obj.get< std::string >(
                  "address_type",
                  address_type );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        array_t addresses;
        ret = obj.get< array_t >(
                  "addresses",
                  addresses );
        if ( !ret.ok() ) {
            irods::log( PASS( ret ) );
            continue;
        }

        rodsServerHost_t* svr_host = ( rodsServerHost_t* )malloc(
                                         sizeof( rodsServerHost_t ) );
        memset(
            svr_host,
            0,
            sizeof( rodsServerHost_t ) );

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

        for ( size_t addr_idx = 0;
                addr_idx < addresses.size();
                ++addr_idx ) {

            object_t& obj_ent = addresses[ addr_idx ];

            std::string entry;
            ret = obj_ent.get< std::string >(
                      "address",
                      entry );
            if ( !ret.ok() ) {
                irods::log( PASS( ret ) );
                continue;
            }

            if ( queHostName(
                        svr_host,
                        entry.c_str(),
                        0 ) < 0 ) {
                rodsLog(
                    LOG_ERROR,
                    "queHostName failed" );

            }

        } // for addr_idx

    } // for i

    return 0;

} // initHostConfigByFile

int
initRsComm( rsComm_t *rsComm ) {
    memset( rsComm, 0, sizeof( rsComm_t ) );

    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    std::string zone_user;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_USER,
              zone_user );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    std::string zone_auth_scheme;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_AUTH_SCHEME,
              zone_auth_scheme );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

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

    return 0;
}

void
daemonize( int runMode, int logFd ) {
#ifndef _WIN32
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
#endif
}

/* logFileOpen - Open the logFile for the reServer.
 *
 * Input - None
 * OutPut - the log file descriptor
 */

int
logFileOpen( int runMode, char *logDir, char *logFileName ) {
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
        rstrcpy( rsComm->option, startupPack->option, NAME_LEN );
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
#ifndef windows_platform
        if ( tmpStr == NULL ) {
            rodsLog( LOG_NOTICE,
                     "initRsCommWithStartupPack: env %s does not exist",
                     SP_OPTION );
        }
        else {
            rstrcpy( rsComm->option, tmpStr, NAME_LEN );
        }
#else
        if ( tmpStr != NULL ) {
            rstrcpy( rsComm->option, tmpStr, NAME_LEN );
        }
#endif

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

int
initConnectControl() {
    char buf[LONG_NAME_LEN * 5];
    char *bufPtr;
    int status;
    struct allowedUser *tmpAllowedUser;
    int allowUserFlag = 0;
    int disallowUserFlag = 0;
    char *configDir = getConfigDir();
    int len = strlen( configDir ) + strlen( CONNECT_CONTROL_FILE ) + 2;
    char *conFile = ( char * ) malloc( len );

    snprintf( conFile, len, "%s/%s", configDir, CONNECT_CONTROL_FILE );
    FILE *file = fopen( conFile, "r" );

    if ( file == NULL ) {
#ifdef DEBUG_CONNECT_CONTROL
        fprintf( stderr, "Unable to open CONNECT_CONTROL_FILE file %s\n",
                 conFile );
#endif
        free( conFile );
        return 0;
    }

    free( conFile );

    MaxConnections = DEF_MAX_CONNECTION;        /* no limit */
    freeAllAllowedUser( AllowedUserHead );
    freeAllAllowedUser( DisallowedUserHead );
    AllowedUserHead = DisallowedUserHead = NULL;
    while ( fgets( buf, LONG_NAME_LEN * 5, file ) != NULL ) {
        char myuser[NAME_LEN];
        char myZone[NAME_LEN];
        char myInput[NAME_LEN * 2];

        if ( *buf == '#' ) {    /* a comment */
            continue;
        }

        bufPtr = buf;

        while ( copyStrFromBuf( &bufPtr, myInput, NAME_LEN * 2 ) > 0 ) {
            if ( strcmp( myInput, MAX_CONNECTIONS_KW ) == 0 ) {
                if ( copyStrFromBuf( &bufPtr, myInput, NAME_LEN ) > 0 ) {
                    /* sanity check */
                    if ( isdigit( *myInput ) ) {
                        MaxConnections = atoi( myInput );
                    }
                    else {
                        rodsLog( LOG_ERROR,
                                 "initConnectControl: inp maxConnections %d is not an int",
                                 myInput );
                    }
                    break;
                }
            }
            else if ( strcmp( myInput, ALLOWED_USER_LIST_KW ) == 0 ) {
                if ( disallowUserFlag == 0 ) {
                    allowUserFlag = 1;
                    break;
                }
                else {
                    rodsLog( LOG_ERROR,
                             "initConnectControl: both allowUserList and disallowUserList are set" );
                    fclose( file );
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
                }
            }
            else if ( strcmp( myInput, DISALLOWED_USER_LIST_KW ) == 0 ) {
                if ( allowUserFlag == 0 ) {
                    disallowUserFlag = 1;
                    break;
                }
                else {
                    rodsLog( LOG_ERROR,
                             "initConnectControl: both allowUserList and disallowUserList are set" );
                    fclose( file );
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
                }
            }
            status = parseUserName( myInput, myuser, myZone );
            if ( status >= 0 ) {
                if ( strlen( myZone ) == 0 ) {
                    zoneInfo_t *tmpZoneInfo;
                    if ( getLocalZoneInfo( &tmpZoneInfo ) >= 0 ) {
                        rstrcpy( myZone, tmpZoneInfo->zoneName, NAME_LEN );
                    }
                }
                tmpAllowedUser = ( struct allowedUser* )
                                 malloc( sizeof( struct allowedUser ) );
                memset( tmpAllowedUser, 0, sizeof( struct allowedUser ) );
                rstrcpy( tmpAllowedUser->userName, myuser, NAME_LEN );
                rstrcpy( tmpAllowedUser->rodsZone, myZone, NAME_LEN );
                /* queue it */

                if ( allowUserFlag != 0 ) {
                    queAllowedUser( tmpAllowedUser, &AllowedUserHead );
                }
                else if ( disallowUserFlag != 0 ) {
                    queAllowedUser( tmpAllowedUser, &DisallowedUserHead );
                }
                else {
                    rodsLog( LOG_ERROR,
                             "initConnectControl: neither allowUserList nor disallowUserList has been set" );
                    return SYS_CONNECT_CONTROL_CONFIG_ERR;
                }
            }
            else {
                rodsLog( LOG_NOTICE,
                         "initConnectControl: cannot parse input %s. status = %d",
                         myInput, status );
            }
        }
    }

    fclose( file );
    return 0;
}

int
chkAllowedUser( char *userName, char *rodsZone ) {
    int status;

    if ( userName == NULL || rodsZone == 0 ) {
        return SYS_USER_NOT_ALLOWED_TO_CONN;
    }

    if ( strlen( userName ) == 0 ) {
        /* XXXXXXXXXX userName not yet defined. allow it for now */
        return 0;
    }

    if ( AllowedUserHead != NULL ) {
        status = matchAllowedUser( userName, rodsZone, AllowedUserHead );
        if ( status == 1 ) {    /* a match */
            return 0;
        }
        else {
            return SYS_USER_NOT_ALLOWED_TO_CONN;
        }
    }
    else if ( DisallowedUserHead != NULL ) {
        status = matchAllowedUser( userName, rodsZone, DisallowedUserHead );
        if ( status == 1 ) {    /* a match, disallow */
            return SYS_USER_NOT_ALLOWED_TO_CONN;
        }
        else {
            return 0;
        }
    }
    else {
        /* no control, return 0 */
        return 0;
    }
}

int
matchAllowedUser( char *userName, char *rodsZone,
                  struct allowedUser *allowedUserHead ) {
    struct allowedUser *tmpAllowedUser;

    if ( allowedUserHead == NULL ) {
        return 0;
    }

    tmpAllowedUser = allowedUserHead;
    while ( tmpAllowedUser != NULL ) {
        if ( strcmp( tmpAllowedUser->userName, userName ) == 0 &&
                strcmp( tmpAllowedUser->rodsZone, rodsZone ) == 0 ) {
            /* we have a match */
            break;
        }
        tmpAllowedUser = tmpAllowedUser->next;
    }
    if ( tmpAllowedUser == NULL ) {
        /* no match */
        return 0;
    }
    else {
        return 1;
    }
}

int
queAllowedUser( struct allowedUser *allowedUser,
                struct allowedUser **allowedUserHead ) {
    if ( allowedUserHead == NULL || allowedUser == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( *allowedUserHead == NULL ) {
        *allowedUserHead = allowedUser;
    }
    else {
        allowedUser->next = *allowedUserHead;
        *allowedUserHead = allowedUser;
    }
    return 0;
}

int
freeAllAllowedUser( struct allowedUser *allowedUserHead ) {
    struct allowedUser *tmpAllowedUser, *nextAllowedUser;
    tmpAllowedUser = allowedUserHead;
    while ( tmpAllowedUser != NULL ) {
        nextAllowedUser = tmpAllowedUser->next;
        free( tmpAllowedUser );
        tmpAllowedUser = nextAllowedUser;
    }
    return 0;
}

int
setRsCommFromRodsEnv( rsComm_t *rsComm ) {
    irods::server_properties& props = irods::server_properties::getInstance();
    irods::error ret = props.capture_if_needed();
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();
    }

    std::string zone_name;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_NAME,
              zone_name );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    std::string zone_user;
    ret = props.get_property <
          std::string > (
              irods::CFG_ZONE_USER,
              zone_user );
    if ( !ret.ok() ) {
        irods::log( PASS( ret ) );
        return ret.code();

    }

    rstrcpy( rsComm->proxyUser.userName,  zone_user.c_str(), NAME_LEN );
    rstrcpy( rsComm->clientUser.userName, zone_user.c_str(), NAME_LEN );

    rstrcpy( rsComm->proxyUser.rodsZone,  zone_name.c_str(), NAME_LEN );
    rstrcpy( rsComm->clientUser.rodsZone, zone_name.c_str(), NAME_LEN );

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
    char lockFileDir[MAX_NAME_LEN];
    char lockFilePath[MAX_NAME_LEN * 2];
    struct dirent *myDirent;
    struct stat statbuf;
    int status;
    int savedStatus = 0;
    struct flock myflock;
    uint purgeTime;

    snprintf( lockFileDir, MAX_NAME_LEN, "%-s/%-s", getConfigDir(), LOCK_FILE_DIR );

    DIR *dirPtr = opendir( lockFileDir );
    if ( dirPtr == NULL ) {
        rodsLog( LOG_ERROR,
                 "purgeLockFileDir: opendir error for %s, errno = %d",
                 lockFileDir, errno );
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
                  lockFileDir, myDirent->d_name );
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
