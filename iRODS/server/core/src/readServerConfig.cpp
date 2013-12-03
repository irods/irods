/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
 This routine opens and reads in parameters (RCAT ones) from the
 server configuration file.  Note that initServer also reads thru the
 server server configuration file, to set up the server rcatHost
 parameters.  We thought it would be best to have a single
 configuration file (plus the rules and client env) to simplify server
 setup, so both this and initServer read thru the file.
*/

#include "rods.hpp"

#include "readServerConfig.hpp"
#include "initServer.hpp"

#include <string>
#include <algorithm>

#define BUF_LEN 500

char *
getServerConfigDir()
{
    char *myDir;

    if ((myDir = (char *) getenv("irodsConfigDir")) != (char *) NULL) {
        return (myDir);
    }
    return (DEF_CONFIG_DIR);
}

int 
readServerConfig(rodsServerConfig_t *rodsServerConfig) {
    FILE *fptr;
    char buf[BUF_LEN];
    char *fchar;
    int len;
    char *key;
    char *serverConfigFile;

    serverConfigFile =  (char *) malloc((strlen (getServerConfigDir()) +
                strlen(SERVER_CONFIG_FILE) + 24));

    sprintf (serverConfigFile, "%s/%s", getServerConfigDir(), 
            SERVER_CONFIG_FILE);

    fptr = fopen (serverConfigFile, "r");

    if (fptr == NULL) {
        rodsLog (LOG_NOTICE, 
                "Cannot open SERVER_CONFIG_FILE file %s. errno = %d\n",
                serverConfigFile, errno);
        free (serverConfigFile);
        return (SYS_CONFIG_FILE_ERR);
    }
    free (serverConfigFile);

    buf[BUF_LEN-1]='\0';
    fchar = fgets(buf, BUF_LEN-1, fptr);
    for(;fchar!='\0';) {
        if (buf[0]=='#' || buf[0]=='/') {
            buf[0]='\0'; /* Comment line, ignore */
        }
        key=strstr(buf, DB_PASSWORD_KW);
        if (key != NULL) {
            len = strlen(DB_PASSWORD_KW);
            rstrcpy(rodsServerConfig->DBPassword, 
                    findNextTokenAndTerm(key+len), MAX_PASSWORD_LEN);
            rodsLog(LOG_DEBUG1, "DBPassword=%s", rodsServerConfig->DBPassword);
        }
        key=strstr(buf, DB_KEY_KW);
        if (key != NULL) {
            len = strlen(DB_KEY_KW);
            rstrcpy(rodsServerConfig->DBKey, 
                    findNextTokenAndTerm(key+len), MAX_PASSWORD_LEN);
            rodsLog(LOG_DEBUG1, "DBKey=%s", 
                    rodsServerConfig->DBKey);
        }
        key=strstr(buf, DB_USERNAME_KW);
        if (key != NULL) {
            len = strlen(DB_USERNAME_KW);
            rstrcpy(rodsServerConfig->DBUsername, 
                    findNextTokenAndTerm(key+len), NAME_LEN);
            rodsLog(LOG_DEBUG1, "DBUsername=%s", rodsServerConfig->DBUsername);
        }

        // =-=-=-=-=-=-=-
        // pam configuration - init pam values in config struct
        rodsServerConfig->eirods_pam_auth_no_extend = false;
        rodsServerConfig->eirods_pam_password_len   = 20;
        strncpy( 
            rodsServerConfig->eirods_pam_password_min_time,
            "121",
            NAME_LEN );
        strncpy( 
            rodsServerConfig->eirods_pam_password_max_time,
            "1209600",
            NAME_LEN );

        key = strstr( buf, PAM_PW_LEN_KW );
        if( key != NULL ) {
            len = strlen( PAM_PW_LEN_KW );
            char val[ NAME_LEN ];
            rstrcpy( 
               val,
               findNextTokenAndTerm( key + len ), 
               NAME_LEN );
            rodsServerConfig->eirods_pam_password_len = atoi( val );
            rodsLog( 
                LOG_NOTICE, 
                "%s=%s", 
                PAM_PW_LEN_KW, 
                val );
        
        } // PAM_PW_LEN_KW

        key = strstr( buf, PAM_NO_EXTEND_KW );
        if( key != NULL ) {
            len = strlen( PAM_NO_EXTEND_KW );
            char val[ NAME_LEN ];
            rstrcpy( 
               val,
               findNextTokenAndTerm( key + len ), 
               NAME_LEN );
           
           std::string val_str = val;
           std::transform( 
               val_str.begin(), 
               val_str.end(), 
               val_str.begin(), 
               ::tolower );
            if( val_str == "true" ) {
                rodsServerConfig->eirods_pam_auth_no_extend = true;
            } else {
                rodsServerConfig->eirods_pam_auth_no_extend = false;
            }
            
            rodsLog( 
                LOG_NOTICE, 
                "%s=%s", 
                PAM_NO_EXTEND_KW, 
                val );
        
        } // PAM_NO_EXTEND_KW

        key = strstr( buf, PAM_PW_MIN_TIME_KW );
        if( key != NULL ) {
            len = strlen( PAM_PW_MIN_TIME_KW );
            rstrcpy( 
               rodsServerConfig->eirods_pam_password_min_time,
               findNextTokenAndTerm( key + len ), 
               NAME_LEN );
            rodsLog( 
                LOG_NOTICE, 
                "%s=%s", 
                PAM_PW_MIN_TIME_KW, 
                rodsServerConfig->eirods_pam_password_min_time );
        
        } // PAM_PW_MIN_TIME_KW

        key = strstr( buf, PAM_PW_MAX_TIME_KW );
        if( key != NULL ) {
            len = strlen( PAM_PW_MAX_TIME_KW );
            rstrcpy( 
               rodsServerConfig->eirods_pam_password_min_time,
               findNextTokenAndTerm( key + len ), 
               NAME_LEN );
            rodsLog( 
                LOG_NOTICE, 
                "%s=%s", 
                PAM_PW_MAX_TIME_KW, 
                rodsServerConfig->eirods_pam_password_min_time );
        
        } // PAM_PW_MAX_TIME_KW











           


        fchar = fgets(buf, BUF_LEN-1, fptr);
    }
    fclose (fptr);

    if (strlen(rodsServerConfig->DBKey) > 0 &&
            strlen(rodsServerConfig->DBPassword) > 0) {
        char sPassword[MAX_PASSWORD_LEN+10];
        strncpy(sPassword, rodsServerConfig->DBPassword, MAX_PASSWORD_LEN);
        obfDecodeByKey(sPassword, 
                rodsServerConfig->DBKey,
                rodsServerConfig->DBPassword);
        memset(sPassword, 0, MAX_PASSWORD_LEN);
    }

    return(0);
}




















