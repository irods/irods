#include "procLog.h"
#include "miscUtil.hpp"
#include "rsGlobalExtern.hpp"
#include "rodsConnect.h"
#include "rsLog.hpp"

#include "arpa/inet.h"

#include <fstream>
#include <boost/lexical_cast.hpp>

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
