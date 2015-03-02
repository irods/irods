/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.cpp - Server initialization routines
 */

#include "initServer.hpp"
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

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

static time_t LastBrokenPipeTime = 0;
static int BrokenPipeCnt = 0;

int
resolveHost( rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost = 0;
    char *myHostAddr = 0;
    char *myZoneName = 0;

    /* check if host exist */

    myHostAddr = addr->hostAddr;

    if ( strlen( myHostAddr ) == 0 ) {
        *rodsServerHost = ServerHostHead;
        return LOCAL_HOST;
    }
    if ( strlen( addr->zoneName ) == 0 ) {
        myZoneName = ZoneInfoHead->zoneName;
    }
    else {
        myZoneName = addr->zoneName;
    }

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        hostName_t *tmpName;
        zoneInfo_t *serverZoneInfo = ( zoneInfo_t * ) tmpRodsServerHost->zoneInfo;
        if ( strcmp( myZoneName, serverZoneInfo->zoneName ) == 0 ) {
            tmpName = tmpRodsServerHost->hostName;
            while ( tmpName != NULL ) {
                if ( strcasecmp( tmpName->name, myHostAddr ) == 0 ) {
                    *rodsServerHost = tmpRodsServerHost;
                    return tmpRodsServerHost->localFlag;
                }
                tmpName = tmpName->next;
            }
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    /* no match */

    tmpRodsServerHost = mkServerHost( myHostAddr, myZoneName );

    if ( tmpRodsServerHost == NULL ) {
        rodsLog( LOG_ERROR,
                 "resolveHost: mkServerHost error" );
        return SYS_INVALID_SERVER_HOST;
    }

    /* assume it is remote */
    if ( tmpRodsServerHost->localFlag == UNKNOWN_HOST_LOC ) {
        tmpRodsServerHost->localFlag = REMOTE_HOST;
    }

    int status = queRodsServerHost( &ServerHostHead, tmpRodsServerHost );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "resolveHost - queRodsServerHost failed." );
    }
    *rodsServerHost = tmpRodsServerHost;

    return tmpRodsServerHost->localFlag;
}

int
resoAndConnHostByDataObjInfo( rsComm_t *rsComm, dataObjInfo_t *dataObjInfo,
                              rodsServerHost_t **rodsServerHost ) {
    int status;
    rodsHostAddr_t addr;
    int remoteFlag;
    if ( dataObjInfo == NULL ) {
        rodsLog( LOG_NOTICE,
                 "resolveHostByDataObjInfo: NULL dataObjInfo" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    // =-=-=-=-=-=-=-
    // extract the host location from the resource hierarchy
    std::string location;
    irods::error ret = irods::get_loc_for_hier_string( dataObjInfo->rescHier, location );
    if ( !ret.ok() ) {
        irods::log( PASSMSG( "resoAndConnHostByDataObjInfo - failed in get_loc_for_hier_string", ret ) );
        return ret.code();
    }


    memset( &addr, 0, sizeof( addr ) );
    rstrcpy( addr.hostAddr, location.c_str(), NAME_LEN );

    remoteFlag = resolveHost( &addr, rodsServerHost );

    if ( remoteFlag == REMOTE_HOST ) {
        status = svrToSvrConnect( rsComm, *rodsServerHost );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "resAndConnHostByDataObjInfo: svrToSvrConnect to %s failed",
                     ( *rodsServerHost )->hostName->name );
        }
    }
    return remoteFlag;
}

rodsServerHost_t *
mkServerHost( char *myHostAddr, char *zoneName ) {
    rodsServerHost_t *tmpRodsServerHost;
    int status;

    tmpRodsServerHost = ( rodsServerHost_t* )malloc( sizeof( rodsServerHost_t ) );
    memset( tmpRodsServerHost, 0, sizeof( rodsServerHost_t ) );

    /* XXXXX need to lookup the zone table when availiable */
    status = queHostName( tmpRodsServerHost, myHostAddr, 0 );
    if ( status < 0 ) {
        free( tmpRodsServerHost );
        return NULL;
    }

    tmpRodsServerHost->localFlag = UNKNOWN_HOST_LOC;

    status = queAddr( tmpRodsServerHost, myHostAddr );

    status = matchHostConfig( tmpRodsServerHost );

    status = getZoneInfo( zoneName,
                          ( zoneInfo_t ** )( static_cast< void * >( &tmpRodsServerHost->zoneInfo ) ) );

    if ( status < 0 ) {
        free( tmpRodsServerHost );
        return NULL;
    }
    else {
        return tmpRodsServerHost;
    }
}

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
printServerHost( rodsServerHost_t *myServerHost ) {
    hostName_t *tmpHostName;

    if ( myServerHost->localFlag == LOCAL_HOST ) {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    LocalHostName: " );
#else /* SYSLOG */
        fprintf( stderr, "    LocalHostName: " );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, "    LocalHostName: " );
#endif
    }
    else {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    RemoteHostName: " );
#else /* SYSLOG */
        fprintf( stderr, "    RemoteHostName: " );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, "    RemoteHostName: " );
#endif
    }

    tmpHostName = myServerHost->hostName;

    while ( tmpHostName != NULL ) {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, " %s,", tmpHostName->name );
#else /* SYSLOG */
        fprintf( stderr, " %s,", tmpHostName->name );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, " %s,", tmpHostName->name );
#endif
        tmpHostName = tmpHostName->next;
    }

#ifndef windows_platform
#ifdef SYSLOG
    rodsLog( LOG_NOTICE, " Port Num: %d.\n\n", ( ( zoneInfo_t * )myServerHost->zoneInfo )->portNum );
#else /* SYSLOG */
    fprintf( stderr, " Port Num: %d.\n\n",
             ( ( zoneInfo_t * )myServerHost->zoneInfo )->portNum );
#endif /* SYSLOG */
#else
    rodsLog( LOG_NOTICE, " Port Num: %d.\n\n", ( ( zoneInfo_t * )myServerHost->zoneInfo )->portNum );
#endif

    return 0;
}

int
printZoneInfo() {
    zoneInfo_t *tmpZoneInfo;
    rodsServerHost_t *tmpRodsServerHost;

    tmpZoneInfo = ZoneInfoHead;
#ifndef windows_platform
#ifdef SYSLOG
    rodsLog( LOG_NOTICE, "Zone Info:\n" );
#else /* SYSLOG */
    fprintf( stderr, "Zone Info:\n" );
#endif /* SYSLOG */
#else
    rodsLog( LOG_NOTICE, "Zone Info:\n" );
#endif
    while ( tmpZoneInfo != NULL ) {
        /* print the master */
        tmpRodsServerHost = ( rodsServerHost_t * ) tmpZoneInfo->masterServerHost;
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
#else /* SYSLOG */
        fprintf( stderr, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
#endif
        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ) {
#ifndef windows_platform
#ifdef SYSLOG
            rodsLog( LOG_NOTICE, "Type: LOCAL_ICAT   " );
#else /* SYSLOG */
            fprintf( stderr, "Type: LOCAL_ICAT   " );
#endif /* SYSLOG */
#else
            rodsLog( LOG_NOTICE, "Type: LOCAL_ICAT   " );
#endif
        }
        else {
#ifndef windows_platform
#ifdef SYSLOG
            rodsLog( LOG_NOTICE, "Type: REMOTE_ICAT   " );
#else /* SYSLOG */
            fprintf( stderr, "Type: REMOTE_ICAT   " );
#endif /* SYSLOG */
#else
            rodsLog( LOG_NOTICE, "Type: REMOTE_ICAT   " );
#endif
        }

#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
                 tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#else /* SYSLOG */
        fprintf( stderr, " HostAddr: %s   PortNum: %d\n\n",
                 tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
                 tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#endif

        /* print the slave */
        tmpRodsServerHost = ( rodsServerHost_t * ) tmpZoneInfo->slaveServerHost;
        if ( tmpRodsServerHost != NULL ) {
#ifndef windows_platform
#ifdef SYSLOG
            rodsLog( LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
            rodsLog( LOG_NOTICE, "Type: LOCAL_SLAVE_ICAT   " );
            rodsLog( LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
                     tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#else /* SYSLOG */
            fprintf( stderr, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
            fprintf( stderr, "Type: LOCAL_SLAVE_ICAT   " );
            fprintf( stderr, " HostAddr: %s   PortNum: %d\nn",
                     tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#endif /* SYSLOG */
#else
            rodsLog( LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
            rodsLog( LOG_NOTICE, "Type: LOCAL_SLAVE_ICAT   " );
            rodsLog( LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
                     tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#endif
        }

        tmpZoneInfo = tmpZoneInfo->next;
    }
    /* print the reHost */
    if ( getReHost( &tmpRodsServerHost ) >= 0 ) {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "reHost:   %s", tmpRodsServerHost->hostName->name );
#else /* SYSLOG */
        fprintf( stderr, "reHost:   %s\n\n", tmpRodsServerHost->hostName->name );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, "reHost:   %s", tmpRodsServerHost->hostName->name );
#endif
    }
    else {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_ERROR, "reHost error" );
#else /* SYSLOG */
        fprintf( stderr, "reHost error" );
#endif /* SYSLOG */
#else
        rodsLog( LOG_ERROR, "reHost error" );
#endif
    }

    if ( getXmsgHost( &tmpRodsServerHost ) >= 0 ) {
#ifndef windows_platform
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "xmsgHost:  %s", tmpRodsServerHost->hostName->name );
#else /* SYSLOG */
        fprintf( stderr, "xmsgHost:  %s\n\n", tmpRodsServerHost->hostName->name );
#endif /* SYSLOG */
#else
        rodsLog( LOG_NOTICE, "xmsgHost:  %s", tmpRodsServerHost->hostName->name );
#endif
    }

    return 0;
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
            remote_SID_key_map[fed_zone_name] = std::make_pair(fed_zone_id, fed_zone_key);
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
            	size_t pos = rem_sids[i].find("-");
            	if (pos == std::string::npos) {
            		rodsLog(LOG_ERROR, "initRcatServerHostByFile - Unable to parse remote SID %s", rem_sids[i].c_str());
            	} else {
            		// store in remote_SID_key_map
            		std::string fed_zone_name = rem_sids[i].substr(0, pos);
            		std::string fed_zone_id = rem_sids[i].substr(pos+1);
            		remote_SID_key_map[fed_zone_name] = std::make_pair(fed_zone_id, "");
            	}
            }
        }
    } // else

    return 0;
}

int
queAddr( rodsServerHost_t *rodsServerHost, char *myHostName ) {
    struct hostent *hostEnt;
    time_t beforeTime, afterTime;
    int status;

    if ( rodsServerHost == NULL || myHostName == NULL ) {
        return 0;
    }

    /* gethostbyname could hang for some address */
    // =-=-=-=-=-=-=-
    // JMC :: consider empty host for coordinating nodes
    if ( irods::EMPTY_RESC_HOST != myHostName ) {
        beforeTime = time( 0 );
        if ( ( hostEnt = gethostbyname( myHostName ) ) == NULL ) {
            status = SYS_GET_HOSTNAME_ERR - errno;
            if ( ProcessType == SERVER_PT ) {
                rodsLog( LOG_NOTICE,
                         "queAddr: gethostbyname error for %s ,errno = %d\n",
                         myHostName, errno );
            }
            return status;
        }
        afterTime = time( 0 );
        if ( afterTime - beforeTime >= 2 ) {
            rodsLog( LOG_NOTICE,
                     "WARNING WARNING: gethostbyname of %s is taking %d sec. This could severely affect interactivity of your Rods system",
                     myHostName, afterTime - beforeTime );
            /* XXXXXX may want to mark resource down later */
        }
        if ( strcasecmp( myHostName, hostEnt->h_name ) != 0 ) {
            queHostName( rodsServerHost, hostEnt->h_name, 0 );
        }
    }

    return 0;
}

int
queHostName( rodsServerHost_t *rodsServerHost, const char *myName, int topFlag ) {
    hostName_t *myHostName, *lastHostName;
    hostName_t *tmpHostName;

    myHostName = lastHostName = rodsServerHost->hostName;

    while ( myHostName != NULL ) {
        if ( strcmp( myName, myHostName->name ) == 0 ) {
            return 0;
        }
        lastHostName = myHostName;
        myHostName = myHostName->next;
    }

    tmpHostName = ( hostName_t* )malloc( sizeof( hostName_t ) );
    tmpHostName->name = strdup( myName );

    if ( topFlag > 0 ) {
        tmpHostName->next = rodsServerHost->hostName;
        rodsServerHost->hostName = tmpHostName;
    }
    else {
        if ( lastHostName == NULL ) {
            rodsServerHost->hostName = tmpHostName;
        }
        else {
            lastHostName->next = tmpHostName;
        }
        tmpHostName->next = NULL;
    }

    return 0;
}

int
queRodsServerHost( rodsServerHost_t **rodsServerHostHead,
                   rodsServerHost_t *myRodsServerHost ) {
    rodsServerHost_t *lastRodsServerHost, *tmpRodsServerHost;

    lastRodsServerHost = tmpRodsServerHost = *rodsServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        lastRodsServerHost = tmpRodsServerHost;
        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    if ( lastRodsServerHost == NULL ) {
        *rodsServerHostHead = myRodsServerHost;
    }
    else {
        lastRodsServerHost->next = myRodsServerHost;
    }
    myRodsServerHost->next = NULL;

    return 0;
}

char *
getConfigDir() {
#ifndef windows_platform
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsConfigDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_CONFIG_DIR;
#else
    return iRODSNtGetServerConfigPath();
#endif
}

char *
getLogDir() {
#ifndef windows_platform
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsLogDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_LOG_DIR;
#else
    return iRODSNtServerGetLogDir;
#endif
}

/* getAndConnRcatHost - get the rcat enabled host (result given in
 * rodsServerHost) based on the rcatZoneHint.
 * rcatZoneHint is the hint for which zone to go it. It can be :
 *      a full path - e.g., /A/B/C. In this case, "A" will be taken as the zone
 *      a zone name - a string wth the first character that is no '/' is taken
 *         as a zone name.
 *      NULL string - default to local zone
 * If the rcat host is remote, it will automatically connect to the rcat host.
 */

int getAndConnRcatHost(
    rsComm_t *rsComm,
    int rcatType,
    const char *rcatZoneHint,
    rodsServerHost_t **rodsServerHost ) {

    int status = getRcatHost( rcatType, rcatZoneHint, rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "getAndConnRcatHost:getRcatHost() failed. erro=%d", status );
        return status;
    }

    if ( ( *rodsServerHost )->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }
    status = svrToSvrConnect( rsComm, *rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "getAndConnRcatHost: svrToSvrConnect to %s failed",
                 ( *rodsServerHost )->hostName->name );
        if ( ( *rodsServerHost )->rcatEnabled == REMOTE_ICAT ) {
            status = convZoneSockError( status );
        }
    }
    if ( status >= 0 ) {
        return REMOTE_HOST;
    }
    else {
        return status;
    }
}

int
getAndConnRcatHostNoLogin( rsComm_t *rsComm, int rcatType, char *rcatZoneHint,
                           rodsServerHost_t **rodsServerHost ) {
    int status = getRcatHost( rcatType, rcatZoneHint, rodsServerHost );

    if ( status < 0 ) {
        return status;
    }

    if ( ( *rodsServerHost )->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    status = svrToSvrConnectNoLogin( rsComm, *rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "getAndConnRcatHost: svrToSvrConnectNoLogin to %s failed",
                 ( *rodsServerHost )->hostName->name );
        if ( ( *rodsServerHost )->rcatEnabled == REMOTE_ICAT ) {
            status = convZoneSockError( status );
        }
    }
    return status;
}

int
convZoneSockError( int inStatus ) {
    int unixErr = getErrno( inStatus );
    if ( inStatus + unixErr == USER_SOCK_CONNECT_ERR ) {
        return CROSS_ZONE_SOCK_CONNECT_ERR - unixErr;
    }
    else {
        return inStatus;
    }
}

/* getRcatHost - get the rodsServerHost of the rcat enable host based
 * on the rcatZoneHint for identifying the zone.
 *   rcatZoneHint == NULL ==> local rcat zone
 *   rcatZoneHint in the form of a full path,e.g.,/x/y/z ==> x is the zoneName.
 *   rcatZoneHint not in the form of a full path ==> rcatZoneHint is the zone.
 */

int
getZoneInfo( const char *rcatZoneHint, zoneInfo_t **myZoneInfo ) {
    int status;
    zoneInfo_t *tmpZoneInfo;
    int zoneInput;
    char zoneName[NAME_LEN];

    if ( rcatZoneHint == NULL || strlen( rcatZoneHint ) == 0 ) {
        zoneInput = 0;
    }
    else {
        zoneInput = 1;
        getZoneNameFromHint( rcatZoneHint, zoneName, NAME_LEN );
    }

    *myZoneInfo = NULL;
    tmpZoneInfo = ZoneInfoHead;
    while ( tmpZoneInfo != NULL ) {
        if ( zoneInput == 0 ) { /* assume local */
            if ( tmpZoneInfo->masterServerHost->rcatEnabled == LOCAL_ICAT ) {
                *myZoneInfo = tmpZoneInfo;
            }
        }
        else {          /* remote zone */
            if ( strcmp( zoneName, tmpZoneInfo->zoneName ) == 0 ) {
                *myZoneInfo = tmpZoneInfo;
            }
        }
        if ( *myZoneInfo != NULL ) {
            return 0;
        }
        tmpZoneInfo = tmpZoneInfo->next;
    }

    if ( zoneInput == 0 ) {
        rodsLog( LOG_ERROR,
                 "getRcatHost: No local Rcat" );
        return SYS_INVALID_ZONE_NAME;
    }
    else {
        rodsLog( LOG_DEBUG,
                 "getZoneInfo: Invalid zone name from hint %s", rcatZoneHint );
        status = getZoneInfo( ( const char* )NULL, myZoneInfo );
        if ( status < 0 ) {
            return SYS_INVALID_ZONE_NAME;
        }
        else {
            return 0;
        }
    }
}

int
getRcatHost( int rcatType, const char *rcatZoneHint,
             rodsServerHost_t **rodsServerHost ) {
    int status;
    zoneInfo_t *myZoneInfo = NULL;

    status = getZoneInfo( rcatZoneHint, &myZoneInfo );
    if ( status < 0 ) {
        return status;
    }

    if ( rcatType == MASTER_RCAT ||
            myZoneInfo->slaveServerHost == NULL ) {
        *rodsServerHost = myZoneInfo->masterServerHost;
        return myZoneInfo->masterServerHost->localFlag;
    }
    else {
        *rodsServerHost = myZoneInfo->slaveServerHost;
        return myZoneInfo->slaveServerHost->localFlag;
    }
}

int
getLocalZoneInfo( zoneInfo_t **outZoneInfo ) {
    zoneInfo_t *tmpZoneInfo;

    tmpZoneInfo = ZoneInfoHead;
    while ( tmpZoneInfo != NULL ) {
        if ( tmpZoneInfo->masterServerHost->rcatEnabled == LOCAL_ICAT ) {
            *outZoneInfo = tmpZoneInfo;
            return 0;
        }
        tmpZoneInfo = tmpZoneInfo->next;
    }
    rodsLog( LOG_ERROR,
             "getLocalZoneInfo: Local Zone does not exist" );

    *outZoneInfo = NULL;
    return SYS_INVALID_ZONE_NAME;
}

char*
getLocalZoneName() {
    zoneInfo_t *tmpZoneInfo;

    if ( getLocalZoneInfo( &tmpZoneInfo ) >= 0 ) {
        return tmpZoneInfo->zoneName;
    }
    else {
        return NULL;
    }
}

/* Check if there is a connected ICAT host, and if there is, disconnect */
int
getAndDisconnRcatHost( int rcatType, char *rcatZoneHint,
                       rodsServerHost_t **rodsServerHost ) {
    int status;

    status = getRcatHost( rcatType, rcatZoneHint, rodsServerHost );

    if ( status < 0 ) {
        return status;
    }

    if ( ( *rodsServerHost )->conn != NULL ) { /* a connection exists */
        status = rcDisconnect( ( *rodsServerHost )->conn );
        return status;
    }
    return 0;
}

int
disconnRcatHost( int rcatType, const char *rcatZoneHint ) {
    int status;
    rodsServerHost_t *rodsServerHost = NULL;

    status = getRcatHost( rcatType, rcatZoneHint, &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( ( rodsServerHost )->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    if ( rodsServerHost->conn != NULL ) { /* a connection exists */
        status = rcDisconnect( rodsServerHost->conn );
        rodsServerHost->conn = NULL;
    }
    if ( status >= 0 ) {
        return REMOTE_HOST;
    }
    else {
        return status;
    }
}

/* resetRcatHost is similar to disconnRcatHost except it does not disconnect */

int
resetRcatHost( int rcatType, const char *rcatZoneHint ) {
    rodsServerHost_t *rodsServerHost = NULL;
    int status = getRcatHost( rcatType, rcatZoneHint, &rodsServerHost );

    if ( status < 0 || NULL == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    rodsServerHost->conn = NULL;
    return REMOTE_HOST;
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

int queZone(
    const char*       zoneName,
    int               portNum,
    rodsServerHost_t* masterServerHost,
    rodsServerHost_t* slaveServerHost ) {

    zoneInfo_t *tmpZoneInfo, *lastZoneInfo;
    zoneInfo_t *myZoneInfo;

    myZoneInfo = ( zoneInfo_t * ) malloc( sizeof( zoneInfo_t ) );

    memset( myZoneInfo, 0, sizeof( zoneInfo_t ) );

    rstrcpy( myZoneInfo->zoneName, zoneName, NAME_LEN );
    if ( masterServerHost != NULL ) {
        myZoneInfo->masterServerHost = masterServerHost;
        masterServerHost->zoneInfo = myZoneInfo;
    }
    if ( slaveServerHost != NULL ) {
        myZoneInfo->slaveServerHost = slaveServerHost;
        slaveServerHost->zoneInfo = myZoneInfo;
    }

    if ( portNum <= 0 ) {
        if ( ZoneInfoHead != NULL ) {
            myZoneInfo->portNum = ZoneInfoHead->portNum;
        }
        else {
            rodsLog( LOG_ERROR,
                     "queZone:  Bad input portNum %d for %s", portNum, zoneName );
            free( myZoneInfo );
            return SYS_INVALID_SERVER_HOST;
        }
    }
    else {
        myZoneInfo->portNum = portNum;
    }

    /* queue it */

    lastZoneInfo = tmpZoneInfo = ZoneInfoHead;
    while ( tmpZoneInfo != NULL ) {
        lastZoneInfo = tmpZoneInfo;
        tmpZoneInfo = tmpZoneInfo->next;
    }

    if ( lastZoneInfo == NULL ) {
        ZoneInfoHead = myZoneInfo;
    }
    else {
        lastZoneInfo->next = myZoneInfo;
    }
    myZoneInfo->next = NULL;

    if ( masterServerHost == NULL ) {
        rodsLog( LOG_DEBUG,
                 "queZone:  masterServerHost for %s is NULL", zoneName );
        return SYS_INVALID_SERVER_HOST;
    }
    else {
        return 0;
    }
}

std::vector<std::string>
setExecArg( const char *commandArgv ) {

    std::vector<std::string> arguments;
    if ( commandArgv != NULL ) {
        int len = 0;
        bool openQuote = false;
        const char* cur = commandArgv;
        for ( int i = 0; commandArgv[i] != '\0'; i++ ) {
            if ( commandArgv[i] == ' ' && !openQuote ) {
                if ( len > 0 ) {    /* end of a argv */
                    std::string( cur, len );
                    arguments.push_back( std::string( cur, len ) );
                    /* reset inx and pointer */
                    cur = &commandArgv[i + 1];
                    len = 0;
                }
                else {      /* skip over blanks */
                    cur = &commandArgv[i + 1];
                }
            }
            else if ( commandArgv[i] == '\'' || commandArgv[i] == '\"' ) {
                openQuote ^= true;
                if ( openQuote ) {
                    /* skip the quote */
                    cur = &commandArgv[i + 1];
                }
            }
            else {
                len ++;
            }
        }
        if ( len > 0 ) {    /* handle the last argv */
            arguments.push_back( std::string( cur, len ) );
        }
    }

    return arguments;
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
rsPipSigalHandler( int ) {
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
        signal( SIGPIPE, ( void ( * )( int ) ) rsPipSigalHandler );
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
matchHostConfig( rodsServerHost_t *myRodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost;
    int status;

    if ( myRodsServerHost == NULL ) {
        return 0;
    }

    if ( myRodsServerHost->localFlag == LOCAL_HOST ) {
        tmpRodsServerHost = HostConfigHead;
        while ( tmpRodsServerHost != NULL ) {
            if ( tmpRodsServerHost->localFlag == LOCAL_HOST ) {
                status = queConfigName( tmpRodsServerHost, myRodsServerHost );
                return status;
            }
            tmpRodsServerHost = tmpRodsServerHost->next;
        }
    }
    else {
        tmpRodsServerHost = HostConfigHead;
        while ( tmpRodsServerHost != NULL ) {
            hostName_t *tmpHostName, *tmpConfigName;

            if ( tmpRodsServerHost->localFlag == LOCAL_HOST &&
                    myRodsServerHost->localFlag != UNKNOWN_HOST_LOC ) {
                tmpRodsServerHost = tmpRodsServerHost->next;
                continue;
            }

            tmpConfigName = tmpRodsServerHost->hostName;
            while ( tmpConfigName != NULL ) {
                tmpHostName = myRodsServerHost->hostName;
                while ( tmpHostName != NULL ) {
                    if ( strcmp( tmpHostName->name, tmpConfigName->name ) == 0 ) {
                        myRodsServerHost->localFlag =
                            tmpRodsServerHost->localFlag;
                        status = queConfigName( tmpRodsServerHost,
                                                myRodsServerHost );
                        return 0;
                    }
                    tmpHostName = tmpHostName->next;
                }
                tmpConfigName = tmpConfigName->next;
            }
            tmpRodsServerHost = tmpRodsServerHost->next;
        }
    }

    return 0;
}

int
queConfigName( rodsServerHost_t *configServerHost,
               rodsServerHost_t *myRodsServerHost ) {
    hostName_t *tmpHostName = configServerHost->hostName;
    int cnt = 0;

    while ( tmpHostName != NULL ) {
        if ( cnt == 0 ) {
            /* queue the first one on top */
            queHostName( myRodsServerHost, tmpHostName->name, 1 );
        }
        else {
            queHostName( myRodsServerHost, tmpHostName->name, 0 );
        }
        cnt ++;
        tmpHostName = tmpHostName->next;
    }

    return 0;
}

int
disconnectAllSvrToSvrConn() {
    rodsServerHost_t *tmpRodsServerHost;

    /* check if host exist */

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->conn != NULL ) {
            rcDisconnect( tmpRodsServerHost->conn );
            tmpRodsServerHost->conn = NULL;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    return 0;
}

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
        return -1 * errno;
    }


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

/* getAndConnRemoteZone - get the remote zone host (result given in
 * rodsServerHost) based on the dataObjInp->objPath as zoneHint.
 * If the host is a remote zone, automatically connect to the host.
 */

int
getAndConnRemoteZone( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                      rodsServerHost_t **rodsServerHost, char *remoteZoneOpr ) {
    int status;

    status = getRemoteZoneHost( rsComm, dataObjInp, rodsServerHost,
                                remoteZoneOpr );

    if ( status == LOCAL_HOST ) {
        return LOCAL_HOST;
    }
    else if ( status < 0 ) {
        return status;
    }

    status = svrToSvrConnect( rsComm, *rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getAndConnRemoteZone: svrToSvrConnect to %s failed",
                 ( *rodsServerHost )->hostName->name );
    }
    if ( status >= 0 ) {
        return REMOTE_HOST;
    }
    else {
        return status;
    }
}

int
getAndConnRemoteZoneForCopy( rsComm_t *rsComm, dataObjCopyInp_t *dataObjCopyInp,
                             rodsServerHost_t **rodsServerHost ) {
    int status;
    dataObjInp_t *srcDataObjInp, *destDataObjInp;
    rodsServerHost_t *srcIcatServerHost = NULL;
    rodsServerHost_t *destIcatServerHost = NULL;

    srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    destDataObjInp = &dataObjCopyInp->destDataObjInp;

    status = getRcatHost( MASTER_RCAT, srcDataObjInp->objPath,
                          &srcIcatServerHost );

    if ( status < 0 || NULL == srcIcatServerHost ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR,
                 "getAndConnRemoteZoneForCopy: getRcatHost error for %s",
                 srcDataObjInp->objPath );
        return status;
    }

    if ( srcIcatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. nothing to do */
        return LOCAL_HOST;
    }

    status = getRcatHost( MASTER_RCAT, destDataObjInp->objPath,
                          &destIcatServerHost );

    if ( status < 0 || NULL == destIcatServerHost ) { // JMC cppcheck - nullptr
        rodsLog( LOG_ERROR,
                 "getAndConnRemoteZoneForCopy: getRcatHost error for %s",
                 destDataObjInp->objPath );
        return status;
    }

    if ( destIcatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. nothing to do */
        return LOCAL_HOST;
    }

    /* remote zone to different remote zone copy. Have to handle it
     * locally because of proxy admin user privilege issue */
    if ( srcIcatServerHost != destIcatServerHost ) {
        return LOCAL_HOST;
    }

    /* from the same remote zone. do it in the remote zone */

    status = getAndConnRemoteZone( rsComm, destDataObjInp, rodsServerHost,
                                   REMOTE_CREATE );

    return status;
}

int
isLocalZone( char *zoneHint ) {
    int status;
    rodsServerHost_t *icatServerHost = NULL;

    status = getRcatHost( MASTER_RCAT, zoneHint, &icatServerHost );

    if ( status < 0 || NULL == icatServerHost ) { // JMC cppcheck - nullptr
        return 0;
    }

    if ( icatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. */
        return 1;
    }
    else {
        return 0;
    }
}

/* isSameZone - return 1 if from same zone, otherwise return 0
 */
int
isSameZone( char *zoneHint1, char *zoneHint2 ) {
    char zoneName1[NAME_LEN], zoneName2[NAME_LEN];

    if ( zoneHint1 == NULL || zoneHint2 == NULL ) {
        return 0;
    }

    getZoneNameFromHint( zoneHint1, zoneName1, NAME_LEN );
    getZoneNameFromHint( zoneHint2, zoneName2, NAME_LEN );

    if ( strcmp( zoneName1, zoneName2 ) == 0 ) {
        return 1;
    }
    else {
        return 0;
    }
}

int
getRemoteZoneHost( rsComm_t *rsComm, dataObjInp_t *dataObjInp,
                   rodsServerHost_t **rodsServerHost, char *remoteZoneOpr ) {
    int status;
    rodsServerHost_t *icatServerHost = NULL;
    rodsHostAddr_t *rescAddr = NULL;

    status = getRcatHost( MASTER_RCAT, dataObjInp->objPath, &icatServerHost );

    if ( status < 0 || NULL == icatServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( icatServerHost->rcatEnabled != REMOTE_ICAT ) {
        /* local zone. nothing to do */
        return LOCAL_HOST;
    }

    status = svrToSvrConnect( rsComm, icatServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getRemoteZoneHost: svrToSvrConnect to %s failed, status = %d",
                 icatServerHost->hostName->name, status );
        status = convZoneSockError( status );
        return status;
    }

    addKeyVal( &dataObjInp->condInput, REMOTE_ZONE_OPR_KW, remoteZoneOpr );

    status = rcGetRemoteZoneResc( icatServerHost->conn, dataObjInp, &rescAddr );
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "getRemoteZoneHost: rcGetRemoteZoneResc for %s failed, status = %d",
                 dataObjInp->objPath, status );
        return status;
    }

    status = resolveHost( rescAddr, rodsServerHost );

    free( rescAddr );

    return status;
}

int
isLocalHost( const char *hostAddr ) {
    int remoteFlag;
    rodsServerHost_t *rodsServerHost;
    rodsHostAddr_t addr;

    bzero( &addr, sizeof( addr ) );
    rstrcpy( addr.hostAddr, hostAddr, NAME_LEN );
    remoteFlag = resolveHost( &addr, &rodsServerHost );
    if ( remoteFlag == LOCAL_HOST ) {
        return 1;
    }
    else {
        return 0;
    }
}

int
getReHost( rodsServerHost_t **rodsServerHost ) {
    int status;

    rodsServerHost_t *tmpRodsServerHost;

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->reHostFlag == 1 ) {
            *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    status = getRcatHost( MASTER_RCAT, ( const char* )NULL, rodsServerHost );

    return status;
}

int
getXmsgHost( rodsServerHost_t **rodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost;

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != NULL ) {
        if ( tmpRodsServerHost->xmsgHostFlag == 1 ) {
            *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    *rodsServerHost = NULL;

    return SYS_INVALID_SERVER_HOST;
}


/* getAndConnReHost - Get the irodsReServer host (result given in
 * rodsServerHost).
 * If the is remote, it will automatically connect to the server where
 * irodsReServer is run.
 */

int
getAndConnReHost( rsComm_t *rsComm, rodsServerHost_t **rodsServerHost ) {
    int status;

    status = getReHost( rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "getAndConnReHost:getReHost() failed. erro=%d", status );
        return status;
    }

    if ( ( *rodsServerHost )->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    status = svrToSvrConnect( rsComm, *rodsServerHost );

    if ( status < 0 ) {
        rodsLog( LOG_NOTICE,
                 "getAndConnReHost: svrToSvrConnect to %s failed",
                 ( *rodsServerHost )->hostName->name );
    }
    if ( status >= 0 ) {
        return REMOTE_HOST;
    }
    else {
        return status;
    }
}

int
initConnectControl() {
    char *conFile;
    char *configDir;
    FILE *file;
    char buf[LONG_NAME_LEN * 5];
    int len;
    char *bufPtr;
    int status;
    struct allowedUser *tmpAllowedUser;
    int allowUserFlag = 0;
    int disallowUserFlag = 0;

    configDir = getConfigDir();
    len = strlen( configDir ) + strlen( CONNECT_CONTROL_FILE ) + 2;
    ;

    conFile = ( char * ) malloc( len );

    snprintf( conFile, len, "%s/%s", configDir, CONNECT_CONTROL_FILE );
    file = fopen( conFile, "r" );

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
initAndClearProcLog() {
    initProcLog();
    int error_code = mkdir( ProcLogDir, DEFAULT_DIR_MODE );
    int errsv = errno;
    if ( error_code != 0 && errsv != EEXIST ) {
        rodsLog( LOG_ERROR, "mkdir failed in initAndClearProcLog with error code %d", error_code );
    }
    rmFilesInDir( ProcLogDir );

    return 0;
}

int
initProcLog() {
    snprintf( ProcLogDir, MAX_NAME_LEN, "%s/%s",
              getLogDir(), PROC_LOG_DIR_NAME );
    return 0;
}

int
logAgentProc( rsComm_t *rsComm ) {
    FILE *fptr;
    char procPath[MAX_NAME_LEN];
    char *remoteAddr;
    char *progName;
    char *clientZone, *proxyZone;

    if ( rsComm->procLogFlag == PROC_LOG_DONE ) {
        return 0;
    }

    if ( *rsComm->clientUser.userName == '\0' ||
            *rsComm->proxyUser.userName == '\0' ) {
        return 0;
    }

    if ( *rsComm->clientUser.rodsZone == '\0' ) {
        if ( ( clientZone = getLocalZoneName() ) == NULL ) {
            clientZone = "UNKNOWN";
        }
    }
    else {
        clientZone = rsComm->clientUser.rodsZone;
    }

    if ( *rsComm->proxyUser.rodsZone == '\0' ) {
        if ( ( proxyZone = getLocalZoneName() ) == NULL ) {
            proxyZone = "UNKNOWN";
        }
    }
    else {
        proxyZone = rsComm->proxyUser.rodsZone;
    }

    remoteAddr = inet_ntoa( rsComm->remoteAddr.sin_addr );
    if ( remoteAddr == NULL || *remoteAddr == '\0' ) {
        remoteAddr = "UNKNOWN";
    }
    if ( *rsComm->option == '\0' ) {
        progName = "UNKNOWN";
    }
    else {
        progName = rsComm->option;
    }

    snprintf( procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, getpid() );

    fptr = fopen( procPath, "w" );

    if ( fptr == NULL ) {
        rodsLog( LOG_ERROR,
                 "logAgentProc: Cannot open input file %s. errno = %d",
                 procPath, errno );
        return UNIX_FILE_OPEN_ERR - errno;
    }

    fprintf( fptr, "%s %s %s %s %s %s %u\n",
             rsComm->proxyUser.userName, clientZone,
             rsComm->clientUser.userName, proxyZone,
             progName, remoteAddr, ( unsigned int ) time( 0 ) );

    rsComm->procLogFlag = PROC_LOG_DONE;
    fclose( fptr );
    return 0;
}

int
rmProcLog( int pid ) {
    char procPath[MAX_NAME_LEN];

    snprintf( procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, pid );
    unlink( procPath );
    return 0;
}

int
readProcLog( int pid, procLog_t *procLog ) {

    if ( procLog == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    char procPath[MAX_NAME_LEN];
    snprintf( procPath, MAX_NAME_LEN, "%s/%-d", ProcLogDir, pid );

    std::fstream procStream( procPath, std::ios::in );
    std::vector<std::string> procTokens;
    while ( !procStream.eof() && procTokens.size() < 7 ) {
        std::string token;
        procStream >> token;
        procTokens.push_back( token );
    }

    if ( procTokens.size() != 7 ) {
        rodsLog( LOG_ERROR,
                 "readProcLog: error fscanf file %s. Number of param read = %d",
                 procPath, procTokens.size() );
        return UNIX_FILE_READ_ERR;
    }

    procLog->pid = pid;

    snprintf( procLog->clientName, sizeof( procLog->clientName ), "%s", procTokens[0].c_str() );
    snprintf( procLog->clientZone, sizeof( procLog->clientZone ), "%s", procTokens[1].c_str() );
    snprintf( procLog->proxyName, sizeof( procLog->proxyName ), "%s", procTokens[2].c_str() );
    snprintf( procLog->proxyZone, sizeof( procLog->proxyZone ), "%s", procTokens[3].c_str() );
    snprintf( procLog->progName, sizeof( procLog->progName ), "%s", procTokens[4].c_str() );
    snprintf( procLog->remoteAddr, sizeof( procLog->remoteAddr ), "%s", procTokens[5].c_str() );
    try {
        procLog->startTime = boost::lexical_cast<unsigned int>( procTokens[6].c_str() );
    }
    catch ( ... ) {
        rodsLog( LOG_ERROR, "Could not convert %s to unsigned int.", procTokens[6].c_str() );
        return INVALID_LEXICAL_CAST;
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
    DIR *dirPtr;
    struct dirent *myDirent;
    struct stat statbuf;
    int status;
    int savedStatus = 0;
    struct flock myflock;
    uint purgeTime;

    snprintf( lockFileDir, MAX_NAME_LEN, "%-s/%-s", getConfigDir(),
              LOCK_FILE_DIR );

    dirPtr = opendir( lockFileDir );
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
