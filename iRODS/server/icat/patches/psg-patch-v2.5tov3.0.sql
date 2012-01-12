--- Run these SQL statements using the PostgreSQL client psql
--- to upgrade from a 2.5 Postgres ICAT to 3.0.

create table R_RULE_DVM_MAP
 (
   map_dvm_version varchar(250) DEFAULT '0',
   map_dvm_base_name varchar(250) not null,
   dvm_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32) ,
   modify_ts varchar(32)
 );

create table R_RULE_FNM_MAP
 (
   map_fnm_version varchar(250) DEFAULT '0',
   map_fnm_base_name varchar(250) not null,
   fnm_id bigint not null,
   map_owner_name varchar(250) not null,
   map_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32) ,
   modify_ts varchar(32)
 );

create table R_MICROSRVC_MAIN
 (
   msrvc_id bigint not null,
   msrvc_name varchar(250) not null,
   msrvc_module_name  varchar(250) not null,
   msrvc_signature varchar(2700) not null,
   msrvc_doxygen varchar(2500) not null,
   msrvc_variations varchar(2500) not null,
   msrvc_owner_name varchar(250) not null,
   msrvc_owner_zone varchar(250) not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 );

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
   create_ts varchar(32) ,
   modify_ts varchar(32)
 );

create index idx_data_main5 on R_DATA_MAIN (data_type_name);

insert into R_TOKN_MAIN values ('resc_type',406,'mso','','','','','1312910000','1312910000');
insert into R_TOKN_MAIN values ('data_type',1696,'mso','','','','','1312910000','1312910000');
