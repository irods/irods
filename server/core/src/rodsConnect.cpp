/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.cpp - Server initialization routines
 */

#include "rcMisc.h"
#include "rodsConnect.h"
#include "rsGlobalExtern.hpp"
#include "rcGlobalExtern.h"
#include "miscServerFunct.hpp"
#include "getRemoteZoneResc.h"
#include "irods_resource_backport.hpp"
#include "rsLog.hpp"

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
getRcatHost( int rcatType, const char *rcatZoneHint,
             rodsServerHost_t **rodsServerHost ) {
    int status;
    zoneInfo_t *myZoneInfo = nullptr;

    status = getZoneInfo( rcatZoneHint, &myZoneInfo );
    if ( status < 0 ) {
        return status;
    }

    if ( rcatType == MASTER_RCAT ||
            myZoneInfo->slaveServerHost == nullptr ) {
        *rodsServerHost = myZoneInfo->masterServerHost;
        return myZoneInfo->masterServerHost->localFlag;
    }
    else {
        *rodsServerHost = myZoneInfo->slaveServerHost;
        return myZoneInfo->slaveServerHost->localFlag;
    }
}

rodsServerHost_t *
mkServerHost( char *myHostAddr, char *zoneName ) {
    rodsServerHost_t* tmpRodsServerHost = ( rodsServerHost_t* )malloc( sizeof( rodsServerHost_t ) );
    memset( tmpRodsServerHost, 0, sizeof( rodsServerHost_t ) );

    /* XXXXX need to lookup the zone table when availiable */
    if ( queHostName( tmpRodsServerHost, myHostAddr, 0 ) < 0 ) {
        free( tmpRodsServerHost );
        return nullptr;
    }

    tmpRodsServerHost->localFlag = UNKNOWN_HOST_LOC;

    if( queAddr( tmpRodsServerHost, myHostAddr ) < 0 ||
        matchHostConfig( tmpRodsServerHost ) < 0 ||
        getZoneInfo( zoneName, ( zoneInfo_t ** )( static_cast< void * >( &tmpRodsServerHost->zoneInfo ) ) ) < 0 ) {
        free( tmpRodsServerHost );
        return nullptr;
    }
    else {
        return tmpRodsServerHost;
    }
}

int
queAddr( rodsServerHost_t *rodsServerHost, char *myHostName ) {
    if ( rodsServerHost == nullptr || myHostName == nullptr ) {
        return 0;
    }

    // JMC :: consider empty host for coordinating nodes
    if ( irods::EMPTY_RESC_HOST != myHostName ) {
        time_t beforeTime = time( nullptr );
        char canonicalName[260]; // DNS specification limits hostnames to 255-ish bytes
        const int ret_get_canonical_name = get_canonical_name(myHostName, canonicalName, sizeof(canonicalName));
        if (ret_get_canonical_name != 0) {
            if ( ProcessType == SERVER_PT ) {
                rodsLog( LOG_NOTICE,
                         "queAddr: get_canonical_name error for [%s], status [%d]",
                         myHostName, ret_get_canonical_name);
            }
            return SYS_GET_HOSTNAME_ERR;
        }
        time_t afterTime = time( nullptr );
        if ( afterTime - beforeTime >= 2 ) {
            rodsLog( LOG_NOTICE,
                     "WARNING WARNING: get_canonical_name of %s is taking %d sec. This could severely affect interactivity of your Rods system",
                     myHostName, afterTime - beforeTime );
            /* XXXXXX may want to mark resource down later */
        }
        if ( strcasecmp( myHostName, canonicalName ) != 0 ) {
            queHostName( rodsServerHost, canonicalName, 0 );
        }
    }

    return 0;
}

int
queHostName( rodsServerHost_t *rodsServerHost, const char *myName, int topFlag ) {
    hostName_t *myHostName, *lastHostName;
    hostName_t *tmpHostName;

    myHostName = lastHostName = rodsServerHost->hostName;

    while ( myHostName != nullptr ) {
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
        if ( lastHostName == nullptr ) {
            rodsServerHost->hostName = tmpHostName;
        }
        else {
            lastHostName->next = tmpHostName;
        }
        tmpHostName->next = nullptr;
    }

    return 0;
}

int
queRodsServerHost( rodsServerHost_t **rodsServerHostHead,
                   rodsServerHost_t *myRodsServerHost ) {
    rodsServerHost_t *lastRodsServerHost, *tmpRodsServerHost;

    lastRodsServerHost = tmpRodsServerHost = *rodsServerHostHead;
    while ( tmpRodsServerHost != nullptr ) {
        lastRodsServerHost = tmpRodsServerHost;
        tmpRodsServerHost = tmpRodsServerHost->next;
    }

    if ( lastRodsServerHost == nullptr ) {
        *rodsServerHostHead = myRodsServerHost;
    }
    else {
        lastRodsServerHost->next = myRodsServerHost;
    }
    myRodsServerHost->next = nullptr;

    return 0;
}

int queZone(
    const char*       zoneName,
    int               portNum,
    rodsServerHost_t* masterServerHost,
    rodsServerHost_t* slaveServerHost ) {

    bool zoneAlreadyInList = false;

    zoneInfo_t *tmpZoneInfo, *lastZoneInfo;
    zoneInfo_t *myZoneInfo;

    myZoneInfo = ( zoneInfo_t * ) malloc( sizeof( zoneInfo_t ) );

    memset( myZoneInfo, 0, sizeof( zoneInfo_t ) );

    rstrcpy( myZoneInfo->zoneName, zoneName, NAME_LEN );
    if ( masterServerHost != nullptr ) {
        myZoneInfo->masterServerHost = masterServerHost;
        masterServerHost->zoneInfo = myZoneInfo;
    }
    if ( slaveServerHost != nullptr ) {
        myZoneInfo->slaveServerHost = slaveServerHost;
        slaveServerHost->zoneInfo = myZoneInfo;
    }

    if ( portNum <= 0 ) {
        if ( ZoneInfoHead != nullptr ) {
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
    while ( tmpZoneInfo != nullptr ) {
        if (strcmp(tmpZoneInfo->zoneName, myZoneInfo->zoneName) == 0 ) {
            zoneAlreadyInList = true;
        }
        lastZoneInfo = tmpZoneInfo;
        tmpZoneInfo = tmpZoneInfo->next;
    }

    if ( lastZoneInfo == nullptr ) {
        ZoneInfoHead = myZoneInfo;
    } else if (!zoneAlreadyInList) {
        lastZoneInfo->next = myZoneInfo;
    }
    myZoneInfo->next = nullptr;

    if ( masterServerHost == nullptr ) {
        rodsLog( LOG_DEBUG,
                 "queZone:  masterServerHost for %s is NULL", zoneName );
        return SYS_INVALID_SERVER_HOST;
    }
    else {
        return 0;
    }
}

int
matchHostConfig( rodsServerHost_t *myRodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost;
    int status;

    if ( myRodsServerHost == nullptr ) {
        return 0;
    }

    if ( myRodsServerHost->localFlag == LOCAL_HOST ) {
        tmpRodsServerHost = HostConfigHead;
        while ( tmpRodsServerHost != nullptr ) {
            if ( tmpRodsServerHost->localFlag == LOCAL_HOST ) {
                status = queConfigName( tmpRodsServerHost, myRodsServerHost );
                return status;
            }
            tmpRodsServerHost = tmpRodsServerHost->next;
        }
    }
    else {
        tmpRodsServerHost = HostConfigHead;
        while ( tmpRodsServerHost != nullptr ) {
            hostName_t *tmpHostName, *tmpConfigName;

            if ( tmpRodsServerHost->localFlag == LOCAL_HOST &&
                    myRodsServerHost->localFlag != UNKNOWN_HOST_LOC ) {
                tmpRodsServerHost = tmpRodsServerHost->next;
                continue;
            }

            tmpConfigName = tmpRodsServerHost->hostName;
            while ( tmpConfigName != nullptr ) {
                tmpHostName = myRodsServerHost->hostName;
                while ( tmpHostName != nullptr ) {
                    if ( strcmp( tmpHostName->name, tmpConfigName->name ) == 0 ) {
                        myRodsServerHost->localFlag = tmpRodsServerHost->localFlag;
                        queConfigName( tmpRodsServerHost, myRodsServerHost );
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

    while ( tmpHostName != nullptr ) {
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
resolveHost( rodsHostAddr_t *addr, rodsServerHost_t **rodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost = nullptr;
    char *myHostAddr = nullptr;
    char *myZoneName = nullptr;

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
    while ( tmpRodsServerHost != nullptr ) {
        hostName_t *tmpName;
        zoneInfo_t *serverZoneInfo = ( zoneInfo_t * ) tmpRodsServerHost->zoneInfo;
        if ( strcmp( myZoneName, serverZoneInfo->zoneName ) == 0 ) {
            tmpName = tmpRodsServerHost->hostName;
            while ( tmpName != nullptr ) {
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

    if ( tmpRodsServerHost == nullptr ) {
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
    if ( dataObjInfo == nullptr ) {
        rodsLog( LOG_NOTICE,
                 "resoAndConnHostByDataObjInfo: NULL dataObjInfo" );
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

int
printServerHost( rodsServerHost_t *myServerHost ) {
    hostName_t *tmpHostName;

    if ( myServerHost->localFlag == LOCAL_HOST ) {
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    LocalHostName: " );
#else /* SYSLOG */
        fprintf( stderr, "    LocalHostName: " );
#endif /* SYSLOG */
    }
    else {
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    RemoteHostName: " );
#else /* SYSLOG */
        fprintf( stderr, "    RemoteHostName: " );
#endif /* SYSLOG */
    }

    tmpHostName = myServerHost->hostName;

    while ( tmpHostName != nullptr ) {
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, " %s,", tmpHostName->name );
#else /* SYSLOG */
        fprintf( stderr, " %s,", tmpHostName->name );
#endif /* SYSLOG */
        tmpHostName = tmpHostName->next;
    }

#ifdef SYSLOG
    rodsLog( LOG_NOTICE, " Port Num: %d.\n\n", ( ( zoneInfo_t * )myServerHost->zoneInfo )->portNum );
#else /* SYSLOG */
    fprintf( stderr, " Port Num: %d.\n\n",
             ( ( zoneInfo_t * )myServerHost->zoneInfo )->portNum );
#endif /* SYSLOG */

    return 0;
}

int
printZoneInfo() {
    zoneInfo_t *tmpZoneInfo;
    rodsServerHost_t *tmpRodsServerHost;

    tmpZoneInfo = ZoneInfoHead;
#ifdef SYSLOG
    rodsLog( LOG_NOTICE, "Zone Info:\n" );
#else /* SYSLOG */
    fprintf( stderr, "Zone Info:\n" );
#endif /* SYSLOG */
    while ( tmpZoneInfo != nullptr ) {
        /* print the master */
        tmpRodsServerHost = ( rodsServerHost_t * ) tmpZoneInfo->masterServerHost;
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
#else /* SYSLOG */
        fprintf( stderr, "    ZoneName: %s   ", tmpZoneInfo->zoneName );
#endif /* SYSLOG */

        if ( tmpRodsServerHost->rcatEnabled == LOCAL_ICAT ) {
#ifdef SYSLOG
            rodsLog( LOG_NOTICE, "Type: LOCAL_ICAT   " );
#else /* SYSLOG */
            fprintf( stderr, "Type: LOCAL_ICAT   " );
#endif /* SYSLOG */
        }
        else {
#ifdef SYSLOG
            rodsLog( LOG_NOTICE, "Type: REMOTE_ICAT   " );
#else /* SYSLOG */
            fprintf( stderr, "Type: REMOTE_ICAT   " );
#endif /* SYSLOG */
        }

#ifdef SYSLOG
        rodsLog( LOG_NOTICE, " HostAddr: %s   PortNum: %d\n\n",
                 tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#else /* SYSLOG */
        fprintf( stderr, " HostAddr: %s   PortNum: %d\n\n",
                 tmpRodsServerHost->hostName->name, tmpZoneInfo->portNum );
#endif /* SYSLOG */

        /* print the slave */
        tmpRodsServerHost = ( rodsServerHost_t * ) tmpZoneInfo->slaveServerHost;
        if ( tmpRodsServerHost != nullptr ) {
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
        }

        tmpZoneInfo = tmpZoneInfo->next;
    }
    /* print the reHost */
    if ( getReHost( &tmpRodsServerHost ) >= 0 ) {
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "reHost:   %s", tmpRodsServerHost->hostName->name );
#else /* SYSLOG */
        fprintf( stderr, "reHost:   %s\n\n", tmpRodsServerHost->hostName->name );
#endif /* SYSLOG */
    }
    else {
#ifdef SYSLOG
        rodsLog( LOG_ERROR, "reHost error" );
#else /* SYSLOG */
        fprintf( stderr, "reHost error" );
#endif /* SYSLOG */
    }

    if ( getXmsgHost( &tmpRodsServerHost ) >= 0 ) {
#ifdef SYSLOG
        rodsLog( LOG_NOTICE, "xmsgHost", tmpRodsServerHost->hostName->name );
#else /* SYSLOG */
        fprintf( stderr, "xmsgHost:  %s\n\n", tmpRodsServerHost->hostName->name );
#endif /* SYSLOG */
    }

    return 0;
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

int
getZoneInfo( const char *rcatZoneHint, zoneInfo_t **myZoneInfo ) {
    int status;
    zoneInfo_t *tmpZoneInfo;
    int zoneInput;
    char zoneName[NAME_LEN];

    if ( rcatZoneHint == nullptr || strlen( rcatZoneHint ) == 0 ) {
        zoneInput = 0;
    }
    else {
        zoneInput = 1;
        getZoneNameFromHint( rcatZoneHint, zoneName, NAME_LEN );
    }

    *myZoneInfo = nullptr;
    tmpZoneInfo = ZoneInfoHead;
    while ( tmpZoneInfo != nullptr ) {
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
        if ( *myZoneInfo != nullptr ) {
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
        status = getZoneInfo( ( const char* )nullptr, myZoneInfo );
        if ( status < 0 ) {
            return SYS_INVALID_ZONE_NAME;
        }
        else {
            return 0;
        }
    }
}

int
getLocalZoneInfo( zoneInfo_t **outZoneInfo ) {
    zoneInfo_t *tmpZoneInfo;

    tmpZoneInfo = ZoneInfoHead;
    while ( tmpZoneInfo != nullptr ) {
        if ( tmpZoneInfo->masterServerHost->rcatEnabled == LOCAL_ICAT ) {
            *outZoneInfo = tmpZoneInfo;
            return 0;
        }
        tmpZoneInfo = tmpZoneInfo->next;
    }
    rodsLog( LOG_ERROR,
             "getLocalZoneInfo: Local Zone does not exist" );

    *outZoneInfo = nullptr;
    return SYS_INVALID_ZONE_NAME;
}

char*
getLocalZoneName() {
    zoneInfo_t *tmpZoneInfo;

    if ( getLocalZoneInfo( &tmpZoneInfo ) >= 0 ) {
        return tmpZoneInfo->zoneName;
    }
    else {
        return nullptr;
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

    if ( ( *rodsServerHost )->conn != nullptr ) { /* a connection exists */
        status = rcDisconnect( ( *rodsServerHost )->conn );
        return status;
    }
    return 0;
}

int
disconnRcatHost( int rcatType, const char *rcatZoneHint ) {
    int status;
    rodsServerHost_t *rodsServerHost = nullptr;

    status = getRcatHost( rcatType, rcatZoneHint, &rodsServerHost );

    if ( status < 0 || nullptr == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( ( rodsServerHost )->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    if ( rodsServerHost->conn != nullptr ) { /* a connection exists */
        status = rcDisconnect( rodsServerHost->conn );
        rodsServerHost->conn = nullptr;
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
    rodsServerHost_t *rodsServerHost = nullptr;
    int status = getRcatHost( rcatType, rcatZoneHint, &rodsServerHost );

    if ( status < 0 || nullptr == rodsServerHost ) { // JMC cppcheck - nullptr
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
        return LOCAL_HOST;
    }

    rodsServerHost->conn = nullptr;
    return REMOTE_HOST;
}

int
disconnectAllSvrToSvrConn() {
    rodsServerHost_t *tmpRodsServerHost;

    /* check if host exist */

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != nullptr ) {
        if ( tmpRodsServerHost->conn != nullptr ) {
            rcDisconnect( tmpRodsServerHost->conn );
            tmpRodsServerHost->conn = nullptr;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
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
    rodsServerHost_t *srcIcatServerHost = nullptr;
    rodsServerHost_t *destIcatServerHost = nullptr;

    srcDataObjInp = &dataObjCopyInp->srcDataObjInp;
    destDataObjInp = &dataObjCopyInp->destDataObjInp;

    status = getRcatHost( MASTER_RCAT, srcDataObjInp->objPath,
                          &srcIcatServerHost );

    if ( status < 0 || nullptr == srcIcatServerHost ) { // JMC cppcheck - nullptr
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

    if ( status < 0 || nullptr == destIcatServerHost ) { // JMC cppcheck - nullptr
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
    rodsServerHost_t *icatServerHost = nullptr;

    status = getRcatHost( MASTER_RCAT, zoneHint, &icatServerHost );

    if ( status < 0 || nullptr == icatServerHost ) { // JMC cppcheck - nullptr
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

    if ( zoneHint1 == nullptr || zoneHint2 == nullptr ) {
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
    rodsServerHost_t *icatServerHost = nullptr;
    rodsHostAddr_t *rescAddr = nullptr;

    status = getRcatHost( MASTER_RCAT, dataObjInp->objPath, &icatServerHost );

    if ( status < 0 || nullptr == icatServerHost ) { // JMC cppcheck - nullptr
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
    while ( tmpRodsServerHost != nullptr ) {
        if ( tmpRodsServerHost->reHostFlag == 1 ) {
            *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    status = getRcatHost( MASTER_RCAT, ( const char* )nullptr, rodsServerHost );

    return status;
}

int
getXmsgHost( rodsServerHost_t **rodsServerHost ) {
    rodsServerHost_t *tmpRodsServerHost;

    tmpRodsServerHost = ServerHostHead;
    while ( tmpRodsServerHost != nullptr ) {
        if ( tmpRodsServerHost->xmsgHostFlag == 1 ) {
            *rodsServerHost = tmpRodsServerHost;
            return 0;
        }
        tmpRodsServerHost = tmpRodsServerHost->next;
    }
    *rodsServerHost = nullptr;

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
