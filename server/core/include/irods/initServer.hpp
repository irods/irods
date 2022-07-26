/// WORKING CHANGES(june): removed unused InformationRequiredToSafelyRenameProcess struct

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* initServer.hpp - common header file for initServer.cpp
 */

#ifndef INIT_SERVER_HPP
#define INIT_SERVER_HPP

#include "irods/rodsConnect.h"
#include <vector>
#include <string>

/* server host configuration */

int initServerInfo( int processType, rsComm_t *rsComm );
int initLocalServerHost();
int initRcatServerHostByFile();
int initZone( rsComm_t *rsComm );
int initAgent( int processType, rsComm_t *rsComm );

int initHostConfigByFile();
int initRsComm( rsComm_t *rsComm );
int initRsCommWithStartupPack( rsComm_t *rsComm, startupPack_t *startupPack );
int chkAllowedUser( const char *userName, const char *rodsZone );
int setRsCommFromRodsEnv( rsComm_t *rsComm );

#endif	/* INIT_SERVER_HPP */
