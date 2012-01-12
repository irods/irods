/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* readServerConfig.h - common header file for readServerConfig.c
 */

/*

 readServerConfig.c opens and reads in parameters (ICAT ones) from the
 server configuration file.  Note that initServer also reads thru the
 server server configuration file, to set up the server rcatHost
 parameters.  We thought it would be best to have a single
 configuration file (plus the rules and client env) to simplify server
 setup, so both this and initServer read thru the file.

 */


#ifndef READ_SERVER_CONFIG_H
#define READ_SERVER_CONFIG_H

#include "rods.h"

/* server host configuration */

#define SERVER_CONFIG_FILE  "server.config"

/* keywords for the ICAT_HOST_FILE */
#define DB_PASSWORD_KW		"DBPassword"
#define DB_KEY_KW	        "DBKey"
#define DB_USERNAME_KW	        "DBUsername"

typedef struct rodsServerConfig {
   char DBUsername[NAME_LEN];
   char DBPassword[MAX_PASSWORD_LEN];
   char DBKey[MAX_PASSWORD_LEN];  /* used to descramble password */
} rodsServerConfig_t;

int
readServerConfig (rodsServerConfig_t *rodsServerConfig);

char *
findNextTokenAndTerm(char *inPtr);

#endif	/* READ_SERVER_CONFIG_H */
