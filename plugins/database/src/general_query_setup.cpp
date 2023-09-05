/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*

  This routine is called to set up the tables needed by
  chlGeneralQuery.

  A series of sTable calls are made to define the table names.  When
  the 2nd argument is two strings, it's an alias and will be of the
  form sTable("name1", "name2 name1", n) and name1 will be the alias
  for name2.  The From portion of the SQL with have "name2 name1" and
  name1 will be used for the table name elsewhere.  This provides a
  more meaningful name to the client and allows the Where clause to do
  multiple comparisons of fields in the same table in different ways.

  The last argument to sTable is 0 or 1 to indicate if the table is a
  'cycler'.  When 1, the the sql generation function will stop at this
  table to avoid cycles in the graph.  This is being used for the
  quota tables to avoid finding extraneous links.

  A series for sColumns calls are made to map the #define values to
  tables and columns.

  And a series of sFklink's are called to initialize the Foreign
  Key table specifying how tables are related, how foreign keys can be
  used to tie them together.  Arguments are: table1, table2, and sql
  text to link them together.  This means that table1 and table2 can
  be related together via the sql text string.  This creates a tree
  structure describing the ICAT schema.
*/
#include "irods/rodsClient.h"
#include "irods/icatHighLevelRoutines.hpp"
#include "irods/rodsGenQuery.h"

void
icatGeneralQuerySetup() {
    sTableInit();   /* initialize */

    sTable( "R_USER_PASSWORD",  "R_USER_PASSWORD", 0 );
    sTable( "R_USER_SESSION_KEY",  "R_USER_SESSION_KEY", 0 );
    sTable( "R_TOKN_MAIN",  "R_TOKN_MAIN", 0 );
    sTable( "R_RESC_GROUP",  "R_RESC_GROUP", 0 );
    sTable( "R_ZONE_MAIN",  "R_ZONE_MAIN", 0 );
    sTable( "R_RESC_MAIN",  "R_RESC_MAIN", 0 );
    sTable( "R_COLL_MAIN",  "R_COLL_MAIN", 0 );
    sTable( "R_DATA_MAIN",  "R_DATA_MAIN", 0 );

    sTable( "r_met2_main", "R_META_MAIN r_met2_main" , 0 );
    sTable( "R_META_MAIN", "R_META_MAIN" , 0 );

    sTable( "R_RULE_MAIN",  "R_RULE_MAIN", 0 );
    sTable( "R_USER_MAIN",  "R_USER_MAIN", 0 );
    sTable( "r_resc_access", "R_OBJT_ACCESS r_resc_access", 0 );
    sTable( "r_coll_access", "R_OBJT_ACCESS r_coll_access", 0 );
    sTable( "r_data_access", "R_OBJT_ACCESS r_data_access", 0 );
    sTable( "r_met2_access", "R_OBJT_ACCESS r_met2_access", 0 );
    sTable( "r_rule_access", "R_OBJT_ACCESS r_rule_access", 0 );
    sTable( "r_msrvc_access", "R_OBJT_ACCESS r_msrvc_access", 0 );
    sTable( "r_resc_audit", "R_OBJT_AUDIT r_resc_audit", 0 );
    sTable( "r_coll_audit", "R_OBJT_AUDIT r_coll_audit", 0 );
    sTable( "r_data_audit", "R_OBJT_AUDIT r_data_audit", 0 );
    sTable( "r_met2_audit", "R_OBJT_AUDIT r_met2_audit", 0 );
    sTable( "r_rule_audit", "R_OBJT_AUDIT r_rule_audit", 0 );
    sTable( "r_resc_deny_access", "R_OBJT_DENY_ACCESS r_resc_deny_access", 0 );
    sTable( "r_coll_deny_access", "R_OBJT_DENY_ACCESS r_coll_deny_access", 0 );
    sTable( "r_data_deny_access", "R_OBJT_DENY_ACCESS r_data_deny_access", 0 );
    sTable( "r_met2_deny_access", "R_OBJT_DENY_ACCESS r_met2_deny_access", 0 );
    sTable( "r_rule_deny_access", "R_OBJT_DENY_ACCESS r_rule_deny_access", 0 );
    sTable( "r_resc_metamap", "R_OBJT_METAMAP r_resc_metamap", 0 );
    sTable( "r_resc_grp_metamap", "R_OBJT_METAMAP r_resc_grp_metamap", 0 );
    sTable( "r_coll_metamap", "R_OBJT_METAMAP r_coll_metamap", 0 );
    sTable( "r_data_metamap", "R_OBJT_METAMAP r_data_metamap", 0 );
    sTable( "r_met2_metamap", "R_OBJT_METAMAP r_met2_metamap", 0 );
    sTable( "r_rule_metamap", "R_OBJT_METAMAP r_rule_metamap", 0 );
    sTable( "r_msrvc_metamap", "R_OBJT_METAMAP r_msrvc_metamap", 0 );
    sTable( "r_user_metamap", "R_OBJT_METAMAP r_user_metamap", 0 );
    sTable( "r_resc_user_group", "R_USER_GROUP r_resc_user_group", 0 );
    sTable( "r_coll_user_group", "R_USER_GROUP r_coll_user_group", 0 );
    sTable( "r_data_user_group", "R_USER_GROUP r_data_user_group", 0 );
    sTable( "r_met2_user_group", "R_USER_GROUP r_met2_user_group", 0 );
    sTable( "r_rule_user_group", "R_USER_GROUP r_rule_user_group", 0 );
    sTable( "r_resc_da_user_group", "R_USER_GROUP r_resc_da_user_group", 0 );
    sTable( "r_coll_da_user_group", "R_USER_GROUP r_coll_da_user_group", 0 );
    sTable( "r_data_da_user_group", "R_USER_GROUP r_data_da_user_group", 0 );
    sTable( "r_met2_da_user_group", "R_USER_GROUP r_met2_da_user_group", 0 );
    sTable( "r_rule_da_user_group", "R_USER_GROUP r_rule_da_user_group", 0 );

    sTable( "r_resc_au_user_group", "R_USER_GROUP r_resc_au_user_group", 0 );
    sTable( "r_coll_au_user_group", "R_USER_GROUP r_coll_au_user_group", 0 );
    sTable( "r_data_au_user_group", "R_USER_GROUP r_data_au_user_group", 0 );
    sTable( "r_met2_au_user_group", "R_USER_GROUP r_met2_au_user_group", 0 );
    sTable( "r_rule_au_user_group", "R_USER_GROUP r_rule_au_user_group", 0 );
    sTable( "r_resc_user_main", "R_USER_MAIN r_resc_user_main", 0 );
    sTable( "r_coll_user_main", "R_USER_MAIN r_coll_user_main", 0 );
    sTable( "r_data_user_main", "R_USER_MAIN r_data_user_main", 0 );
    sTable( "r_met2_user_main", "R_USER_MAIN r_met2_user_main", 0 );
    sTable( "r_rule_user_main", "R_USER_MAIN r_rule_user_main", 0 );
    sTable( "r_resc_da_user_main", "R_USER_MAIN r_resc_da_user_main", 0 );
    sTable( "r_coll_da_user_main", "R_USER_MAIN r_coll_da_user_main", 0 );
    sTable( "r_data_da_user_main", "R_USER_MAIN r_data_da_user_main", 0 );
    sTable( "r_met2_da_user_main", "R_USER_MAIN r_met2_da_user_main", 0 );
    sTable( "r_rule_da_user_main", "R_USER_MAIN r_rule_da_user_main", 0 );
    sTable( "r_resc_au_user_main", "R_USER_MAIN r_resc_au_user_main", 0 );
    sTable( "r_coll_au_user_main", "R_USER_MAIN r_coll_au_user_main", 0 );
    sTable( "r_data_au_user_main", "R_USER_MAIN r_data_au_user_main", 0 );
    sTable( "r_met2_au_user_main", "R_USER_MAIN r_met2_au_user_main", 0 );
    sTable( "r_rule_au_user_main", "R_USER_MAIN r_rule_au_user_main", 0 );

    sTable( "r_quota_user_main", "R_USER_MAIN r_quota_user_main", 1 );
    sTable( "r_quota_user_group", "R_USER_MAIN r_quota_user_group", 1 );
    sTable( "r_quota_resc_main", "R_RESC_MAIN r_quota_resc_main", 1 );

    sTable( "r_resc_tokn_accs", "R_TOKN_MAIN r_resc_tokn_accs", 0 );
    sTable( "r_coll_tokn_accs", "R_TOKN_MAIN r_coll_tokn_accs", 0 );
    sTable( "r_data_tokn_accs", "R_TOKN_MAIN r_data_tokn_accs", 0 );

    sTable( "r_rule_tokn_accs", "R_TOKN_MAIN r_rule_tokn_accs", 0 );
    sTable( "r_met2_tokn_accs", "R_TOKN_MAIN r_met2_tokn_accs", 0 );
    sTable( "r_resc_tokn_deny_accs", "R_TOKN_MAIN r_resc_tokn_deny_accs", 0 );
    sTable( "r_coll_tokn_deny_accs", "R_TOKN_MAIN r_coll_tokn_deny_accs", 0 );
    sTable( "r_data_tokn_deny_accs", "R_TOKN_MAIN r_data_tokn_deny_accs", 0 );
    sTable( "r_rule_tokn_deny_accs", "R_TOKN_MAIN r_rule_tokn_deny_accs", 0 );
    sTable( "r_met2_tokn_deny_accs", "R_TOKN_MAIN r_met2_tokn_deny_accs", 0 );
    sTable( "r_resc_tokn_audit", "R_TOKN_MAIN r_resc_tokn_audit", 0 );
    sTable( "r_coll_tokn_audit", "R_TOKN_MAIN r_coll_tokn_audit", 0 );
    sTable( "r_data_tokn_audit", "R_TOKN_MAIN r_data_tokn_audit", 0 );
    sTable( "r_rule_tokn_audit", "R_TOKN_MAIN r_rule_tokn_audit", 0 );
    sTable( "r_met2_tokn_audit", "R_TOKN_MAIN r_met2_tokn_audit", 0 );
    sTable( "r_resc_meta_main", "R_META_MAIN r_resc_meta_main", 0 );
    sTable( "r_resc_grp_meta_main", "R_META_MAIN r_resc_grp_meta_main", 0 );
    sTable( "r_coll_meta_main", "R_META_MAIN r_coll_meta_main", 0 );
    sTable( "r_data_meta_main", "R_META_MAIN r_data_meta_main", 0 );
    sTable( "r_rule_meta_main", "R_META_MAIN r_rule_meta_main", 0 );
    sTable( "r_user_meta_main", "R_META_MAIN r_user_meta_main", 0 );
    sTable( "r_met2_meta_main", "R_META_MAIN r_met2_meta_main", 0 );

    sTable( "R_USER_GROUP",  "R_USER_GROUP", 0 );
    sTable( "r_group_main", "R_USER_MAIN r_group_main", 0 );

    sTable( "R_RULE_EXEC", "R_RULE_EXEC", 0 );

    sTable( "R_OBJT_AUDIT", "R_OBJT_AUDIT", 0 );

    sTable( "R_SERVER_LOAD", "R_SERVER_LOAD", 0 );

    sTable( "R_SERVER_LOAD_DIGEST", "R_SERVER_LOAD_DIGEST", 0 );

    sTable( "R_USER_AUTH", "R_USER_AUTH", 0 );

    sTable( "R_RULE_BASE_MAP", "R_RULE_BASE_MAP", 0 );
    sTable( "R_RULE_DVM_MAP", "R_RULE_DVM_MAP", 0 );
    sTable( "R_RULE_FNM_MAP", "R_RULE_FNM_MAP", 0 );
    sTable( "R_RULE_DVM",  "R_RULE_DVM", 0 );
    sTable( "R_RULE_FNM",  "R_RULE_FNM", 0 );

    sTable( "R_QUOTA_MAIN", "R_QUOTA_MAIN", 0 );
    sTable( "R_QUOTA_USAGE", "R_QUOTA_USAGE", 0 );

    sTable( "R_MICROSRVC_MAIN", "R_MICROSRVC_MAIN", 0 );
    sTable( "R_MICROSRVC_VER", "R_MICROSRVC_VER", 0 );

    sTable( "r_msrvc_deny_access", "R_OBJT_DENY_ACCESS r_msrvc_deny_access", 0 );
    sTable( "r_msrvc_audit", "R_OBJT_AUDIT r_msrvc_audit", 0 );
    sTable( "r_msrvc_meta_main", "R_META_MAIN r_msrvc_meta_main", 0 );
    sTable( "r_msrvc_tokn_accs", "R_TOKN_MAIN r_msrvc_tokn_accs", 0 );
    sTable( "r_msrvc_user_group", "R_USER_GROUP r_msrvc_user_group", 0 );
    sTable( "r_msrvc_user_main", "R_USER_MAIN r_msrvc_user_main", 0 );
    sTable( "r_msrvc_tokn_deny_accs", "R_TOKN_MAIN r_msrvc_tokn_deny_accs", 0 );
    sTable( "r_msrvc_da_user_group", "R_USER_GROUP r_msrvc_da_user_group", 0 );
    sTable( "r_msrvc_da_user_main", "R_USER_MAIN r_msrvc_da_user_main", 0 );
    sTable( "r_msrvc_tokn_audit", "R_TOKN_MAIN r_msrvc_tokn_audit", 0 );
    sTable( "r_msrvc_au_user_group", "R_USER_GROUP r_msrvc_au_user_group", 0 );
    sTable( "r_msrvc_au_user_main", "R_USER_MAIN r_msrvc_au_user_main", 0 );

    sTable( "R_TICKET_MAIN", "R_TICKET_MAIN", 0 );
    sTable( "R_TICKET_ALLOWED_HOSTS", "R_TICKET_ALLOWED_HOSTS", 0 );
    sTable( "R_TICKET_ALLOWED_USERS", "R_TICKET_ALLOWED_USERS", 0 );
    sTable( "R_TICKET_ALLOWED_GROUPS", "R_TICKET_ALLOWED_GROUPS", 0 );
    sTable( "r_ticket_coll_main", "R_COLL_MAIN r_ticket_coll_main", 1 );
    sTable( "r_ticket_user_main", "R_USER_MAIN r_ticket_user_main", 1 );
    sTable( "r_ticket_data_coll_main", "R_COLL_MAIN r_ticket_data_coll_main", 1 );

    /* Map the #define values to tables and columns */

    sColumn( COL_ZONE_ID, "R_ZONE_MAIN", "zone_id" );
    sColumn( COL_ZONE_NAME, "R_ZONE_MAIN", "zone_name" );
    sColumn( COL_ZONE_TYPE, "R_ZONE_MAIN", "zone_type_name" );
    sColumn( COL_ZONE_CONNECTION, "R_ZONE_MAIN", "zone_conn_string" );
    sColumn( COL_ZONE_COMMENT, "R_ZONE_MAIN", "r_comment" );
    sColumn( COL_ZONE_CREATE_TIME, "R_ZONE_MAIN", "create_ts" );
    sColumn( COL_ZONE_MODIFY_TIME, "R_ZONE_MAIN", "modify_ts" );

    sColumn( COL_USER_ID,   "R_USER_MAIN", "user_id" );
    sColumn( COL_USER_NAME, "R_USER_MAIN", "user_name" );
    sColumn( COL_USER_TYPE, "R_USER_MAIN", "user_type_name" );
    sColumn( COL_USER_ZONE, "R_USER_MAIN", "zone_name" );
    sColumn( COL_USER_INFO, "R_USER_MAIN", "user_info" );
    sColumn( COL_USER_COMMENT,     "R_USER_MAIN", "r_comment" );
    sColumn( COL_USER_CREATE_TIME, "R_USER_MAIN", "create_ts" );
    sColumn( COL_USER_MODIFY_TIME, "R_USER_MAIN", "modify_ts" );

    sColumn( COL_USER_AUTH_ID, "R_USER_AUTH", "user_id" );
    sColumn( COL_USER_DN,   "R_USER_AUTH", "user_auth_name" );

    /* The following is for backward compatibility for clients, such as
       the PHP client, that might be asking for the previous COL_USER_DN
       (column id 205).  In iRODS 2.2+ there can be multiple DNs so the
       client will need to change to handle those.  But to avoid an error
       (although returning invalid info), we just map the old 205 request to
       the comment field.
    */
    sColumn( COL_USER_DN_INVALID, "R_USER_MAIN", "r_comment" ); /* compatibility */


    sColumn( COL_R_RESC_ID, "R_RESC_MAIN", "resc_id" );
    sColumn( COL_R_RESC_NAME, "R_RESC_MAIN", "resc_name" );
    sColumn( COL_R_ZONE_NAME, "R_RESC_MAIN", "zone_name" );
    sColumn( COL_R_TYPE_NAME, "R_RESC_MAIN", "resc_type_name" );
    sColumn( COL_R_CLASS_NAME, "R_RESC_MAIN", "resc_class_name" );
    sColumn( COL_R_LOC, "R_RESC_MAIN", "resc_net" );
    sColumn( COL_R_VAULT_PATH, "R_RESC_MAIN", "resc_def_path " );
    sColumn( COL_R_FREE_SPACE, "R_RESC_MAIN", "free_space" );
    sColumn( COL_R_FREE_SPACE_TIME, "R_RESC_MAIN", "free_space_ts" );
    sColumn( COL_R_RESC_INFO, "R_RESC_MAIN", "resc_info" );
    sColumn( COL_R_RESC_COMMENT, "R_RESC_MAIN", "r_comment" );
    sColumn( COL_R_RESC_STATUS, "R_RESC_MAIN", "resc_status" );
    sColumn( COL_R_CREATE_TIME, "R_RESC_MAIN", "create_ts" );
    sColumn( COL_R_MODIFY_TIME, "R_RESC_MAIN", "modify_ts " );
    sColumn(COL_R_MODIFY_TIME_MILLIS, "R_RESC_MAIN", "modify_ts_millis");
    sColumn( COL_R_RESC_CHILDREN, "R_RESC_MAIN", "resc_children " );
    sColumn( COL_R_RESC_CONTEXT, "R_RESC_MAIN", "resc_context " );
    sColumn( COL_R_RESC_PARENT,  "R_RESC_MAIN", "resc_parent " );
    sColumn( COL_R_RESC_PARENT_CONTEXT,  "R_RESC_MAIN", "resc_parent_context" );
    sColumn( COL_D_DATA_ID, "R_DATA_MAIN", "data_id" );
    sColumn( COL_D_COLL_ID, "R_DATA_MAIN", "coll_id" );
    sColumn( COL_DATA_NAME, "R_DATA_MAIN", "data_name" );
    sColumn( COL_DATA_REPL_NUM, "R_DATA_MAIN", "data_repl_num" );
    sColumn( COL_DATA_VERSION, "R_DATA_MAIN", "data_version" );
    sColumn( COL_DATA_TYPE_NAME, "R_DATA_MAIN", "data_type_name" );
    sColumn( COL_DATA_SIZE, "R_DATA_MAIN", "data_size" );
    sColumn( COL_D_RESC_NAME, "R_RESC_MAIN", "resc_name" );
    sColumn( COL_D_DATA_PATH, "R_DATA_MAIN", "data_path" );
    sColumn( COL_D_OWNER_NAME, "R_DATA_MAIN", "data_owner_name" );
    sColumn( COL_D_OWNER_ZONE, "R_DATA_MAIN", "data_owner_zone" );
    sColumn( COL_D_REPL_STATUS, "R_DATA_MAIN", "data_is_dirty" );
    sColumn( COL_D_DATA_STATUS, "R_DATA_MAIN", "data_status" );
    sColumn( COL_D_DATA_CHECKSUM, "R_DATA_MAIN", "data_checksum" );
    sColumn( COL_D_EXPIRY, "R_DATA_MAIN", "data_expiry_ts" );
    sColumn( COL_D_MAP_ID, "R_DATA_MAIN", "data_map_id" );
    sColumn( COL_D_COMMENTS, "R_DATA_MAIN", "r_comment" );
    sColumn( COL_D_CREATE_TIME, "R_DATA_MAIN", "create_ts" );
    sColumn( COL_D_MODIFY_TIME, "R_DATA_MAIN", "modify_ts" );
    sColumn( COL_DATA_MODE, "R_DATA_MAIN", "data_mode" );

    sColumn( COL_D_RESC_ID, "R_DATA_MAIN", "resc_id" );

    sColumn( COL_DATA_ACCESS_TYPE, "r_data_access", "access_type_id" );
    sColumn( COL_DATA_ACCESS_NAME, "r_data_tokn_accs", "token_name" );
    sColumn( COL_DATA_TOKEN_NAMESPACE, "r_data_tokn_accs", "token_namespace" );
    sColumn( COL_DATA_ACCESS_USER_ID, "r_data_access", "user_id" );
    sColumn( COL_DATA_ACCESS_DATA_ID, "r_data_access", "object_id" );

    sColumn( COL_COLL_ACCESS_TYPE, "r_coll_access", "access_type_id" );
    sColumn( COL_COLL_ACCESS_NAME, "r_coll_tokn_accs", "token_name" );
    sColumn( COL_COLL_TOKEN_NAMESPACE, "r_coll_tokn_accs", "token_namespace" );
    sColumn( COL_COLL_ACCESS_USER_ID, "r_coll_access", "user_id" );
    sColumn( COL_COLL_ACCESS_COLL_ID, "r_coll_access", "object_id" );

    sColumn( COL_RESC_ACCESS_TYPE, "r_resc_access", "access_type_id" );
    sColumn( COL_RESC_ACCESS_NAME, "r_resc_tokn_accs", "token_name" );
    sColumn( COL_RESC_TOKEN_NAMESPACE, "r_resc_tokn_accs", "token_namespace" );
    sColumn( COL_RESC_ACCESS_USER_ID, "r_resc_access", "user_id" );
    sColumn( COL_RESC_ACCESS_RESC_ID, "r_resc_access", "object_id" );

    sColumn( COL_COLL_ID, "R_COLL_MAIN", "coll_id" );
    sColumn( COL_COLL_NAME, "R_COLL_MAIN", "coll_name" );
    sColumn( COL_COLL_PARENT_NAME, "R_COLL_MAIN", "parent_coll_name" );
    sColumn( COL_COLL_OWNER_NAME, "R_COLL_MAIN", "coll_owner_name" );
    sColumn( COL_COLL_OWNER_ZONE, "R_COLL_MAIN", "coll_owner_zone" );
    sColumn( COL_COLL_MAP_ID, "R_COLL_MAIN", "coll_map_id" );
    sColumn( COL_COLL_INHERITANCE, "R_COLL_MAIN", "coll_inheritance" );
    sColumn( COL_COLL_COMMENTS, "R_COLL_MAIN", "r_comment" );
    sColumn( COL_COLL_CREATE_TIME, "R_COLL_MAIN", "create_ts" );
    sColumn( COL_COLL_MODIFY_TIME, "R_COLL_MAIN", "modify_ts" );
    sColumn( COL_COLL_TYPE, "R_COLL_MAIN", "coll_type" );
    sColumn( COL_COLL_INFO1, "R_COLL_MAIN", "coll_info1" );
    sColumn( COL_COLL_INFO2, "R_COLL_MAIN", "coll_info2" );

    sColumn( COL_META_DATA_ATTR_NAME, "r_data_meta_main", "meta_attr_name" );
    sColumn( COL_META_DATA_ATTR_VALUE, "r_data_meta_main", "meta_attr_value" );
    sColumn( COL_META_DATA_ATTR_UNITS, "r_data_meta_main", "meta_attr_unit" );
    sColumn( COL_META_DATA_ATTR_ID,   "r_data_meta_main", "meta_id" );
    sColumn( COL_META_DATA_CREATE_TIME, "r_data_meta_main", "create_ts" );
    sColumn( COL_META_DATA_MODIFY_TIME, "r_data_meta_main", "modify_ts" );

    sColumn( COL_META_COLL_ATTR_NAME, "r_coll_meta_main", "meta_attr_name" );
    sColumn( COL_META_COLL_ATTR_VALUE, "r_coll_meta_main", "meta_attr_value" );
    sColumn( COL_META_COLL_ATTR_UNITS, "r_coll_meta_main", "meta_attr_unit" );
    sColumn( COL_META_COLL_ATTR_ID,   "r_coll_meta_main", "meta_id" );
    sColumn( COL_META_COLL_CREATE_TIME, "r_coll_meta_main", "create_ts" );
    sColumn( COL_META_COLL_MODIFY_TIME, "r_coll_meta_main", "modify_ts" );

    sColumn( COL_META_NAMESPACE_COLL, "r_coll_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_DATA, "r_data_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_RESC, "r_resc_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_RESC_GROUP, "r_resc_grp_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_USER, "r_user_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_RULE, "r_rule_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_MSRVC, "r_msrvc_meta_main", "meta_namespace" );
    sColumn( COL_META_NAMESPACE_MET2, "r_met2_meta_main", "meta_namespace" );


    sColumn( COL_META_RESC_ATTR_NAME, "r_resc_meta_main", "meta_attr_name" );
    sColumn( COL_META_RESC_ATTR_VALUE, "r_resc_meta_main", "meta_attr_value" );
    sColumn( COL_META_RESC_ATTR_UNITS, "r_resc_meta_main", "meta_attr_unit" );
    sColumn( COL_META_RESC_ATTR_ID,   "r_resc_meta_main", "meta_id" );
    sColumn( COL_META_RESC_CREATE_TIME, "r_resc_meta_main", "create_ts" );
    sColumn( COL_META_RESC_MODIFY_TIME, "r_resc_meta_main", "modify_ts" );

    sColumn( COL_META_RESC_GROUP_ATTR_NAME, "r_resc_grp_meta_main", "meta_attr_name" );
    sColumn( COL_META_RESC_GROUP_ATTR_VALUE, "r_resc_grp_meta_main", "meta_attr_value" );
    sColumn( COL_META_RESC_GROUP_ATTR_UNITS, "r_resc_grp_meta_main", "meta_attr_unit" );
    sColumn( COL_META_RESC_GROUP_ATTR_ID,   "r_resc_grp_meta_main", "meta_id" );
    sColumn( COL_META_RESC_GROUP_CREATE_TIME, "r_resc_grp_meta_main", "create_ts" );
    sColumn( COL_META_RESC_GROUP_MODIFY_TIME, "r_resc_grp_meta_main", "modify_ts" );

    sColumn( COL_META_USER_ATTR_NAME, "r_user_meta_main", "meta_attr_name" );
    sColumn( COL_META_USER_ATTR_VALUE, "r_user_meta_main", "meta_attr_value" );
    sColumn( COL_META_USER_ATTR_UNITS, "r_user_meta_main", "meta_attr_unit" );
    sColumn( COL_META_USER_ATTR_ID,   "r_user_meta_main", "meta_id" );
    sColumn( COL_META_USER_CREATE_TIME, "r_user_meta_main", "create_ts" );
    sColumn( COL_META_USER_MODIFY_TIME, "r_user_meta_main", "modify_ts" );

    sColumn( COL_META_RULE_ATTR_NAME, "r_rule_meta_main", "meta_attr_name" );
    sColumn( COL_META_RULE_ATTR_VALUE, "r_rule_meta_main", "meta_attr_value" );
    sColumn( COL_META_RULE_ATTR_UNITS, "r_rule_meta_main", "meta_attr_unit" );
    sColumn( COL_META_RULE_ATTR_ID,   "r_rule_meta_main", "meta_id" );
    sColumn( COL_META_RULE_CREATE_TIME, "r_rule_meta_main", "create_ts" );
    sColumn( COL_META_RULE_MODIFY_TIME, "r_rule_meta_main", "modify_ts" );

    sColumn( COL_META_MSRVC_ATTR_NAME, "r_msrvc_meta_main", "meta_attr_name" );
    sColumn( COL_META_MSRVC_ATTR_VALUE, "r_msrvc_meta_main", "meta_attr_value" );
    sColumn( COL_META_MSRVC_ATTR_UNITS, "r_msrvc_meta_main", "meta_attr_unit" );
    sColumn( COL_META_MSRVC_ATTR_ID,   "r_msrvc_meta_main", "meta_id" );
    sColumn( COL_META_MSRVC_CREATE_TIME, "r_msrvc_meta_main", "create_ts" );
    sColumn( COL_META_MSRVC_MODIFY_TIME, "r_msrvc_meta_main", "modify_ts" );

    sColumn( COL_META_MET2_ATTR_NAME, "r_met2_meta_main", "meta_attr_name" );
    sColumn( COL_META_MET2_ATTR_VALUE, "r_met2_meta_main", "meta_attr_value" );
    sColumn( COL_META_MET2_ATTR_UNITS, "r_met2_meta_main", "meta_attr_unit" );
    sColumn( COL_META_MET2_ATTR_ID,   "r_met2_meta_main", "meta_id" );
    sColumn( COL_META_MET2_CREATE_TIME, "r_met2_meta_main", "create_ts" );
    sColumn( COL_META_MET2_MODIFY_TIME, "r_met2_meta_main", "modify_ts" );

//    sColumn( COL_RESC_GROUP_RESC_ID, "R_RESC_GROUP", "resc_id" );		// gone in 4.1 #1472
//    sColumn( COL_RESC_GROUP_NAME, "R_RESC_GROUP", "resc_group_name" );
//    sColumn( COL_RESC_GROUP_ID, "R_RESC_GROUP", "resc_group_id" );

    sColumn( COL_USER_GROUP_ID, "R_USER_GROUP",  "group_user_id" );
    sColumn( COL_USER_GROUP_NAME, "r_group_main", "user_name" );

    sColumn( COL_RULE_EXEC_ID, "R_RULE_EXEC", "rule_exec_id" );
    sColumn( COL_RULE_EXEC_NAME, "R_RULE_EXEC", "rule_name" );
    sColumn( COL_RULE_EXEC_REI_FILE_PATH, "R_RULE_EXEC", "rei_file_path" );
    sColumn( COL_RULE_EXEC_USER_NAME, "R_RULE_EXEC", "user_name" );
    sColumn( COL_RULE_EXEC_ADDRESS, "R_RULE_EXEC", "exe_address" );
    sColumn( COL_RULE_EXEC_TIME, "R_RULE_EXEC", "exe_time" );
    sColumn( COL_RULE_EXEC_FREQUENCY, "R_RULE_EXEC", "exe_frequency" );
    sColumn( COL_RULE_EXEC_PRIORITY, "R_RULE_EXEC", "priority" );
    sColumn( COL_RULE_EXEC_ESTIMATED_EXE_TIME, "R_RULE_EXEC", "estimated_exe_time" );
    sColumn( COL_RULE_EXEC_NOTIFICATION_ADDR, "R_RULE_EXEC", "notification_addr" );
    sColumn( COL_RULE_EXEC_LAST_EXE_TIME, "R_RULE_EXEC", "last_exe_time" );
    sColumn( COL_RULE_EXEC_STATUS, "R_RULE_EXEC", "exe_status" );
    sColumn( COL_RULE_EXEC_CONTEXT, "R_RULE_EXEC", "exe_context" );

    sColumn( COL_TOKEN_NAMESPACE, "R_TOKN_MAIN", "token_namespace" );
    sColumn( COL_TOKEN_ID, "R_TOKN_MAIN", "token_id" );
    sColumn( COL_TOKEN_NAME, "R_TOKN_MAIN", "token_name" );
    sColumn( COL_TOKEN_VALUE, "R_TOKN_MAIN", "token_value" );
    sColumn( COL_TOKEN_VALUE2, "R_TOKN_MAIN", "token_value2" );
    sColumn( COL_TOKEN_VALUE3, "R_TOKN_MAIN", "token_value3" );
    sColumn( COL_TOKEN_COMMENT, "R_TOKN_MAIN", "r_comment" );
    sColumn( COL_TOKEN_CREATE_TIME, "R_TOKN_MAIN", "create_ts" );
    sColumn( COL_TOKEN_MODIFY_TIME, "R_TOKN_MAIN", "modify_ts" );

    sColumn( COL_AUDIT_OBJ_ID,      "R_OBJT_AUDIT", "object_id" );
    sColumn( COL_AUDIT_USER_ID,     "R_OBJT_AUDIT", "user_id" );
    sColumn( COL_AUDIT_ACTION_ID,   "R_OBJT_AUDIT", "action_id" );
    sColumn( COL_AUDIT_COMMENT,     "R_OBJT_AUDIT", "r_comment" );
    sColumn( COL_AUDIT_CREATE_TIME, "R_OBJT_AUDIT", "create_ts" );
    sColumn( COL_AUDIT_MODIFY_TIME, "R_OBJT_AUDIT", "modify_ts" );

    sColumn( COL_COLL_USER_NAME, "r_coll_user_main", "user_name" );
    sColumn( COL_COLL_USER_ZONE, "r_coll_user_main", "zone_name" );

    sColumn( COL_DATA_USER_NAME, "r_data_user_main", "user_name" );
    sColumn( COL_DATA_USER_ZONE, "r_data_user_main", "zone_name" );

    sColumn( COL_RESC_USER_NAME, "r_resc_user_main", "user_name" );
    sColumn( COL_RESC_USER_ZONE, "r_resc_user_main", "zone_name" );

    sColumn( COL_SL_HOST_NAME, "R_SERVER_LOAD", "host_name" );
    sColumn( COL_SL_RESC_NAME, "R_SERVER_LOAD", "resc_name" );
    sColumn( COL_SL_CPU_USED, "R_SERVER_LOAD", "cpu_used" );
    sColumn( COL_SL_MEM_USED, "R_SERVER_LOAD", "mem_used" );
    sColumn( COL_SL_SWAP_USED, "R_SERVER_LOAD", "swap_used" );
    sColumn( COL_SL_RUNQ_LOAD, "R_SERVER_LOAD", "runq_load" );
    sColumn( COL_SL_DISK_SPACE, "R_SERVER_LOAD", "disk_space" );
    sColumn( COL_SL_NET_INPUT, "R_SERVER_LOAD", "net_input" );
    sColumn( COL_SL_NET_OUTPUT, "R_SERVER_LOAD", "net_output" );
    sColumn( COL_SL_CREATE_TIME, "R_SERVER_LOAD", "create_ts" );

    sColumn( COL_SLD_RESC_NAME, "R_SERVER_LOAD_DIGEST", "resc_name" );
    sColumn( COL_SLD_LOAD_FACTOR, "R_SERVER_LOAD_DIGEST", "load_factor" );
    sColumn( COL_SLD_CREATE_TIME, "R_SERVER_LOAD_DIGEST", "create_ts" );

    sColumn( COL_RULE_ID,         "R_RULE_MAIN", "rule_id" );
    sColumn( COL_RULE_VERSION,    "R_RULE_MAIN", "rule_version" );
    sColumn( COL_RULE_BASE_NAME,  "R_RULE_MAIN", "rule_base_name" );
    sColumn( COL_RULE_NAME,       "R_RULE_MAIN", "rule_name" );
    sColumn( COL_RULE_EVENT,      "R_RULE_MAIN", "rule_event" );
    sColumn( COL_RULE_CONDITION,  "R_RULE_MAIN", "rule_condition" );
    sColumn( COL_RULE_BODY,       "R_RULE_MAIN", "rule_body" );
    sColumn( COL_RULE_RECOVERY,   "R_RULE_MAIN", "rule_recovery" );
    sColumn( COL_RULE_STATUS,     "R_RULE_MAIN", "rule_status" );
    sColumn( COL_RULE_OWNER_NAME, "R_RULE_MAIN", "rule_owner_name" );
    sColumn( COL_RULE_OWNER_ZONE, "R_RULE_MAIN", "rule_owner_zone" );
    sColumn( COL_RULE_DESCR_1,    "R_RULE_MAIN", "rule_descr_1" );
    sColumn( COL_RULE_DESCR_2,    "R_RULE_MAIN", "rule_descr_2" );
    sColumn( COL_RULE_INPUT_PARAMS,    "R_RULE_MAIN", "input_params" );
    sColumn( COL_RULE_OUTPUT_PARAMS,   "R_RULE_MAIN", "output_params" );
    sColumn( COL_RULE_DOLLAR_VARS,     "R_RULE_MAIN", "dollar_vars" );
    sColumn( COL_RULE_ICAT_ELEMENTS,   "R_RULE_MAIN", "icat_elements" );
    sColumn( COL_RULE_SIDEEFFECTS,     "R_RULE_MAIN", "sideeffects" );
    sColumn( COL_RULE_COMMENT,    "R_RULE_MAIN", "r_comment" );
    sColumn( COL_RULE_CREATE_TIME, "R_RULE_MAIN", "create_ts" );
    sColumn( COL_RULE_MODIFY_TIME, "R_RULE_MAIN", "modify_ts" );

    sColumn( COL_RULE_BASE_MAP_VERSION,    "R_RULE_BASE_MAP", "map_version" );
    sColumn( COL_RULE_BASE_MAP_PRIORITY,    "R_RULE_BASE_MAP", "map_priority" );
    sColumn( COL_RULE_BASE_MAP_BASE_NAME,  "R_RULE_BASE_MAP", "map_base_name" );
    sColumn( COL_RULE_BASE_MAP_OWNER_NAME, "R_RULE_BASE_MAP", "map_owner_name" );
    sColumn( COL_RULE_BASE_MAP_OWNER_ZONE, "R_RULE_BASE_MAP", "map_owner_zone" );
    sColumn( COL_RULE_BASE_MAP_COMMENT,    "R_RULE_BASE_MAP", "r_comment" );
    sColumn( COL_RULE_BASE_MAP_CREATE_TIME, "R_RULE_BASE_MAP", "create_ts" );
    sColumn( COL_RULE_BASE_MAP_MODIFY_TIME, "R_RULE_BASE_MAP", "modify_ts" );

    sColumn( COL_DVM_ID,           "R_RULE_DVM", "dvm_id" );
    sColumn( COL_DVM_VERSION,      "R_RULE_DVM", "dvm_version" );
    sColumn( COL_DVM_BASE_NAME,    "R_RULE_DVM", "dvm_base_name" );
    sColumn( COL_DVM_EXT_VAR_NAME, "R_RULE_DVM", "dvm_ext_var_name" );
    sColumn( COL_DVM_CONDITION,    "R_RULE_DVM", "dvm_condition" );
    sColumn( COL_DVM_INT_MAP_PATH, "R_RULE_DVM", "dvm_int_map_path" );
    sColumn( COL_DVM_STATUS,       "R_RULE_DVM", "dvm_status" );
    sColumn( COL_DVM_OWNER_NAME,   "R_RULE_DVM", "dvm_owner_name" );
    sColumn( COL_DVM_OWNER_ZONE,   "R_RULE_DVM", "dvm_owner_zone" );
    sColumn( COL_DVM_COMMENT,      "R_RULE_DVM", "r_comment" );
    sColumn( COL_DVM_CREATE_TIME,  "R_RULE_DVM", "create_ts" );
    sColumn( COL_DVM_MODIFY_TIME,  "R_RULE_DVM", "modify_ts" );

    sColumn( COL_DVM_BASE_MAP_VERSION,    "R_RULE_DVM_MAP", "map_dvm_version" );
    sColumn( COL_DVM_BASE_MAP_BASE_NAME,  "R_RULE_DVM_MAP", "map_dvm_base_name" );
    sColumn( COL_DVM_BASE_MAP_OWNER_NAME, "R_RULE_DVM_MAP", "map_owner_name" );
    sColumn( COL_DVM_BASE_MAP_OWNER_ZONE, "R_RULE_DVM_MAP", "map_owner_zone" );
    sColumn( COL_DVM_BASE_MAP_COMMENT,    "R_RULE_DVM_MAP", "r_comment" );
    sColumn( COL_DVM_BASE_MAP_CREATE_TIME, "R_RULE_DVM_MAP", "create_ts" );
    sColumn( COL_DVM_BASE_MAP_MODIFY_TIME, "R_RULE_DVM_MAP", "modify_ts" );

    sColumn( COL_FNM_ID,           "R_RULE_FNM", "fnm_id" );
    sColumn( COL_FNM_VERSION,      "R_RULE_FNM", "fnm_version" );
    sColumn( COL_FNM_BASE_NAME,    "R_RULE_FNM", "fnm_base_name" );
    sColumn( COL_FNM_EXT_FUNC_NAME, "R_RULE_FNM", "fnm_ext_func_name" );
    sColumn( COL_FNM_INT_FUNC_NAME, "R_RULE_FNM", "fnm_int_func_name" );
    sColumn( COL_FNM_STATUS,       "R_RULE_FNM", "fnm_status" );
    sColumn( COL_FNM_OWNER_NAME,   "R_RULE_FNM", "fnm_owner_name" );
    sColumn( COL_FNM_OWNER_ZONE,   "R_RULE_FNM", "fnm_owner_zone" );
    sColumn( COL_FNM_COMMENT,      "R_RULE_FNM", "r_comment" );
    sColumn( COL_FNM_CREATE_TIME,  "R_RULE_FNM", "create_ts" );
    sColumn( COL_FNM_MODIFY_TIME,  "R_RULE_FNM", "modify_ts" );

    sColumn( COL_FNM_BASE_MAP_VERSION,    "R_RULE_FNM_MAP", "map_fnm_version" );
    sColumn( COL_FNM_BASE_MAP_BASE_NAME,  "R_RULE_FNM_MAP", "map_fnm_base_name" );
    sColumn( COL_FNM_BASE_MAP_OWNER_NAME, "R_RULE_FNM_MAP", "map_owner_name" );
    sColumn( COL_FNM_BASE_MAP_OWNER_ZONE, "R_RULE_FNM_MAP", "map_owner_zone" );
    sColumn( COL_FNM_BASE_MAP_COMMENT,    "R_RULE_FNM_MAP", "r_comment" );
    sColumn( COL_FNM_BASE_MAP_CREATE_TIME, "R_RULE_FNM_MAP", "create_ts" );
    sColumn( COL_FNM_BASE_MAP_MODIFY_TIME, "R_RULE_FNM_MAP", "modify_ts" );

    sColumn( COL_QUOTA_USER_ID, "R_QUOTA_MAIN", "user_id" );
    sColumn( COL_QUOTA_RESC_ID, "R_QUOTA_MAIN", "resc_id" );
    sColumn( COL_QUOTA_LIMIT,   "R_QUOTA_MAIN", "quota_limit" );
    sColumn( COL_QUOTA_OVER,    "R_QUOTA_MAIN", "quota_over" );
    sColumn( COL_QUOTA_MODIFY_TIME, "R_QUOTA_MAIN", "modify_ts" );

    sColumn( COL_QUOTA_USAGE_USER_ID, "R_QUOTA_USAGE", "user_id" );
    sColumn( COL_QUOTA_USAGE_RESC_ID, "R_QUOTA_USAGE", "resc_id" );
    sColumn( COL_QUOTA_USAGE,   "R_QUOTA_USAGE", "quota_usage" );
    sColumn( COL_QUOTA_USAGE_MODIFY_TIME, "R_QUOTA_USAGE", "modify_ts" );

    sColumn( COL_QUOTA_USER_NAME, "r_quota_user_main", "user_name" );
    sColumn( COL_QUOTA_USER_TYPE, "r_quota_user_group", "user_type_name" );
    sColumn( COL_QUOTA_RESC_NAME, "r_quota_resc_main", "resc_name" );
    sColumn( COL_QUOTA_USER_ZONE, "r_quota_user_main", "zone_name" );

    sColumn( COL_MSRVC_ID,           "R_MICROSRVC_MAIN", "msrvc_id" );
    sColumn( COL_MSRVC_MODULE_NAME,         "R_MICROSRVC_MAIN", "msrvc_module_name" );
    sColumn( COL_MSRVC_NAME,         "R_MICROSRVC_MAIN", "msrvc_name" );
    sColumn( COL_MSRVC_SIGNATURE,    "R_MICROSRVC_MAIN", "msrvc_signature" );
    sColumn( COL_MSRVC_DOXYGEN,      "R_MICROSRVC_MAIN", "msrvc_doxygen" );
    sColumn( COL_MSRVC_VARIATIONS,   "R_MICROSRVC_MAIN", "msrvc_variations" );
    sColumn( COL_MSRVC_OWNER_NAME,   "R_MICROSRVC_MAIN", "msrvc_owner_name" );
    sColumn( COL_MSRVC_OWNER_ZONE,   "R_MICROSRVC_MAIN", "msrvc_owner_zone" );
    sColumn( COL_MSRVC_COMMENT,      "R_MICROSRVC_MAIN", "r_comment" );
    sColumn( COL_MSRVC_CREATE_TIME,  "R_MICROSRVC_MAIN", "create_ts" );
    sColumn( COL_MSRVC_MODIFY_TIME,  "R_MICROSRVC_MAIN", "modify_ts" );

    sColumn( COL_MSRVC_VERSION,    "R_MICROSRVC_VER", "msrvc_version" );
    sColumn( COL_MSRVC_HOST,  "R_MICROSRVC_VER", "msrvc_host" );
    sColumn( COL_MSRVC_LOCATION,  "R_MICROSRVC_VER", "msrvc_location" );
    sColumn( COL_MSRVC_LANGUAGE,  "R_MICROSRVC_VER", "msrvc_language" );
    sColumn( COL_MSRVC_TYPE_NAME,  "R_MICROSRVC_VER", "msrvc_type_name" );
    sColumn( COL_MSRVC_STATUS,       "R_MICROSRVC_VER", "msrvc_status" );
    sColumn( COL_MSRVC_VER_OWNER_NAME, "R_MICROSRVC_VER", "msrvc_owner_name" );
    sColumn( COL_MSRVC_VER_OWNER_ZONE, "R_MICROSRVC_VER", "msrvc_owner_zone" );
    sColumn( COL_MSRVC_VER_COMMENT,    "R_MICROSRVC_VER", "r_comment" );
    sColumn( COL_MSRVC_VER_CREATE_TIME, "R_MICROSRVC_VER", "create_ts" );
    sColumn( COL_MSRVC_VER_MODIFY_TIME, "R_MICROSRVC_VER", "modify_ts" );

    sColumn( COL_META_ACCESS_TYPE, "r_meta_access", "access_type_id" );
    sColumn( COL_META_ACCESS_NAME, "r_meta_tokn_accs", "token_name" );
    sColumn( COL_META_TOKEN_NAMESPACE, "r_meta_tokn_accs", "token_namespace" );
    sColumn( COL_META_ACCESS_USER_ID, "r_meta_access", "user_id" );
    sColumn( COL_META_ACCESS_META_ID, "r_meta_access", "object_id" );

    sColumn( COL_RESC_ACCESS_TYPE, "r_resc_access", "access_type_id" );
    sColumn( COL_RESC_ACCESS_NAME, "r_resc_tokn_accs", "token_name" );
    sColumn( COL_RESC_TOKEN_NAMESPACE, "r_resc_tokn_accs", "token_namespace" );
    sColumn( COL_RESC_ACCESS_USER_ID, "r_resc_access", "user_id" );
    sColumn( COL_RESC_ACCESS_RESC_ID, "r_resc_access", "object_id" );

    sColumn( COL_RULE_ACCESS_TYPE, "r_rule_access", "access_type_id" );
    sColumn( COL_RULE_ACCESS_NAME, "r_rule_tokn_accs", "token_name" );
    sColumn( COL_RULE_TOKEN_NAMESPACE, "r_rule_tokn_accs", "token_namespace" );
    sColumn( COL_RULE_ACCESS_USER_ID, "r_rule_access", "user_id" );
    sColumn( COL_RULE_ACCESS_RULE_ID, "r_rule_access", "object_id" );

    sColumn( COL_MSRVC_ACCESS_TYPE, "r_msrvc_access", "access_type_id" );
    sColumn( COL_MSRVC_ACCESS_NAME, "r_msrvc_tokn_accs", "token_name" );
    sColumn( COL_MSRVC_TOKEN_NAMESPACE, "r_msrvc_tokn_accs", "token_namespace" );
    sColumn( COL_MSRVC_ACCESS_USER_ID, "r_msrvc_access", "user_id" );
    sColumn( COL_MSRVC_ACCESS_MSRVC_ID, "r_msrvc_access", "object_id" );

    sColumn( COL_TICKET_ID, "R_TICKET_MAIN", "ticket_id" );
    sColumn( COL_TICKET_STRING, "R_TICKET_MAIN", "ticket_string" );
    sColumn( COL_TICKET_TYPE, "R_TICKET_MAIN", "ticket_type" );
    sColumn( COL_TICKET_USER_ID, "R_TICKET_MAIN", "user_id" );
    sColumn( COL_TICKET_OBJECT_ID, "R_TICKET_MAIN", "object_id" );
    sColumn( COL_TICKET_OBJECT_TYPE, "R_TICKET_MAIN", "object_type" );
    sColumn( COL_TICKET_USES_LIMIT, "R_TICKET_MAIN", "uses_limit" );
    sColumn( COL_TICKET_USES_COUNT, "R_TICKET_MAIN", "uses_count" );
    sColumn( COL_TICKET_WRITE_FILE_LIMIT, "R_TICKET_MAIN", "write_file_limit" );
    sColumn( COL_TICKET_WRITE_FILE_COUNT, "R_TICKET_MAIN", "write_file_count" );
    sColumn( COL_TICKET_WRITE_BYTE_LIMIT, "R_TICKET_MAIN", "write_byte_limit" );
    sColumn( COL_TICKET_WRITE_BYTE_COUNT, "R_TICKET_MAIN", "write_byte_count" );
    sColumn( COL_TICKET_EXPIRY_TS, "R_TICKET_MAIN", "ticket_expiry_ts" );
    sColumn( COL_TICKET_CREATE_TIME, "R_TICKET_MAIN", "create_ts" );
    sColumn( COL_TICKET_MODIFY_TIME, "R_TICKET_MAIN", "modify_ts" );

    sColumn( COL_TICKET_ALLOWED_HOST, "R_TICKET_ALLOWED_HOSTS", "host" );
    sColumn( COL_TICKET_ALLOWED_HOST_TICKET_ID, "R_TICKET_ALLOWED_HOSTS", "ticket_id" );
    sColumn( COL_TICKET_ALLOWED_USER_NAME, "R_TICKET_ALLOWED_USERS", "user_name" );
    sColumn( COL_TICKET_ALLOWED_USER_TICKET_ID, "R_TICKET_ALLOWED_USERS", "ticket_id" );
    sColumn( COL_TICKET_ALLOWED_GROUP_NAME, "R_TICKET_ALLOWED_GROUPS", "group_name" );
    sColumn( COL_TICKET_ALLOWED_GROUP_TICKET_ID, "R_TICKET_ALLOWED_GROUPS", "ticket_id" );

    sColumn( COL_TICKET_DATA_NAME, "R_DATA_MAIN", "data_name" );
    sColumn( COL_TICKET_COLL_NAME, "r_ticket_coll_main", "coll_name" );
    sColumn( COL_TICKET_OWNER_NAME, "r_ticket_user_main", "user_name" );
    sColumn( COL_TICKET_OWNER_ZONE, "r_ticket_user_main", "zone_name" );

    sColumn( COL_TICKET_DATA_COLL_NAME, "r_ticket_data_coll_main", "coll_name" );

    /* Define the Foreign Key links between tables */

    sFklink( "R_COLL_MAIN", "R_DATA_MAIN", "R_COLL_MAIN.coll_id = R_DATA_MAIN.coll_id" );
    sFklink( "R_RESC_GROUP", "R_RESC_MAIN", "R_RESC_GROUP.resc_id = R_RESC_MAIN.resc_id" );
    sFklink( "R_RESC_MAIN", "r_resc_metamap", "R_RESC_MAIN.resc_id = r_resc_metamap.object_id" );
//    sFklink( "R_RESC_MAIN", "R_DATA_MAIN", "R_RESC_MAIN.resc_name = R_DATA_MAIN.resc_name" );
    sFklink( "R_RESC_MAIN", "R_DATA_MAIN", "R_RESC_MAIN.resc_id = R_DATA_MAIN.resc_id" );
    sFklink( "R_RESC_GROUP", "r_resc_grp_metamap", "R_RESC_GROUP.resc_group_id = r_resc_grp_metamap.object_id" );
    sFklink( "R_COLL_MAIN", "r_coll_metamap", "R_COLL_MAIN.coll_id = r_coll_metamap.object_id" );
    sFklink( "R_DATA_MAIN", "r_data_metamap", "R_DATA_MAIN.data_id = r_data_metamap.object_id" );
    sFklink( "R_RULE_MAIN", "r_rule_metamap", "R_RULE_MAIN.rule_id = r_rule_metamap.object_id" );
    sFklink( "R_USER_MAIN", "r_user_metamap", "R_USER_MAIN.user_id = r_user_metamap.object_id" );
    sFklink( "r_met2_main", "r_met2_metamap", "r_met2_main.meta_id = r_met2_metamap.object_id" );
    sFklink( "R_RESC_MAIN", "r_resc_access", "R_RESC_MAIN.resc_id = r_resc_access.object_id" );
    sFklink( "R_COLL_MAIN", "r_coll_access", "R_COLL_MAIN.coll_id = r_coll_access.object_id" );
    sFklink( "R_DATA_MAIN", "r_data_access", "R_DATA_MAIN.data_id = r_data_access.object_id" );
    sFklink( "R_RULE_MAIN", "r_rule_access", "R_RULE_MAIN.rule_id = r_rule_access.object_id" );
    sFklink( "r_met2_main", "r_met2_access", "r_met2_main.meta_id = r_met2_access.object_id" );
    sFklink( "R_RESC_MAIN", "r_resc_deny_access", "R_RESC_MAIN.resc_id = r_resc_deny_access.object_id" );
    sFklink( "R_COLL_MAIN", "r_coll_deny_access", "R_COLL_MAIN.coll_id = r_coll_deny_access.object_id" );
    sFklink( "R_DATA_MAIN", "r_data_deny_access", "R_DATA_MAIN.data_id = r_data_deny_access.object_id" );
    sFklink( "R_RULE_MAIN", "r_rule_deny_access", "R_RULE_MAIN.rule_id = r_rule_deny_access.object_id" );
    sFklink( "r_met2_main", "r_met2_deny_access", "r_met2_main.meta_id = r_met2_deny_access.object_id" );
    sFklink( "R_RESC_MAIN", "r_resc_audit", "R_RESC_MAIN.resc_id = r_resc_audit.object_id" );
    sFklink( "R_COLL_MAIN", "r_coll_audit", "R_COLL_MAIN.coll_id = r_coll_audit.object_id" );
    sFklink( "R_DATA_MAIN", "r_data_audit", "R_DATA_MAIN.data_id = r_data_audit.object_id" );
    sFklink( "R_RULE_MAIN", "r_rule_audit", "R_RULE_MAIN.rule_id = r_rule_audit.object_id" );
    sFklink( "r_met2_main", "r_met2_audit", "r_met2_main.meta_id = r_met2_audit.object_id.meta_id" );
    sFklink( "r_resc_metamap", "r_resc_meta_main",  "r_resc_metamap.meta_id = r_resc_meta_main.meta_id" );
    sFklink( "r_resc_grp_metamap", "r_resc_grp_meta_main",  "r_resc_grp_metamap.meta_id = r_resc_grp_meta_main.meta_id" );
    sFklink( "r_coll_metamap", "r_coll_meta_main",  "r_coll_metamap.meta_id = r_coll_meta_main.meta_id" );
    sFklink( "r_data_metamap", "r_data_meta_main",  "r_data_metamap.meta_id = r_data_meta_main.meta_id" );
    sFklink( "r_rule_metamap", "r_rule_meta_main",  "r_rule_metamap.meta_id = r_rule_meta_main.meta_id" );
    sFklink( "r_user_metamap", "r_user_meta_main",  "r_user_metamap.meta_id = r_user_meta_main.meta_id" );
    sFklink( "r_met2_metamap", "r_met2_meta_main",  "r_met2_metamap.object_id = r_met2_meta_main.meta_id" );
    sFklink( "r_resc_access", "r_resc_tokn_accs",  "r_resc_access.access_type_id = r_resc_tokn_accs.token_id" );
    sFklink( "r_resc_access", "r_resc_user_group", "r_resc_access.user_id = r_resc_user_group.group_user_id" );
    sFklink( "r_coll_access", "r_coll_tokn_accs",  "r_coll_access.access_type_id = r_coll_tokn_accs.token_id" );
    sFklink( "r_coll_access", "r_coll_user_group", "r_coll_access.user_id = r_coll_user_group.group_user_id" );
    sFklink( "r_data_access", "r_data_tokn_accs",  "r_data_access.access_type_id = r_data_tokn_accs.token_id" );
    sFklink( "r_data_access", "r_data_user_group", "r_data_access.user_id = r_data_user_group.group_user_id" );
    sFklink( "r_rule_access", "r_rule_tokn_accs",  "r_rule_access.access_typ_id = r_rule_tokn_accs.token_id" );
    sFklink( "r_rule_access", "r_rule_user_group", "r_rule_access.user_id = r_rule_user_group.group_user_id" );
    sFklink( "r_met2_access", "r_met2_tokn_accs",  "r_met2_access.access_typ_id = r_met2_tokn_accs.token_id" );
    sFklink( "r_met2_access", "r_met2_user_group", "r_met2_access.user_id = r_met2_user_group.group_user_id" );
    sFklink( "r_resc_deny_access", "r_resc_tokn_deny_accs", "r_resc_deny_access.access_typ_id = r_resc_tokn_deny_accs.token_id" );
    sFklink( "r_resc_deny_access", "r_resc_da_user_group",     "r_resc_deny_access.user_id = r_resc_da_user_group.group_user_id" );
    sFklink( "r_coll_deny_access", "r_coll_tokn_deny_accs", "r_coll_deny_access.access_typ_id = r_coll_tokn_deny_accs.token_id" );
    sFklink( "r_coll_deny_access", "r_coll_da_user_group",     "r_coll_deny_access.user_id = r_coll_da_user_group.group_user_id" );
    sFklink( "r_data_deny_access", "r_data_tokn_deny_accs", "r_data_deny_access.access_typ_id = r_data_tokn_deny_accs.token_id" );
    sFklink( "r_data_deny_access", "r_data_da_user_group",     "r_data_deny_access.user_id = r_data_da_user_group.group_user_id" );
    sFklink( "r_rule_deny_access", "r_rule_tokn_deny_accs", "r_rule_deny_access.access_typ_id = r_rule_tokn_deny_accs.token_id" );
    sFklink( "r_rule_deny_access", "r_rule_da_user_group",     "r_rule_deny_access.user_id = r_rule_da_user_group.group_user_id" );
    sFklink( "r_met2_deny_access", "r_met2_tokn_deny_accs", "r_met2_deny_access.access_typ_id = r_met2_tokn_deny_accs.token_id" );
    sFklink( "r_met2_deny_access", "r_met2_da_user_group",     "r_met2_deny_access.user_id = r_met2_da_user_group.group_user_id" );
    sFklink( "r_resc_audit", "r_resc_tokn_audit", "r_resc_audit.action_id = r_resc_tokn_audit.token_id" );
    sFklink( "r_resc_audit", "r_resc_au_user_group", "r_resc_audit.user_id = r_resc_au_user_group.group_user_id" );
    sFklink( "r_coll_audit", "r_coll_tokn_audit", "r_coll_audit.action_id = r_coll_tokn_audit.token_id" );
    sFklink( "r_coll_audit", "r_coll_au_user_group", "r_coll_audit.user_id = r_coll_au_user_group.group_user_id" );
    sFklink( "r_data_audit", "r_data_tokn_audit", "r_data_audit.action_id = r_data_tokn_audit.token_id" );
    sFklink( "r_data_audit", "r_data_au_user_group", "r_data_audit.user_id = r_data_au_user_group.group_user_id" );
    sFklink( "r_rule_audit", "r_rule_tokn_audit", "r_rule_audit.action_id = r_rule_tokn_audit.token_id" );
    sFklink( "r_rule_audit", "r_rule_au_user_group", "r_rule_audit.user_id = r_rule_au_user_group.group_user_id" );
    sFklink( "r_met2_audit", "r_met2_tokn_audit", "r_met2_audit.action_id = r_met2_tokn_audit.token_id" );
    sFklink( "r_met2_audit", "r_met2_au_user_group", "r_met2_audit.user_id = r_met2_au_user_group.group_user_id" );
    sFklink( "r_resc_user_group", "r_resc_user_main", "r_resc_user_group.user_id = r_resc_user_main.user_id" );
    sFklink( "r_coll_user_group", "r_coll_user_main", "r_coll_user_group.user_id = r_coll_user_main.user_id" );
    sFklink( "r_data_user_group", "r_data_user_main", "r_data_user_group.user_id = r_data_user_main.user_id" );
    sFklink( "r_rule_user_group", "r_rule_user_main", "r_rule_user_group.user_id = r_rule_user_main.user_id" );
    sFklink( "r_met2_user_group", "r_met2_user_main", "r_met2_user_group.user_id = r_met2_user_main.user_id" );
    sFklink( "r_resc_da_user_group", "r_resc_da_user_main", "r_resc_da_user_group.user_id = r_resc_da_user_main.user_id" );
    sFklink( "r_coll_da_user_group", "r_coll_da_user_main", "r_coll_da_user_group.user_id = r_coll_da_user_main.user_id" );
    sFklink( "r_data_da_user_group", "r_data_da_user_main", "r_data_da_user_group.user_id = r_data_da_user_main.user_id" );
    sFklink( "r_rule_da_user_group", "r_rule_da_user_main", "r_rule_da_user_group.user_id = r_rule_da_user_main.user_id" );
    sFklink( "r_met2_da_user_group", "r_met2_da_user_main", "r_met2_da_user_group.user_id = r_met2_da_user_main.user_id" );
    sFklink( "r_resc_au_user_group", "r_resc_au_user_main", "r_resc_au_user_group.user_id = r_resc_au_user_main.user_id" );
    sFklink( "r_coll_au_user_group", "r_coll_au_user_main", "r_coll_au_user_group.user_id = r_coll_au_user_main.user_id" );
    sFklink( "r_data_au_user_group", "r_data_au_user_main", "r_data_au_user_group.user_id = r_data_au_user_main.user_id" );
    sFklink( "r_rule_au_user_group", "r_rule_au_user_main", "r_rule_au_user_group.user_id = r_rule_au_user_main.user_id" );
    sFklink( "r_met2_au_user_group", "r_met2_au_user_main", "r_met2_au_user_group.user_id = r_met2_au_user_main.user_id" );
    sFklink( "R_USER_MAIN", "R_USER_PASSWORD", "R_USER_MAIN.user_id = R_USER_PASSWORD.user_id" );
    sFklink( "R_USER_MAIN", "R_USER_AUTH", "R_USER_MAIN.user_id = R_USER_AUTH.user_id" );
    sFklink( "R_USER_MAIN", "R_USER_SESSION_KEY", "R_USER_MAIN.user_id = R_USER_SESSION_KEY.user_id" );
    sFklink( "R_USER_MAIN", "r_data_access", "R_USER_MAIN.user_id = r_data_access.user_id" );
    sFklink( "R_USER_MAIN", "R_USER_GROUP", "R_USER_MAIN.user_id = R_USER_GROUP.user_id" );
    sFklink( "R_USER_GROUP", "r_group_main", "R_USER_GROUP.group_user_id = r_group_main.user_id" );
    sFklink( "R_USER_MAIN", "R_OBJT_AUDIT", "R_USER_MAIN.user_id = R_OBJT_AUDIT.user_id" );

    sFklink( "R_QUOTA_MAIN", "r_quota_user_main", "R_QUOTA_MAIN.user_id = r_quota_user_main.user_id" );
    sFklink( "R_QUOTA_MAIN", "r_quota_resc_main", "R_QUOTA_MAIN.resc_id = r_quota_resc_main.resc_id" );
    sFklink( "R_QUOTA_MAIN", "r_quota_user_group", "R_QUOTA_MAIN.user_id = r_quota_user_group.user_id" );
    sFklink( "R_QUOTA_USAGE", "r_quota_user_main", "R_QUOTA_USAGE.user_id = r_quota_user_main.user_id" );
    sFklink( "R_QUOTA_USAGE", "r_quota_resc_main", "R_QUOTA_USAGE.resc_id = r_quota_resc_main.resc_id" );
    sFklink( "R_QUOTA_USAGE", "r_quota_user_group", "R_QUOTA_USAGE.user_id = r_quota_user_group.user_id" );
    sFklink( "R_RULE_BASE_MAP", "R_RULE_MAIN", "R_RULE_BASE_MAP.rule_id = R_RULE_MAIN.rule_id" );
    sFklink( "R_RULE_DVM_MAP", "R_RULE_DVM", "R_RULE_DVM_MAP.dvm_id = R_RULE_DVM.dvm_id" );
    sFklink( "R_RULE_FNM_MAP", "R_RULE_FNM", "R_RULE_FNM_MAP.fnm_id = R_RULE_FNM.fnm_id" );


    sFklink( "R_MICROSRVC_MAIN", "R_MICROSRVC_VER", "R_MICROSRVC_MAIN.msrvc_id = R_MICROSRVC_VER.msrvc_id" );
    sFklink( "R_MICROSRVC_MAIN", "r_msrvc_metamap", "R_MICROSRVC_MAIN.msrvc_id = r_msrvc_metamap.object_id" );
    sFklink( "r_msrvc_metamap", "r_msrvc_meta_main",  "r_msrvc_metamap.meta_id = r_msrvc_meta_main.meta_id" );
    sFklink( "R_MICROSRVC_MAIN", "r_msrvc_access", "R_MICROSRVC_MAIN.msrvc_id = r_msrvc_access.object_id" );
    sFklink( "r_msrvc_access", "r_msrvc_tokn_accs",  "r_msrvc_access.access_typ_id = r_msrvc_tokn_accs.token_id" );
    sFklink( "r_msrvc_access", "r_msrvc_user_group", "r_msrvc_access.user_id = r_msrvc_user_group.group_user_id" );
    sFklink( "r_msrvc_user_group", "r_msrvc_user_main", "r_msrvc_user_group.user_id = r_msrvc_user_main.user_id" );
    sFklink( "R_MICROSRVC_MAIN", "r_msrvc_deny_access", "R_MICROSRVC_MAIN.msrvc_id = r_msrvc_deny_access.object_id" );
    sFklink( "r_msrvc_deny_access", "r_msrvc_tokn_deny_accs", "r_msrvc_deny_access.access_typ_id = r_msrvc_tokn_deny_accs.token_id" );
    sFklink( "r_msrvc_deny_access", "r_msrvc_da_user_group",     "r_msrvc_deny_access.user_id = r_msrvc_da_user_group.group_user_id" );
    sFklink( "r_msrvc_da_user_group", "r_msrvc_da_user_main", "r_msrvc_da_user_group.user_id = r_msrvc_da_user_main.user_id" );
    sFklink( "R_MICROSRVC_MAIN", "r_msrvc_audit", "R_MICROSRVC_MAIN.msrvc_id = r_msrvc_audit.object_id" );
    sFklink( "r_msrvc_audit", "r_msrvc_tokn_audit", "r_msrvc_audit.action_id = r_msrvc_tokn_audit.token_id" );
    sFklink( "r_msrvc_audit", "r_msrvc_au_user_group", "r_msrvc_audit.user_id = r_msrvc_au_user_group.group_user_id" );
    sFklink( "r_msrvc_au_user_group", "r_msrvc_au_user_main", "r_msrvc_au_user_group.user_id = r_msrvc_au_user_main.user_id" );

    sFklink( "R_TICKET_MAIN", "R_TICKET_ALLOWED_HOSTS", "R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_HOSTS.ticket_id" );
    sFklink( "R_TICKET_MAIN", "R_TICKET_ALLOWED_USERS", "R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_USERS.ticket_id" );
    sFklink( "R_TICKET_MAIN", "R_TICKET_ALLOWED_GROUPS", "R_TICKET_MAIN.ticket_id = R_TICKET_ALLOWED_GROUPS.ticket_id" );

    sFklink( "R_TICKET_MAIN", "R_DATA_MAIN", "R_TICKET_MAIN.object_id = R_DATA_MAIN.data_id" );
    sFklink( "R_TICKET_MAIN", "r_ticket_coll_main", "R_TICKET_MAIN.object_id = r_ticket_coll_main.coll_id" );
    sFklink( "R_TICKET_MAIN", "r_ticket_data_coll_main", "R_TICKET_MAIN.object_id = R_DATA_MAIN.data_id AND R_DATA_MAIN.coll_id = r_ticket_data_coll_main.coll_id" );
    sFklink( "R_TICKET_MAIN", "r_ticket_user_main", "R_TICKET_MAIN.user_id = r_ticket_user_main.user_id" );

}
