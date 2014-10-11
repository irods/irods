

#ifndef CONFIGURATION_KEYWORODS_HPP
#define CONFIGURATION_KEYWORODS_HPP

#include <string>

namespace irods {
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
    const std::string CFG_RE_FUNCTION_NAME_MAPPING_SET_KW( "re_function_name_mapping_set" );
    const std::string CFG_RE_DATA_VARIABLE_MAPPING_SET_KW( "re_data_variable_mapping_set" );
    const std::string CFG_RUN_SERVER_AS_ROOT_KW( "run_server_as_root" );
    const std::string CFG_DEFAULT_DIR_MODE_KW( "default_dir_mode" );
    const std::string CFG_DEFAULT_FILE_MODE_KW( "default_file_mode" );
    const std::string CFG_DEFAULT_HASH_SCHEME_KW( "default_hash_scheme" );
    const std::string CFG_MATCH_HASH_POLICY_KW( "match_hash_policy" );
    const std::string CFG_FEDERATION_KW( "federation" );

}; // namespace irods

#endif // CONFIGURATION_KEYWORODS_HPP



