#ifndef CONFIGURATION_KEYWORDS_HPP
#define CONFIGURATION_KEYWORDS_HPP

#include <string>

namespace irods
{
    // server_config.json keywords
    extern const std::string CFG_KERBEROS_NAME_KW;
    extern const std::string CFG_KERBEROS_KEYTAB_KW;
    extern const std::string CFG_PAM_PASSWORD_LENGTH_KW;
    extern const std::string CFG_PAM_NO_EXTEND_KW;
    extern const std::string CFG_PAM_PASSWORD_MIN_TIME_KW;
    extern const std::string CFG_PAM_PASSWORD_MAX_TIME_KW;

    extern const std::string CFG_DB_USERNAME_KW;
    extern const std::string CFG_DB_PASSWORD_KW;
    extern const std::string CFG_DB_SSLMODE_KW;
    extern const std::string CFG_DB_SSLROOTCERT_KW;
    extern const std::string CFG_DB_SSLCERT_KW;
    extern const std::string CFG_DB_SSLKEY_KW;
    extern const std::string CFG_ZONE_NAME_KW;
    extern const std::string CFG_ZONE_KEY_KW;
    extern const std::string CFG_NEGOTIATION_KEY_KW;
    extern const std::string CFG_RE_RULEBASE_SET_KW;
    extern const std::string CFG_RE_NAMESPACE_SET_KW;
    extern const std::string CFG_NAMESPACE_KW;
    extern const std::string CFG_RE_FUNCTION_NAME_MAPPING_SET_KW;
    extern const std::string CFG_RE_DATA_VARIABLE_MAPPING_SET_KW;
    extern const std::string CFG_RE_PEP_REGEX_SET_KW;
    extern const std::string CFG_DEFAULT_DIR_MODE_KW;
    extern const std::string CFG_DEFAULT_FILE_MODE_KW;
    extern const std::string CFG_DEFAULT_HASH_SCHEME_KW;
    extern const std::string CFG_MATCH_HASH_POLICY_KW;
    extern const std::string CFG_FEDERATION_KW;
    extern const std::string CFG_ENVIRONMENT_VARIABLES_KW;
    extern const std::string CFG_ADVANCED_SETTINGS_KW;

    extern const std::string CFG_SERVER_PORT_RANGE_START_KW;
    extern const std::string CFG_SERVER_PORT_RANGE_END_KW;

    extern const std::string CFG_LOG_LEVEL_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_LEGACY_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_SERVER_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_AGENT_FACTORY_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_AGENT_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_RESOURCE_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_DATABASE_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_AUTHENTICATION_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_API_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_MICROSERVICE_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_NETWORK_KW;
    extern const std::string CFG_LOG_LEVEL_CATEGORY_RULE_ENGINE_KW;

    extern const std::string CFG_CLIENT_API_WHITELIST_POLICY_KW;

    // advanced settings
    extern const std::string CFG_MAX_SIZE_FOR_SINGLE_BUFFER;
    extern const std::string CFG_DEF_NUMBER_TRANSFER_THREADS;
    extern const std::string CFG_TRANS_CHUNK_SIZE_PARA_TRANS;
    extern const std::string CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS;
    extern const std::string CFG_DEF_TEMP_PASSWORD_LIFETIME;
    extern const std::string CFG_MAX_TEMP_PASSWORD_LIFETIME;
    extern const std::string CFG_MAX_NUMBER_OF_CONCURRENT_RE_PROCS;
    extern const std::string DEFAULT_LOG_ROTATION_IN_DAYS;

    extern const std::string CFG_RE_CACHE_SALT_KW;
    extern const std::string CFG_RE_SERVER_SLEEP_TIME;
    extern const std::string CFG_RE_SERVER_EXEC_TIME;

    extern const std::string CFG_DNS_CACHE_KW;
    extern const std::string CFG_HOSTNAME_CACHE_KW;

    extern const std::string CFG_SHARED_MEMORY_SIZE_IN_BYTES_KW;
    extern const std::string CFG_EVICTION_AGE_IN_SECONDS_KW;

    // service_account_environment.json keywords
    extern const std::string CFG_IRODS_USER_NAME_KW;
    extern const std::string CFG_IRODS_HOST_KW;
    extern const std::string CFG_IRODS_PORT_KW;
    extern const std::string CFG_IRODS_XMSG_HOST_KW;
    extern const std::string CFG_IRODS_XMSG_PORT_KW;
    extern const std::string CFG_IRODS_HOME_KW;
    extern const std::string CFG_IRODS_CWD_KW;
    extern const std::string CFG_IRODS_AUTHENTICATION_SCHEME_KW;
    extern const std::string CFG_IRODS_DEFAULT_RESOURCE_KW;
    extern const std::string CFG_IRODS_ZONE_KW;
    extern const std::string CFG_IRODS_GSI_SERVER_DN_KW;
    extern const std::string CFG_IRODS_LOG_LEVEL_KW;
    extern const std::string CFG_IRODS_AUTHENTICATION_FILE_KW;
    extern const std::string CFG_IRODS_DEBUG_KW;
    extern const std::string CFG_IRODS_CLIENT_SERVER_POLICY_KW;
    extern const std::string CFG_IRODS_CLIENT_SERVER_NEGOTIATION_KW;
    extern const std::string CFG_IRODS_ENCRYPTION_KEY_SIZE_KW;
    extern const std::string CFG_IRODS_ENCRYPTION_SALT_SIZE_KW;
    extern const std::string CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS_KW;
    extern const std::string CFG_IRODS_ENCRYPTION_ALGORITHM_KW;
    extern const std::string CFG_IRODS_DEFAULT_HASH_SCHEME_KW;
    extern const std::string CFG_IRODS_MATCH_HASH_POLICY_KW;

    extern const std::string CFG_IRODS_ENVIRONMENT_FILE_KW;
    extern const std::string CFG_IRODS_SESSION_ENVIRONMENT_FILE_KW;
    extern const std::string CFG_IRODS_SERVER_CONTROL_PLANE_PORT;

    extern const std::string CFG_IRODS_SERVER_CONTROL_PLANE_KEY;
    extern const std::string CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW;
    extern const std::string CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW;

    // irods environment advanced settings
    extern const std::string CFG_IRODS_MAX_SIZE_FOR_SINGLE_BUFFER;
    extern const std::string CFG_IRODS_DEF_NUMBER_TRANSFER_THREADS;
    extern const std::string CFG_IRODS_MAX_NUMBER_TRANSFER_THREADS;
    extern const std::string CFG_IRODS_TRANS_BUFFER_SIZE_FOR_PARA_TRANS;
    extern const std::string CFG_IRODS_CONNECTION_POOL_REFRESH_TIME;

    // legacy ssl environment variables
    extern const std::string CFG_IRODS_SSL_CA_CERTIFICATE_PATH;
    extern const std::string CFG_IRODS_SSL_CA_CERTIFICATE_FILE;
    extern const std::string CFG_IRODS_SSL_VERIFY_SERVER;
    extern const std::string CFG_IRODS_SSL_CERTIFICATE_CHAIN_FILE;
    extern const std::string CFG_IRODS_SSL_CERTIFICATE_KEY_FILE;
    extern const std::string CFG_IRODS_SSL_DH_PARAMS_FILE;

    // irods environment values now included in server_config
    extern const std::string CFG_ZONE_NAME;
    extern const std::string CFG_ZONE_USER;
    extern const std::string CFG_ZONE_PORT;
    extern const std::string CFG_ZONE_AUTH_SCHEME;

    // irods control plane values
    extern const std::string CFG_SERVER_CONTROL_PLANE_PORT;
    extern const std::string CFG_RULE_ENGINE_CONTROL_PLANE_PORT;
    extern const std::string CFG_SERVER_CONTROL_PLANE_TIMEOUT;
    extern const std::string CFG_SERVER_CONTROL_PLANE_KEY;
    extern const std::string CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS_KW;
    extern const std::string CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM_KW;

    extern const std::string CFG_CATALOG_PROVIDER_HOSTS_KW;
    extern const std::string CFG_CATALOG_SERVICE_ROLE;
    extern const std::string CFG_SERVICE_ROLE_PROVIDER;
    extern const std::string CFG_SERVICE_ROLE_CONSUMER;
    extern const std::string CFG_SERVICE_ROLE_PROXY;

    extern const std::string CFG_IRODS_PLUGINS_HOME_KW;

    extern const std::string CFG_PLUGIN_CONFIGURATION_KW;

    // plugin types
    extern const std::string PLUGIN_TYPE_API;
    extern const std::string PLUGIN_TYPE_RULE_ENGINE;
    extern const std::string PLUGIN_TYPE_AUTHENTICATION;
    extern const std::string PLUGIN_TYPE_NETWORK;
    extern const std::string PLUGIN_TYPE_DATABASE;
    extern const std::string PLUGIN_TYPE_RESOURCE;
    extern const std::string PLUGIN_TYPE_MICROSERVICE;

    extern const std::string CFG_PLUGIN_SPECIFIC_CONFIGURATION_KW;
    extern const std::string CFG_INSTANCE_NAME_KW;
    extern const std::string CFG_PLUGIN_NAME_KW;

    extern const std::string CFG_SHARED_MEMORY_INSTANCE_KW;
    extern const std::string CFG_SHARED_MEMORY_MUTEX_KW;

    extern const std::string DEFAULT_RULE_ENGINE_PLUGIN_NAME_KW;
    extern const std::string DEFAULT_RULE_ENGINE_INSTANCE_NAME_KW;

    // misc. keywords
    extern const std::string HOSTS_CONFIG_JSON_OBJECT_KW;
} // namespace irods

#endif // CONFIGURATION_KEYWORDS_HPP
