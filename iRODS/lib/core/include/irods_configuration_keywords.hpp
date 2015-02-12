

#ifndef CONFIGURATION_KEYWORDS_HPP
#define CONFIGURATION_KEYWORDS_HPP

#include <string>

namespace irods {
    // server_config.json keywords
    const std::string CFG_FILENAME_KW( "filename" );
    const std::string CFG_KERBEROS_NAME_KW( "kerberos_name" );
    const std::string CFG_KERBEROS_KEYTAB_KW( "kerberos_keytab" );
    const std::string CFG_PAM_PASSWORD_LENGTH_KW( "pam_password_length" );
    const std::string CFG_PAM_NO_EXTEND_KW( "pam_no_extend" );
    const std::string CFG_PAM_PASSWORD_MIN_TIME_KW( "pam_password_min_time" );
    const std::string CFG_PAM_PASSWORD_MAX_TIME_KW( "pam_password_max_time" );

    const std::string CFG_CATALOG_DATABASE_TYPE_KW( "catalog_database_type" );
    const std::string CFG_DB_USERNAME_KW( "db_username" );
    const std::string CFG_DB_PASSWORD_KW( "db_password" );
    const std::string CFG_ZONE_NAME_KW( "zone_name" );
    const std::string CFG_ZONE_ID_KW( "zone_id" );
    const std::string CFG_NEGOTIATION_KEY_KW( "negotiation_key" );
    const std::string CFG_ICAT_HOST_KW( "icat_host" );
    const std::string CFG_RE_RULEBASE_SET_KW( "re_rulebase_set" );
    const std::string CFG_RE_FUNCTION_NAME_MAPPING_SET_KW(
        "re_function_name_mapping_set" );
    const std::string CFG_RE_DATA_VARIABLE_MAPPING_SET_KW(
        "re_data_variable_mapping_set" );
    const std::string CFG_DEFAULT_DIR_MODE_KW( "default_dir_mode" );
    const std::string CFG_DEFAULT_FILE_MODE_KW( "default_file_mode" );
    const std::string CFG_DEFAULT_HASH_SCHEME_KW( "default_hash_scheme" );
    const std::string CFG_MATCH_HASH_POLICY_KW( "match_hash_policy" );
    const std::string CFG_FEDERATION_KW( "federation" );
    const std::string CFG_ENVIRONMENT_VARIABLES_KW( "environment_variables" );



    // service_account_environment.json keywords
    const std::string CFG_IRODS_USER_NAME_KW( "irods_user_name" );
    const std::string CFG_IRODS_HOST_KW( "irods_host" );
    const std::string CFG_IRODS_PORT_KW( "irods_port" );
    const std::string CFG_IRODS_XMSG_HOST_KW( "xmsg_host" );
    const std::string CFG_IRODS_XMSG_PORT_KW( "xmsg_port" );
    const std::string CFG_IRODS_HOME_KW( "irods_home" );
    const std::string CFG_IRODS_CWD_KW( "irods_cwd" );
    const std::string CFG_IRODS_AUTHENTICATION_SCHEME_KW(
        "irods_authentication_scheme" );
    const std::string CFG_IRODS_DEFAULT_RESOURCE_KW(
        "irods_default_resource" );
    const std::string CFG_IRODS_ZONE_KW( "irods_zone" );
    const std::string CFG_IRODS_GSI_SERVER_DN_KW( "irods_gsi_server_dn" );
    const std::string CFG_IRODS_LOG_LEVEL_KW( "irods_log_level" );
    const std::string CFG_IRODS_AUTHENTICATION_FILE_NAME_KW(
        "irods_authentication_file_name" );
    const std::string CFG_IRODS_DEBUG_KW( "irods_debug" );
    const std::string CFG_IRODS_CLIENT_SERVER_POLICY_KW(
        "irods_client_server_policy" );
    const std::string CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW(
        "irods_client_server_negotiation" );
    const std::string CFG_IRODS_ENCRYPTION_KEY_SIZE_KW(
        "irods_encryption_key_size" );
    const std::string CFG_IRODS_ENCRYPTION_SALT_SIZE_KW(
        "irods_encryption_salt_size" );
    const std::string CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW(
        "irods_encryption_num_hash_rounds" );
    const std::string CFG_IRODS_ENCRYPTION_ALGORITHM_KW(
        "irods_encryption_algorithm" );
    const std::string CFG_IRODS_DEFAULT_HASH_SCHEME_KW(
        "irods_default_hash_scheme" );
    const std::string CFG_IRODS_MATCH_HASH_POLICY_KW(
        "irods_match_hash_policy" );

    const std::string CFG_IRODS_ENVIRONMENT_FILE_KW(
        "irods_environment_file" );
    const std::string CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW(
        "irods_session_environment_file" );
    const std::string CFG_IRODS_SERVER_CONTROL_PLANE_PORT(
        "irods_server_control_plane_port" );


    const std::string CFG_IRODS_SERVER_CONTROL_PLANE_KEY(
        "irods_server_control_plane_key" );
    const std::string CFG_IRODS_SERVER_CONOTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW(
        "irods_server_control_plane_encryption_num_hash_rounds" );
    const std::string CFG_IRODS_SERVER_CONOTROL_PLANE_ENCRYPTION_ALGORITHM_KW(
        "irods_server_control_plane_encryption_algorithm" );

    // legacy ssl environment variables
    const std::string CFG_IRODS_SSL_CA_CERTIFICATE_PATH(
        "irods_ssl_ca_certificate_path" );
    const std::string CFG_IRODS_SSL_CA_CERTIFICATE_FILE(
        "irods_ssl_ca_certificate_file" );
    const std::string CFG_IRODS_SSL_VERIFY_SERVER(
        "irods_ssl_verify_server" );
    const std::string CFG_IRODS_SSL_CERTIFICATE_CHAIN_FILE(
        "irods_ssl_certificate_chain_file" );
    const std::string CFG_IRODS_SSL_CERTIFICATE_KEY_FILE(
        "irods_ssl_certificate_key_file" );
    const std::string CFG_IRODS_SSL_DH_PARAMS_FILE( "irods_ssl_dh_params_file" );

    // irods environment values now included in server_config
    const std::string CFG_ZONE_NAME( "zone_name" );
    const std::string CFG_ZONE_USER( "zone_user" );
    const std::string CFG_ZONE_PORT( "zone_port" );
    const std::string CFG_ZONE_AUTH_SCHEME( "zone_auth_scheme" );
    const std::string CFG_XMSG_PORT( "xmsg_port" );


    // irods control plane values
    const std::string CFG_SERVER_CONTROL_PLANE_PORT(
        "server_control_plane_port" );
    const std::string CFG_RULE_ENGINE_CONTROL_PLANE_PORT(
        "rule_engine_control_plane_port" );
    const std::string CFG_SERVER_CONTROL_PLANE_TIMEOUT(
        "server_control_plane_timeout_milliseconds" );


    const std::string CFG_SERVER_CONTROL_PLANE_KEY(
        "server_control_plane_key" );
    const std::string CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW(
        "server_control_plane_encryption_num_hash_rounds" );
    const std::string CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW(
        "server_control_plane_encryption_algorithm" );

}; // namespace irods

#endif // CONFIGURATION_KEYWORDS_HPP



