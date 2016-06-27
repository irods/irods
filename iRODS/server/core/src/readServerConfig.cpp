/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/**
 * The server configuration settings are now stored in a singleton irods::server_properties
 * and server_config.json is read by irods::server_properties::capture()
 *
 */


#include "readServerConfig.hpp"
#include "rsLog.hpp"

char *
getServerConfigDir() {
    char *myDir;

    if ( ( myDir = ( char * ) getenv( "irodsConfigDir" ) ) != ( char * ) NULL ) {
        return myDir;
    }
    return DEF_CONFIG_DIR;
}

