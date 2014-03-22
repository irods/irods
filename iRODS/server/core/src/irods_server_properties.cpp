/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * irods_server_properties.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#include "irods_server_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"

#include "rods.hpp"

#include "readServerConfig.hpp"
#include "initServer.hpp"

#include <string>
#include <algorithm>

#define BUF_LEN 500


namespace irods {

    // Access method for singleton
    server_properties& server_properties::getInstance() {
        static server_properties instance;
        return instance;
    }


    error server_properties::capture_if_needed() {
        error result = SUCCESS();
        if ( !captured_ ) {
            result = capture();
        }
        return result;
    }

    // Read server.config and fill server_properties::properties
    error server_properties::capture() {
        error result = SUCCESS();
        std::string prop_name, prop_setting; // property name and setting

        FILE *fptr;
        char buf[BUF_LEN];
        char *fchar;
        int len;
        char *key;

        char DBKey[MAX_PASSWORD_LEN], DBPassword[MAX_PASSWORD_LEN];
        memset( &DBKey, '\0', MAX_PASSWORD_LEN );
        memset( &DBPassword, '\0', MAX_PASSWORD_LEN );

        std::string cfg_file;
        error ret = irods::get_full_path_for_config_file( SERVER_CONFIG_FILE, cfg_file );
        if ( !ret.ok() ) {
            return PASS( ret );
        }


        fptr = fopen( cfg_file.c_str(), "r" );

        if ( fptr == NULL ) {
            printf( "Cannot open SERVER_CONFIG_FILE file %s. errno = %d\n",
                    cfg_file.c_str(), errno );
            fflush( stdout );
            rodsLog( LOG_NOTICE,
                     "Cannot open SERVER_CONFIG_FILE file %s. errno = %d\n",
                     cfg_file.c_str(), errno );
            return ERROR( SYS_CONFIG_FILE_ERR, "server config file error" );
        }

        buf[BUF_LEN - 1] = '\0';
        fchar = fgets( buf, BUF_LEN - 1, fptr );
        for ( ; fchar != '\0'; ) {
            if ( buf[0] == '#' || buf[0] == '/' ) {
                buf[0] = '\0'; /* Comment line, ignore */
            }

            /**
             * Parsing of server configuration settings
             */
            key = strstr( buf, DB_PASSWORD_KW );
            if ( key != NULL ) {
                len = strlen( DB_PASSWORD_KW );

                // Store password in temporary string
                strncpy( DBPassword, findNextTokenAndTerm( key + len ), MAX_PASSWORD_LEN );

            } // DB_PASSWORD_KW

            key = strstr( buf, DB_KEY_KW );
            if ( key != NULL ) {
                len = strlen( DB_KEY_KW );

                // Store key in temporary string
                strncpy( DBKey, findNextTokenAndTerm( key + len ), MAX_PASSWORD_LEN );

            } // DB_KEY_KW

            // =-=-=-=-=-=-=-
            // PAM configuration - init PAM values
            result = properties.set<bool>( PAM_NO_EXTEND_KW, false );
            result = properties.set<size_t>( PAM_PW_LEN_KW, 20 );

            key = strstr( buf, DB_USERNAME_KW );
            if ( key != NULL ) {
                len = strlen( DB_USERNAME_KW );

                // Set property name and setting
                prop_name.assign( DB_USERNAME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // DB_USERNAME_KW

            // =-=-=-=-=-=-=-
            // PAM configuration - init PAM values
            result = properties.set<bool>( PAM_NO_EXTEND_KW, false );
            result = properties.set<size_t>( PAM_PW_LEN_KW, 20 );

            prop_setting.assign( "121" );
            result = properties.set<std::string>( PAM_PW_MIN_TIME_KW, prop_setting );

            prop_setting.assign( "1209600" );
            result = properties.set<std::string>( PAM_PW_MAX_TIME_KW, prop_setting );
            // init PAM values

            key = strstr( buf, PAM_PW_LEN_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_LEN_KW );

                // Set property name and setting
                prop_name.assign( PAM_PW_LEN_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = properties.set<size_t>( prop_name, atoi( prop_setting.c_str() ) );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_PW_LEN_KW

            key = strstr( buf, PAM_NO_EXTEND_KW );
            if ( key != NULL ) {
                len = strlen( PAM_NO_EXTEND_KW );

                // Set property name and setting
                prop_name.assign( PAM_NO_EXTEND_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                std::transform( prop_setting.begin(), prop_setting.end(), prop_setting.begin(), ::tolower );
                if ( prop_setting == "true" ) {
                    result = properties.set<bool>( PAM_NO_EXTEND_KW, true );
                }
                else {
                    result = properties.set<bool>( PAM_NO_EXTEND_KW, false );
                }

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_NO_EXTEND_KW

            key = strstr( buf, PAM_PW_MIN_TIME_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_MIN_TIME_KW );

                // Set property name and setting
                prop_name.assign( PAM_PW_MIN_TIME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_PW_MIN_TIME_KW

            key = strstr( buf, PAM_PW_MAX_TIME_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_MAX_TIME_KW );

                // Set property name and setting
                prop_name.assign( PAM_PW_MAX_TIME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_PW_MAX_TIME_KW

            key = strstr( buf, RUN_SERVER_AS_ROOT_KW );
            if ( key != NULL ) {
                len = strlen( RUN_SERVER_AS_ROOT_KW );

                // Set property name and setting
                prop_name.assign( RUN_SERVER_AS_ROOT_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                std::transform( prop_setting.begin(), prop_setting.end(), prop_setting.begin(), ::tolower );
                if ( prop_setting == "true" ) {
                    result = properties.set<bool>( RUN_SERVER_AS_ROOT_KW, true );
                }
                else {
                    result = properties.set<bool>( RUN_SERVER_AS_ROOT_KW, false );
                }

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // RUN_SERVER_AS_ROOT_KW


            key = strstr( buf, CATALOG_DATABASE_TYPE_KW );
            if ( key != NULL ) {
                len = strlen( CATALOG_DATABASE_TYPE_KW );

                // Set property name and setting
                prop_name.assign( CATALOG_DATABASE_TYPE_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // CATALOG_DATABASE_TYPE_KW

            key = strstr( buf, KERBEROS_NAME_KW );
            if ( key != NULL ) {
                len = strlen( KERBEROS_NAME_KW );
                // Set property name and setting
                prop_name.assign( KERBEROS_NAME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );
                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );
                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // KERBEROS_NAME_KW



            key = strstr( buf, DEFAULT_HASH_SCHEME_KW );
            if ( key != NULL ) {
                len = strlen( DEFAULT_HASH_SCHEME_KW );

                // Set property name and setting
                prop_name.assign( DEFAULT_HASH_SCHEME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );
                std::transform(
                    prop_setting.begin(),
                    prop_setting.end(),
                    prop_setting.begin(),
                    ::tolower );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // DEFAULT_HASH_SCHEME_KW

            key = strstr( buf, MATCH_HASH_POLICY_KW );
            if ( key != NULL ) {
                len = strlen( MATCH_HASH_POLICY_KW );

                // Set property name and setting
                prop_name.assign( MATCH_HASH_POLICY_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );
                std::transform(
                    prop_setting.begin(),
                    prop_setting.end(),
                    prop_setting.begin(),
                    ::tolower );

                // Update properties table
                result = properties.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // MATCH_HASH_POLICY_KW

            fchar = fgets( buf, BUF_LEN - 1, fptr );

        } // for ( ; fchar != '\0'; )

        fclose( fptr );

        // unscramble password
        if ( strlen( DBKey ) > 0 && strlen( DBPassword ) > 0 ) {
            char sPassword[MAX_PASSWORD_LEN + 10];
            strncpy( sPassword, DBPassword, MAX_PASSWORD_LEN );
            obfDecodeByKey( sPassword, DBKey, DBPassword );
            memset( sPassword, 0, MAX_PASSWORD_LEN );
        }

        // store password and key in server properties
        prop_name.assign( DB_PASSWORD_KW );
        prop_setting.assign( DBPassword );
        result = properties.set<std::string>( prop_name, prop_setting );
        rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

        prop_name.assign( DB_KEY_KW );
        prop_setting.assign( DBKey );
        result = properties.set<std::string>( prop_name, prop_setting );
        rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

        // set the captured flag so we no its already been captured
        captured_ = true;

        return result;

    } // server_properties::capture()


} // namespace irods


