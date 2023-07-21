#ifndef IRODS_CONFIGURATION_KEYWORDS_HPP
#define IRODS_CONFIGURATION_KEYWORDS_HPP

namespace irods
{
    // server_config.json keywords
    extern const char* const KW_CFG_KERBEROS_NAME;
    extern const char* const KW_CFG_KERBEROS_KEYTAB;
    extern const char* const KW_CFG_PAM_PASSWORD_LENGTH;
    extern const char* const KW_CFG_PAM_NO_EXTEND;
    extern const char* const KW_CFG_PAM_PASSWORD_MIN_TIME;
    extern const char* const KW_CFG_PAM_PASSWORD_MAX_TIME;

    extern const char* const KW_CFG_DB_HOST;
    extern const char* const KW_CFG_DB_PORT;
    extern const char* const KW_CFG_DB_NAME;
    extern const char* const KW_CFG_DB_ODBC_DRIVER;
    extern const char* const KW_CFG_DB_USERNAME;
    extern const char* const KW_CFG_DB_PASSWORD;
    extern const char* const KW_CFG_DB_SSLMODE;
    extern const char* const KW_CFG_DB_SSLROOTCERT;
    extern const char* const KW_CFG_DB_SSLCERT;
    extern const char* const KW_CFG_DB_SSLKEY;
    extern const char* const KW_CFG_ZONE_NAME;
    extern const char* const KW_CFG_ZONE_KEY;
    extern const char* const KW_CFG_NEGOTIATION_KEY;
    extern const char* const KW_CFG_RE_RULEBASE_SET;
    extern const char* const KW_CFG_RE_NAMESPACE_SET;
    extern const char* const KW_CFG_NAMESPACE;
    extern const char* const KW_CFG_RE_FUNCTION_NAME_MAPPING_SET;
    extern const char* const KW_CFG_RE_DATA_VARIABLE_MAPPING_SET;
    extern const char* const KW_CFG_RE_PEP_REGEX_SET;
    extern const char* const KW_CFG_DEFAULT_DIR_MODE;
    extern const char* const KW_CFG_DEFAULT_FILE_MODE;
    extern const char* const KW_CFG_DEFAULT_HASH_SCHEME;
    extern const char* const KW_CFG_MATCH_HASH_POLICY;
    extern const char* const KW_CFG_FEDERATION;
    extern const char* const KW_CFG_ENVIRONMENT_VARIABLES;
    extern const char* const KW_CFG_ADVANCED_SETTINGS;

    extern const char* const KW_CFG_SERVER_PORT_RANGE_START;
    extern const char* const KW_CFG_SERVER_PORT_RANGE_END;

    // log_level
    extern const char* const KW_CFG_LOG_LEVEL;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_LEGACY;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_SERVER;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_AGENT_FACTORY;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_AGENT;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_DELAY_SERVER;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_RESOURCE;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_DATABASE;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_AUTHENTICATION;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_API;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_MICROSERVICE;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_NETWORK;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_RULE_ENGINE;
    extern const char* const KW_CFG_LOG_LEVEL_CATEGORY_SQL;

    // client_allow_list_policy
    extern const char* const KW_CFG_CLIENT_API_ALLOWLIST_POLICY;

    // host_access_control
    extern const char* const KW_CFG_HOST_ACCESS_CONTROL;
    extern const char* const KW_CFG_ACCESS_ENTRIES;
    extern const char* const KW_CFG_USER;
    extern const char* const KW_CFG_GROUP;
    extern const char* const KW_CFG_MASK;

    // host_resolution
    extern const char* const KW_CFG_HOST_RESOLUTION;
    extern const char* const KW_CFG_HOST_ENTRIES;
    extern const char* const KW_CFG_ADDRESS_TYPE;
    extern const char* const KW_CFG_ADDRESSES;
    extern const char* const KW_CFG_ADDRESS;

    // advanced settings
    extern const char* const KW_CFG_DELAY_RULE_EXECUTORS;
    extern const char* const KW_CFG_MAX_SIZE_FOR_SINGLE_BUFFER;
    extern const char* const KW_CFG_DEF_NUMBER_TRANSFER_THREADS;
    extern const char* const KW_CFG_TRANS_CHUNK_SIZE_PARA_TRANS;
    extern const char* const KW_CFG_TRANS_BUFFER_SIZE_FOR_PARA_TRANS;
    extern const char* const KW_CFG_DEF_TEMP_PASSWORD_LIFETIME;
    extern const char* const KW_CFG_MAX_TEMP_PASSWORD_LIFETIME;
    extern const char* const KW_CFG_NUMBER_OF_CONCURRENT_DELAY_RULE_EXECUTORS;
    extern const char* const KW_CFG_MAX_SIZE_OF_DELAY_QUEUE_IN_BYTES;
    extern const char* const KW_CFG_STACKTRACE_FILE_PROCESSOR_SLEEP_TIME_IN_SECONDS;
    extern const char* const KW_CFG_AGENT_FACTORY_WATCHER_SLEEP_TIME_IN_SECONDS;
    extern const char* const KW_CFG_MIGRATE_DELAY_SERVER_SLEEP_TIME_IN_SECONDS;

    extern const char* const KW_CFG_RE_CACHE_SALT;
    extern const char* const KW_CFG_DELAY_SERVER_SLEEP_TIME_IN_SECONDS;

    extern const char* const KW_CFG_DNS_CACHE;
    extern const char* const KW_CFG_HOSTNAME_CACHE;

    extern const char* const KW_CFG_SHARED_MEMORY_SIZE_IN_BYTES;
    extern const char* const KW_CFG_EVICTION_AGE_IN_SECONDS;
    extern const char* const KW_CFG_CACHE_CLEARER_SLEEP_TIME_IN_SECONDS;

    extern const char* const KW_CFG_IRODS_TCP_KEEPALIVE_PROBES;
    extern const char* const KW_CFG_IRODS_TCP_KEEPALIVE_TIME_IN_SECONDS;
    extern const char* const KW_CFG_IRODS_TCP_KEEPALIVE_INTVL_IN_SECONDS;

    // service_account_environment.json keywords
    extern const char* const KW_CFG_IRODS_USER_NAME;
    extern const char* const KW_CFG_IRODS_HOST;
    extern const char* const KW_CFG_IRODS_PORT;
    extern const char* const KW_CFG_IRODS_HOME;
    extern const char* const KW_CFG_IRODS_CWD;
    extern const char* const KW_CFG_IRODS_AUTHENTICATION_SCHEME;
    extern const char* const KW_CFG_IRODS_DEFAULT_RESOURCE;
    extern const char* const KW_CFG_IRODS_ZONE;
    extern const char* const KW_CFG_IRODS_GSI_SERVER_DN;
    extern const char* const KW_CFG_IRODS_LOG_LEVEL;
    extern const char* const KW_CFG_IRODS_AUTHENTICATION_FILE;
    extern const char* const KW_CFG_IRODS_DEBUG;
    extern const char* const KW_CFG_IRODS_CLIENT_SERVER_POLICY;
    extern const char* const KW_CFG_IRODS_CLIENT_SERVER_NEGOTIATION;
    extern const char* const KW_CFG_IRODS_ENCRYPTION_KEY_SIZE;
    extern const char* const KW_CFG_IRODS_ENCRYPTION_SALT_SIZE;
    extern const char* const KW_CFG_IRODS_ENCRYPTION_NUM_HASH_ROUNDS;
    extern const char* const KW_CFG_IRODS_ENCRYPTION_ALGORITHM;
    extern const char* const KW_CFG_IRODS_DEFAULT_HASH_SCHEME;
    extern const char* const KW_CFG_IRODS_MATCH_HASH_POLICY;

    extern const char* const KW_CFG_IRODS_ENVIRONMENT_FILE;
    extern const char* const KW_CFG_IRODS_SESSION_ENVIRONMENT_FILE;
    extern const char* const KW_CFG_IRODS_SERVER_CONTROL_PLANE_PORT;

    extern const char* const KW_CFG_IRODS_SERVER_CONTROL_PLANE_KEY;
    extern const char* const KW_CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS;
    extern const char* const KW_CFG_IRODS_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM;

    // irods environment advanced settings
    extern const char* const KW_CFG_IRODS_MAX_SIZE_FOR_SINGLE_BUFFER;
    extern const char* const KW_CFG_IRODS_DEF_NUMBER_TRANSFER_THREADS;
    extern const char* const KW_CFG_IRODS_MAX_NUMBER_TRANSFER_THREADS;
    extern const char* const KW_CFG_IRODS_TRANS_BUFFER_SIZE_FOR_PARA_TRANS;
    extern const char* const KW_CFG_IRODS_CONNECTION_POOL_REFRESH_TIME;

    // legacy ssl environment variables
    extern const char* const KW_CFG_IRODS_SSL_CA_CERTIFICATE_PATH;
    extern const char* const KW_CFG_IRODS_SSL_CA_CERTIFICATE_FILE;
    extern const char* const KW_CFG_IRODS_SSL_VERIFY_SERVER;
    extern const char* const KW_CFG_IRODS_SSL_CERTIFICATE_CHAIN_FILE;
    extern const char* const KW_CFG_IRODS_SSL_CERTIFICATE_KEY_FILE;
    extern const char* const KW_CFG_IRODS_SSL_DH_PARAMS_FILE;

    // irods environment values now included in server_config
    extern const char* const KW_CFG_ZONE_USER;
    extern const char* const KW_CFG_ZONE_PORT;
    extern const char* const KW_CFG_ZONE_AUTH_SCHEME;

    // irods control plane values
    extern const char* const KW_CFG_SERVER_CONTROL_PLANE_PORT;
    extern const char* const KW_CFG_RULE_ENGINE_CONTROL_PLANE_PORT;
    extern const char* const KW_CFG_SERVER_CONTROL_PLANE_TIMEOUT;
    extern const char* const KW_CFG_SERVER_CONTROL_PLANE_KEY;
    extern const char* const KW_CFG_SERVER_CONTROL_PLANE_ENCRYPTION_NUM_HASH_ROUNDS;
    extern const char* const KW_CFG_SERVER_CONTROL_PLANE_ENCRYPTION_ALGORITHM;

    extern const char* const KW_CFG_CATALOG_PROVIDER_HOSTS;
    extern const char* const KW_CFG_CATALOG_SERVICE_ROLE;
    extern const char* const KW_CFG_SERVICE_ROLE_PROVIDER;
    extern const char* const KW_CFG_SERVICE_ROLE_CONSUMER;
    extern const char* const KW_CFG_SERVICE_ROLE_PROXY;

    extern const char* const KW_CFG_IRODS_PLUGINS_HOME;

    extern const char* const KW_CFG_PLUGIN_CONFIGURATION;

    // plugin types
    extern const char* const KW_CFG_PLUGIN_TYPE_API;
    extern const char* const KW_CFG_PLUGIN_TYPE_RULE_ENGINE;
    extern const char* const KW_CFG_PLUGIN_TYPE_AUTHENTICATION;
    extern const char* const KW_CFG_PLUGIN_TYPE_NETWORK;
    extern const char* const KW_CFG_PLUGIN_TYPE_DATABASE;
    extern const char* const KW_CFG_PLUGIN_TYPE_RESOURCE;
    extern const char* const KW_CFG_PLUGIN_TYPE_MICROSERVICE;

    extern const char* const KW_CFG_PLUGIN_SPECIFIC_CONFIGURATION;
    extern const char* const KW_CFG_INSTANCE_NAME;
    extern const char* const KW_CFG_PLUGIN_NAME;

    extern const char* const KW_CFG_SHARED_MEMORY_INSTANCE;
    extern const char* const KW_CFG_SHARED_MEMORY_MUTEX;

    extern const char* const KW_CFG_DEFAULT_RULE_ENGINE_PLUGIN_NAME;
    extern const char* const KW_CFG_DEFAULT_RULE_ENGINE_INSTANCE_NAME;
} // namespace irods

#endif // IRODS_CONFIGURATION_KEYWORDS_HPP
