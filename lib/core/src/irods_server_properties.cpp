/*
 * irods_server_properties.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#include "irods_server_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"

#include "rods.h"
#include "rodsConnect.h"
#include "irods_log.hpp"
#include "irods_exception.hpp"
#include "irods_lookup_table.hpp"
#include "readServerConfig.hpp"

#include <string>
#include <sstream>
#include <algorithm>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#define BUF_LEN 500

//Singleton

namespace irods {

    server_properties& server_properties::instance() {
        static server_properties singleton;
        return singleton;
    }

    server_properties::server_properties() {
        key_map_[ DB_PASSWORD_KW ] = CFG_DB_PASSWORD_KW;
        key_map_[ ICAT_HOST_KW ]   = CFG_ICAT_HOST_KW;
        key_map_[ RE_RULESET_KW ]  = CFG_RE_RULEBASE_SET_KW;
        key_map_[ RE_FUNCMAPSET_KW ] = CFG_RE_FUNCTION_NAME_MAPPING_SET_KW;
        key_map_[ RE_VARIABLEMAPSET_KW ] = CFG_RE_DATA_VARIABLE_MAPPING_SET_KW;
        key_map_[ DB_USERNAME_KW ]       = CFG_DB_USERNAME_KW;
        key_map_[ PAM_PW_LEN_KW ]        = CFG_PAM_PASSWORD_LENGTH_KW;
        key_map_[ PAM_NO_EXTEND_KW ]     = CFG_PAM_NO_EXTEND_KW;
        key_map_[ PAM_PW_MIN_TIME_KW ]   = CFG_PAM_PASSWORD_MIN_TIME_KW;
        key_map_[ PAM_PW_MAX_TIME_KW ]   = CFG_PAM_PASSWORD_MAX_TIME_KW;
        key_map_[ DEF_DIR_MODE_KW ]       = CFG_DEFAULT_DIR_MODE_KW;
        key_map_[ DEF_FILE_MODE_KW ]      = CFG_DEFAULT_FILE_MODE_KW;
        key_map_[ CATALOG_DATABASE_TYPE_KW ] = CFG_CATALOG_DATABASE_TYPE_KW;
        key_map_[ KERBEROS_NAME_KW ]         = CFG_KERBEROS_NAME_KW;
        key_map_[ KERBEROS_KEYTAB_KW ]       = CFG_KERBEROS_KEYTAB_KW;
        key_map_[ DEFAULT_HASH_SCHEME_KW ]   = CFG_DEFAULT_HASH_SCHEME_KW;;
        key_map_[ MATCH_HASH_POLICY_KW ]     = CFG_MATCH_HASH_POLICY_KW;
        key_map_[ AGENT_KEY_KW ]             = CFG_NEGOTIATION_KEY_KW;

        key_map_[ CFG_ZONE_NAME ]             = CFG_ZONE_NAME;
        key_map_[ CFG_ZONE_USER ]             = CFG_ZONE_USER;
        key_map_[ CFG_ZONE_PORT ]             = CFG_ZONE_PORT;
        key_map_[ CFG_XMSG_PORT ]             = CFG_XMSG_PORT;
        key_map_[ CFG_ZONE_AUTH_SCHEME ]      = CFG_ZONE_AUTH_SCHEME;

        capture();
    } // ctor

    void server_properties::capture() {
        // if a json version exists, then attempt to capture
        // that
        std::string svr_cfg;
        irods::error ret = irods::get_full_path_for_config_file(
                               "server_config.json",
                               svr_cfg );
        if ( ret.ok() ) {
            capture_json( svr_cfg );

            std::string db_cfg;
            ret = irods::get_full_path_for_config_file(
                      "database_config.json",
                      db_cfg );
            if ( ret.ok() ) {
                capture_json( db_cfg );
            }

        }
        else {
            capture_legacy();
        }

    } // capture

    void server_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );
        if ( !ret.ok() ) {
            THROW( ret.code(), ret.result() );
        }

    } // capture_json

// Read LEGACY_SERVER_CONFIG_FILE and fill server_properties::properties
    void server_properties::capture_legacy() {
        error result = SUCCESS();
        std::string property; // property setting

        FILE *fptr;
        char buf[BUF_LEN];
        char *fchar;
        int len;
        char *key;

        char DBKey[MAX_PASSWORD_LEN], DBPassword[MAX_PASSWORD_LEN];
        memset( &DBKey, '\0', MAX_PASSWORD_LEN );
        memset( &DBPassword, '\0', MAX_PASSWORD_LEN );

        std::string cfg_file;
        error ret = irods::get_full_path_for_config_file(
                        LEGACY_SERVER_CONFIG_FILE,
                        cfg_file );
        if ( !ret.ok() ) {
            THROW( ret.code(), ret.result() );
        }

        if ( !fs::exists( cfg_file ) ) {
            return;
        }

        rodsLog(
            LOG_ERROR,
            "server_properties::capture_legacy - use of legacy configuration is deprecated" );

        fptr = fopen( cfg_file.c_str(), "r" );

        if ( fptr == NULL ) {
            rodsLog( LOG_DEBUG,
                     "Cannot open LEGACY_SERVER_CONFIG_FILE file %s. errno = %d\n",
                     cfg_file.c_str(), errno );
            std::stringstream msg;
            msg << LEGACY_SERVER_CONFIG_FILE << " file error";
            THROW( SYS_CONFIG_FILE_ERR, msg.str().c_str() );
        }

        // =-=-=-=-=-=-=-
        // PAM configuration - init PAM values
        config_props_.set<bool>( PAM_NO_EXTEND_KW, false );
        config_props_.set<size_t>( PAM_PW_LEN_KW, 20 );

        property.assign( "121" );
        config_props_.set<std::string>( PAM_PW_MIN_TIME_KW, property );

        property.assign( "1209600" );
        config_props_.set<std::string>( PAM_PW_MAX_TIME_KW, property );
        // init PAM values

        std::string initializer[] = {
            ICAT_HOST_KW,
            RE_RULESET_KW,
            RE_FUNCMAPSET_KW,
            RE_VARIABLEMAPSET_KW,
            DB_USERNAME_KW,
            PAM_PW_MIN_TIME_KW,
            PAM_PW_MAX_TIME_KW,
            CATALOG_DATABASE_TYPE_KW,
            KERBEROS_NAME_KW,
            LOCAL_ZONE_SID_KW };
        std::vector<std::string> keys( initializer, initializer + sizeof( initializer ) / sizeof( std::string ) );

        buf[BUF_LEN - 1] = '\0';
        fchar = fgets( buf, BUF_LEN - 1, fptr );
        while ( fchar ) {
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
                snprintf( DBPassword, sizeof( DBPassword ), "%s", findNextTokenAndTerm( key + len ) );

            } // DB_PASSWORD_KW

            key = strstr( buf, DB_KEY_KW );
            if ( key != NULL ) {
                len = strlen( DB_KEY_KW );

                // Store key in temporary string
                snprintf( DBKey, sizeof( DBKey ), "%s", findNextTokenAndTerm( key + len ) );

            } // DB_KEY_KW

            for ( std::vector<std::string>::iterator it = keys.begin(); it != keys.end(); it++ ) {
                char * pos = strstr( buf, it->c_str() );
                if ( pos != NULL ) {

                    // Set property
                    property.assign( findNextTokenAndTerm( pos + it->size() ) );

                    // Update properties table
                    config_props_.set<std::string>( *it, property );
                    rodsLog( LOG_DEBUG1, "%s=%s", it->c_str(), property.c_str() );
                }
            }

            key = strstr( buf, PAM_PW_LEN_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_LEN_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                config_props_.set<size_t>( PAM_PW_LEN_KW, atoi( property.c_str() ) );

                rodsLog( LOG_DEBUG, "%s=%s", PAM_PW_LEN_KW, property.c_str() );
            } // PAM_PW_LEN_KW

            key = strstr( buf, PAM_NO_EXTEND_KW );
            if ( key != NULL ) {
                len = strlen( PAM_NO_EXTEND_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                std::transform( property.begin(), property.end(), property.begin(), ::tolower );
                if ( property == "true" ) {
                    config_props_.set<bool>( PAM_NO_EXTEND_KW, true );
                }
                else {
                    config_props_.set<bool>( PAM_NO_EXTEND_KW, false );
                }

                rodsLog( LOG_DEBUG, "%s=%s", PAM_NO_EXTEND_KW, property.c_str() );
            } // PAM_NO_EXTEND_KW

            key = strstr( buf, DEF_DIR_MODE_KW );
            if ( key != NULL ) {
                len = strlen( DEF_DIR_MODE_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                config_props_.set<int>( DEF_DIR_MODE_KW, strtol( property.c_str(), 0, 0 ) );

                rodsLog( LOG_DEBUG, "%s=%s", DEF_DIR_MODE_KW, property.c_str() );
            } // DEF_DIR_MODE_KW


            key = strstr( buf, DEF_FILE_MODE_KW );
            if ( key != NULL ) {
                len = strlen( DEF_FILE_MODE_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                config_props_.set<int>( DEF_FILE_MODE_KW, strtol( property.c_str(), 0, 0 ) );

                rodsLog( LOG_DEBUG, "%s=%s", DEF_FILE_MODE_KW, property.c_str() );
            } // DEF_FILE_MODE_KW

            key = strstr( buf, KERBEROS_KEYTAB_KW );
            if ( key != NULL ) {
                len = strlen( KERBEROS_KEYTAB_KW );
                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );
                // Update properties table
                config_props_.set<std::string>( KERBEROS_KEYTAB_KW, property );
                rodsLog( LOG_DEBUG, "%s=%s", KERBEROS_KEYTAB_KW, property.c_str() );

                // Now set the appropriate kerberos environment variable
                setenv( "KRB5_KTNAME", property.c_str(), 1 );

            } // KERBEROS_KEYTAB_KW

            key = strstr( buf, DEFAULT_HASH_SCHEME_KW );
            if ( key != NULL ) {
                len = strlen( DEFAULT_HASH_SCHEME_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );
                std::transform(
                    property.begin(),
                    property.end(),
                    property.begin(),
                    ::tolower );

                // Update properties table
                config_props_.set<std::string>( DEFAULT_HASH_SCHEME_KW, property );

                rodsLog( LOG_DEBUG, "%s=%s", DEFAULT_HASH_SCHEME_KW, property.c_str() );
            } // DEFAULT_HASH_SCHEME_KW

            key = strstr( buf, MATCH_HASH_POLICY_KW );
            if ( key != NULL ) {
                len = strlen( MATCH_HASH_POLICY_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );
                std::transform(
                    property.begin(),
                    property.end(),
                    property.begin(),
                    ::tolower );

                // Update properties table
                config_props_.set<std::string>( MATCH_HASH_POLICY_KW, property );

                rodsLog( LOG_DEBUG, "%s=%s", MATCH_HASH_POLICY_KW, property.c_str() );
            } // MATCH_HASH_POLICY_KW

            key = strstr( buf, REMOTE_ZONE_SID_KW );
            if ( key != NULL ) {
                len = strlen( REMOTE_ZONE_SID_KW );

                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                try {
                    std::vector<std::string>& rem_sids = config_props_.get< std::vector< std::string > >( REMOTE_ZONE_SID_KW );

                    // do not want duplicate entries
                    if ( std::find( rem_sids.begin(), rem_sids.end(), property ) == rem_sids.end() ) {
                        rem_sids.push_back( property );
                    }

                } catch ( const irods::exception& e ) {
                    if ( e.code() != KEY_NOT_FOUND ) {
                        rodsLog( LOG_ERROR, e.what() );
                    }
                }
                rodsLog( LOG_DEBUG, "%s=%s", REMOTE_ZONE_SID_KW, property.c_str() );

            } // REMOTE_ZONE_SID_KW

            key = strstr( buf, AGENT_KEY_KW.c_str() );
            if ( key != NULL ) {
                len = strlen( AGENT_KEY_KW.c_str() );
                // Set property
                property.assign( findNextTokenAndTerm( key + len ) );

                if ( 32 != property.size() ) {
                    rodsLog( LOG_ERROR,
                             "%s field in %s must be 32 characters in length (currently %d characters in length).",
                             AGENT_KEY_KW.c_str(), LEGACY_SERVER_CONFIG_FILE, property.size() );
                    fclose( fptr );
                    std::stringstream msg;
                    msg << LEGACY_SERVER_CONFIG_FILE << " file error";
                    THROW( SYS_CONFIG_FILE_ERR, msg.str().c_str() );
                }

                // Update properties table
                config_props_.set<std::string>( AGENT_KEY_KW, property );

            } // AGENT_KEY_KW

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
        property.assign( DBPassword );
        config_props_.set<std::string>( DB_PASSWORD_KW, property );
        rodsLog( LOG_DEBUG1, "%s=%s", DB_PASSWORD_KW, property.c_str() );

        property.assign( DBKey );
        config_props_.set<std::string>( DB_KEY_KW, property );
        rodsLog( LOG_DEBUG1, "%s=%s", DB_KEY_KW, property.c_str() );

        // add expected zone_name, zone_user, zone_port, zone_auth_scheme
        // as these are now read from server_properties
        rodsEnv env;
        int status = getRodsEnv( &env );
        if ( status < 0 ) {
            THROW( status, "failure in getRodsEnv" );
        }
        config_props_.set<std::string>(
                     irods::CFG_ZONE_NAME_KW,
                     env.rodsZone );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        config_props_.set<std::string>(
                     irods::CFG_ZONE_USER,
                     env.rodsUserName );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        config_props_.set<std::string>(
                     irods::CFG_ZONE_AUTH_SCHEME,
                     env.rodsAuthScheme );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        config_props_.set<int>(
                     irods::CFG_ZONE_PORT,
                     env.rodsPort );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        if ( 0 != env.xmsgPort ) {
            config_props_.set<int>(
                         irods::CFG_XMSG_PORT,
                         env.xmsgPort );
            if ( !result.ok() ) {
                irods::log( PASS( result ) );

            }
        }

    } // server_properties::capture()

    void server_properties::remove( const std::string& _key ) {
        config_props_.remove( _key );
    }

    void delete_server_property( const std::string& _prop ) {
        irods::server_properties::instance().remove(_prop);
    } // delete_server_property

} // namespace irods


