-- run these SQL statements, using the postgresql client psql 
--
create unique index idx_coll_main3 on R_COLL_MAIN (coll_name);

drop table R_RULE_MAIN;

create table R_RULE_MAIN
(
rule_id bigint not null,
rule_version varchar(250) DEFAULT '0',
rule_base_name varchar(250) not null,
rule_name varchar(250) not null DEFAULT 'null',
rule_event varchar(2700) not null,
rule_condition varchar(2700),
rule_body varchar(2700) not null,
rule_recovery varchar(2700) not null,
rule_status INTEGER DEFAULT 1,
rule_owner_name varchar(250) not null,
rule_owner_zone varchar(250) not null,
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

create table R_QUOTA_MAIN
(
user_id bigint,
resc_id bigint,
quota_limit bigint,
quota_over bigint,
modify_ts varchar(32)
);

create table R_QUOTA_USAGE
(
user_id bigint,
resc_id bigint,
quota_usage bigint,
modify_ts varchar(32)
);

