/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/* readServerConfig.h - common header file for readServerConfig.c
 */

/**
 * The server configuration settings are now stored in a singleton irods::server_properties
 * and server_config.json is read by irods::server_properties::getInstance().capture()
 *
 */


#ifndef READ_SERVER_CONFIG_HPP
#define READ_SERVER_CONFIG_HPP

#include "rods.hpp"
#include "authenticate.hpp"

/* server host configuration */

#define LEGACY_SERVER_CONFIG_FILE	"server.config"
#define SERVER_CONFIG_FILE	"server_config.json"

/* keywords for the ICAT_HOST_FILE */
#define DB_PASSWORD_KW		"DBPassword"
#define DB_KEY_KW			"DBKey"
#define DB_USERNAME_KW		"DBUsername"

#define PAM_PW_LEN_KW		"pam_password_length"
#define PAM_NO_EXTEND_KW	"pam_no_extend"
#define PAM_PW_MIN_TIME_KW	"pam_password_min_time"
#define PAM_PW_MAX_TIME_KW	"pam_password_max_time"

#define CATALOG_DATABASE_TYPE_KW	"catalog_database_type"

#define DEFAULT_HASH_SCHEME_KW	"default_hash_scheme"
#define MATCH_HASH_POLICY_KW	"match_hash_policy"

#define DEF_DIR_MODE_KW		"default_dir_mode"
#define DEF_FILE_MODE_KW	"default_file_mode"
extern "C" {

    typedef struct rodsServerConfig {

        char DBUsername[NAME_LEN];
        char DBPassword[MAX_PASSWORD_LEN];
        char DBKey[MAX_PASSWORD_LEN];  /* used to descramble password */

        // =-=-=-=-=-=-=-
        // agent side pam configuration
        bool   irods_pam_auth_no_extend;
        size_t irods_pam_password_len;
        char   irods_pam_password_min_time[ NAME_LEN ];
        char   irods_pam_password_max_time[ NAME_LEN ];

        // =-=-=-=-=-=-=-
        // agent side database plugin configuration
        char   catalog_database_type[ NAME_LEN ];

        // =-=-=-=-=-=-=-
        // desired hash scheme
        char   default_hash_scheme[NAME_LEN];
        char   match_hash_policy  [NAME_LEN];

    } rodsServerConfig_t;

    char *getServerConfigDir();
    char* findNextTokenAndTerm( char *inPtr );

}

#endif	/* READ_SERVER_CONFIG_H */
