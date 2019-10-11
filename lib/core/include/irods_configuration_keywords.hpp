#ifndef CONFIGURATION_KEYWORDS_HPP
#define CONFIGURATION_KEYWORDS_HPP

#include <string>

namespace irods {
    // server_config.json keywords
    const std::string CFG_KERBEROS_NAME_KW( "kerberos_name" );
    const std::string CFG_KERBEROS_KEYTAB_KW( "kerberos_keytab" );
    const std::string CFG_PAM_PASSWORD_LENGTH_KW( "password_length" );
    const std::string CFG_PAM_NO_EXTEND_KW( "no_extend" );
    const std::string CFG_PAM_PASSWORD_MIN_TIME_KW( "password_min_time" );
    const std::string CFG_PAM_PASSWORD_MAX_TIME_KW( "password_max_time" );

    const std::string CFG_DB_USERNAME_KW( "db_username" );
    const std::string CFG_DB_PASSWORD_KW( "db_password" );
    const std::string CFG_DB_SSLMODE_KW( "db_sslmode" );
    const std::string CFG_DB_SSLROOTCERT_KW( "db_sslrootcert" );
    const std::string CFG_DB_SSLCERT_KW( "db_sslcert" );
    const std::string CFG_DB_SSLKEY_KW( "db_sslkey" );
    const std::string CFG_ZONE_NAME_KW( "zone_name" );
    const std::string CFG_ZONE_KEY_KW( "zone_key" );
    const std::string CFG_NEGOTIATION_KEY_KW( "negotiation_key" );
    const std::string CFG_RE_RULEBASE_SET_KW( "re_rulebase_set" );
    const std::string CFG_RE_NAMESPACE_SET_KW( "rule_engine_namespaces" );
    const std::string CFG_NAMESPACE_KW( "namespace" );
    const std::string CFG_RE_FUNCTION_NAME_MAPPING_SET_KW(
        "re_function_name_mapping_set" );
    const std::string CFG_RE_DATA_VARIABLE_MAPPING_SET_KW(
        "re_data_variable_mapping_set" );
    const std::string CFG_RE_PEP_REGEX_SET_KW( "regexes_for_supported_peps" );
    const std::string CFG_DEFAULT_DIR_MODE_KW( "default_dir_mode" );
    const std::string CFG_DEFAULT_FILE_MODE_KW( "default_file_mode" );
    const std::string CFG_DEFAULT_HASH_SCHEME_KW( "default_hash_scheme" );
    const std::string CFG_MATCH_HASH_POLICY_KW( "match_hash_policy" );
    const std::string CFG_FEDERATION_KW( "federation" );
    const std::string CFG_ENVIRONMENT_VARIABLES_KW( "environment_variables" );
    const std::string CFG_ADVANCED_SETTINGS_KW( "advanced_settings" );

    const std::string CFG_SERVER_PORT_RANGE_START_KW( "server_port_range_start" );
    const std::string CFG_SERVER_PORT_RANGE_END_KW( "server_port_range_end" );

    const std::string CFG_LOG_LEVEL_KW{"log_level"};
    const std::string CFG_LOG_LEVEL_CATEGORY_LEGACY_KW{"legacy"};
    const std::string CFG_LOG_LEVEL_CATEGORY_SERVER_KW{"server"};
    const std::string CFG_LOG_LEVEL_CATEGORY_AGENT_FACTORY_KW{"agent_factory"};
    const std::string CFG_LOG_LEVEL_CATEGORY_AGENT_KW{"agent"};
    const std::string CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER_KW{"delay_server"};
    const std::string CFG_LOG_LEVEL_CATEGORY_RESOURCE_KW{"resource"};
    const std::string CFG_LOG_LEVEL_CATEGORY_DATABASE_KW{"database"};
    const std::string CFG_LOG_LEVEL_CATEGORY_AUTHENTICATION_KW{"authentication"};
    const std::string CFG_LOG_LEVEL_CATEGORY_API_KW{"api"};
    const std::string CFG_LOG_LEVEL_CATEGORY_MICROSERVICE_KW{"microservice"};
    const std::string CFG_LOG_LEVEL_CATEGORY_NETWORK_KW{"network"};
    const std::string CFG_LOG_LEVEL_CATEGORY_RULE_ENGINE_KW{"rule_engine"};

    // advanced settings
    const std::string CFG_MAX_SIZE_FOR_SINGLE_BUFFER(
        "maximum_size_for_single_buffer_in_megabytes" );
    const std::string CFG_DEF_NUMBER_TRANSFER_THREADS(
        "default_number_of_transfer_threads" );
    const std::string CFG_TRANS_CHUNK_SIZE_PARA_TRANS(
        "transfer_chunk_size_for_parallel_transfer_in_megabytes" );
    const std::string CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS(
        "transfer_buffer_size_for_parallel_transfer_in_megabytes" );
    const std::string CFG_DEF_TEMP_PASSWORD_LIFETIME(
        "default_temporary_password_lifetime_in_seconds" );
    const std::string CFG_MAX_TEMP_PASSWORD_LIFETIME(
        "maximum_temporary_password_lifetime_in_seconds" );
    const std::string CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS(
        "maximum_number_of_concurrent_rule_engine_server_processes" );
    const std::string DEFAULT_LOG_ROTATION_IN_DAYS("default_log_rotation_in_days");

    const std::string CFG_RE_CACHE_SALT_KW("reCacheSalt");
    const std::string CFG_RE_SERVER_SLEEP_TIME(
        "rule_engine_server_sleep_time_in_seconds");
    const std::string CFG_RE_SERVER_EXEC_TIME(
        "rule_engine_server_execution_time_in_seconds");

    // service_account_environment.json keywords
    const std::string CFG_IRODS_USER_NAME_KW( "irods_user_name" );
    const std::string CFG_IRODS_HOST_KW( "irods_host" );
    const std::string CFG_IRODS_PORT_KW( "irods_port" );
    const std::string CFG_IRODS_HOME_KW( "irods_home" );
    const std::string CFG_IRODS_CWD_KW( "irods_cwd" );
    const std::string CFG_IRODS_AUTHENTICATION_SCHEME_KW(
        "irods_authentication_scheme" );
    const std::string CFG_IRODS_DEFAULT_RESOURCE_KW(
        "irods_default_resource" );
    const std::string CFG_IRODS_ZONE_KW( "irods_zone_name" );
    const std::string CFG_IRODS_GSI_SERVER_DN_KW( "irods_gsi_server_dn" );
    const std::string CFG_IRODS_LOG_LEVEL_KW( "irods_log_level" );
    const std::string CFG_IRODS_AUTHENTICATION_FILE_KW(
        "irods_authentication_file" );
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
    const std::string CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW(
        "irods_server_control_plane_encryption_num_hash_rounds" );
    const std::string CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW(
        "irods_server_control_plane_encryption_algorithm" );

    // irods environment advanced settings
    const std::string CFG_IRODS_MAX_SIZE_FOR_SINGLE_BUFFER(
        "irods_maximum_size_for_single_buffer_in_megabytes" );
    const std::string CFG_IRODS_DEF_NUMBER_TRANSFER_THREADS(
        "irods_default_number_of_transfer_threads" );
    const std::string CFG_IRODS_MAX_NUMBER_TRANSFER_THREADS(
        "irods_maximum_number_of_transfer_threads" );
    const std::string CFG_IRODS_TRANS_BUFFER_SIZE_FOR_PARA_TRANS(
        "irods_transfer_buffer_size_for_parallel_transfer_in_megabytes" );
    const std::string CFG_IRODS_CONNECTION_POOL_REFRESH_TIME(
        "irods_connection_pool_refresh_time_in_seconds");

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

    const std::string CFG_CATALOG_PROVIDER_HOSTS_KW("catalog_provider_hosts");
    const std::string CFG_CATALOG_SERVICE_ROLE("catalog_service_role");
    const std::string CFG_SERVICE_ROLE_PROVIDER("provider");
    const std::string CFG_SERVICE_ROLE_CONSUMER("consumer");
    const std::string CFG_SERVICE_ROLE_PROXY("proxy");

    const std::string CFG_IRODS_PLUGINS_HOME_KW( "irods_plugins_home" );

    const std::string CFG_PLUGIN_CONFIGURATION_KW("plugin_configuration");

    // plugin types
    const std::string PLUGIN_TYPE_API( "api" );
    const std::string PLUGIN_TYPE_RULE_ENGINE( "rule_engines" );
    const std::string PLUGIN_TYPE_AUTHENTICATION( "auth" );
    const std::string PLUGIN_TYPE_NETWORK( "network" );
    const std::string PLUGIN_TYPE_DATABASE( "database" );
    const std::string PLUGIN_TYPE_RESOURCE( "resources" );
    const std::string PLUGIN_TYPE_MICROSERVICE( "microservices" );


    const std::string CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW("plugin_specific_configuration");
    const std::string CFG_INSTANCE_NAME_KW("instance_name");
    const std::string CFG_PLUGIN_NAME_KW("plugin_name");

    const std::string CFG_SHARED_MEMORY_INSTANCE_KW("shared_memory_instance");
    const std::string CFG_SHARED_MEMORY_MUTEX_KW("shared_memory_mutex");

    const std::string DEFAULT_RULE_ENGINE_PLUGIN_NAME_KW("re-irods");
    const std::string DEFAULT_RULE_ENGINE_INSTANCE_NAME_KW("default_rule_engine_instance");

}; // namespace irods

#endif // CONFIGURATION_KEYWORDS_HPP
