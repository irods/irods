-- run these SQL statements, using the MySQL client mysql
--

ALTER TABLE R_RESC_GROUP ADD COLUMN resc_group_id bigint not null;

-- Should generate the id for the existing resc_group ...
UPDATE R_RESC_GROUP SET resc_group_id=r_objectid_nextval() WHERE resc_group_id IS NULL;

-- Changes to the Rule tables
-- Some of these tables may not exist in your ICAT,
-- in which case you may need to remove the drop statements.

DROP TABLE R_RULE_MAIN;
DROP TABLE R_RULE_BASE_MAP;
DROP TABLE R_RULE_DVM;
DROP TABLE R_RULE_FNM;

create table R_RULE_MAIN
 (
   rule_id             bigint not null,
   rule_version        varchar(250) DEFAULT '0',
   rule_base_name      varchar(250) not null,
   rule_name           varchar(2700) not null,
   rule_event          varchar(2700) not null,
   rule_condition      varchar(2700),
   rule_body           varchar(2700) not null,
   rule_recovery       varchar(2700) not null,
   rule_status         bigint DEFAULT 1,
   rule_owner_name     varchar(250) not null,
   rule_owner_zone     varchar(250) not null,
   rule_descr_1        varchar(2700),
   rule_descr_2        varchar(2700),
   input_params        varchar(2700),
   output_params       varchar(2700),
   dollar_vars         varchar(2700),
   icat_elements       varchar(2700),
   sideeffects         varchar(2700),
   r_comment           varchar(1000),
   create_ts           varchar(32),
   modify_ts           varchar(32)
 );

create table R_RULE_BASE_MAP
 (
   map_version varchar(250) DEFAULT '0',
   map_base_name varchar(250) not null,
   map_priority INTEGER not null,
   rule_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32) ,
   modify_ts varchar(32)
 );

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
   create_ts varchar(32) ,
   modify_ts varchar(32)
);

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
   create_ts varchar(32) ,
   modify_ts varchar(32)
);


-- New indexes

create index idx_data_main3 on R_DATA_MAIN (coll_id);
create index idx_data_main4 on R_DATA_MAIN (data_name (767));
create index idx_meta_main2 on R_META_MAIN (meta_attr_name (767));
create index idx_meta_main3 on R_META_MAIN (meta_attr_value (767));
create index idx_meta_main4 on R_META_MAIN (meta_attr_unit);
create index idx_objt_metamap2 on R_OBJT_METAMAP (object_id);
create index idx_objt_metamap3 on R_OBJT_METAMAP (meta_id);
create index idx_tokn_main1 on R_TOKN_MAIN (token_id);
create index idx_tokn_main2 on R_TOKN_MAIN (token_name);
create index idx_tokn_main3 on R_TOKN_MAIN (token_value);
create index idx_tokn_main4 on R_TOKN_MAIN (token_namespace);
