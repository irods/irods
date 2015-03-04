/*
 * irods_server_properties.cpp
 *
 *  Created on: Jan 15, 2014
 *      Author: adt
 */

#include "irods_server_properties.hpp"
#include "irods_get_full_path_for_config_file.hpp"

#include "rods.hpp"
#include "rodsConnect.h"
#include "irods_log.hpp"
#include "irods_lookup_table.hpp"
#include "readServerConfig.hpp"

#include <string>
#include <sstream>
#include <algorithm>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;



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

    server_properties::server_properties() : captured_( false ) {
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
        key_map_[ LOCAL_ZONE_SID_KW ]        = CFG_ZONE_ID_KW;
        key_map_[ AGENT_KEY_KW ]             = CFG_NEGOTIATION_KEY_KW;

        key_map_[ CFG_ZONE_NAME ]             = CFG_ZONE_NAME;
        key_map_[ CFG_ZONE_USER ]             = CFG_ZONE_USER;
        key_map_[ CFG_ZONE_PORT ]             = CFG_ZONE_PORT;
        key_map_[ CFG_XMSG_PORT ]             = CFG_XMSG_PORT;
        key_map_[ CFG_ZONE_AUTH_SCHEME ]      = CFG_ZONE_AUTH_SCHEME;

    } // ctor

    error server_properties::capture() {
        // if a json version exists, then attempt to capture
        // that
        std::string svr_cfg;
        irods::error ret = irods::get_full_path_for_config_file(
                               "server_config.json",
                               svr_cfg );
        if ( ret.ok() ) {
            ret = capture_json( svr_cfg );
            if ( !ret.ok() ) {
                return PASS( ret );
            }

            std::string db_cfg;
            ret = irods::get_full_path_for_config_file(
                      "database_config.json",
                      db_cfg );
            if ( ret.ok() ) {
                ret = capture_json( db_cfg );
                if ( !ret.ok() ) {
                    return PASS( ret );
                }
            }

        }
        else {
            return capture_legacy();

        }

        // set the captured flag so we no its already been captured
        captured_ = true;

        return SUCCESS();

    } // capture

    error server_properties::capture_json(
        const std::string& _fn ) {
        error ret = config_props_.load( _fn );

        return ret;

    } // capture_json

// Read LEGACY_SERVER_CONFIG_FILE and fill server_properties::properties
    error server_properties::capture_legacy() {
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
        error ret = irods::get_full_path_for_config_file(
                        LEGACY_SERVER_CONFIG_FILE,
                        cfg_file );
        if ( !ret.ok() ) {
            return PASS( ret );
        }


        fptr = fopen( cfg_file.c_str(), "r" );

        if ( fptr == NULL ) {
            rodsLog( LOG_DEBUG,
                     "Cannot open LEGACY_SERVER_CONFIG_FILE file %s. errno = %d\n",
                     cfg_file.c_str(), errno );
            std::stringstream msg;
            msg << LEGACY_SERVER_CONFIG_FILE << " file error";
            return ERROR( SYS_CONFIG_FILE_ERR, msg.str().c_str() );
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
                snprintf( DBPassword, sizeof( DBPassword ), "%s", findNextTokenAndTerm( key + len ) );

            } // DB_PASSWORD_KW

            key = strstr( buf, DB_KEY_KW );
            if ( key != NULL ) {
                len = strlen( DB_KEY_KW );

                // Store key in temporary string
                snprintf( DBKey, sizeof( DBKey ), "%s", findNextTokenAndTerm( key + len ) );

            } // DB_KEY_KW

            // =-=-=-=-=-=-=-
            // PAM configuration - init PAM values
            result = config_props_.set<bool>( PAM_NO_EXTEND_KW, false );
            result = config_props_.set<size_t>( PAM_PW_LEN_KW, 20 );

            key = strstr( buf, ICAT_HOST_KW );
            if ( key != NULL ) {
                len = strlen( ICAT_HOST_KW );

                // Set property name and setting
                prop_name.assign( ICAT_HOST_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // ICAT_HOST_KW

            key = strstr( buf, RE_RULESET_KW );
            if ( key != NULL ) {
                len = strlen( RE_RULESET_KW );

                // Set property name and setting
                prop_name.assign( RE_RULESET_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // RE_RULESET_KW

            key = strstr( buf, RE_FUNCMAPSET_KW );
            if ( key != NULL ) {
                len = strlen( RE_FUNCMAPSET_KW );

                // Set property name and setting
                prop_name.assign( RE_FUNCMAPSET_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // RE_FUNCMAPSET_KW

            key = strstr( buf, RE_VARIABLEMAPSET_KW );
            if ( key != NULL ) {
                len = strlen( RE_VARIABLEMAPSET_KW );

                // Set property name and setting
                prop_name.assign( RE_VARIABLEMAPSET_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // RE_VARIABLEMAPSET_KW

            key = strstr( buf, DB_USERNAME_KW );
            if ( key != NULL ) {
                len = strlen( DB_USERNAME_KW );

                // Set property name and setting
                prop_name.assign( DB_USERNAME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // DB_USERNAME_KW

            // =-=-=-=-=-=-=-
            // PAM configuration - init PAM values
            result = config_props_.set<bool>( PAM_NO_EXTEND_KW, false );
            result = config_props_.set<size_t>( PAM_PW_LEN_KW, 20 );

            prop_setting.assign( "121" );
            result = config_props_.set<std::string>( PAM_PW_MIN_TIME_KW, prop_setting );

            prop_setting.assign( "1209600" );
            result = config_props_.set<std::string>( PAM_PW_MAX_TIME_KW, prop_setting );
            // init PAM values

            key = strstr( buf, PAM_PW_LEN_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_LEN_KW );

                // Set property name and setting
                prop_name.assign( PAM_PW_LEN_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<size_t>( prop_name, atoi( prop_setting.c_str() ) );

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
                    result = config_props_.set<bool>( PAM_NO_EXTEND_KW, true );
                }
                else {
                    result = config_props_.set<bool>( PAM_NO_EXTEND_KW, false );
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
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_PW_MIN_TIME_KW

            key = strstr( buf, PAM_PW_MAX_TIME_KW );
            if ( key != NULL ) {
                len = strlen( PAM_PW_MAX_TIME_KW );

                // Set property name and setting
                prop_name.assign( PAM_PW_MAX_TIME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // PAM_PW_MAX_TIME_KW

            key = strstr( buf, DEF_DIR_MODE_KW );
            if ( key != NULL ) {
                len = strlen( DEF_DIR_MODE_KW );

                // Set property name and setting
                prop_name.assign( DEF_DIR_MODE_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<int>( prop_name, strtol( prop_setting.c_str(), 0, 0 ) );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // DEF_DIR_MODE_KW


            key = strstr( buf, DEF_FILE_MODE_KW );
            if ( key != NULL ) {
                len = strlen( DEF_FILE_MODE_KW );

                // Set property name and setting
                prop_name.assign( DEF_FILE_MODE_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<int>( prop_name, strtol( prop_setting.c_str(), 0, 0 ) );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // DEF_FILE_MODE_KW


            key = strstr( buf, CATALOG_DATABASE_TYPE_KW );
            if ( key != NULL ) {
                len = strlen( CATALOG_DATABASE_TYPE_KW );

                // Set property name and setting
                prop_name.assign( CATALOG_DATABASE_TYPE_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // CATALOG_DATABASE_TYPE_KW

            key = strstr( buf, KERBEROS_NAME_KW );
            if ( key != NULL ) {
                len = strlen( KERBEROS_NAME_KW );
                // Set property name and setting
                prop_name.assign( KERBEROS_NAME_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );
                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );
                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // KERBEROS_NAME_KW

            key = strstr( buf, KERBEROS_KEYTAB_KW );
            if ( key != NULL ) {
                len = strlen( KERBEROS_KEYTAB_KW );
                // Set property name and setting
                prop_name.assign( KERBEROS_KEYTAB_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );
                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );
                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

                // Now set the appropriate kerberos environment variable
                setenv( "KRB5_KTNAME", prop_setting.c_str(), 1 );

            } // KERBEROS_KEYTAB_KW

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
                result = config_props_.set<std::string>( prop_name, prop_setting );

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
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // MATCH_HASH_POLICY_KW

            key = strstr( buf, LOCAL_ZONE_SID_KW );
            if ( key != NULL ) {
                len = strlen( LOCAL_ZONE_SID_KW );

                // Set property name and setting
                prop_name.assign( LOCAL_ZONE_SID_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );
            } // LOCAL_ZONE_SID_KW

            key = strstr( buf, REMOTE_ZONE_SID_KW );
            if ( key != NULL ) {
                len = strlen( REMOTE_ZONE_SID_KW );

                // Set property name and setting
                prop_name.assign( REMOTE_ZONE_SID_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                // Update properties table
                std::vector<std::string>           rem_sids;
                std::vector<std::string>::iterator sid_itr;
                if ( config_props_.has_entry( prop_name ) ) {
                    result = config_props_.get< std::vector< std::string > >( prop_name, rem_sids );

                    // do not want duplicate entries
                    sid_itr = std::find(
                                  rem_sids.begin(),
                                  rem_sids.end(),
                                  prop_setting );

                }

                if ( sid_itr == rem_sids.end() ) {
                    rem_sids.push_back( prop_setting );
                    result = config_props_.set< std::vector< std::string > >( prop_name, rem_sids );
                }

                rodsLog( LOG_DEBUG, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

            } // REMOTE_ZONE_SID_KW

            key = strstr( buf, AGENT_KEY_KW.c_str() );
            if ( key != NULL ) {
                len = strlen( AGENT_KEY_KW.c_str() );
                // Set property name and setting
                prop_name.assign( AGENT_KEY_KW );
                prop_setting.assign( findNextTokenAndTerm( key + len ) );

                if ( 32 != prop_setting.size() ) {
                    rodsLog( LOG_ERROR,
                             "%s field in %s must be 32 characters in length (currently %d characters in length).",
                             prop_name.c_str(), LEGACY_SERVER_CONFIG_FILE, prop_setting.size() );
                    fclose( fptr );
                    std::stringstream msg;
                    msg << LEGACY_SERVER_CONFIG_FILE << " file error";
                    return ERROR( SYS_CONFIG_FILE_ERR, msg.str().c_str() );
                }

                // Update properties table
                result = config_props_.set<std::string>( prop_name, prop_setting );

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
        prop_name.assign( DB_PASSWORD_KW );
        prop_setting.assign( DBPassword );
        result = config_props_.set<std::string>( prop_name, prop_setting );
        rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

        prop_name.assign( DB_KEY_KW );
        prop_setting.assign( DBKey );
        result = config_props_.set<std::string>( prop_name, prop_setting );
        rodsLog( LOG_DEBUG1, "%s=%s", prop_name.c_str(), prop_setting.c_str() );

        // add expected zone_name, zone_user, zone_port, zone_auth_scheme
        // as these are now read from server_properties
        rodsEnv env;
        int status = getRodsEnv( &env );
        if ( status < 0 ) {
            return ERROR( status, "failure in getRodsEnv" );
        }
        result = config_props_.set<std::string>(
                     irods::CFG_ZONE_NAME_KW,
                     env.rodsZone );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        result = config_props_.set<std::string>(
                     irods::CFG_ZONE_USER,
                     env.rodsUserName );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        result = config_props_.set<std::string>(
                     irods::CFG_ZONE_AUTH_SCHEME,
                     env.rodsAuthScheme );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        result = config_props_.set<int>(
                     irods::CFG_ZONE_PORT,
                     env.rodsPort );
        if ( !result.ok() ) {
            irods::log( PASS( result ) );

        }

        if ( 0 != env.xmsgPort ) {
            result = config_props_.set<int>(
                         irods::CFG_XMSG_PORT,
                         env.xmsgPort );
            if ( !result.ok() ) {
                irods::log( PASS( result ) );

            }
        }

        return result;

    } // server_properties::capture()


} // namespace irods


