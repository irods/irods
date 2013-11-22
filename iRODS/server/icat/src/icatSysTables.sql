create table R_ZONE_MAIN
 (
   zone_id bigint not null,
   zone_name varchar(250) not null,
   zone_type_name varchar(250) not null,
   zone_conn_string varchar(1000),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_USER_MAIN
 (
   user_id bigint not null,
   user_name varchar(250) not null,
   user_type_name varchar(250) not null,
   zone_name varchar(250) not null,
   user_info varchar(1000),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RESC_MAIN
 (
   resc_id bigint not null,
   resc_name varchar(250) not null,
   zone_name varchar(250) not null,
   resc_type_name varchar(250) not null,
   resc_class_name varchar(250) not null,
   resc_net varchar(250) not null,
   resc_def_path varchar(1000) not null,
   free_space varchar(250),
   free_space_ts varchar(32),
   resc_info varchar(1000),
   r_comment varchar(1000),
   resc_status varchar(32),
   create_ts varchar(32),
   modify_ts varchar(32),
   resc_children varchar(1000),
   resc_context varchar(1000),
   resc_parent varchar(1000),
   resc_objcount bigint DEFAULT 0
 ) ;

create table R_COLL_MAIN
 (
   coll_id bigint not null,
   parent_coll_name varchar(2700) not null,
   coll_name varchar(2700) not null,
   coll_owner_name varchar(250) not null,
   coll_owner_zone varchar(250) not null,
   coll_map_id bigint DEFAULT 0,
   coll_inheritance varchar(1000),
   coll_type varchar(250) DEFAULT '0',
   coll_info1 varchar(2700),
   coll_info2 varchar(2700),
   coll_expiry_ts varchar(32),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;





create table R_DATA_MAIN
 (
   data_id bigint not null,
   coll_id bigint not null,
   data_name varchar(1000) not null,
   data_repl_num INTEGER not null,
   data_version varchar(250) DEFAULT '0',
   data_type_name varchar(250) not null,
   data_size bigint not null,
   resc_group_name varchar(250),
   resc_name varchar(250) not null,
   data_path varchar(2700) not null,
   data_owner_name varchar(250) not null,
   data_owner_zone varchar(250) not null,
   data_is_dirty INTEGER DEFAULT 0,
   data_status varchar(250),
   data_checksum varchar(1000),
   data_expiry_ts varchar(32),
   data_map_id bigint DEFAULT 0,
   data_mode varchar(32),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32),
   resc_hier varchar(1000)
 ) ;

create table R_META_MAIN
 (
   meta_id bigint not null,
   meta_namespace varchar(250),
   meta_attr_name varchar(2700) not null,
   meta_attr_value varchar(2700) not null,
   meta_attr_unit varchar(250),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_TOKN_MAIN
 (
   token_namespace varchar(250) not null,
   token_id bigint not null,
   token_name varchar(250) not null,
   token_value varchar(250),
   token_value2 varchar(250),
   token_value3 varchar(250),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_MAIN
 (
   rule_id bigint not null,
   rule_version varchar(250) DEFAULT '0',
   rule_base_name varchar(250) not null,
   rule_name varchar(2700) not null,
   rule_event varchar(2700) not null,
   rule_condition varchar(2700),
   rule_body varchar(2700) not null,
   rule_recovery varchar(2700) not null,
   rule_status bigint DEFAULT 1,
   rule_owner_name varchar(250) not null,
   rule_owner_zone varchar(250) not null,
   rule_descr_1 varchar(2700),
   rule_descr_2 varchar(2700),
   input_params varchar(2700),
   output_params varchar(2700),
   dollar_vars varchar(2700),
   icat_elements varchar(2700),
   sideeffects varchar(2700),
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_BASE_MAP
 (
   map_version varchar(250) DEFAULT '0',
   map_base_name varchar(250) not null,
   map_priority INTEGER not null,
   rule_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_DVM
 (
   dvm_id bigint not null,
   dvm_version varchar(250) DEFAULT '0',
   dvm_base_name varchar(250) not null,
   dvm_ext_var_name varchar(250) not null,
   dvm_condition varchar(2700),
   dvm_int_map_path varchar(2700) not null,
   dvm_status INTEGER DEFAULT 1,
   dvm_owner_name varchar(250) not null,
   dvm_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_DVM_MAP
 (
   map_dvm_version varchar(250) DEFAULT '0',
   map_dvm_base_name varchar(250) not null,
   dvm_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_FNM
 (
   fnm_id bigint not null,
   fnm_version varchar(250) DEFAULT '0',
   fnm_base_name varchar(250) not null,
   fnm_ext_func_name varchar(250) not null,
   fnm_int_func_name varchar(2700) not null,
   fnm_status INTEGER DEFAULT 1,
   fnm_owner_name varchar(250) not null,
   fnm_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_FNM_MAP
 (
   map_fnm_version varchar(250) DEFAULT '0',
   map_fnm_base_name varchar(250) not null,
   fnm_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_MICROSRVC_MAIN
 (
   msrvc_id bigint not null,
   msrvc_name varchar(250) not null,
   msrvc_module_name varchar(250) not null,
   msrvc_signature varchar(2700) not null,
   msrvc_doxygen varchar(2500) not null,
   msrvc_variations varchar(2500) not null,
   msrvc_owner_name varchar(250) not null,
   msrvc_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_MICROSRVC_VER
 (
   msrvc_id bigint not null,
   msrvc_version varchar(250) DEFAULT '0',
   msrvc_host varchar(250) DEFAULT 'ALL',
   msrvc_location varchar(500),
   msrvc_language varchar(250) DEFAULT 'C',
   msrvc_type_name varchar(250) DEFAULT 'IRODS COMPILED',
   msrvc_status bigint DEFAULT 1,
   msrvc_owner_name varchar(250) not null,
   msrvc_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_RULE_EXEC
 (
   rule_exec_id bigint not null,
   rule_name varchar(2700) not null,
   rei_file_path varchar(2700),
   user_name varchar(250),
   exe_address varchar(250),
   exe_time varchar(32),
   exe_frequency varchar(250),
   priority varchar(32),
   estimated_exe_time varchar(32),
   notification_addr varchar(250),
   last_exe_time varchar(32),
   exe_status varchar(32),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_USER_GROUP
 (
   group_user_id bigint not null,
   user_id bigint not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_USER_SESSION_KEY
 (
   user_id bigint not null,
   session_key varchar(1000) not null,
   session_info varchar(1000) ,
   auth_scheme varchar(250) not null,
   session_expiry_ts varchar(32) not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_USER_PASSWORD
 (
   user_id bigint not null,
   rcat_password varchar(250) not null,
   pass_expiry_ts varchar(32) not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;



create table R_RESC_GROUP
 (
   resc_group_id bigint not null,
   resc_group_name varchar(250) not null,
   resc_id bigint not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_OBJT_METAMAP
 (
   object_id bigint not null,
   meta_id bigint not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_OBJT_ACCESS
 (
   object_id bigint not null,
   user_id bigint not null,
   access_type_id bigint not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_OBJT_DENY_ACCESS
 (
   object_id bigint not null,
   user_id bigint not null,
   access_type_id bigint not null,
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_OBJT_AUDIT
 (
   object_id bigint not null,
   user_id bigint not null,
   action_id bigint not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 ) ;

create table R_SERVER_LOAD
 (
   host_name varchar(250) not null,
   resc_name varchar(250) not null,
   cpu_used INTEGER,
   mem_used INTEGER,
   swap_used INTEGER,
   runq_load INTEGER,
   disk_space INTEGER,
   net_input INTEGER,
   net_output INTEGER,
   create_ts varchar(32)
 ) ;

create table R_SERVER_LOAD_DIGEST
 (
   resc_name varchar(250) not null,
   load_factor INTEGER,
   create_ts varchar(32)
 ) ;





create table R_USER_AUTH
 (
   user_id bigint not null,
   user_auth_name varchar(1000),
   create_ts varchar(32)
 ) ;




create table R_QUOTA_MAIN
 (
   user_id bigint,
   resc_id bigint,
   quota_limit bigint,
   quota_over bigint,
   modify_ts varchar(32)
 ) ;

create table R_QUOTA_USAGE
 (
   user_id bigint,
   resc_id bigint,
   quota_usage bigint,
   modify_ts varchar(32)
 ) ;

create table R_SPECIFIC_QUERY
 (
   alias varchar(1000),
   sqlStr varchar(2700),
   create_ts varchar(32)
 ) ;
create table R_TICKET_MAIN
(
   ticket_id bigint not null,
   ticket_string varchar(100),
   ticket_type varchar(20),
   user_id bigint not null,
   object_id bigint not null,
   object_type varchar(16),
   uses_limit int DEFAULT 0,
   uses_count int DEFAULT 0,
   write_file_limit int DEFAULT 10,
   write_file_count int DEFAULT 0,
   write_byte_limit int DEFAULT 0,
   write_byte_count int DEFAULT 0,
   ticket_expiry_ts varchar(32),
   restrictions varchar(16),
   create_ts varchar(32),
   modify_ts varchar(32)
);

create table R_TICKET_ALLOWED_HOSTS
(
   ticket_id bigint not null,
   host varchar(32)
);

create table R_TICKET_ALLOWED_USERS
(
   ticket_id bigint not null,
   user_name varchar(250) not null
);

create table R_TICKET_ALLOWED_GROUPS
(
   ticket_id bigint not null,
   group_name varchar(250) not null
);

create table R_OBJT_FILESYSTEM_META
(
   object_id bigint not null,
   file_uid varchar(32),
   file_gid varchar(32),
   file_owner varchar(250),
   file_group varchar(250),
   file_mode varchar(32),
   file_ctime varchar(32),
   file_mtime varchar(32),
   file_source_path varchar(2700),
   create_ts varchar(32),
   modify_ts varchar(32)
);
create sequence R_ObjectId increment by 1 start with 10000;






create unique index idx_zone_main1 on R_ZONE_MAIN (zone_id);
create unique index idx_zone_main2 on R_ZONE_MAIN (zone_name);
create index idx_user_main1 on R_USER_MAIN (user_id);
create unique index idx_user_main2 on R_USER_MAIN (user_name,zone_name);
create index idx_resc_main1 on R_RESC_MAIN (resc_id);
create unique index idx_resc_main2 on R_RESC_MAIN (zone_name,resc_name);
create unique index idx_coll_main3 on R_COLL_MAIN (coll_name );
create index idx_coll_main1 on R_COLL_MAIN (coll_id);
create unique index idx_coll_main2 on R_COLL_MAIN (parent_coll_name ,coll_name );
create index idx_data_main1 on R_DATA_MAIN (data_id);
create unique index idx_data_main2 on R_DATA_MAIN (coll_id,data_name ,data_repl_num,data_version);
create index idx_data_main3 on R_DATA_MAIN (coll_id);
create index idx_data_main4 on R_DATA_MAIN (data_name );
create index idx_data_main5 on R_DATA_MAIN (data_type_name);
create index idx_data_main6 on R_DATA_MAIN (data_path);
create unique index idx_meta_main1 on R_META_MAIN (meta_id);
create index idx_meta_main2 on R_META_MAIN (meta_attr_name );
create index idx_meta_main3 on R_META_MAIN (meta_attr_value );
create index idx_meta_main4 on R_META_MAIN (meta_attr_unit);
create unique index idx_rule_main1 on R_RULE_MAIN (rule_id);
create unique index idx_rule_exec on R_RULE_EXEC (rule_exec_id);
create unique index idx_user_group1 on R_USER_GROUP (group_user_id,user_id);
create unique index idx_resc_logical1 on R_RESC_GROUP (resc_group_name,resc_id);
create unique index idx_objt_metamap1 on R_OBJT_METAMAP (object_id,meta_id);
create index idx_objt_metamap2 on R_OBJT_METAMAP (object_id);
create index idx_objt_metamap3 on R_OBJT_METAMAP (meta_id);
create unique index idx_objt_access1 on R_OBJT_ACCESS (object_id,user_id);
create unique index idx_objt_daccs1 on R_OBJT_DENY_ACCESS (object_id,user_id);
create index idx_tokn_main1 on R_TOKN_MAIN (token_id);
create index idx_tokn_main2 on R_TOKN_MAIN (token_name);
create index idx_tokn_main3 on R_TOKN_MAIN (token_value);
create index idx_tokn_main4 on R_TOKN_MAIN (token_namespace);
create index idx_specific_query1 on R_SPECIFIC_QUERY (sqlStr );
create index idx_specific_query2 on R_SPECIFIC_QUERY (alias);




create unique index idx_ticket on R_TICKET_MAIN (ticket_string);
create unique index idx_ticket_host on R_TICKET_ALLOWED_HOSTS (ticket_id, host);
create unique index idx_ticket_user on R_TICKET_ALLOWED_USERS (ticket_id, user_name);
create unique index idx_ticket_group on R_TICKET_ALLOWED_GROUPS (ticket_id, group_name);

create unique index idx_obj_filesystem_meta1 on R_OBJT_FILESYSTEM_META (object_id);
