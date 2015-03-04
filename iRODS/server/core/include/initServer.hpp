/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.h - common header file for initServer.c
 */



#ifndef INIT_SERVER_HPP
#define INIT_SERVER_HPP

#include "rodsConnect.h"
//#include "sockComm.hpp"
#include <vector>
#include <string>

/* server host configuration */

int
initServerInfo( rsComm_t *rsComm );
int
initLocalServerHost();
int
initRcatServerHostByFile();
int
initZone( rsComm_t *rsComm );
int
initAgent( int processType, rsComm_t *rsComm );
void cleanup();
void cleanupAndExit( int status );
void signalExit( int );
void
rsPipeSignalHandler( int );

int
initHostConfigByFile();
int
initRsComm( rsComm_t *rsComm );
void
daemonize( int runMode, int logFd );
int
logFileOpen( int runMode, char *logDir, char *logFileName );
int
initRsCommWithStartupPack( rsComm_t *rsComm, startupPack_t *startupPack );
int
initConnectControl();
int
chkAllowedUser( char *userName, char *rodsZone );
int
queAllowedUser( struct allowedUser *allowedUser,
                struct allowedUser **allowedUserHead );
int
matchAllowedUser( char *userName, char *rodsZone,
                  struct allowedUser *allowedUserHead );
int
freeAllAllowedUser( struct allowedUser *allowedUserHead );
int
setRsCommFromRodsEnv( rsComm_t *rsComm );
int
queAgentProc( agentProc_t *agentPorc, agentProc_t **agentPorcHead,
              irodsPosition_t position );
int
purgeLockFileDir( int chkLockFlag ); // JMC - backport 4612

#endif	/* INIT_SERVER_H */
