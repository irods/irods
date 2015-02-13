/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  ips.c - Command to get the stat of irods agents running in the federation..
*/

#include "rodsClient.hpp"
#include "parseCommandLine.hpp"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"

int
printProcStat( rodsArguments_t *myRodsArgs, genQueryOut_t *procStatOut );
int
getUptimeStr( uint startTime, uint curTime, char *outStr );
void usage();
int
initCondForProcStat( rodsArguments_t *rodsArgs,
                     procStatInp_t *procStatInp );

int
main( int argc, char **argv ) {

    signal( SIGPIPE, SIG_IGN );

    int status;
    rodsEnv myEnv;
    rcComm_t *conn;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    char *optStr;
    procStatInp_t procStatInp;
    genQueryOut_t *procStatOut = NULL;

    optStr = "ahH:R:vz:";

    status = parseCmdLineOpt( argc, argv,  optStr, 0, &myRodsArgs );
    if ( status < 0 ) {
        printf( "Use -h for help.\n" );
        return 1;
    }

    if ( myRodsArgs.help == True ) {
        usage();
        return 0;
    }

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    init_api_table( api_tbl, pk_tbl );

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
        return 2;
    }

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            return 7;
        }
    }

    initCondForProcStat( &myRodsArgs, &procStatInp );

    status = rcProcStat( conn, &procStatInp, &procStatOut );

    if ( procStatOut != NULL ) {
        printProcStat( &myRodsArgs, procStatOut );
        freeGenQueryOut( &procStatOut );
    }

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
        if ( SYS_INVALID_SERVER_HOST == status ) {
            rodsLogError( LOG_ERROR, status,
                          "rcProcStat for at least one of the servers failed. "
                          "Check to make sure you did not call this on a "
                          "coordinating resource. Coordinating resources are "
                          "virtual and have no host to query." );
            return 3;
        }
        else {
            rodsLogError( LOG_ERROR, status,
                          "rcProcStat for at least one of the servers failed." );
            return 3;
        }
    }


    return 0;
}

int
printProcStat( rodsArguments_t *myRodsArgs, genQueryOut_t *procStatOut ) {
    char *prevServerAddr = NULL;
    int i, rowCnt;
    sqlResult_t *pid, *startTime, *clientName, *clientZone, *proxyName,
                *proxyZone, *remoteAddr, *serverAddr, *progName;
    uint curTime;

    if ( myRodsArgs == NULL || procStatOut == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    curTime = time( 0 );

    if ( ( pid = getSqlResultByInx( procStatOut, PID_INX ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for PID_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( startTime = getSqlResultByInx( procStatOut, STARTTIME_INX ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for STARTTIME_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( clientName = getSqlResultByInx( procStatOut, CLIENT_NAME_INX ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for CLIENT_NAME_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( clientZone = getSqlResultByInx( procStatOut, CLIENT_ZONE_INX ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for CLIENT_ZONE_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( proxyName = getSqlResultByInx( procStatOut, PROXY_NAME_INX ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for PROXY_NAME_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( proxyZone = getSqlResultByInx( procStatOut, PROXY_ZONE_INX ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for PROXY_ZONE_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( remoteAddr = getSqlResultByInx( procStatOut, REMOTE_ADDR_INX ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for REMOTE_ADDR_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( serverAddr = getSqlResultByInx( procStatOut, SERVER_ADDR_INX ) ) ==
            NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for SERVER_ADDR_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    if ( ( progName = getSqlResultByInx( procStatOut, PROG_NAME_INX ) ) == NULL ) {
        rodsLog( LOG_ERROR,
                 "printProcStat: getSqlResultByInx for PROG_NAME_INX failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }
    rowCnt = procStatOut->rowCnt;

    for ( i = 0; i < rowCnt; i++ ) {
        char *pidVal;
        uint startTimeVal;
        char *clientNameVal, *clientZoneVal, *proxyNameVal, *proxyZoneVal,
             *remoteAddrVal, *serverAddrVal, *progNameVal;
        char uptimeStr[NAME_LEN];

        pidVal = pid->value + pid->len * i;
        startTimeVal =  atoi( startTime->value + startTime->len * i );
        clientNameVal = clientName->value + clientName->len * i;
        clientZoneVal = clientZone->value + clientZone->len * i;
        proxyNameVal = proxyName->value + proxyName->len * i;
        proxyZoneVal = proxyZone->value + proxyZone->len * i;
        remoteAddrVal = remoteAddr->value + remoteAddr->len * i;
        serverAddrVal = serverAddr->value + serverAddr->len * i;
        progNameVal = progName->value + progName->len * i;

        if ( prevServerAddr == NULL ||
                strcmp( prevServerAddr, serverAddrVal ) != 0 ) {
            prevServerAddr = serverAddrVal;
            /* print serverAddrVal for now */
            printf( "Server: %s\n", serverAddrVal );
        }
        if ( *clientNameVal == '\0' ) {
            continue;	/* no connection for this server */
        }
        getUptimeStr( startTimeVal, curTime, uptimeStr );
        if ( myRodsArgs->verbose == True ) {
            printf( "   %6s %s#%s  %s#%s  %s  %s  %s\n",
                    pidVal, clientNameVal, clientZoneVal, proxyNameVal, proxyZoneVal,
                    uptimeStr, progNameVal, remoteAddrVal );
        }
        else {
            printf( "   %6s %s#%s  %s  %s  %s\n",
                    pidVal, clientNameVal, clientZoneVal,
                    uptimeStr, progNameVal, remoteAddrVal );
        }

    }
    return 0;
}

int
getUptimeStr( uint startTime, uint curTime, char *outStr ) {
    uint upTimeSec, hr, min, sec;

    if ( outStr == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    if ( startTime >= curTime ) {
        sec = min = hr = 0;
    }
    else {
        upTimeSec = curTime - startTime;
        sec = upTimeSec % 60;
        min = upTimeSec / 60;
        hr = min / 60;
        min = min % 60;
    }

    snprintf( outStr, NAME_LEN, "%d:%02d:%02d", hr, min, sec );

    return 0;
}

int
initCondForProcStat( rodsArguments_t *rodsArgs, procStatInp_t *procStatInp ) {
    if ( procStatInp == NULL ) {
        rodsLog( LOG_ERROR,
                 "initCondForProcStat: NULL procStatInp input" );
        return USER__NULL_INPUT_ERR;
    }

    bzero( procStatInp, sizeof( procStatInp_t ) );

    if ( rodsArgs == NULL ) {
        return 0;
    }

    if ( rodsArgs->all == True ) {
        addKeyVal( &procStatInp->condInput, ALL_KW, "" );
    }

    if ( rodsArgs->resource == True ) {
        if ( rodsArgs->resourceString == NULL ) {
            rodsLog( LOG_ERROR,
                     "initCondForProcStat: NULL resourceString error" );
            return USER__NULL_INPUT_ERR;
        }
        else {
            addKeyVal( &procStatInp->condInput, RESC_NAME_KW,
                       rodsArgs->resourceString );
        }
    }

    if ( rodsArgs->zone == True ) {
        rstrcpy( procStatInp->rodsZone, rodsArgs->zoneName, NAME_LEN );
    }

    if ( rodsArgs->hostAddr == True ) {
        rstrcpy( procStatInp->addr, rodsArgs->hostAddrString, NAME_LEN );
    }

    return 0;
}



void
usage() {
    char *msgs[] = {
        "Usage: ips [-ahv] [-R resource] [-z zone] [-H hostAddr]",
        " ",
        "Display connection information of iRods agents currently running in",
        "the iRods federation. By default, agent info for the iCat enabled server",
        "(IES) is displayed.",
        " ",
        "The -H and -R option can be used to specify other servers for the",
        "info display. The -z option can be used to specify a remote zone for",
        "the info display. If the -a option is used, agent info for all servers",
        "in the iRods federation will be displayed.",
        " ",
        "By default, a line is output for each connection. Each line contains",
        "items given in the following order:",
        "   - pid of the agent process",
        "   - client user",
        "   - wall clock time of the connection",
        "   - the client process",
        "   - the 'from' address of the connection",
        " ",
        "If the -v option is specified, the proxy user of the connection is added",
        "following the client user.",
        " ",
        "Options are:",
        " ",
        " -a  all servers",
        " -h  this help",
        " -H  hostAddr - the host address of the server",
        " -R  resource - the server where the resource is located",
        " -v  verbose",
        " -z  zone - the remote zone",
        ""
    };
    int i;
    for ( i = 0;; i++ ) {
        if ( strlen( msgs[i] ) == 0 ) {
            break;
        }
        printf( "%s\n", msgs[i] );
    }
    printReleaseInfo( "ips" );
}
