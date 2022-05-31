/*****************************************************************************
  These are the System Tables in the RODS Catalog
    R_DATA_xxx            - Tables defining Data Info
    R_ZONE_xxx            - Tables defining Zone Info
    R_USER_xxx            - Tables defining User Info
    R_RESC_xxx            - Tables defining Resource Info
    R_COLL_xxx            - Tables defining Collection Info
    R_META_xxx            - Tables defining Metadata Info
    R_TOKN_xxx            - Tables defining Token Info
    R_RULE_xxx            - Tables defining Rules Info
    R_OBJT_xxx            - Tables defining info applicable to multiple
                                 first-class objects
  The column lengths are used as follows:
    ids                - bigint (64 bit)
    dates              - varchar(32)
    short strings      - varchar(250)
    long strings       - varchar(1000)
    very long strings  - varchar(2700)

  R_TOKN_MAIN table is like a meta table for holding all
    reserved keywords/tokens/systemic ontologies that are used by
    the other RODS table. For example, one may store information
    about the data_types in here instead of storing them in a
    separate table.

    Hence rows such as
        token_id        = 1000
        token_namespace = 'data_type'
        token_name      = 'gif image'
    or
        token_id        = 20
        token_namespace = 'access_type'
        token_name      = 'write'
        token_value     = '020'
    will provide the keyword for the different token types.

    This is actually a multi-hierarchy table because what is used in the
    'token_namespace' column should be validated. To assist with that,
    we use the string 'token_namespace' as a reserved keyword and use it to
    boot-strap the other tokens. Hence, on installation, there will be a
    namespace called 'token_namespace' with the following token_names:
          'data_type', 'object_type','zone_type','resc_type',
          'user_type','action_type','rulexec_type','access_type',
          'resc_class','coll_map', 'data_type_dot_ext', 'data_type_mime',
          'auth_scheme_type'.

    On installation, each of the above mentioned namespaces will have
    at least one token_name called 'generic' associated with them.
    On installation, other values might be populated for a few
    token_namespaces.

    Whenever a new value for a token is introduced. the token_type is checked
    to see if the token_namespace is a valid one, and the triple
    (token_namespace, token_name, token_value) is checked for uniqueness
    before being added.

    Whenever a token value is filled in any other table, it is checked
    against the R_TOKN_MAIN table for validity.
*******************************************************/

#if defined(postgres) || defined(mysql)
#define INT64TYPE bigint
#else
#define INT64TYPE integer
#endif

create table R_ZONE_MAIN ( zone_id INT64TYPE not null, zone_name varchar(250) not null, zone_type_name varchar(250) not null, zone_conn_string varchar(1000), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_USER_MAIN ( user_id INT64TYPE not null, user_name varchar(250) not null, user_type_name varchar(250) not null, zone_name varchar(250) not null, user_info varchar(1000), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RESC_MAIN ( resc_id INT64TYPE not null, resc_name varchar(250) not null, zone_name varchar(250) not null, resc_type_name varchar(250) not null, resc_class_name varchar(250) not null, resc_net varchar(250) not null, resc_def_path varchar(1000) not null, free_space varchar(250), free_space_ts varchar(32), resc_info varchar(1000), r_comment varchar(1000), resc_status varchar(32), create_ts varchar(32), modify_ts varchar(32), resc_children varchar(1000), resc_context varchar(1000), resc_parent varchar(1000), resc_objcount INT64TYPE DEFAULT 0) ;

create table R_COLL_MAIN ( coll_id INT64TYPE not null, parent_coll_name varchar(2700) not null, coll_name varchar(2700) not null, coll_owner_name varchar(250) not null, coll_owner_zone varchar(250) not null, coll_map_id INT64TYPE DEFAULT 0, coll_inheritance varchar(1000), coll_type varchar(250) DEFAULT '0', coll_info1 varchar(2700) DEFAULT '0', coll_info2 varchar(2700) DEFAULT '0', coll_expiry_ts varchar(32), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

/*
  The data_is_dirty column is replStatus in the DataObjStatus structure.
  The data_status column is statusString (unused, currently).
*/
create table R_DATA_MAIN ( data_id INT64TYPE not null, coll_id INT64TYPE not null, data_name varchar(1000) not null, data_repl_num INTEGER not null, data_version varchar(250) DEFAULT '0', data_type_name varchar(250) not null, data_size INT64TYPE not null, resc_group_name varchar(250), resc_name varchar(250) not null, data_path varchar(2700) not null, data_owner_name varchar(250) not null, data_owner_zone varchar(250) not null, data_is_dirty INTEGER  DEFAULT 0, data_status varchar(250), data_checksum varchar(1000), data_expiry_ts varchar(32), data_map_id INT64TYPE DEFAULT 0, data_mode varchar(32), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32), resc_hier varchar(1000)) ;

create table R_META_MAIN ( meta_id INT64TYPE not null, meta_namespace varchar(250), meta_attr_name varchar(2700) not null, meta_attr_value varchar(2700) not null, meta_attr_unit varchar(250), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_TOKN_MAIN ( token_namespace varchar(250) not null, token_id INT64TYPE not null, token_name varchar(250) not null, token_value varchar(250), token_value2 varchar(250), token_value3 varchar(250), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

#ifdef mysql
create table R_RULE_MAIN ( rule_id INT64TYPE not null, rule_version varchar(250) DEFAULT '0', rule_base_name varchar(250) not null, rule_name text not null, rule_event text not null, rule_condition text, rule_body text not null, rule_recovery text not null, rule_status INT64TYPE DEFAULT 1, rule_owner_name varchar(250) not null, rule_owner_zone varchar(250) not null, rule_descr_1 text, rule_descr_2 text, input_params text, output_params text, dollar_vars text, icat_elements text, sideeffects text, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;
#else
create table R_RULE_MAIN ( rule_id INT64TYPE not null, rule_version varchar(250) DEFAULT '0', rule_base_name varchar(250) not null, rule_name varchar(2700) not null, rule_event varchar(2700) not null, rule_condition varchar(2700), rule_body varchar(2700) not null, rule_recovery varchar(2700) not null, rule_status INT64TYPE DEFAULT 1, rule_owner_name varchar(250) not null, rule_owner_zone varchar(250) not null, rule_descr_1 varchar(2700), rule_descr_2 varchar(2700), input_params varchar(2700), output_params varchar(2700), dollar_vars varchar(2700), icat_elements varchar(2700), sideeffects varchar(2700), r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;
#endif

create table R_RULE_BASE_MAP ( map_version varchar(250) DEFAULT '0', map_base_name varchar(250) not null, map_priority INTEGER not null, rule_id INT64TYPE not null, map_owner_name varchar(250) not null, map_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RULE_DVM ( dvm_id INT64TYPE not null, dvm_version varchar(250) DEFAULT '0', dvm_base_name varchar(250) not null, dvm_ext_var_name varchar(250) not null, dvm_condition varchar(2700), dvm_int_map_path varchar(2700) not null, dvm_status INTEGER DEFAULT 1, dvm_owner_name varchar(250) not null, dvm_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RULE_DVM_MAP ( map_dvm_version varchar(250) DEFAULT '0', map_dvm_base_name varchar(250) not null, dvm_id INT64TYPE not null, map_owner_name varchar(250) not null, map_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RULE_FNM ( fnm_id INT64TYPE not null, fnm_version varchar(250) DEFAULT '0', fnm_base_name varchar(250) not null, fnm_ext_func_name varchar(250) not null, fnm_int_func_name varchar(2700) not null, fnm_status INTEGER DEFAULT 1, fnm_owner_name varchar(250) not null, fnm_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RULE_FNM_MAP ( map_fnm_version varchar(250) DEFAULT '0', map_fnm_base_name varchar(250) not null, fnm_id INT64TYPE not null, map_owner_name varchar(250) not null, map_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_MICROSRVC_MAIN ( msrvc_id INT64TYPE not null, msrvc_name varchar(250) not null, msrvc_module_name varchar(250) not null, msrvc_signature varchar(2700) not null, msrvc_doxygen varchar(2500) not null, msrvc_variations varchar(2500) not null, msrvc_owner_name varchar(250) not null, msrvc_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_MICROSRVC_VER ( msrvc_id INT64TYPE not null, msrvc_version varchar(250) DEFAULT '0', msrvc_host varchar(250) DEFAULT 'ALL', msrvc_location varchar(500), msrvc_language varchar(250) DEFAULT 'C', msrvc_type_name varchar(250) DEFAULT 'IRODS COMPILED', msrvc_status INT64TYPE DEFAULT 1, msrvc_owner_name varchar(250) not null, msrvc_owner_zone varchar(250) not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RULE_EXEC ( rule_exec_id INT64TYPE not null, rule_name varchar(2700) not null, rei_file_path varchar(2700), user_name varchar(250), exe_address varchar(250), exe_time varchar(32), exe_frequency varchar(250), priority varchar(32), estimated_exe_time varchar(32), notification_addr varchar(250), last_exe_time varchar(32), exe_status varchar(32), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_USER_GROUP ( group_user_id INT64TYPE not null, user_id INT64TYPE not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_USER_SESSION_KEY ( user_id INT64TYPE not null, session_key varchar(1000) not null, session_info varchar(1000) , auth_scheme varchar(250) not null, session_expiry_ts varchar(32) not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_USER_PASSWORD ( user_id INT64TYPE not null, rcat_password varchar(250) not null, pass_expiry_ts varchar(32) not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_RESC_GROUP ( resc_group_id INT64TYPE not null, resc_group_name varchar(250) not null, resc_id INT64TYPE not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_OBJT_METAMAP ( object_id INT64TYPE not null, meta_id INT64TYPE not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_OBJT_ACCESS ( object_id INT64TYPE not null, user_id INT64TYPE not null, access_type_id INT64TYPE not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_OBJT_DENY_ACCESS ( object_id INT64TYPE not null, user_id INT64TYPE not null, access_type_id INT64TYPE not null, create_ts varchar(32), modify_ts varchar(32)) ;

create table R_OBJT_AUDIT ( object_id INT64TYPE not null, user_id INT64TYPE not null, action_id INT64TYPE not null, r_comment varchar(1000), create_ts varchar(32), modify_ts varchar(32)) ;

create table R_SERVER_LOAD ( host_name varchar(250) not null, resc_name varchar(250) not null, cpu_used INTEGER, mem_used INTEGER, swap_used INTEGER, runq_load INTEGER, disk_space INTEGER, net_input INTEGER, net_output INTEGER, create_ts varchar(32)) ;

create table R_SERVER_LOAD_DIGEST ( resc_name varchar(250) not null, load_factor INTEGER, create_ts varchar(32)) ;

/*
 Optional user authentication information,
 GSI DN(s) or Kerberos Principal name(s)
*/
create table R_USER_AUTH ( user_id INT64TYPE not null, user_auth_name varchar(1000), create_ts varchar(32)) ;

create table R_QUOTA_MAIN ( user_id INT64TYPE, resc_id INT64TYPE, quota_limit INT64TYPE, quota_over INT64TYPE, modify_ts varchar(32)) ;

create table R_QUOTA_USAGE ( user_id INT64TYPE, resc_id INT64TYPE, quota_usage INT64TYPE, modify_ts varchar(32)) ;

create table R_SPECIFIC_QUERY ( alias varchar(1000), sqlStr varchar(2700), create_ts varchar(32)) ;

/* Main ticket table.
Column ticket_type: read or write;
          object_type: data or collection;
          restrictions: flag for hosts, users, both or neither,
                  may be used to avoid unneeded queries on
                  the ticket_allowed tables below,
                  default is any are allowed. */

create table R_TICKET_MAIN ( ticket_id INT64TYPE not null, ticket_string varchar(100), ticket_type varchar(20), user_id INT64TYPE not null, object_id INT64TYPE not null, object_type varchar(16), uses_limit int  DEFAULT 0, uses_count int  DEFAULT 0, write_file_limit int  DEFAULT 10, write_file_count int  DEFAULT 0, write_byte_limit int  DEFAULT 0, write_byte_count int  DEFAULT 0, ticket_expiry_ts varchar(32), restrictions varchar(16), create_ts varchar(32), modify_ts varchar(32));

create table R_TICKET_ALLOWED_HOSTS ( ticket_id INT64TYPE not null, host varchar(32));

create table R_TICKET_ALLOWED_USERS ( ticket_id INT64TYPE not null, user_name varchar(250) not null);

create table R_TICKET_ALLOWED_GROUPS ( ticket_id INT64TYPE not null, group_name varchar(250) not null);

create table R_GRID_CONFIGURATION ( namespace varchar(2700), option_name varchar(2700), option_value varchar(2700));

#ifdef mysql

/* The max size for a MySQL InnoDB index prefix is 767 bytes,
   but since utf8 characters each take up three bytes,
   the max is 255 utf8 characters (255x3=765)
   http://dev.mysql.com/doc/refman/5.0/en/create-index.html
*/
#define VARCHAR_MAX_IDX_SIZE (767)

/* For MySQL we provide an emulation of the sequences using the
   auto-increment field in a special table
*/

drop table if exists R_ObjectId_seq_tbl;
create table R_ObjectId_seq_tbl ( nextval bigint not null primary key auto_increment ) engine = MyISAM;
alter table R_ObjectId_seq_tbl AUTO_INCREMENT = 10000;

#else

/* For Oracle and Postgres just use regular sequence */
create sequence R_ObjectId increment by 1 start with 10000;

/* And use a blank VARCHAR_MAX_IDX_SIZE */
#define VARCHAR_MAX_IDX_SIZE

#endif

create unique index idx_zone_main1 on R_ZONE_MAIN (zone_id);
create unique index idx_zone_main2 on R_ZONE_MAIN (zone_name);
create index idx_user_main1 on R_USER_MAIN (user_id);
create unique index idx_user_main2 on R_USER_MAIN (user_name,zone_name);
create index idx_resc_main1 on R_RESC_MAIN (resc_id);
create unique index idx_resc_main2 on R_RESC_MAIN (zone_name,resc_name);
create unique index idx_coll_main3 on R_COLL_MAIN (coll_name VARCHAR_MAX_IDX_SIZE);
create index idx_coll_main1 on R_COLL_MAIN (coll_id);

#ifdef mysql
create index idx_coll_main2 on R_COLL_MAIN (parent_coll_name(767));
#else
create unique index idx_coll_main2 on R_COLL_MAIN (parent_coll_name VARCHAR_MAX_IDX_SIZE,coll_name VARCHAR_MAX_IDX_SIZE);
#endif

create index idx_data_main1 on R_DATA_MAIN (data_id);

#ifdef mysql
create unique index idx_data_main2 on R_DATA_MAIN (coll_id, data_name(515), data_repl_num, data_version);
#else
create unique index idx_data_main2 on R_DATA_MAIN (coll_id,data_name VARCHAR_MAX_IDX_SIZE,data_repl_num,data_version);
#endif

create index idx_data_main3 on R_DATA_MAIN (coll_id);

#ifdef mysql
create index idx_data_main4 on R_DATA_MAIN (data_name(515));
#else
create index idx_data_main4 on R_DATA_MAIN (data_name VARCHAR_MAX_IDX_SIZE);
#endif

create index idx_data_main5 on R_DATA_MAIN (data_type_name);

/* this is not possible for MySQL reference :: http://stackoverflow.com/a/1827099 */
create index idx_data_main6 on R_DATA_MAIN (data_path VARCHAR_MAX_IDX_SIZE);

create unique index idx_meta_main1 on R_META_MAIN (meta_id);
create index idx_meta_main2 on R_META_MAIN (meta_attr_name VARCHAR_MAX_IDX_SIZE);
create index idx_meta_main3 on R_META_MAIN (meta_attr_value VARCHAR_MAX_IDX_SIZE);
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
create index idx_specific_query1 on R_SPECIFIC_QUERY (sqlStr VARCHAR_MAX_IDX_SIZE);
create index idx_specific_query2 on R_SPECIFIC_QUERY (alias VARCHAR_MAX_IDX_SIZE);


/* these indexes enforce the uniqueness constraint on the ticket strings
   (which can be provided by users), hosts, and users */
create unique index idx_ticket on R_TICKET_MAIN (ticket_string);
create unique index idx_ticket_host on R_TICKET_ALLOWED_HOSTS (ticket_id, host);
create unique index idx_ticket_user on R_TICKET_ALLOWED_USERS (ticket_id, user_name);
create unique index idx_ticket_group on R_TICKET_ALLOWED_GROUPS (ticket_id, group_name);

#ifdef mysql
create unique index idx_grid_configuration on R_GRID_CONFIGURATION (namespace(383), option_name(383));
#else
create unique index idx_grid_configuration on R_GRID_CONFIGURATION (namespace VARCHAR_MAX_IDX_SIZE, option_name VARCHAR_MAX_IDX_SIZE);
#endif
