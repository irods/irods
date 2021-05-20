#ifndef IRODS_DATABASE_CONSTANTS_HPP
#define IRODS_DATABASE_CONSTANTS_HPP

#include <string>

namespace irods
{
    const std::string DATABASE_OP_START( "database_start" );
    const std::string DATABASE_OP_STOP( "database_stop" );

    const std::string DATABASE_OP_DEBUG( "database_debug" );
    const std::string DATABASE_OP_OPEN( "database_open" );
    const std::string DATABASE_OP_CLOSE( "database_close" );
    const std::string DATABASE_OP_GET_LOCAL_ZONE( "database_get_local_zone" );
    const std::string DATABASE_OP_UPDATE_RESC_OBJ_COUNT( "database_update_resc_obj_count" );
    const std::string DATABASE_OP_MOD_DATA_OBJ_META( "database_mod_data_obj_meta" );
    const std::string DATABASE_OP_REG_DATA_OBJ( "database_reg_data_obj" );
    const std::string DATABASE_OP_REG_REPLICA( "database_reg_replica" );
    const std::string DATABASE_OP_UNREG_REPLICA( "database_unreg_replica" );
    const std::string DATABASE_OP_REG_RULE_EXEC( "database_reg_rule_exec" );
    const std::string DATABASE_OP_MOD_RULE_EXEC( "database_mod_rule_exec" );
    const std::string DATABASE_OP_DEL_RULE_EXEC( "database_del_rule_exec" );
    const std::string DATABASE_OP_RESC_OBJ_COUNT( "database_resc_obj_count" );
    const std::string DATABASE_OP_ADD_CHILD_RESC( "database_add_child_resc" );
    const std::string DATABASE_OP_REG_RESC( "database_reg_resc" );
    const std::string DATABASE_OP_DEL_CHILD_RESC( "database_del_child_resc" );
    const std::string DATABASE_OP_DEL_RESC( "database_del_resc" );
    const std::string DATABASE_OP_ROLLBACK( "database_rollback" );
    const std::string DATABASE_OP_COMMIT( "database_commit" );
    const std::string DATABASE_OP_DEL_USER_RE( "database_del_user_re" );
    const std::string DATABASE_OP_REG_COLL_BY_ADMIN( "database_reg_coll_by_admin" );
    const std::string DATABASE_OP_REG_COLL( "database_reg_coll" );
    const std::string DATABASE_OP_MOD_COLL( "database_mod_coll" );
    [[deprecated("SimpleQuery is deprecated. Use GenQuery or SpecificQuery instead.")]]
    const std::string DATABASE_OP_SIMPLE_QUERY( "database_simple_query" );
    const std::string DATABASE_OP_GEN_QUERY( "database_gen_query" );
    const std::string DATABASE_OP_GEN_QUERY_ACCESS_CONTROL_SETUP( "database_gen_query_access_control_setup" );
    const std::string DATABASE_OP_GEN_QUERY_TICKET_SETUP( "database_gen_query_ticket_setup" );
    const std::string DATABASE_OP_SPECIFIC_QUERY( "database_specific_query" );
    const std::string DATABASE_OP_GENERAL_UPDATE( "database_general_update" );
    const std::string DATABASE_OP_DEL_COLL_BY_ADMIN( "database_del_coll_by_admin" );
    const std::string DATABASE_OP_DEL_COLL( "database_del_coll" );
    const std::string DATABASE_OP_CHECK_AUTH( "database_check_auth" );
    const std::string DATABASE_OP_MAKE_TEMP_PW( "database_make_tmp_pw" );
    const std::string DATABASE_OP_MAKE_LIMITED_PW( "database_make_limited_pw" );
    const std::string DATABASE_OP_MOD_USER( "database_mod_user" );
    const std::string DATABASE_OP_MOD_GROUP( "database_mod_group" );
    const std::string DATABASE_OP_MOD_RESC( "database_mod_resc" );
    const std::string DATABASE_OP_MOD_RESC_DATA_PATHS( "database_mod_resc_data_paths" );
    const std::string DATABASE_OP_MOD_RESC_FREESPACE( "database_mod_resc_freespace" );
    const std::string DATABASE_OP_REG_USER_RE( "database_reg_user_re" );
    const std::string DATABASE_OP_ADD_AVU_METADATA( "database_add_avu_metadata" );
    const std::string DATABASE_OP_ADD_AVU_METADATA_WILD( "database_add_avu_metadata_wild" );
    const std::string DATABASE_OP_DEL_AVU_METADATA( "database_del_avu_metadata" );
    const std::string DATABASE_OP_SET_AVU_METADATA( "database_set_avu_metadata" );
    const std::string DATABASE_OP_COPY_AVU_METADATA( "database_copy_avu_metadata" );
    const std::string DATABASE_OP_MOD_AVU_METADATA( "database_mod_avu_metadata" );
    const std::string DATABASE_OP_MOD_ACCESS_CONTROL( "database_mod_access_control" );
    const std::string DATABASE_OP_MOD_ACCESS_CONTROL_RESC( "database_mod_access_control_resc" );

    const std::string DATABASE_OP_RENAME_OBJECT( "database_rename_object" );
    const std::string DATABASE_OP_MOVE_OBJECT( "database_move_object" );

    const std::string DATABASE_OP_REG_TOKEN( "database_reg_token" );
    const std::string DATABASE_OP_DEL_TOKEN( "database_del_token" );

    const std::string DATABASE_OP_REG_ZONE( "database_reg_zone" );
    const std::string DATABASE_OP_MOD_ZONE( "database_mod_zone" );
    const std::string DATABASE_OP_MOD_ZONE_COLL_ACL( "database_mod_zone_coll_acl" );
    const std::string DATABASE_OP_DEL_ZONE( "database_del_zone" );
    const std::string DATABASE_OP_RENAME_LOCAL_ZONE( "database_rename_local_zone" );

    const std::string DATABASE_OP_RENAME_COLL( "database_rename_coll" );

    const std::string DATABASE_OP_REG_SERVER_LOAD( "database_reg_server_load" );
    const std::string DATABASE_OP_REG_SERVER_LOAD_DIGEST( "database_reg_server_load_digest" );
    const std::string DATABASE_OP_PURGE_SERVER_LOAD( "database_purge_server_load" );
    const std::string DATABASE_OP_PURGE_SERVER_LOAD_DIGEST( "database_purge_server_load_digest" );

    const std::string DATABASE_OP_CALC_USAGE_AND_QUOTA( "database_calc_usage_and_quota" );
    const std::string DATABASE_OP_SET_QUOTA( "database_set_quota" );
    const std::string DATABASE_OP_CHECK_QUOTA( "database_check_quota" );

    const std::string DATABASE_OP_GET_GRID_CONFIGURATION_VALUE( "database_get_grid_configuration_value" );
    const std::string DATABASE_OP_SET_GRID_CONFIGURATION_VALUE( "database_set_grid_configuration_value" );

    const std::string DATABASE_OP_DEL_UNUSED_AVUS( "database_del_unused_avus" );

    const std::string DATABASE_OP_ADD_SPECIFIC_QUERY( "database_add_specific_query" );
    const std::string DATABASE_OP_DEL_SPECIFIC_QUERY( "database_del_specific_query" );

    const std::string DATABASE_OP_DEBUG_QUERY( "database_debug_query" );
    const std::string DATABASE_OP_DEBUG_GEN_UPDATE( "database_debug_gen_update" );

    const std::string DATABASE_OP_IS_RULE_IN_TABLE( "database_is_rule_in_table" );
    const std::string DATABASE_OP_VERSION_RULE_BASE( "database_version_rule_base" );
    const std::string DATABASE_OP_VERSION_DVM_BASE( "database_version_dvm_base" );
    const std::string DATABASE_OP_INS_RULE_TABLE( "database_ins_rule_table" );
    const std::string DATABASE_OP_INS_DVM_TABLE( "database_ins_dvm_table" );
    const std::string DATABASE_OP_INS_FNM_TABLE( "database_ins_fnm_table" );
    const std::string DATABASE_OP_INS_MSRVC_TABLE( "database_ins_msrvc_table" );

    const std::string DATABASE_OP_VERSION_FNM_BASE( "database_version_fnm_base" );

    const std::string DATABASE_OP_MOD_TICKET( "database_mod_ticket" );
    const std::string DATABASE_OP_UPDATE_PAM_PASSWORD( "database_update_pam_password" );

    const std::string DATABASE_OP_GET_DISTINCT_DATA_OBJS_MISSING_FROM_CHILD_GIVEN_PARENT( "database_get_distinct_data_objs_missing_from_child_given_parent" );
    const std::string DATABASE_OP_GET_DISTINCT_DATA_OBJ_COUNT_ON_RESOURCE( "database_get_distinct_data_obj_count_on_resource" );
    const std::string DATABASE_OP_GET_HIERARCHY_FOR_RESC( "database_get_hierarchy_for_resc" );
    const std::string DATABASE_OP_CHECK_AND_GET_OBJ_ID( "database_check_and_get_obj_id" );
    const std::string DATABASE_OP_GET_RCS( "database_get_rcs" );
    const std::string DATABASE_OP_GET_REPL_LIST_FOR_LEAF_BUNDLES( "database_get_repl_list_for_leaf_bundles" );
    const std::string DATABASE_OP_CHECK_PERMISSION_TO_MODIFY_DATA_OBJECT{"database_check_permission_to_modify_data_object"};
    const std::string DATABASE_OP_UPDATE_TICKET_WRITE_BYTE_COUNT{"database_update_ticket_write_byte_count"};
    const std::string DATABASE_OP_GET_DELAY_RULE_INFO{"database_get_delay_rule_info"};
    const std::string DATABASE_OP_DATA_OBJECT_FINALIZE{"database_data_object_finalize"};
}; // namespace irods

#endif // IRODS_DATABASE_CONSTANTS_HPP
