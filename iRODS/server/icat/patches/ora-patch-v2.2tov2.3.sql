-- run these SQL statements, using the Oracle client sqlplus
--
create unique index idx_coll_main3 on R_COLL_MAIN (coll_name);

drop table R_RULE_MAIN;

create table R_RULE_MAIN
(
rule_id integer not null,
rule_version varchar(250) DEFAULT '0',
rule_base_name varchar(250) not null,
rule_name varchar(250) DEFAULT 'null',
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
dvm_id integer not null,
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
fnm_id integer not null,
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
user_id integer,
resc_id integer,
quota_limit integer,
quota_over integer,
modify_ts varchar(32)
);

create table R_QUOTA_USAGE
(
user_id integer,
resc_id integer,
quota_usage integer,
modify_ts varchar(32)
);

